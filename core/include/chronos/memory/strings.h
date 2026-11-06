#pragma once
#include "chronos/common/types.h"
#include <vector>
#include <string>

namespace chronos {
namespace memory {

// Extract printable ASCII strings (length >= min_len) from raw bytes.
std::vector<std::string> extract_strings(const std::vector<u8>& data, size_t min_len = 4);

} // namespace memory
} // namespace chronos
