#pragma once
#include "chronos/common/types.h"
#include <variant>
#include <string>
#include <vector>
#include <map>

namespace chronos {

enum class EventType : u8 {
    Unknown = 0,
    ProcessStart,
    ProcessExit,
    ThreadCreate,
    ThreadExit,
    FunctionEnter,
    FunctionExit,
    VariableChange,
    MemoryWrite,
    MemoryAllocate,
    MemoryFree,
    ExceptionThrown,
    ExceptionCaught,
    SyscallEnter,
    SyscallExit,
    ExternalInput,
    Checkpoint,
    Marker,
    Custom,
};

const char* event_type_name(EventType t);

// Payload variants
struct FunctionEvent {
    std::string name;
    Address entry_address = 0;
    std::vector<std::string> args; // stringified for early phases
};

struct VariableEvent {
    std::string name;
    std::string type_name;
    std::string old_value;
    std::string new_value;
    Address address = 0;
};

struct MemoryEvent {
    Address address = 0;
    u64 size = 0;
    std::vector<u8> old_bytes;
    std::vector<u8> new_bytes;
};

struct ExceptionEvent {
    std::string type_name;
    std::string message;
    Address throw_address = 0;
};

struct SyscallEvent {
    std::string name;
    i64 number = 0;
    std::vector<std::string> args;
    std::string result;
};

struct ProcessEvent {
    ProcessId pid = 0;
    ThreadId tid = 0;
    std::string detail;
};

struct CheckpointEvent {
    CheckpointId checkpoint_id = 0;
    std::string label;
};

struct MarkerEvent {
    std::string label;
    std::string note;
};

struct CustomEvent {
    std::string kind;
    std::string payload;
};

using EventPayload = std::variant<
    std::monostate,
    FunctionEvent,
    VariableEvent,
    MemoryEvent,
    ExceptionEvent,
    SyscallEvent,
    ProcessEvent,
    CheckpointEvent,
    MarkerEvent,
    CustomEvent
>;

struct Event {
    EventId id = 0;
    TimePoint time;
    EventType type = EventType::Unknown;
    ProcessId pid = 0;
    ThreadId tid = 0;
    Timestamp timestamp{};
    EventPayload payload{};
    // optional source location
    std::optional<SourceLocation> location;
};

} // namespace chronos
