#include "faraziq/gpu/gpu_resource_manager.hpp"
#include <mutex>
#include <utility>
namespace faraziq::gpu {
struct GPUResourceManagerState { mutable std::mutex mutex; Backend backend{Backend::cpu}; std::string name{"CPU fallback"}; std::size_t budget{0}; std::size_t in_use{0}; };
namespace {
void release_bytes(const std::shared_ptr<GPUResourceManagerState>& state, const std::size_t bytes) noexcept {
    if (!state || bytes == 0) return;
    std::lock_guard lock(state->mutex);
    state->in_use = bytes > state->in_use ? 0 : state->in_use - bytes;
}
}
GPUResourceLease::GPUResourceLease(std::shared_ptr<GPUResourceManagerState> state, const std::size_t bytes) noexcept : state_(std::move(state)), bytes_(bytes) {}
GPUResourceLease::~GPUResourceLease() { reset(); }
GPUResourceLease::GPUResourceLease(GPUResourceLease&& other) noexcept : state_(std::move(other.state_)), bytes_(std::exchange(other.bytes_, 0)) {}
GPUResourceLease& GPUResourceLease::operator=(GPUResourceLease&& other) noexcept {
    if (this != &other) { reset(); state_ = std::move(other.state_); bytes_ = std::exchange(other.bytes_, 0); }
    return *this;
}
void GPUResourceLease::reset() noexcept { release_bytes(state_, bytes_); state_.reset(); bytes_ = 0; }
GPUResourceManager::GPUResourceManager() : state_(std::make_shared<GPUResourceManagerState>()) {}
core::Status GPUResourceManager::configure(const DeviceCapabilities& capabilities, const std::size_t budget) {
    std::lock_guard lock(state_->mutex);
    if (state_->in_use != 0) return core::Status::failure(core::ErrorCode::unavailable, "Cannot reconfigure GPU capabilities while reservations are active");
    state_->backend = capabilities.vulkan_available ? Backend::vulkan : capabilities.opengl_es_available ? Backend::opengl_es : Backend::cpu;
    state_->name = state_->backend == Backend::cpu ? "CPU fallback" : (capabilities.device_name.empty() ? "Unnamed graphics device" : capabilities.device_name);
    state_->budget = state_->backend == Backend::cpu ? 0 : budget;
    return {};
}
Backend GPUResourceManager::selected_backend() const noexcept { std::lock_guard lock(state_->mutex); return state_->backend; }
std::string GPUResourceManager::device_name() const { std::lock_guard lock(state_->mutex); return state_->name; }
std::size_t GPUResourceManager::memory_budget_bytes() const noexcept { std::lock_guard lock(state_->mutex); return state_->budget; }
std::size_t GPUResourceManager::memory_in_use_bytes() const noexcept { std::lock_guard lock(state_->mutex); return state_->in_use; }
std::optional<GPUResourceLease> GPUResourceManager::try_reserve(const std::size_t bytes) {
    if (bytes == 0) return std::nullopt;
    std::lock_guard lock(state_->mutex);
    if (state_->backend == Backend::cpu || state_->in_use > state_->budget || bytes > state_->budget - state_->in_use) return std::nullopt;
    state_->in_use += bytes;
    return GPUResourceLease(state_, bytes);
}
}
