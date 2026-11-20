#pragma once

#include "faraziq/core/status.hpp"
#include "faraziq/media/media_buffers.hpp"
#include "faraziq/scheduler/task_scheduler.hpp"

#include <cstdint>

namespace faraziq::rendering {
struct Adjustments {
    // Exposure is an EV offset; 0 leaves pixel values unchanged.
    float exposure_ev{0.0F};
    bool invert{false};
    bool grayscale{false};
};

// Deterministic CPU renderer. It validates formats and processes each active pixel,
// respecting row stride and preserving the alpha channel for RGBA frames.
class SoftwareRenderer final {
public:
    [[nodiscard]] static core::Status apply(const media::VideoFrame& source,
                                            media::VideoFrame& destination,
                                            const Adjustments& adjustments,
                                            const scheduler::CancellationToken* cancellation = nullptr);
};
} // namespace faraziq::rendering
