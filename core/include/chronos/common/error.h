#pragma once
#include <string>
#include <cstdint>

namespace chronos {

enum class ErrorCode : std::uint32_t {
    Ok = 0,
    Unknown,
    IoError,
    CorruptData,
    InvalidFormat,
    UnsupportedVersion,
    OutOfBounds,
    NotFound,
    InvalidArgument,
    AlreadyExists,
    ResourceExhausted,
    InconsistentState,
};

struct Error {
    ErrorCode code = ErrorCode::Ok;
    std::string message;
    // context
    std::string context;

    bool ok() const { return code == ErrorCode::Ok; }
    static Error none() { return {}; }
    static Error make(ErrorCode c, std::string msg, std::string ctx = {}) {
        return Error{c, std::move(msg), std::move(ctx)};
    }
};

} // namespace chronos
