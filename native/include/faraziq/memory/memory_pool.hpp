#pragma once
#include <cstddef>
#include <memory>
#include <optional>
namespace faraziq::memory {
struct MemoryPoolState;
class BufferLease final {
public:
    BufferLease() = default;
    ~BufferLease();
    BufferLease(const BufferLease&) = delete;
    BufferLease& operator=(const BufferLease&) = delete;
    BufferLease(BufferLease&& other) noexcept;
    BufferLease& operator=(BufferLease&& other) noexcept;
    [[nodiscard]] std::byte* data() noexcept;
    [[nodiscard]] const std::byte* data() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }
    void reset() noexcept;
private:
    friend class MemoryPool;
    BufferLease(std::shared_ptr<MemoryPoolState>, std::size_t offset, std::size_t size) noexcept;
    std::shared_ptr<MemoryPoolState> state_;
    std::size_t offset_{0};
    std::size_t size_{0};
};
struct MemoryPoolStats { std::size_t capacity_bytes{0}; std::size_t used_bytes{0}; std::size_t high_water_bytes{0}; };
class MemoryPool final {
public:
    explicit MemoryPool(std::size_t capacity_bytes);
    [[nodiscard]] std::optional<BufferLease> try_allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t));
    [[nodiscard]] MemoryPoolStats stats() const noexcept;
private:
    std::shared_ptr<MemoryPoolState> state_;
};
}
