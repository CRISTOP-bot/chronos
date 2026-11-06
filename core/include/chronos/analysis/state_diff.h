#pragma once
#include "chronos/common/types.h"
#include "chronos/events/event_log.h"
#include <vector>
#include <string>
#include <map>

namespace chronos {
namespace analysis {

struct VariableState {
    std::string name;
    std::string type_name;
    std::string value;
};

struct StateSnapshot {
    TimePoint time;
    std::map<std::string, VariableState> variables;
    // memory region hashes or bytes optional
};

struct StateDiffResult {
    std::vector<std::string> added_variables;
    std::vector<std::string> removed_variables;
    std::vector<std::pair<std::string,std::pair<std::string,std::string>>> changed_variables; // name, old, new
    u64 events_between = 0;
};

class StateDiffer {
public:
    // Build a snapshot by replaying events up to tp (variable changes only).
    static StateSnapshot build(const EventLog& log, TimePoint tp);
    static StateDiffResult diff(const StateSnapshot& a, const StateSnapshot& b, const EventLog& log);
};

} // namespace analysis
} // namespace chronos
