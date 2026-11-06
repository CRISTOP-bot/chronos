#include "chronos/timeline/timeline.h"
namespace chronos { namespace timeline {

TimePoint Timeline::begin() const {
    TimePoint tp; tp.sequence = (log_ && log_->size()>0) ? 1 : 0; return tp;
}
TimePoint Timeline::end() const {
    TimePoint tp; tp.sequence = log_ ? log_->size() : 0; return tp;
}
TimePoint Timeline::next(TimePoint tp) const {
    if (!log_) return tp;
    if (tp.sequence >= log_->size()) return tp;
    TimePoint n; n.sequence = tp.sequence + 1; return n;
}
TimePoint Timeline::prev(TimePoint tp) const {
    if (tp.sequence <= 1) { TimePoint z; z.sequence=0; return z; }
    TimePoint p; p.sequence = tp.sequence - 1; return p;
}
bool Timeline::valid(TimePoint tp) const {
    if (!log_) return tp.sequence==0;
    return tp.sequence <= log_->size();
}
CheckpointId Timeline::add_checkpoint(TimePoint tp, const std::string& label) {
    Checkpoint cp; cp.id = next_cp_++; cp.time = tp; cp.label = label;
    checkpoints_.push_back(cp);
    return cp.id;
}
std::vector<Checkpoint> Timeline::checkpoints() const { return checkpoints_; }
std::optional<Checkpoint> Timeline::find_checkpoint(CheckpointId id) const {
    for (const auto& c : checkpoints_) if (c.id==id) return c;
    return std::nullopt;
}
std::vector<EventId> Timeline::find_events(EventType type) const {
    std::vector<EventId> out;
    if (!log_) return out;
    auto snap = log_->snapshot();
    for (const auto& e : snap) if (e.type==type) out.push_back(e.id);
    return out;
}
std::vector<EventId> Timeline::find_events(const std::string& text) const {
    std::vector<EventId> out;
    if (!log_) return out;
    auto snap = log_->snapshot();
    for (const auto& e : snap) {
        // crude: search in event type name and variable names
        std::string hay = event_type_name(e.type);
        if (e.type==EventType::VariableChange) {
            auto v = std::get<VariableEvent>(e.payload);
            hay += " " + v.name + " " + v.new_value;
        }
        if (e.type==EventType::SyscallEnter || e.type==EventType::SyscallExit) {
            auto s = std::get<SyscallEvent>(e.payload);
            hay += " " + s.name;
        }
        if (e.type==EventType::FunctionEnter || e.type==EventType::FunctionExit) {
            auto f = std::get<FunctionEvent>(e.payload);
            hay += " " + f.name;
        }
        if (hay.find(text) != std::string::npos) out.push_back(e.id);
    }
    return out;
}
void Timeline::add_marker(TimePoint tp, const std::string& label, const std::string& note) {
    markers_.push_back({tp,label,note});
}

} }
