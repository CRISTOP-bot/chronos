#pragma once
#include "chronos/common/types.h"
#include "chronos/events/event_log.h"
#include "chronos/storage/writer.h"
#include "chronos/common/result.h"
#include <string>
#include <vector>

namespace chronos {
namespace recording {

struct SessionConfig {
    std::string name;
    std::string target_path;
    int instrumentation_level = 1; // 0 minimal, 1 standard, 2 detailed
};

class Session {
public:
    Session(SessionConfig cfg, storage::RecordingMetadata meta);

    const SessionConfig& config() const { return cfg_; }
    storage::RecordingMetadata& metadata() { return meta_; }
    EventLog& log() { return log_; }
    const EventLog& log() const { return log_; }

    TimePoint current_time() const;

    EventId record_event(Event e);

    // checkpoints
    u64 add_checkpoint(u64 time_sequence, const std::string& label, const std::vector<storage::CheckpointRegion>& regions);
    const std::vector<storage::CheckpointRecord>& checkpoints() const { return checkpoints_; }

    // persist to disk
    Result<void> save(const std::string& path) const;

private:
    SessionConfig cfg_;
    storage::RecordingMetadata meta_;
    EventLog log_;
    std::vector<storage::CheckpointRecord> checkpoints_;
    u64 next_checkpoint_id_ = 1;
};

} // namespace recording
} // namespace chronos
