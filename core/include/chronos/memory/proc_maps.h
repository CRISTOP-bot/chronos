#pragma once
#include "chronos/memory/region.h"
#include "chronos/common/result.h"
#include <string>

namespace chronos {
namespace memory {

// Read /proc/<pid>/maps into a MemoryMap. Does not read memory bytes.
Result<MemoryMap> read_proc_maps(int pid);

// Read bytes from /proc/<pid>/mem for a region (best-effort).
Result<std::vector<u8>> read_proc_mem(int pid, Address addr, u64 size);

} // namespace memory
} // namespace chronos
