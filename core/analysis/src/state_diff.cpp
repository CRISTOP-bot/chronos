#include "chronos/analysis/state_diff.h"
namespace chronos { namespace analysis {

StateSnapshot StateDiffer::build(const EventLog& log, TimePoint tp){
    StateSnapshot snap; snap.time = tp;
    auto events = log.snapshot();
    for (const auto& e : events) {
        if (e.time.sequence > tp.sequence) break;
        if (e.type == EventType::VariableChange) {
            auto v = std::get<VariableEvent>(e.payload);
            VariableState vs; vs.name=v.name; vs.type_name=v.type_name; vs.value=v.new_value;
            snap.variables[v.name] = vs;
        }
    }
    return snap;
}

StateDiffResult StateDiffer::diff(const StateSnapshot& a, const StateSnapshot& b, const EventLog& log){
    StateDiffResult r;
    // added/removed
    for (const auto& kv : b.variables) {
        if (a.variables.find(kv.first)==a.variables.end()) r.added_variables.push_back(kv.first);
        else {
            if (a.variables.at(kv.first).value != kv.second.value)
                r.changed_variables.push_back({kv.first, {a.variables.at(kv.first).value, kv.second.value}});
        }
    }
    for (const auto& kv : a.variables) {
        if (b.variables.find(kv.first)==b.variables.end()) r.removed_variables.push_back(kv.first);
    }
    // events between
    u64 lo = a.time.sequence, hi = b.time.sequence;
    if (hi < lo) std::swap(lo,hi);
    r.events_between = (hi>lo)? (hi-lo) : 0;
    (void)log;
    return r;
}

} }
