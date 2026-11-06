#pragma once
#include "chronos/common/types.h"
#include "chronos/common/result.h"
#include <string>
#include <map>
#include <memory>

namespace chronos {
namespace execution {

class SessionManager {
public:
    // Register a process under management (metadata only).
    Result<void> register_process(ProcessId pid, const std::string& name);
    bool has_process(ProcessId pid) const;
    std::vector<ProcessId> processes() const;

private:
    std::map<ProcessId, std::string> processes_;
};

} // namespace execution
} // namespace chronos
