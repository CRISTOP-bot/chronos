#include <gtest/gtest.h>
#include "chronos/analysis/state_diff.h"
#include "chronos/events/event_log.h"
#include "chronos/events/event.h"

using namespace chronos;

TEST(StateDiff, BuildAndDiff) {
    EventLog log;
    Event e1; e1.type=EventType::VariableChange; VariableEvent v1; v1.name="x"; v1.new_value="1"; e1.payload=v1; e1.time.sequence=1; log.append(e1);
    Event e2; e2.type=EventType::VariableChange; VariableEvent v2; v2.name="x"; v2.new_value="2"; e2.payload=v2; e2.time.sequence=2; log.append(e2);
    Event e3; e3.type=EventType::VariableChange; VariableEvent v3; v3.name="y"; v3.new_value="9"; e3.payload=v3; e3.time.sequence=3; log.append(e3);

    auto sa = analysis::StateDiffer::build(log, TimePoint{1});
    auto sb = analysis::StateDiffer::build(log, TimePoint{3});
    auto d = analysis::StateDiffer::diff(sa, sb, log);
    EXPECT_EQ(d.changed_variables.size(), 1u);
    EXPECT_EQ(d.added_variables.size(), 1u);
}
