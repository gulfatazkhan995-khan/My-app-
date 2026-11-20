#pragma once
#include <string>
#include <utility>
namespace faraziq::core {
enum class ErrorCode { ok = 0, invalid_argument, resource_exhausted, unavailable, cancelled, shutting_down, internal_error };
class Status final {
public:
    Status() = default;
    [[nodiscard]] static Status failure(ErrorCode code, std::string message) { return Status(code, std::move(message)); }
    [[nodiscard]] bool ok() const noexcept { return code_ == ErrorCode::ok; }
    [[nodiscard]] ErrorCode code() const noexcept { return code_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    explicit operator bool() const noexcept { return ok(); }
private:
    Status(ErrorCode code, std::string message) : code_(code), message_(std::move(message)) {}
    ErrorCode code_{ErrorCode::ok};
    std::string message_;
};
}
