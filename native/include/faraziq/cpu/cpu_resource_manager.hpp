#pragma once
#include <cstddef>
namespace faraziq::cpu {
class CPUResourceManager final {
public:
    [[nodiscard]] static std::size_t available_logical_cpus() noexcept;
    // Leaves reserve_cpus for the OS/UI when possible; max_workers==0 means uncapped.
    [[nodiscard]] static std::size_t recommended_worker_count(std::size_t reserve_cpus = 1,
                                                               std::size_t max_workers = 0) noexcept;
};
}
