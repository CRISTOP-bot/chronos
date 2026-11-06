#include <gtest/gtest.h>
#include "chronos/events/event_log.h"
#include "chronos/events/event.h"
#include "chronos/common/types.h"

using namespace chronos;

TEST(EventLog, AppendAndRetrieve) {
    EventLog log;
    Event e; e.type = EventType::Marker;
    MarkerEvent m; m.label = "m1"; e.payload = m;
    EventId id = log.append(e);
    EXPECT_EQ(log.size(), 1u);
    auto got = log.by_id(id);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(got->type, EventType::Marker);
}

TEST(EventLog, Range) {
    EventLog log;
    for (int i=0;i<5;i++){ Event e; e.type=EventType::Custom; CustomEvent c; c.kind="k"; e.payload=c; log.append(e);}
    auto r = log.range(1,4);
    EXPECT_EQ(r.size(), 3u);
}

TEST(EventLog, Clear) {
    EventLog log;
    Event e; e.type=EventType::Marker; log.append(e);
    log.clear();
    EXPECT_EQ(log.size(),0u);
}
