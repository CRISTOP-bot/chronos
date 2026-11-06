#include "chronos/memory/checkpoint_restore.h"
#include <limits>

namespace chronos::memory {

Result<MemoryMap> CheckpointRestorer::restore(const storage::CheckpointRecord& checkpoint) {
    MemoryMap restored;
    for (const auto& captured : checkpoint.regions) {
        if (captured.bytes.size() > captured.size) {
            return Error::make(ErrorCode::CorruptData, "checkpoint bytes exceed declared region size", captured.name);
        }
        if (captured.size > std::numeric_limits<Address>::max() - captured.base) {
            return Error::make(ErrorCode::CorruptData, "checkpoint region address overflows", captured.name);
        }
        MemoryRegion region;
        region.base = captured.base;
        region.size = captured.size;
        region.name = captured.name;
        region.data = captured.bytes;
        region.readable = captured.readable != 0;
        region.writable = captured.writable != 0;
        region.executable = captured.executable != 0;
        restored.add_region(region);
    }
    return restored;
}

} // namespace chronos::memory
