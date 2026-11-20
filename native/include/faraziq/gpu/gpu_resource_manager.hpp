#pragma once
#include "faraziq/core/status.hpp"
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
namespace faraziq::gpu {
enum class Backend { cpu, opengl_es, vulkan };
struct DeviceCapabilities { bool vulkan_available{false}; bool opengl_es_available{false}; std::string device_name; };
struct GPUResourceManagerState;
class GPUResourceLease final {
public:
    GPUResourceLease() = default;
    ~GPUResourceLease();
    GPUResourceLease(const GPUResourceLease&) = delete;
    GPUResourceLease& operator=(const GPUResourceLease&) = delete;
    GPUResourceLease(GPUResourceLease&& other) noexcept;
    GPUResourceLease& operator=(GPUResourceLease&& other) noexcept;
    [[nodiscard]] std::size_t bytes() const noexcept { return bytes_; }
    [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }
    void reset() noexcept;
private:
    friend class GPUResourceManager;
    GPUResourceLease(std::shared_ptr<GPUResourceManagerState> state, std::size_t bytes) noexcept;
    std::shared_ptr<GPUResourceManagerState> state_;
    std::size_t bytes_{0};
};
// Capability detection/context creation belong to the Android graphics adapter.
class GPUResourceManager final {
public:
    GPUResourceManager();
    [[nodiscard]] core::Status configure(const DeviceCapabilities&, std::size_t memory_budget_bytes);
    [[nodiscard]] Backend selected_backend() const noexcept;
    [[nodiscard]] std::string device_name() const;
    [[nodiscard]] std::size_t memory_budget_bytes() const noexcept;
    [[nodiscard]] std::size_t memory_in_use_bytes() const noexcept;
    [[nodiscard]] std::optional<GPUResourceLease> try_reserve(std::size_t bytes);
private:
    std::shared_ptr<GPUResourceManagerState> state_;
};
}
