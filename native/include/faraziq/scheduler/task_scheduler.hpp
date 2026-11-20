#pragma once
#include "faraziq/core/status.hpp"
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <thread>
namespace faraziq::scheduler {
enum class JobPriority : std::uint8_t { low = 0, normal = 1, high = 2, critical = 3 };
class CancellationToken final {
public:
    [[nodiscard]] bool is_cancelled() const noexcept;
private:
    friend class TaskScheduler;
    explicit CancellationToken(std::shared_ptr<std::atomic_bool>) noexcept;
    std::shared_ptr<std::atomic_bool> state_;
};
class JobHandle final {
public:
    JobHandle() = default;
    void cancel() noexcept;
    [[nodiscard]] bool valid() const noexcept { return result_.valid(); }
    [[nodiscard]] std::future<core::Status> take_result() noexcept { return std::move(result_); }
private:
    friend class TaskScheduler;
    JobHandle(std::shared_ptr<std::atomic_bool>, std::future<core::Status>) noexcept;
    std::shared_ptr<std::atomic_bool> cancellation_;
    std::future<core::Status> result_;
};
class TaskScheduler final {
public:
    explicit TaskScheduler(std::size_t worker_count = 0);
    ~TaskScheduler();
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
    [[nodiscard]] JobHandle submit(JobPriority, std::function<core::Status(const CancellationToken&)>);
    void shutdown() noexcept;
    [[nodiscard]] std::size_t worker_count() const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
