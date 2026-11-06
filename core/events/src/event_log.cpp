#include "chronos/events/event_log.h"

namespace chronos {

EventId EventLog::append(Event e) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (e.id == 0) e.id = next_id_++;
    else if (e.id >= next_id_) next_id_ = e.id + 1;
    if (e.time.sequence == 0) e.time.sequence = events_.size() + 1;
    events_.push_back(std::move(e));
    type_indices_[events_.back().type].push_back(events_.size() - 1);
    return events_.back().id;
}

size_t EventLog::size() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return events_.size();
}

std::optional<Event> EventLog::at(size_t index) const {
    std::lock_guard<std::mutex> lk(mtx_);
    if (index >= events_.size()) return std::nullopt;
    return events_[index];
}

std::optional<Event> EventLog::by_id(EventId id) const {
    std::lock_guard<std::mutex> lk(mtx_);
    for (const auto& e : events_) if (e.id == id) return e;
    return std::nullopt;
}

size_t EventLog::index_of(EventId id) const {
    std::lock_guard<std::mutex> lk(mtx_);
    for (size_t i = 0; i < events_.size(); ++i) if (events_[i].id == id) return i;
    return events_.size(); // npos-ish
}

std::vector<size_t> EventLog::indices_of_type(EventType type) const {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = type_indices_.find(type);
    if (it == type_indices_.end()) return {};
    return it->second;
}

std::vector<Event> EventLog::range(size_t from, size_t to) const {
    std::lock_guard<std::mutex> lk(mtx_);
    if (from > events_.size()) from = events_.size();
    if (to > events_.size()) to = events_.size();
    if (from >= to) return {};
    return std::vector<Event>(events_.begin() + from, events_.begin() + to);
}

void EventLog::clear() {
    std::lock_guard<std::mutex> lk(mtx_);
    events_.clear();
    type_indices_.clear();
}

std::vector<Event> EventLog::snapshot() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return events_;
}

} // namespace chronos
