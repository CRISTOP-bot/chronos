#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <compare>

namespace chronos {

using u8  = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8  = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using Timestamp = std::chrono::nanoseconds;
using Address   = u64;
using ThreadId  = u64;
using ProcessId = u32;

// Unique id for an event in a recording.
using EventId = u64;

// Unique id for a checkpoint.
using CheckpointId = u64;

// A temporal position: index into the global event log.
struct TimePoint {
    u64 sequence = 0; // monotonic event sequence number
    // ordering
    auto operator<=>(const TimePoint&) const = default;
    bool operator==(const TimePoint&) const = default;
};

struct SourceLocation {
    std::string file;
    u32 line = 0;
    u32 column = 0;
};

} // namespace chronos
