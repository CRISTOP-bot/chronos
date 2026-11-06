#include "chronos/memory/strings.h"

namespace chronos {
namespace memory {

std::vector<std::string> extract_strings(const std::vector<u8>& data, size_t min_len) {
    std::vector<std::string> out;
    std::string cur;
    cur.reserve(64);

    for (size_t i = 0; i < data.size(); ++i) {
        const u8 b = data[i];

        // Printable ASCII
        if (b >= 32 && b < 127) {
            cur.push_back(static_cast<char>(b));
            continue;
        }

        // Accept tab and newline as printable within strings
        if (b == '\t' || b == '\n' || b == '\r') {
            if (!cur.empty()) cur.push_back(static_cast<char>(b));
            continue;
        }

        // Basic UTF-8 multi-byte sequences (2–4 bytes) — accept them
        // to avoid breaking strings containing non-ASCII identifiers/paths
        if ((b & 0xE0) == 0xC0 && i + 1 < data.size() && (data[i+1] & 0xC0) == 0x80) {
            cur.push_back(static_cast<char>(b));
            cur.push_back(static_cast<char>(data[++i]));
            continue;
        }
        if ((b & 0xF0) == 0xE0 && i + 2 < data.size() &&
            (data[i+1] & 0xC0) == 0x80 && (data[i+2] & 0xC0) == 0x80) {
            cur.push_back(static_cast<char>(b));
            cur.push_back(static_cast<char>(data[++i]));
            cur.push_back(static_cast<char>(data[++i]));
            continue;
        }
        if ((b & 0xF8) == 0xF0 && i + 3 < data.size() &&
            (data[i+1] & 0xC0) == 0x80 && (data[i+2] & 0xC0) == 0x80 &&
            (data[i+3] & 0xC0) == 0x80) {
            cur.push_back(static_cast<char>(b));
            cur.push_back(static_cast<char>(data[++i]));
            cur.push_back(static_cast<char>(data[++i]));
            cur.push_back(static_cast<char>(data[++i]));
            continue;
        }

        // Non-printable byte — flush current string if long enough
        if (cur.size() >= min_len) out.push_back(cur);
        cur.clear();
    }
    if (cur.size() >= min_len) out.push_back(cur);
    return out;
}

} // namespace memory
} // namespace chronos
