#pragma once
#include "chronos/events/event.h"
#include "chronos/common/result.h"
#include <vector>
#include <mutex>
#include <optional>
#include <unordered_map>

namespace chronos {

class EventLog {
public:
    EventLog() = default;

    // Append an event; assigns sequence/time if needed. Returns assigned EventId.
    EventId append(Event e);

    // Number of events.
    size_t size() const;

    // Access by index (0-based) in chronological order.
    std::optional<Event> at(size_t index) const;
    std::optional<Event> by_id(EventId id) const;

    // Find index of event with given id, or npos.
    size_t index_of(EventId id) const;

    // Indices of events of a given type (in chronological order).
    std::vector<size_t> indices_of_type(EventType type) const;

    // Range [from, to) by sequence index.
    std::vector<Event> range(size_t from, size_t to) const;

    // Clear all.
    void clear();

    // Thread-safe snapshot of all events (copy).
    std::vector<Event> snapshot() const;

private:
    mutable std::mutex mtx_;
    std::vector<Event> events_;
    std::unordered_map<EventType, std::vector<size_t>> type_indices_;
    EventId next_id_ = 1;
};

} // namespace chronos
