#include "faraziq/media/media_buffers.hpp"

#include <limits>

namespace faraziq::media {
namespace {
[[nodiscard]] std::size_t bytes_per_pixel(const PixelFormat format) noexcept {
    switch (format) {
    case PixelFormat::gray8: return 1;
    case PixelFormat::rgb8: return 3;
    case PixelFormat::rgba8: return 4;
    }
    return 0;
}
[[nodiscard]] std::size_t bytes_per_sample(const SampleFormat format) noexcept {
    switch (format) {
    case SampleFormat::signed_16: return sizeof(std::int16_t);
    case SampleFormat::float_32: return sizeof(float);
    }
    return 0;
}
[[nodiscard]] bool align_up(const std::size_t value, const std::size_t alignment,
                            std::size_t& result) noexcept {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) return false;
    const std::size_t remainder = value & (alignment - 1);
    const std::size_t padding = remainder == 0 ? 0 : alignment - remainder;
    if (value > std::numeric_limits<std::size_t>::max() - padding) return false;
    result = value + padding;
    return true;
}
}

VideoFrame::VideoFrame(const std::uint32_t width, const std::uint32_t height,
                       const PixelFormat format, const std::size_t stride,
                       memory::BufferLease storage) noexcept
    : width_(width), height_(height), format_(format), stride_(stride), storage_(std::move(storage)) {}

std::optional<VideoFrame> VideoFrame::allocate(memory::MemoryPool& pool,
                                                const std::uint32_t width,
                                                const std::uint32_t height,
                                                const PixelFormat format,
                                                const std::size_t row_alignment) {
    const auto pixel_size = bytes_per_pixel(format);
    if (width == 0 || height == 0 || pixel_size == 0 || row_alignment == 0 ||
        (row_alignment & (row_alignment - 1)) != 0) return std::nullopt;
    if (static_cast<std::size_t>(width) > std::numeric_limits<std::size_t>::max() / pixel_size) return std::nullopt;
    std::size_t stride = 0;
    if (!align_up(static_cast<std::size_t>(width) * pixel_size, row_alignment, stride) ||
        static_cast<std::size_t>(height) > std::numeric_limits<std::size_t>::max() / stride) return std::nullopt;
    auto buffer = pool.try_allocate(stride * static_cast<std::size_t>(height), row_alignment);
    if (!buffer) return std::nullopt;
    return VideoFrame(width, height, format, stride, std::move(*buffer));
}

AudioBuffer::AudioBuffer(const std::uint64_t frames, const std::uint32_t channels,
                         const SampleFormat format, memory::BufferLease storage) noexcept
    : frame_count_(frames), channels_(channels), format_(format), storage_(std::move(storage)) {}

std::optional<AudioBuffer> AudioBuffer::allocate(memory::MemoryPool& pool,
                                                  const std::uint64_t frame_count,
                                                  const std::uint32_t channel_count,
                                                  const SampleFormat format) {
    const auto sample_size = bytes_per_sample(format);
    if (frame_count == 0 || channel_count == 0 || sample_size == 0) return std::nullopt;
    constexpr auto max = std::numeric_limits<std::size_t>::max();
    if (frame_count > max / channel_count) return std::nullopt;
    const auto sample_count = static_cast<std::size_t>(frame_count) * channel_count;
    if (sample_count > max / sample_size) return std::nullopt;
    auto buffer = pool.try_allocate(sample_count * sample_size, sample_size);
    if (!buffer) return std::nullopt;
    return AudioBuffer(frame_count, channel_count, format, std::move(*buffer));
}

} // namespace faraziq::media
