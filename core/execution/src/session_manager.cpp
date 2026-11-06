#include "chronos/execution/session_manager.h"
namespace chronos { namespace execution {
Result<void> SessionManager::register_process(ProcessId pid, const std::string& name){
    if (processes_.count(pid)) return Result<void>(Error::make(ErrorCode::AlreadyExists,"pid registered"));
    processes_[pid]=name;
    return Result<void>();
}
bool SessionManager::has_process(ProcessId pid) const { return processes_.count(pid)>0; }
std::vector<ProcessId> SessionManager::processes() const {
    std::vector<ProcessId> v;
    for (auto& kv : processes_) v.push_back(kv.first);
    return v;
}
} }
