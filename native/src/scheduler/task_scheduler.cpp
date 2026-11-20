#include "faraziq/scheduler/task_scheduler.hpp"
#include "faraziq/cpu/cpu_resource_manager.hpp"
#include <condition_variable>
#include <exception>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>
namespace faraziq::scheduler {
struct TaskScheduler::Impl {
    struct Job { JobPriority priority; std::uint64_t sequence; std::function<core::Status(const CancellationToken&)> function; std::shared_ptr<std::atomic_bool> cancellation; std::shared_ptr<std::promise<core::Status>> promise; };
    struct Order { bool operator()(const Job& a, const Job& b) const noexcept { if (a.priority != b.priority) return static_cast<unsigned>(a.priority) < static_cast<unsigned>(b.priority); return a.sequence > b.sequence; } };
    explicit Impl(const std::size_t count) {
        workers.reserve(count);
        try { for (std::size_t i = 0; i < count; ++i) workers.emplace_back([this] { worker_loop(); }); }
        catch (...) { { std::lock_guard lock(mutex); stopping = true; } condition.notify_all(); for (auto& worker : workers) if (worker.joinable()) worker.join(); throw; }
    }
    void worker_loop() noexcept {
        for (;;) {
            Job job;
            { std::unique_lock lock(mutex); condition.wait(lock, [this] { return stopping || !jobs.empty(); }); if (jobs.empty()) { if (stopping) return; continue; } job = std::move(const_cast<Job&>(jobs.top())); jobs.pop(); active.push_back(job.cancellation); }
            core::Status result;
            if (job.cancellation->load(std::memory_order_relaxed)) result = core::Status::failure(core::ErrorCode::cancelled, "Job cancelled before execution");
            else {
                try { result = job.function(CancellationToken(job.cancellation)); if (job.cancellation->load(std::memory_order_relaxed) && result.ok()) result = core::Status::failure(core::ErrorCode::cancelled, "Job cancelled during execution"); }
                catch (const std::exception& e) { result = core::Status::failure(core::ErrorCode::internal_error, e.what()); }
                catch (...) { result = core::Status::failure(core::ErrorCode::internal_error, "Job threw a non-standard exception"); }
            }
            try { job.promise->set_value(std::move(result)); } catch (...) {}
            { std::lock_guard lock(mutex); const auto it = std::find(active.begin(), active.end(), job.cancellation); if (it != active.end()) active.erase(it); }
        }
    }
    mutable std::mutex mutex;
    std::condition_variable condition;
    std::priority_queue<Job, std::vector<Job>, Order> jobs;
    std::vector<std::thread> workers;
    std::vector<std::shared_ptr<std::atomic_bool>> active;
    std::uint64_t next_sequence{0};
    bool stopping{false};
};
CancellationToken::CancellationToken(std::shared_ptr<std::atomic_bool> state) noexcept : state_(std::move(state)) {}
bool CancellationToken::is_cancelled() const noexcept { return !state_ || state_->load(std::memory_order_relaxed); }
JobHandle::JobHandle(std::shared_ptr<std::atomic_bool> cancellation, std::future<core::Status> result) noexcept : cancellation_(std::move(cancellation)), result_(std::move(result)) {}
void JobHandle::cancel() noexcept { if (cancellation_) cancellation_->store(true, std::memory_order_relaxed); }
TaskScheduler::TaskScheduler(const std::size_t requested) {
    const auto count = requested == 0 ? cpu::CPUResourceManager::recommended_worker_count() : requested;
    if (count == 0) throw std::invalid_argument("TaskScheduler requires at least one worker");
    impl_ = std::make_unique<Impl>(count);
}
TaskScheduler::~TaskScheduler() { shutdown(); }
JobHandle TaskScheduler::submit(const JobPriority priority, std::function<core::Status(const CancellationToken&)> function) {
    auto cancellation = std::make_shared<std::atomic_bool>(false);
    auto promise = std::make_shared<std::promise<core::Status>>();
    auto future = promise->get_future();
    if (!function) { promise->set_value(core::Status::failure(core::ErrorCode::invalid_argument, "Cannot submit an empty job")); return JobHandle(std::move(cancellation), std::move(future)); }
    { std::lock_guard lock(impl_->mutex); if (impl_->stopping) { promise->set_value(core::Status::failure(core::ErrorCode::shutting_down, "Scheduler is shutting down")); return JobHandle(std::move(cancellation), std::move(future)); } impl_->jobs.push(Impl::Job{priority, impl_->next_sequence++, std::move(function), cancellation, promise}); }
    impl_->condition.notify_one();
    return JobHandle(std::move(cancellation), std::move(future));
}
void TaskScheduler::shutdown() noexcept {
    if (!impl_) return;
    std::vector<std::thread> workers;
    { std::lock_guard lock(impl_->mutex); if (impl_->stopping) return; impl_->stopping = true; for (auto& token : impl_->active) token->store(true, std::memory_order_relaxed); while (!impl_->jobs.empty()) { auto job = std::move(const_cast<Impl::Job&>(impl_->jobs.top())); impl_->jobs.pop(); job.cancellation->store(true, std::memory_order_relaxed); try { job.promise->set_value(core::Status::failure(core::ErrorCode::cancelled, "Job cancelled during scheduler shutdown")); } catch (...) {} } workers.swap(impl_->workers); }
    impl_->condition.notify_all();
    for (auto& worker : workers) if (worker.joinable()) worker.join();
}
std::size_t TaskScheduler::worker_count() const noexcept { return impl_ ? impl_->workers.size() : 0; }
}
