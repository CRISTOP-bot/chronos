#pragma once

#include "chronos/common/result.h"
#include "chronos/memory/region.h"
#include "chronos/storage/format.h"

namespace chronos::memory {

// Recreates the captured bytes in an in-memory map. This never writes to a process.
class CheckpointRestorer {
public:
    static Result<MemoryMap> restore(const storage::CheckpointRecord& checkpoint);
};

} // namespace chronos::memory
