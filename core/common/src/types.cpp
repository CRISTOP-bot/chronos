#include "chronos/common/types.h"
#include "chronos/events/event.h"
#include <cstring>

namespace chronos {

const char* event_type_name(EventType t) {
    switch (t) {
        case EventType::Unknown: return "Unknown";
        case EventType::ProcessStart: return "ProcessStart";
        case EventType::ProcessExit: return "ProcessExit";
        case EventType::ThreadCreate: return "ThreadCreate";
        case EventType::ThreadExit: return "ThreadExit";
        case EventType::FunctionEnter: return "FunctionEnter";
        case EventType::FunctionExit: return "FunctionExit";
        case EventType::VariableChange: return "VariableChange";
        case EventType::MemoryWrite: return "MemoryWrite";
        case EventType::MemoryAllocate: return "MemoryAllocate";
        case EventType::MemoryFree: return "MemoryFree";
        case EventType::ExceptionThrown: return "ExceptionThrown";
        case EventType::ExceptionCaught: return "ExceptionCaught";
        case EventType::SyscallEnter: return "SyscallEnter";
        case EventType::SyscallExit: return "SyscallExit";
        case EventType::ExternalInput: return "ExternalInput";
        case EventType::Checkpoint: return "Checkpoint";
        case EventType::Marker: return "Marker";
        case EventType::Custom: return "Custom";
    }
    return "Unknown";
}

} // namespace chronos
