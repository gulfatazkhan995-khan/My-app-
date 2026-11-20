#include "faraziq/rendering/software_renderer.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace faraziq::rendering {
namespace {
[[nodiscard]] std::size_t channels(const media::PixelFormat format) noexcept {
    switch (format) {
    case media::PixelFormat::gray8: return 1;
    case media::PixelFormat::rgb8: return 3;
    case media::PixelFormat::rgba8: return 4;
    }
    return 0;
}
[[nodiscard]] std::uint8_t clamp_byte(const float value) noexcept {
    return static_cast<std::uint8_t>(std::clamp(value, 0.0F, 255.0F));
}
}

core::Status SoftwareRenderer::apply(const media::VideoFrame& source,
                                     media::VideoFrame& destination,
                                     const Adjustments& adjustments,
                                     const scheduler::CancellationToken* cancellation) {
    if (source.width() != destination.width() || source.height() != destination.height() ||
        source.format() != destination.format() || source.data() == nullptr || destination.data() == nullptr) {
        return core::Status::failure(core::ErrorCode::invalid_argument,
                                     "Renderer input and output frame layouts must match and be allocated");
    }
    if (!std::isfinite(adjustments.exposure_ev) || adjustments.exposure_ev < -16.0F ||
        adjustments.exposure_ev > 16.0F) {
        return core::Status::failure(core::ErrorCode::invalid_argument,
                                     "Exposure must be finite and between -16 and +16 EV");
    }
    const std::size_t count = channels(source.format());
    if (count == 0) return core::Status::failure(core::ErrorCode::invalid_argument, "Unsupported pixel format");
    const float multiplier = std::exp2(adjustments.exposure_ev);
    for (std::uint32_t y = 0; y < source.height(); ++y) {
        if (cancellation != nullptr && cancellation->is_cancelled()) {
            return core::Status::failure(core::ErrorCode::cancelled, "Rendering cancelled");
        }
        const auto* src = source.data() + static_cast<std::size_t>(y) * source.stride_bytes();
        auto* dst = destination.data() + static_cast<std::size_t>(y) * destination.stride_bytes();
        for (std::uint32_t x = 0; x < source.width(); ++x) {
            const std::size_t offset = static_cast<std::size_t>(x) * count;
            if (adjustments.grayscale && count >= 3) {
                const float gray = 0.2126F * static_cast<float>(std::to_integer<std::uint8_t>(src[offset])) +
                                   0.7152F * static_cast<float>(std::to_integer<std::uint8_t>(src[offset + 1])) +
                                   0.0722F * static_cast<float>(std::to_integer<std::uint8_t>(src[offset + 2]));
                const auto value = clamp_byte(adjustments.invert ? (255.0F - gray) * multiplier : gray * multiplier);
                dst[offset] = static_cast<std::byte>(value);
                dst[offset + 1] = static_cast<std::byte>(value);
                dst[offset + 2] = static_cast<std::byte>(value);
            } else {
                const std::size_t color_channels = count == 4 ? 3 : count;
                for (std::size_t c = 0; c < color_channels; ++c) {
                    const float input = static_cast<float>(std::to_integer<std::uint8_t>(src[offset + c]));
                    const float adjusted = adjustments.invert ? (255.0F - input) * multiplier : input * multiplier;
                    dst[offset + c] = static_cast<std::byte>(clamp_byte(adjusted));
                }
            }
            if (count == 4) dst[offset + 3] = src[offset + 3];
        }
    }
    return {};
}
} // namespace faraziq::rendering
