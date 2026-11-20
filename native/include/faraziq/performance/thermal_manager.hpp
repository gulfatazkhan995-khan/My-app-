#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>

namespace faraziq::performance {
enum class ThermalSeverity : std::uint8_t { nominal, light, moderate, severe, critical };
enum class PerformanceProfile : std::uint8_t { high_quality, balanced, power_saver, cooldown };
struct PerformancePolicy {
    PerformanceProfile profile{PerformanceProfile::balanced};
    std::size_t max_parallel_jobs{1};
    bool allow_gpu_work{true};
};

// Platform adapter supplies Android thermal status and app-available memory observations.
class ThermalManager final {
public:
    explicit ThermalManager(std::size_t configured_workers);
    [[nodiscard]] PerformancePolicy update(ThermalSeverity severity,
                                           std::size_t available_memory_bytes) noexcept;
    [[nodiscard]] PerformancePolicy current_policy() const noexcept;
private:
    const std::size_t configured_workers_;
    mutable std::mutex mutex_;
    PerformancePolicy policy_;
};
} // namespace faraziq::performance
