#include "faraziq/cpu/cpu_resource_manager.hpp"
#include <algorithm>
#include <thread>
#if defined(__ANDROID__) || defined(__linux__)
#include <unistd.h>
#endif
namespace faraziq::cpu {
std::size_t CPUResourceManager::available_logical_cpus() noexcept {
    std::size_t count = static_cast<std::size_t>(std::thread::hardware_concurrency());
#if defined(__ANDROID__) || defined(__linux__)
    const long online = ::sysconf(_SC_NPROCESSORS_ONLN);
    if (online > 0) {
        const auto os_count = static_cast<std::size_t>(online);
        count = count == 0 ? os_count : std::min(count, os_count);
    }
#endif
    return std::max<std::size_t>(count, 1);
}
std::size_t CPUResourceManager::recommended_worker_count(const std::size_t reserve_cpus,
                                                          const std::size_t max_workers) noexcept {
    const auto cpus = available_logical_cpus();
    const auto usable = cpus > reserve_cpus ? cpus - reserve_cpus : 1;
    const auto capped = max_workers == 0 ? usable : std::min(usable, max_workers);
    return std::max<std::size_t>(capped, 1);
}
}
