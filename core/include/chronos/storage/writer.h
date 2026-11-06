#pragma once
#include "chronos/storage/format.h"
#include "chronos/events/event.h"
#include "chronos/common/result.h"
#include <string>
#include <vector>
#include <fstream>

namespace chronos {
namespace storage {

class Writer {
public:
    explicit Writer(const std::string& path);
    ~Writer();

    Result<void> write_header(const FileHeader& h);
    Result<void> write_metadata(const RecordingMetadata& m);
    Result<void> write_events(const std::vector<Event>& events);
    Result<void> write_footer(u32 crc);

    // High-level
    static Result<void> write_recording(const std::string& path,
                                        const RecordingMetadata& meta,
                                        const std::vector<Event>& events,
                                        const std::vector<CheckpointRecord>& checkpoints = {});

private:
    std::ofstream out_;
    std::string path_;
};

class Reader {
public:
    explicit Reader(const std::string& path);
    ~Reader();

    Result<FileHeader> read_header();
    Result<RecordingMetadata> read_metadata(const FileHeader& h);
    Result<std::vector<Event>> read_events(const FileHeader& h);
    Result<std::vector<CheckpointRecord>> read_checkpoints(const FileHeader& h);
    Result<bool> verify_integrity(const FileHeader& h);

    struct Recording {
        RecordingMetadata metadata;
        std::vector<Event> events;
        std::vector<CheckpointRecord> checkpoints;
    };
    static Result<Recording> read_recording(const std::string& path);

private:
    std::ifstream in_;
    std::string path_;
};

} // namespace storage
} // namespace chronos
