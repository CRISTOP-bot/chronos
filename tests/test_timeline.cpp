#include <gtest/gtest.h>
#include "chronos/timeline/timeline.h"
#include "chronos/events/event_log.h"
#include "chronos/events/event.h"

using namespace chronos;

TEST(Timeline, Navigation) {
    EventLog log;
    for (int i=0;i<3;i++){ Event e; e.type=EventType::Marker; MarkerEvent m; m.label="m"; e.payload=m; log.append(e);}
    timeline::Timeline tl(&log);
    auto b = tl.begin();
    EXPECT_EQ(b.sequence, 1u);
    auto n = tl.next(b);
    EXPECT_EQ(n.sequence, 2u);
    auto p = tl.prev(n);
    EXPECT_EQ(p.sequence, 1u);
    EXPECT_TRUE(tl.valid(tl.end()));
}

TEST(Timeline, CheckpointAndFind) {
    EventLog log;
    Event e; e.type=EventType::VariableChange; VariableEvent v; v.name="x"; v.new_value="1"; e.payload=v; log.append(e);
    timeline::Timeline tl(&log);
    auto cp = tl.add_checkpoint(tl.end(), "cp1");
    auto found = tl.find_checkpoint(cp);
    ASSERT_TRUE(found.has_value());
    auto ids = tl.find_events(EventType::VariableChange);
    EXPECT_EQ(ids.size(), 1u);
}
