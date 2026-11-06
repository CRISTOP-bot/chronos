#pragma once
#include "chronos/memory/region.h"
#include "chronos/storage/format.h"
#include "chronos/common/result.h"

namespace chronos {
namespace memory {

class CheckpointRestorer {
public:
    static Result<MemoryMap> restore(const storage::CheckpointRecord& checkpoint);
};

} // namespace memory
} // namespace chronos
