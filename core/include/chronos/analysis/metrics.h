#pragma once
#include "chronos/common/types.h"
#include "chronos/events/event_log.h"
#include <map>
#include <string>
#include <vector>
#include <cmath>

namespace chronos {
namespace analysis {

struct SyscallStat {
    std::string name;
    u64 count = 0;
    double mean_gap_ns = 0; // mean time between consecutive calls (if timestamps)
};

struct ExecutionMetrics {
    u64 total_events = 0;
    std::map<std::string, u64> events_by_type;
    std::map<std::string, u64> syscall_counts;
    double events_per_second = 0;
    double mean_inter_event_gap_ns = 0;
    double std_inter_event_gap_ns = 0;
    // memory change entropy (Shannon) across checkpoints, 0..8 bits/byte
    double memory_change_entropy = 0;
    u64 unique_syscalls = 0;
};

class MetricsCalculator {
public:
    static ExecutionMetrics compute(const EventLog& log);
};

} // namespace analysis
} // namespace chronos
