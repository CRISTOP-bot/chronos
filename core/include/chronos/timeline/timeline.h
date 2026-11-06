#pragma once
#include "chronos/common/types.h"
#include "chronos/events/event_log.h"
#include <vector>
#include <string>
#include <optional>

namespace chronos {
namespace timeline {

struct Checkpoint {
    CheckpointId id = 0;
    TimePoint time;
    std::string label;
};

class Timeline {
public:
    Timeline() = default;
    explicit Timeline(const EventLog* log) : log_(log) {}

    void set_log(const EventLog* log) { log_ = log; }

    // navigation
    TimePoint begin() const;
    TimePoint end() const;
    TimePoint next(TimePoint tp) const;
    TimePoint prev(TimePoint tp) const;
    bool valid(TimePoint tp) const;
    // Returns the event at a 1-based event sequence, or null for invalid positions.
    std::optional<Event> seek(TimePoint tp) const;

    // checkpoints
    CheckpointId add_checkpoint(TimePoint tp, const std::string& label);
    std::vector<Checkpoint> checkpoints() const;
    std::optional<Checkpoint> find_checkpoint(CheckpointId id) const;

    // search events by type
    std::vector<EventId> find_events(EventType type) const;
    std::vector<EventId> find_events(const std::string& text) const;

    // markers
    void add_marker(TimePoint tp, const std::string& label, const std::string& note);

private:
    const EventLog* log_ = nullptr;
    std::vector<Checkpoint> checkpoints_;
    struct Marker { TimePoint tp; std::string label; std::string note; };
    std::vector<Marker> markers_;
    CheckpointId next_cp_ = 1;
};

} // namespace timeline
} // namespace chronos
