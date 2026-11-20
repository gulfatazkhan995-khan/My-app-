#include "faraziq/memory/memory_pool.hpp"
#include <cstdint>
#include <limits>
#include <map>
#include <mutex>
#include <utility>
#include <vector>
namespace faraziq::memory {
struct MemoryPoolState {
    explicit MemoryPoolState(const std::size_t capacity) : storage(capacity) { if (capacity != 0) free_ranges.emplace(0, capacity); }
    mutable std::mutex mutex;
    std::vector<std::byte> storage;
    std::map<std::size_t, std::size_t> free_ranges;
    std::size_t used{0};
    std::size_t high_water{0};
};
namespace {
bool valid_alignment(const std::size_t value) noexcept { return value != 0 && (value & (value - 1)) == 0; }
void release_range(const std::shared_ptr<MemoryPoolState>& state, std::size_t offset, std::size_t size) noexcept {
    if (!state || size == 0) return;
    std::lock_guard lock(state->mutex);
    auto next = state->free_ranges.lower_bound(offset);
    if (next != state->free_ranges.begin()) {
        auto previous = std::prev(next);
        if (previous->first + previous->second == offset) { offset = previous->first; size += previous->second; state->free_ranges.erase(previous); }
    }
    next = state->free_ranges.lower_bound(offset);
    if (next != state->free_ranges.end() && offset + size == next->first) { size += next->second; state->free_ranges.erase(next); }
    try { state->free_ranges.emplace(offset, size); state->used = size > state->used ? 0 : state->used - size; }
    catch (...) { /* Preserve process safety; this range is unavailable if metadata allocation fails. */ }
}
}
BufferLease::BufferLease(std::shared_ptr<MemoryPoolState> state, const std::size_t offset, const std::size_t size) noexcept : state_(std::move(state)), offset_(offset), size_(size) {}
BufferLease::~BufferLease() { reset(); }
BufferLease::BufferLease(BufferLease&& other) noexcept : state_(std::move(other.state_)), offset_(std::exchange(other.offset_, 0)), size_(std::exchange(other.size_, 0)) {}
BufferLease& BufferLease::operator=(BufferLease&& other) noexcept {
    if (this != &other) { reset(); state_ = std::move(other.state_); offset_ = std::exchange(other.offset_, 0); size_ = std::exchange(other.size_, 0); }
    return *this;
}
std::byte* BufferLease::data() noexcept { return state_ ? state_->storage.data() + offset_ : nullptr; }
const std::byte* BufferLease::data() const noexcept { return state_ ? state_->storage.data() + offset_ : nullptr; }
void BufferLease::reset() noexcept { release_range(state_, offset_, size_); state_.reset(); offset_ = 0; size_ = 0; }
MemoryPool::MemoryPool(const std::size_t capacity) : state_(std::make_shared<MemoryPoolState>(capacity)) {}
std::optional<BufferLease> MemoryPool::try_allocate(const std::size_t bytes, const std::size_t alignment) {
    if (bytes == 0 || !valid_alignment(alignment)) return std::nullopt;
    std::lock_guard lock(state_->mutex);
    const auto base = reinterpret_cast<std::uintptr_t>(state_->storage.data());
    for (auto it = state_->free_ranges.begin(); it != state_->free_ranges.end(); ++it) {
        const auto start = it->first;
        const auto length = it->second;
        if (start > std::numeric_limits<std::uintptr_t>::max() - base) continue;
        const auto address = base + start;
        const auto mask = static_cast<std::uintptr_t>(alignment - 1);
        if (address > std::numeric_limits<std::uintptr_t>::max() - mask) continue;
        const auto aligned_address = (address + mask) & ~mask;
        const auto aligned = static_cast<std::size_t>(aligned_address - base);
        if (aligned < start || aligned - start > length) continue;
        const auto prefix = aligned - start;
        if (bytes > length - prefix) continue;
        const auto suffix_start = aligned + bytes;
        const auto suffix = length - prefix - bytes;
        const auto old_start = start;
        state_->free_ranges.erase(it);
        try {
            if (prefix != 0) state_->free_ranges.emplace(old_start, prefix);
            if (suffix != 0) state_->free_ranges.emplace(suffix_start, suffix);
        } catch (...) {
            // Metadata split failure: restore a conservative free-range representation.
            state_->free_ranges.clear();
            if (state_->used <= state_->storage.size()) state_->free_ranges.emplace(0, state_->storage.size() - state_->used);
            return std::nullopt;
        }
        state_->used += bytes;
        if (state_->used > state_->high_water) state_->high_water = state_->used;
        return BufferLease(state_, aligned, bytes);
    }
    return std::nullopt;
}
MemoryPoolStats MemoryPool::stats() const noexcept { std::lock_guard lock(state_->mutex); return {state_->storage.size(), state_->used, state_->high_water}; }
}
