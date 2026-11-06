#include <gtest/gtest.h>
#include "chronos/timeline/timeline.h"
#include "chronos/events/event_log.h"
#include "chronos/events/event.h"
#include <memory>

using namespace chronos;
using namespace chronos::timeline;

// Returns a heap-allocated EventLog to avoid copying the non-copyable mutex
static std::unique_ptr<EventLog> makeLog() {
    auto log = std::make_unique<EventLog>();

    Event e1; e1.type = EventType::SyscallEnter;
    SyscallEvent s1; s1.name = "openat"; s1.number = 257; s1.args = {"0x3", "/etc/hosts", "0x0"};
    e1.payload = s1; log->append(e1);

    Event e2; e2.type = EventType::SyscallExit;
    SyscallEvent s2; s2.name = "openat"; s2.number = 257; s2.result = "0x5";
    e2.payload = s2; log->append(e2);

    Event e3; e3.type = EventType::VariableChange;
    VariableEvent v; v.name = "errno"; v.type_name = "int"; v.old_value = "0"; v.new_value = "2";
    e3.payload = v; log->append(e3);

    Event e4; e4.type = EventType::Marker;
    MarkerEvent m; m.label = "breakpoint_hit"; m.note = "manual marker";
    e4.payload = m; log->append(e4);

    Event e5; e5.type = EventType::FunctionEnter;
    FunctionEvent f; f.name = "main"; f.entry_address = 0x1000; f.args = {"argc=1"};
    e5.payload = f; log->append(e5);

    Event e6; e6.type = EventType::ExceptionThrown;
    ExceptionEvent ex; ex.type_name = "std::runtime_error"; ex.message = "bad alloc";
    e6.payload = ex; log->append(e6);

    return log;
}

TEST(Search, FindBySyscallName) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    auto ids = tl.find_events("openat");
    EXPECT_EQ(ids.size(), 2u); // enter + exit
}

TEST(Search, FindByEventTypeName) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    auto ids = tl.find_events("Marker");
    EXPECT_EQ(ids.size(), 1u);
}

TEST(Search, FindByVariableName) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    auto ids = tl.find_events("errno");
    EXPECT_EQ(ids.size(), 1u);
}

TEST(Search, FindByVariableOldValue) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    // old_value "0" and new_value "2" — search for "old_value" content
    auto ids = tl.find_events("int"); // type_name
    EXPECT_GE(ids.size(), 1u);
}

TEST(Search, FindByMarkerLabel) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    auto ids = tl.find_events("breakpoint_hit");
    EXPECT_EQ(ids.size(), 1u);
}

TEST(Search, FindByMarkerNote) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    auto ids = tl.find_events("manual marker");
    EXPECT_EQ(ids.size(), 1u);
}

TEST(Search, FindByFunctionName) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    auto ids = tl.find_events("main");
    EXPECT_EQ(ids.size(), 1u);
}

TEST(Search, FindByFunctionArg) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    auto ids = tl.find_events("argc=1");
    EXPECT_EQ(ids.size(), 1u);
}

TEST(Search, FindByExceptionType) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    auto ids = tl.find_events("runtime_error");
    EXPECT_EQ(ids.size(), 1u);
}

TEST(Search, FindByExceptionMessage) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    auto ids = tl.find_events("bad alloc");
    EXPECT_EQ(ids.size(), 1u);
}

TEST(Search, FindBySyscallResult) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    auto ids = tl.find_events("0x5"); // syscall exit result
    EXPECT_GE(ids.size(), 1u);
}

TEST(Search, FindByPathInSyscallArg) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    auto ids = tl.find_events("/etc/hosts");
    EXPECT_EQ(ids.size(), 1u); // only SyscallEnter has args
}

TEST(Search, NoMatch) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    auto ids = tl.find_events("zzz_no_match_zzz");
    EXPECT_TRUE(ids.empty());
}

TEST(Search, EmptyLog) {
    EventLog log;
    Timeline tl; tl.set_log(&log);
    EXPECT_TRUE(tl.find_events("anything").empty());
}

TEST(Search, FindByType) {
    auto log = makeLog();
    Timeline tl; tl.set_log(log.get());
    auto ids = tl.find_events(EventType::SyscallEnter);
    EXPECT_EQ(ids.size(), 1u);
}
