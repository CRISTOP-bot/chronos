#include <gtest/gtest.h>
#include <cstdio>
#include "chronos/storage/writer.h"
#include "chronos/events/event.h"
#include "chronos/common/types.h"

using namespace chronos;

TEST(Storage, RoundTrip) {
    std::vector<Event> events;
    Event e1; e1.type=EventType::FunctionEnter;
    FunctionEvent fe; fe.name="foo"; fe.entry_address=0x1234; fe.args={"a","b"};
    e1.payload=fe;
    events.push_back(e1);

    Event e2; e2.type=EventType::VariableChange;
    VariableEvent ve; ve.name="x"; ve.type_name="int"; ve.old_value="0"; ve.new_value="7"; ve.address=0x5;
    e2.payload=ve;
    events.push_back(e2);

    Event e3; e3.type=EventType::MemoryWrite;
    MemoryEvent me; me.address=0x10; me.size=4; me.old_bytes={1,2,3,4}; me.new_bytes={5,6,7,8};
    e3.payload=me;
    events.push_back(e3);

    storage::RecordingMetadata meta; meta.name="t"; meta.target_path="prog"; meta.event_count=events.size();
    std::string path = "/tmp/chronos-rt.chronos";
    auto wr = storage::Writer::write_recording(path, meta, events);
    ASSERT_TRUE(wr.ok()) << wr.error().message;

    auto rr = storage::Reader::read_recording(path);
    ASSERT_TRUE(rr.ok()) << rr.error().message;
    auto rec = rr.value();
    ASSERT_EQ(rec.events.size(), 3u);
    EXPECT_EQ(std::get<FunctionEvent>(rec.events[0].payload).name, "foo");
    EXPECT_EQ(std::get<VariableEvent>(rec.events[1].payload).new_value, "7");
    EXPECT_EQ(std::get<MemoryEvent>(rec.events[2].payload).new_bytes[2], 7);
    std::remove(path.c_str());
}
