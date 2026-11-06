#pragma once
#include "chronos/common/types.h"
#include <cstdint>
#include <string>
#include <vector>

namespace chronos {
namespace storage {

constexpr char MAGIC[4] = {'C','H','R','N'};
constexpr u32 FORMAT_VERSION = 2;

enum class SectionType : u32 {
    Header = 1,
    Metadata = 2,
    Events = 3,
    Checkpoints = 4,
    Index = 5,
    Footer = 6,
};

struct FileHeader {
    char magic[4] = {'C','H','R','N'};
    u32 version = FORMAT_VERSION;
    u32 flags = 0;
    u64 created_unix = 0;
    // offsets
    u64 metadata_offset = 0;
    u64 metadata_size = 0;
    u64 events_offset = 0;
    u64 events_size = 0;
    u64 checkpoints_offset = 0;
    u64 checkpoints_size = 0;
    u64 index_offset = 0;
    u64 index_size = 0;
    u64 footer_offset = 0;
    u32 header_crc = 0;
};

struct RecordingMetadata {
    std::string name;
    std::string target_path;
    std::string created_by = "chronos";
    u32 event_count = 0;
    u64 duration_ns = 0;
};

struct CheckpointRegion {
    u64 base = 0;
    u64 size = 0;
    std::string name;
    u8 readable = 1;
    u8 writable = 0;
    u8 executable = 0;
    std::vector<u8> bytes;
};

struct CheckpointRecord {
    u64 id = 0;
    u64 time_sequence = 0;
    std::string label;
    std::vector<CheckpointRegion> regions;
};

} // namespace storage
} // namespace chronos
