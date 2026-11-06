#include "chronos/memory/region.h"
#include <algorithm>
namespace chronos { namespace memory {

void MemoryMap::add_region(const MemoryRegion& r){ regions_.push_back(r); }
bool MemoryMap::remove_region(Address base){
    for (size_t i=0;i<regions_.size();++i) if (regions_[i].base==base){ regions_.erase(regions_.begin()+i); return true; }
    return false;
}
const MemoryRegion* MemoryMap::find(Address addr) const {
    for (const auto& r : regions_) if (addr>=r.base && addr<r.base+r.size) return &r;
    return nullptr;
}
std::vector<MemoryRegion> MemoryMap::regions() const { return regions_; }

std::vector<MemoryMap::ChangeRange> MemoryMap::diff(const MemoryMap& other) const {
    std::vector<ChangeRange> out;
    // for each region in this, find matching in other by base
    for (const auto& r : regions_) {
        // find other
        const MemoryRegion* o = nullptr;
        for (const auto& ro : other.regions_) if (ro.base==r.base) { o=&ro; break; }
        if (!o) continue;
        u64 n = std::min(r.size, o->size);
        u64 start=0; bool in=false; u64 run=0;
        for (u64 i=0;i<n;i++){
            u8 a = (i < r.data.size())? r.data[i]:0;
            u8 b = (i < o->data.size())? o->data[i]:0;
            if (a!=b) {
                if (!in){ start=i; in=true; run=1; }
                else run++;
            } else {
                if (in){ out.push_back({start,run}); in=false; }
            }
        }
        if (in) out.push_back({start,run});
    }
    return out;
}

} }
