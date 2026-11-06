#include "chronos/memory/checkpoint_restore.h"
#include <stdexcept>

namespace chronos {
namespace memory {

Result<MemoryMap> CheckpointRestorer::restore(const storage::CheckpointRecord& checkpoint) {
    MemoryMap mm;
    for (const auto& r : checkpoint.regions) {
        if (r.bytes.size() > r.size) {
            return Result<MemoryMap>(Error::make(ErrorCode::InvalidArgument, "captured bytes exceed region size"));
        }
        if (r.base > UINT64_MAX - r.size) { // overflow
            // if size 0, base+0 okay; but size>0 overflow
            if (r.size != 0) return Result<MemoryMap>(Error::make(ErrorCode::OutOfBounds, "region overflow"));
        }
        MemoryRegion reg;
        reg.base = r.base;
        reg.size = r.size;
        reg.name = r.name;
        reg.readable = r.readable != 0;
        reg.writable = r.writable != 0;
        reg.executable = r.executable != 0;
        reg.data = r.bytes;
        mm.add_region(reg);
    }
    return Result<MemoryMap>(mm);
}

} }
