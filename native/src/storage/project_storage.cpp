#include "faraziq/storage/project_storage.hpp"

#include <atomic>
#include <fstream>
#include <limits>
#include <system_error>
#include <thread>

namespace faraziq::storage {
namespace {
[[nodiscard]] core::Status io_error(const char* message) {
    return core::Status::failure(core::ErrorCode::unavailable, message);
}
[[nodiscard]] bool is_within(const std::filesystem::path& root,
                             const std::filesystem::path& candidate) {
    auto root_it = root.begin();
    auto path_it = candidate.begin();
    for (; root_it != root.end(); ++root_it, ++path_it) {
        if (path_it == candidate.end() || *root_it != *path_it) return false;
    }
    return true;
}
}

ProjectStorage::ProjectStorage(std::filesystem::path project_root) {
    std::error_code error;
    std::filesystem::create_directories(project_root, error);
    if (error) {
        status_ = io_error("Unable to create project storage root");
        return;
    }
    root_ = std::filesystem::weakly_canonical(project_root, error);
    if (error || root_.empty()) {
        status_ = io_error("Unable to canonicalize project storage root");
        root_.clear();
    }
}

std::optional<std::filesystem::path> ProjectStorage::resolve(
    const std::filesystem::path& relative_path, const bool require_existing,
    core::Status& status) const {
    if (!status_) { status = status_; return std::nullopt; }
    if (relative_path.empty() || relative_path.is_absolute()) {
        status = core::Status::failure(core::ErrorCode::invalid_argument, "Storage path must be non-empty and relative");
        return std::nullopt;
    }
    for (const auto& component : relative_path) {
        if (component == ".." || component == ".") {
            status = core::Status::failure(core::ErrorCode::invalid_argument, "Storage path traversal is not allowed");
            return std::nullopt;
        }
    }
    std::error_code error;
    const auto candidate = root_ / relative_path;
    const auto inspected = require_existing ? std::filesystem::canonical(candidate, error)
                                            : std::filesystem::weakly_canonical(candidate.parent_path(), error) / candidate.filename();
    if (error || !is_within(root_, inspected)) {
        status = core::Status::failure(core::ErrorCode::invalid_argument, "Storage path escapes the configured root or cannot be resolved");
        return std::nullopt;
    }
    status = {};
    return inspected;
}

core::Status ProjectStorage::write_atomic(const std::filesystem::path& relative_path,
                                          const std::span<const std::byte> bytes) const {
    core::Status status;
    const auto target = resolve(relative_path, false, status);
    if (!target) return status;
    std::error_code error;
    std::filesystem::create_directories(target->parent_path(), error);
    if (error) return io_error("Unable to create project file directory");
    // Re-check after directory creation to reject parent symlinks introduced at that path.
    const auto safe_target = resolve(relative_path, false, status);
    if (!safe_target) return status;
    static std::atomic_uint64_t sequence{0};
    auto temporary = *safe_target;
    temporary += ".tmp." + std::to_string(sequence.fetch_add(1, std::memory_order_relaxed)) +
                 "." + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) return io_error("Unable to open temporary project file");
        if (!bytes.empty()) output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        output.flush();
        if (!output) {
            output.close();
            std::filesystem::remove(temporary, error);
            return io_error("Failed while writing project file");
        }
    }
    std::filesystem::rename(temporary, *safe_target, error);
    if (error) {
        std::filesystem::remove(temporary, error);
        return io_error("Atomic replacement of project file failed");
    }
    return {};
}

std::optional<std::vector<std::byte>> ProjectStorage::read(
    const std::filesystem::path& relative_path, const std::size_t maximum_bytes,
    core::Status& status) const {
    const auto target = resolve(relative_path, true, status);
    if (!target) return std::nullopt;
    std::error_code error;
    const auto size = std::filesystem::file_size(*target, error);
    if (error || size > maximum_bytes || size > std::numeric_limits<std::size_t>::max() ||
        size > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
        status = core::Status::failure(error ? core::ErrorCode::unavailable : core::ErrorCode::resource_exhausted,
                                      "Project file is unavailable or exceeds the configured read limit");
        return std::nullopt;
    }
    std::vector<std::byte> result(static_cast<std::size_t>(size));
    std::ifstream input(*target, std::ios::binary);
    if (!input || (!result.empty() && !input.read(reinterpret_cast<char*>(result.data()), static_cast<std::streamsize>(result.size())))) {
        status = io_error("Failed to read project file");
        return std::nullopt;
    }
    status = {};
    return result;
}
} // namespace faraziq::storage
