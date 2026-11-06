#include "chronos/common/types.h"
#include "chronos/events/event.h"
#include "chronos/storage/writer.h"
#include <chrono>
#include <iostream>
#include <vector>

int main() {
    using namespace chronos;
    const size_t N = 100000;
    std::vector<Event> events;
    events.reserve(N);
    for (size_t i=0;i<N;i++) {
        Event e; e.type = EventType::SyscallEnter;
        SyscallEvent s; s.name="write"; s.number=1; s.args={"0x1","0x2","0x3"}; s.result="0x4";
        e.payload = s;
        e.time.sequence = i+1;
        events.push_back(e);
    }
    storage::RecordingMetadata meta; meta.name="bench"; meta.event_count=N;
    std::string path = "/tmp/opencode/bench.chronos";
    auto t0 = std::chrono::high_resolution_clock::now();
    auto wr = storage::Writer::write_recording(path, meta, events);
    auto t1 = std::chrono::high_resolution_clock::now();
    auto read = storage::Reader::read_recording(path);
    auto t2 = std::chrono::high_resolution_clock::now();
    if (!wr.ok() || !read.ok()) { std::cerr << "error\n"; return 1; }
    double wms = std::chrono::duration<double,std::milli>(t1-t0).count();
    double rms = std::chrono::duration<double,std::milli>(t2-t1).count();
    std::cout << "events=" << N << " write_ms=" << wms << " read_ms=" << rms
              << " write_eps=" << (N/wms*1000) << " read_eps=" << (N/rms*1000) << "\n";
    return 0;
}
