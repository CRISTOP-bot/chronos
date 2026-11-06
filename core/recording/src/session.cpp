#include "chronos/recording/session.h"

namespace chronos {
namespace recording {

Session::Session(SessionConfig cfg, storage::RecordingMetadata meta)
    : cfg_(std::move(cfg)), meta_(std::move(meta)) {}

TimePoint Session::current_time() const {
    TimePoint tp;
    tp.sequence = log_.size();
    return tp;
}

EventId Session::record_event(Event e) {
    if (e.time.sequence == 0) e.time.sequence = log_.size() + 1;
    meta_.event_count = (u32)log_.size() + 1;
    return log_.append(std::move(e));
}

u64 Session::add_checkpoint(u64 time_sequence, const std::string& label, const std::vector<storage::CheckpointRegion>& regions) {
    storage::CheckpointRecord cp;
    cp.id = next_checkpoint_id_++;
    cp.time_sequence = time_sequence;
    cp.label = label;
    cp.regions = regions;
    checkpoints_.push_back(cp);
    // also record a Checkpoint event in the log
    Event ev; ev.type = EventType::Checkpoint; ev.time.sequence = time_sequence;
    CheckpointEvent ce; ce.checkpoint_id = cp.id; ce.label = label;
    ev.payload = ce;
    log_.append(ev);
    return cp.id;
}

Result<void> Session::save(const std::string& path) const {
    auto snap = log_.snapshot();
    storage::RecordingMetadata m = meta_;
    m.event_count = (u32)snap.size();
    return storage::Writer::write_recording(path, m, snap, checkpoints_);
}

} // namespace recording
} // namespace chronos
