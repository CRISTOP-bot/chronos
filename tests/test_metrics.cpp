#include <gtest/gtest.h>
#include "chronos/analysis/metrics.h"
#include "chronos/events/event_log.h"
#include "chronos/events/event.h"
#include "chronos/common/types.h"
#include <chrono>

using namespace chronos;
using namespace chronos::analysis;

static Event makeSyscall(const std::string& name, long nr, bool enter,
                          i64 timestamp_ns = 0) {
    Event e;
    e.type = enter ? EventType::SyscallEnter : EventType::SyscallExit;
    e.pid = 1; e.tid = 1;
    e.timestamp = Timestamp(timestamp_ns);
    SyscallEvent s; s.name = name; s.number = nr; s.result = enter ? "" : "0x0";
    e.payload = s;
    return e;
}

TEST(Metrics, TotalEvents) {
    EventLog log;
    for (int i = 0; i < 10; ++i) log.append(makeSyscall("read", 0, true, i * 100));
    auto m = MetricsCalculator::compute(log);
    EXPECT_EQ(m.total_events, 10u);
}

TEST(Metrics, SyscallCounts) {
    EventLog log;
    log.append(makeSyscall("read",  0, true));
    log.append(makeSyscall("read",  0, true));
    log.append(makeSyscall("write", 1, true));
    auto m = MetricsCalculator::compute(log);
    EXPECT_EQ(m.syscall_counts["read"],  2u);
    EXPECT_EQ(m.syscall_counts["write"], 1u);
    EXPECT_EQ(m.unique_syscalls, 2u);
}

TEST(Metrics, EventsByType) {
    EventLog log;
    log.append(makeSyscall("read", 0, true));  // SyscallEnter
    log.append(makeSyscall("read", 0, false)); // SyscallExit
    Event e2; e2.type = EventType::Marker;
    MarkerEvent me; me.label = "m"; e2.payload = me;
    log.append(e2);
    auto m = MetricsCalculator::compute(log);
    EXPECT_EQ(m.events_by_type["SyscallEnter"], 1u);
    EXPECT_EQ(m.events_by_type["SyscallExit"],  1u);
    EXPECT_EQ(m.events_by_type["Marker"],        1u);
}

TEST(Metrics, MeanGapMonotonic) {
    EventLog log;
    // 5 events spaced 1000 ns apart
    for (int i = 0; i < 5; ++i)
        log.append(makeSyscall("read", 0, true, i * 1000LL));
    auto m = MetricsCalculator::compute(log);
    // 4 gaps of 1000 ns each → mean = 1000
    EXPECT_NEAR(m.mean_inter_event_gap_ns, 1000.0, 1.0);
    EXPECT_NEAR(m.std_inter_event_gap_ns,  0.0,    1.0);
}

TEST(Metrics, NegativeGapsFiltered) {
    // Non-monotonic timestamps should not corrupt mean/std
    EventLog log;
    log.append(makeSyscall("read", 0, true, 5000));
    log.append(makeSyscall("read", 0, true, 3000)); // negative gap -2000
    log.append(makeSyscall("read", 0, true, 7000)); // positive gap +4000
    auto m = MetricsCalculator::compute(log);
    // Only the +4000 gap should be counted
    EXPECT_NEAR(m.mean_inter_event_gap_ns, 4000.0, 1.0);
}

TEST(Metrics, EmptyLog) {
    EventLog log;
    auto m = MetricsCalculator::compute(log);
    EXPECT_EQ(m.total_events, 0u);
    EXPECT_DOUBLE_EQ(m.mean_inter_event_gap_ns, 0.0);
    EXPECT_DOUBLE_EQ(m.events_per_second, 0.0);
}

TEST(Metrics, EventsPerSecond) {
    EventLog log;
    // 10 events over 1 second (1e9 ns)
    for (int i = 0; i < 10; ++i)
        log.append(makeSyscall("read", 0, true, (i64)(i * 1e9 / 9)));
    auto m = MetricsCalculator::compute(log);
    EXPECT_NEAR(m.events_per_second, 9.0, 1.0);
}
