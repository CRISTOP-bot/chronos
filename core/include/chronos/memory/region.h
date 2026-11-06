#pragma once
#include "chronos/common/types.h"
#include <vector>
#include <string>
#include <map>

namespace chronos {
namespace memory {

struct MemoryRegion {
    Address base = 0;
    u64 size = 0;
    std::string name;
    std::vector<u8> data; // current bytes (if captured)
    bool readable = true;
    bool writable = true;
    bool executable = false;
};

class MemoryMap {
public:
    void add_region(const MemoryRegion& r);
    bool remove_region(Address base);
    const MemoryRegion* find(Address addr) const;
    std::vector<MemoryRegion> regions() const;
    // compare two regions by base address returns list of changed byte ranges
    struct ChangeRange { u64 offset; u64 size; };
    std::vector<ChangeRange> diff(const MemoryMap& other) const;

private:
    std::vector<MemoryRegion> regions_;
};

} // namespace memory
} // namespace chronos
