#pragma once

#include "faraziq/core/status.hpp"

#include <cstdint>
#include <map>
#include <shared_mutex>
#include <vector>

namespace faraziq::timeline {
using TrackId = std::uint64_t;
using ClipId = std::uint64_t;
using TimeNs = std::int64_t;

struct Clip {
    ClipId id{0};
    TrackId track_id{0};
    TimeNs timeline_start_ns{0};
    TimeNs source_start_ns{0};
    TimeNs duration_ns{0};
};

// Thread-safe timeline metadata store. Times are integer nanoseconds; clip overlap is allowed.
class Timeline final {
public:
    [[nodiscard]] core::Status create_track(TrackId id);
    [[nodiscard]] core::Status remove_track(TrackId id);
    [[nodiscard]] core::Status add_clip(const Clip& clip);
    [[nodiscard]] core::Status remove_clip(ClipId id);
    [[nodiscard]] core::Status move_clip(ClipId id, TimeNs new_start_ns);
    [[nodiscard]] std::vector<Clip> clips(TrackId id) const;
    [[nodiscard]] std::vector<TrackId> tracks() const;
    [[nodiscard]] TimeNs duration_ns() const noexcept;

private:
    mutable std::shared_mutex mutex_;
    std::map<TrackId, std::vector<Clip>> tracks_;
};

} // namespace faraziq::timeline
