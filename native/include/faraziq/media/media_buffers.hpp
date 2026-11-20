#pragma once

#include "faraziq/core/status.hpp"
#include "faraziq/memory/memory_pool.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace faraziq::media {

enum class PixelFormat : std::uint8_t { gray8, rgb8, rgba8 };
enum class SampleFormat : std::uint8_t { signed_16, float_32 };

class VideoFrame final {
public:
    VideoFrame(const VideoFrame&) = delete;
    VideoFrame& operator=(const VideoFrame&) = delete;
    VideoFrame(VideoFrame&&) noexcept = default;
    VideoFrame& operator=(VideoFrame&&) noexcept = default;

    [[nodiscard]] static std::optional<VideoFrame> allocate(
        memory::MemoryPool& pool, std::uint32_t width, std::uint32_t height,
        PixelFormat format, std::size_t row_alignment = 1);
    [[nodiscard]] std::uint32_t width() const noexcept { return width_; }
    [[nodiscard]] std::uint32_t height() const noexcept { return height_; }
    [[nodiscard]] std::size_t stride_bytes() const noexcept { return stride_; }
    [[nodiscard]] PixelFormat format() const noexcept { return format_; }
    [[nodiscard]] std::byte* data() noexcept { return storage_.data(); }
    [[nodiscard]] const std::byte* data() const noexcept { return storage_.data(); }
    [[nodiscard]] std::size_t byte_size() const noexcept { return storage_.size(); }

private:
    VideoFrame(std::uint32_t width, std::uint32_t height, PixelFormat format,
               std::size_t stride, memory::BufferLease storage) noexcept;
    std::uint32_t width_;
    std::uint32_t height_;
    PixelFormat format_;
    std::size_t stride_;
    memory::BufferLease storage_;
};

class AudioBuffer final {
public:
    AudioBuffer(const AudioBuffer&) = delete;
    AudioBuffer& operator=(const AudioBuffer&) = delete;
    AudioBuffer(AudioBuffer&&) noexcept = default;
    AudioBuffer& operator=(AudioBuffer&&) noexcept = default;

    [[nodiscard]] static std::optional<AudioBuffer> allocate(
        memory::MemoryPool& pool, std::uint64_t frame_count,
        std::uint32_t channel_count, SampleFormat format);
    [[nodiscard]] std::uint64_t frame_count() const noexcept { return frame_count_; }
    [[nodiscard]] std::uint32_t channel_count() const noexcept { return channels_; }
    [[nodiscard]] SampleFormat format() const noexcept { return format_; }
    [[nodiscard]] std::byte* data() noexcept { return storage_.data(); }
    [[nodiscard]] const std::byte* data() const noexcept { return storage_.data(); }
    [[nodiscard]] std::size_t byte_size() const noexcept { return storage_.size(); }

private:
    AudioBuffer(std::uint64_t frames, std::uint32_t channels, SampleFormat format,
                memory::BufferLease storage) noexcept;
    std::uint64_t frame_count_;
    std::uint32_t channels_;
    SampleFormat format_;
    memory::BufferLease storage_;
};

} // namespace faraziq::media
