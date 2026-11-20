#pragma once

#include "faraziq/core/status.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace faraziq::storage {
// Restricts file access to a caller-provided private project directory. Paths must be
// relative and traversal/symlink escapes are rejected. The Android layer should pass
// an app-private directory; SAF document URIs need a separate ContentResolver adapter.
class ProjectStorage final {
public:
    explicit ProjectStorage(std::filesystem::path project_root);
    [[nodiscard]] const core::Status& initialization_status() const noexcept { return status_; }
    [[nodiscard]] core::Status write_atomic(const std::filesystem::path& relative_path,
                                            std::span<const std::byte> bytes) const;
    [[nodiscard]] std::optional<std::vector<std::byte>> read(
        const std::filesystem::path& relative_path, std::size_t maximum_bytes,
        core::Status& status) const;
private:
    [[nodiscard]] std::optional<std::filesystem::path> resolve(
        const std::filesystem::path& relative_path, bool require_existing,
        core::Status& status) const;
    std::filesystem::path root_;
    core::Status status_;
};
} // namespace faraziq::storage
