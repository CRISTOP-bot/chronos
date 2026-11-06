#pragma once
#include "chronos/common/types.h"
#include "chronos/common/result.h"
#include "chronos/recording/session.h"
#include <string>
#include <vector>

namespace chronos {
namespace instrumentation {

struct TraceConfig {
    std::string program;
    std::vector<std::string> args;
    std::string output_path = "trace.chronos";
};

class PtraceTracer {
public:
    // Launch program under ptrace and record syscalls into session. Returns event count.
    Result<u64> trace(TraceConfig cfg, recording::Session& session);
};

} // namespace instrumentation
} // namespace chronos
