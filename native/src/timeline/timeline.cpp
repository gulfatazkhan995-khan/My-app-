#include "faraziq/timeline/timeline.hpp"

#include <algorithm>
#include <limits>
#include <mutex>

namespace faraziq::timeline {
namespace {
[[nodiscard]] core::Status invalid(const char* message) {
    return core::Status::failure(core::ErrorCode::invalid_argument, message);
}
[[nodiscard]] bool valid_clip(const Clip& clip) noexcept {
    return clip.id != 0 && clip.track_id != 0 && clip.timeline_start_ns >= 0 &&
           clip.source_start_ns >= 0 && clip.duration_ns > 0 &&
           clip.timeline_start_ns <= std::numeric_limits<TimeNs>::max() - clip.duration_ns &&
           clip.source_start_ns <= std::numeric_limits<TimeNs>::max() - clip.duration_ns;
}
}
core::Status Timeline::create_track(const TrackId id) {
    if (id == 0) return invalid("Track ID zero is reserved");
    std::unique_lock lock(mutex_);
    if (!tracks_.try_emplace(id).second) return invalid("Track already exists");
    return {};
}
core::Status Timeline::remove_track(const TrackId id) {
    std::unique_lock lock(mutex_);
    if (tracks_.erase(id) == 0) return core::Status::failure(core::ErrorCode::unavailable, "Track does not exist");
    return {};
}
core::Status Timeline::add_clip(const Clip& clip) {
    if (!valid_clip(clip)) return invalid("Clip metadata is invalid or its end time overflows");
    std::unique_lock lock(mutex_);
    auto track = tracks_.find(clip.track_id);
    if (track == tracks_.end()) return core::Status::failure(core::ErrorCode::unavailable, "Target track does not exist");
    for (const auto& [unused, clips_on_track] : tracks_) {
        (void)unused;
        if (std::any_of(clips_on_track.begin(), clips_on_track.end(), [&clip](const Clip& current) { return current.id == clip.id; }))
            return invalid("Clip ID already exists");
    }
    track->second.push_back(clip);
    std::sort(track->second.begin(), track->second.end(), [](const Clip& a, const Clip& b) {
        return a.timeline_start_ns == b.timeline_start_ns ? a.id < b.id : a.timeline_start_ns < b.timeline_start_ns;
    });
    return {};
}
core::Status Timeline::remove_clip(const ClipId id) {
    std::unique_lock lock(mutex_);
    for (auto& [unused, items] : tracks_) {
        (void)unused;
        const auto it = std::find_if(items.begin(), items.end(), [id](const Clip& clip) { return clip.id == id; });
        if (it != items.end()) { items.erase(it); return {}; }
    }
    return core::Status::failure(core::ErrorCode::unavailable, "Clip does not exist");
}
core::Status Timeline::move_clip(const ClipId id, const TimeNs start) {
    if (start < 0) return invalid("Clip start must be non-negative");
    std::unique_lock lock(mutex_);
    for (auto& [unused, items] : tracks_) {
        (void)unused;
        const auto it = std::find_if(items.begin(), items.end(), [id](const Clip& clip) { return clip.id == id; });
        if (it == items.end()) continue;
        if (start > std::numeric_limits<TimeNs>::max() - it->duration_ns) return invalid("Clip end time overflows");
        it->timeline_start_ns = start;
        std::sort(items.begin(), items.end(), [](const Clip& a, const Clip& b) {
            return a.timeline_start_ns == b.timeline_start_ns ? a.id < b.id : a.timeline_start_ns < b.timeline_start_ns;
        });
        return {};
    }
    return core::Status::failure(core::ErrorCode::unavailable, "Clip does not exist");
}
std::vector<Clip> Timeline::clips(const TrackId id) const {
    std::shared_lock lock(mutex_);
    const auto found = tracks_.find(id);
    return found == tracks_.end() ? std::vector<Clip>{} : found->second;
}
std::vector<TrackId> Timeline::tracks() const {
    std::shared_lock lock(mutex_);
    std::vector<TrackId> result;
    result.reserve(tracks_.size());
    for (const auto& [id, unused] : tracks_) { (void)unused; result.push_back(id); }
    return result;
}
TimeNs Timeline::duration_ns() const noexcept {
    std::shared_lock lock(mutex_);
    TimeNs end = 0;
    for (const auto& [unused, items] : tracks_) {
        (void)unused;
        for (const auto& clip : items) end = std::max(end, clip.timeline_start_ns + clip.duration_ns);
    }
    return end;
}
} // namespace faraziq::timeline
