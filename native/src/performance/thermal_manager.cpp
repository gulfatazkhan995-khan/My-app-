#include "faraziq/performance/thermal_manager.hpp"

#include <algorithm>

namespace faraziq::performance {
ThermalManager::ThermalManager(const std::size_t configured_workers)
    : configured_workers_(std::max<std::size_t>(configured_workers, 1)),
      policy_{PerformanceProfile::balanced, std::max<std::size_t>(configured_workers, 1), true} {}

PerformancePolicy ThermalManager::update(const ThermalSeverity severity,
                                         const std::size_t available_memory_bytes) noexcept {
    PerformancePolicy next;
    switch (severity) {
    case ThermalSeverity::nominal:
        next = {PerformanceProfile::high_quality, configured_workers_, true};
        break;
    case ThermalSeverity::light:
        next = {PerformanceProfile::balanced, std::max<std::size_t>(1, configured_workers_ * 3 / 4), true};
        break;
    case ThermalSeverity::moderate:
        next = {PerformanceProfile::balanced, std::max<std::size_t>(1, configured_workers_ / 2), true};
        break;
    case ThermalSeverity::severe:
        next = {PerformanceProfile::power_saver, 1, true};
        break;
    case ThermalSeverity::critical:
        next = {PerformanceProfile::cooldown, 1, false};
        break;
    }
    if (available_memory_bytes < 64U * 1024U * 1024U) {
        next.profile = PerformanceProfile::power_saver;
        next.max_parallel_jobs = 1;
    }
    std::lock_guard lock(mutex_);
    policy_ = next;
    return policy_;
}

PerformancePolicy ThermalManager::current_policy() const noexcept {
    std::lock_guard lock(mutex_);
    return policy_;
}
} // namespace faraziq::performance
