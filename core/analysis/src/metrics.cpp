#include "chronos/analysis/metrics.h"
#include "chronos/events/event.h"
#include <numeric>
#include <cmath>

namespace chronos {
namespace analysis {

static double shannon_entropy(const std::vector<u8>& data) {
    if (data.empty()) return 0.0;
    std::vector<u64> freq(256,0);
    for (u8 b : data) freq[b]++;
    double h = 0.0;
    double n = (double)data.size();
    for (int i=0;i<256;i++) {
        if (freq[i]==0) continue;
        double p = (double)freq[i]/n;
        h -= p * std::log2(p);
    }
    return h;
}

ExecutionMetrics MetricsCalculator::compute(const EventLog& log) {
    ExecutionMetrics m;
    auto snap = log.snapshot();
    m.total_events = snap.size();
    // events by type
    for (const auto& e : snap) m.events_by_type[event_type_name(e.type)]++;
    // syscall counts
    std::vector<i64> gaps;
    i64 prev_ts = -1;
    for (const auto& e : snap) {
        if (e.type == EventType::SyscallEnter) {
            auto s = std::get<SyscallEvent>(e.payload);
            m.syscall_counts[s.name]++;
        }
        i64 ts = e.timestamp.count();
        if (prev_ts >= 0) gaps.push_back(ts - prev_ts);
        prev_ts = ts;
    }
    m.unique_syscalls = m.syscall_counts.size();
    // gaps stats
    if (!gaps.empty()) {
        double sum = 0;
        for (auto g : gaps) sum += (double)g;
        m.mean_inter_event_gap_ns = sum / (double)gaps.size();
        double var = 0;
        for (auto g : gaps) {
            double d = (double)g - m.mean_inter_event_gap_ns;
            var += d*d;
        }
        m.std_inter_event_gap_ns = std::sqrt(var / (double)gaps.size());
    }
    // memory entropy over MemoryWrite events
    std::vector<u8> membytes;
    for (const auto& e : snap) {
        if (e.type == EventType::MemoryWrite) {
            auto me = std::get<MemoryEvent>(e.payload);
            membytes.insert(membytes.end(), me.old_bytes.begin(), me.old_bytes.end());
            membytes.insert(membytes.end(), me.new_bytes.begin(), me.new_bytes.end());
        }
    }
    m.memory_change_entropy = shannon_entropy(membytes);
    // events per second estimate from total time span
    if (snap.size() >= 2) {
        i64 span = snap.back().timestamp.count() - snap.front().timestamp.count();
        if (span > 0) m.events_per_second = (double)snap.size() / ((double)span / 1e9);
    }
    return m;
}

} }
