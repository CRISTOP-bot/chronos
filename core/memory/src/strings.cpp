#include "chronos/memory/strings.h"

namespace chronos {
namespace memory {

std::vector<std::string> extract_strings(const std::vector<u8>& data, size_t min_len) {
    std::vector<std::string> out;
    std::string cur;
    for (u8 b : data) {
        if (b >= 32 && b < 127) {
            cur.push_back((char)b);
        } else {
            if (cur.size() >= min_len) out.push_back(cur);
            cur.clear();
        }
    }
    if (cur.size() >= min_len) out.push_back(cur);
    return out;
}

} }
