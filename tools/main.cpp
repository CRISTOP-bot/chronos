#include "chronos/common/types.h"
#include "chronos/recording/recording_engine.h"
#include "chronos/events/event.h"
#include "chronos/events/event_log.h"
#include "chronos/storage/writer.h"
#include "chronos/instrumentation/ptrace_tracer.h"
#include "chronos/memory/proc_maps.h"
#include "chronos/memory/checkpoint_restore.h"
#include "chronos/timeline/timeline.h"
#include "chronos/analysis/state_diff.h"
#include <iostream>
#include <string>
#include <map>
#include <algorithm>
#include <fstream>

int main(int argc, char** argv) {
    using namespace chronos;
    std::cout << "CHRONOS " << "0.1.0" << " - Temporal Execution & Reconstruction Engine\n";
    if (argc < 2) {
        std::cout << "Usage: chronos_cli <command>\n";
        std::cout << "Commands: demo, version, trace, info, maps, search, checkpoints, reconstruct, memdiff, state, branch, export-html, export-json\n";
        return 0;
    }
    std::string cmd = argv[1];
    if (cmd == "version") {
        std::cout << "0.1.0\n";
        return 0;
    }
    if (cmd == "trace") {
        if (argc < 3) { std::cerr << "Usage: chronos_cli trace <binary> [output]\n"; return 1; }
        std::string binary = argv[2];
        std::string out = (argc>=4)? argv[3] : "trace.chronos";
        recording::RecordingEngine eng;
        recording::SessionConfig cfg{"trace", binary, 1};
        auto sess = eng.create_session(cfg);
        instrumentation::TraceConfig tcfg{binary, {}, out};
        instrumentation::PtraceTracer tracer;
        auto res = tracer.trace(tcfg, *sess);
        if (!res.ok()) { std::cerr << "trace failed: " << res.error().message << "\n"; return 1; }
        auto save = sess->save(out);
        if (!save.ok()) { std::cerr << "save failed: " << save.error().message << "\n"; return 1; }
        std::cout << "Traced " << res.value() << " events -> " << out << "\n";
        return 0;
    }
    if (cmd == "demo") {
        recording::RecordingEngine eng;
        recording::SessionConfig cfg{"demo", "", 1};
        auto sess = eng.create_session(cfg);
        // record a few events
        Event e1; e1.type = EventType::ProcessStart; e1.pid = 1; e1.tid = 1;
        ProcessEvent pe; pe.pid=1; pe.tid=1; pe.detail="start"; e1.payload = pe;
        sess->record_event(e1);

        Event e2; e2.type = EventType::FunctionEnter; e2.pid=1; e2.tid=1;
        FunctionEvent fe; fe.name="main"; fe.entry_address=0x1000; e2.payload = fe;
        sess->record_event(e2);

        Event e3; e3.type = EventType::VariableChange; e3.pid=1; e3.tid=1;
        VariableEvent ve; ve.name="x"; ve.type_name="int"; ve.old_value="0"; ve.new_value="42"; ve.address=0x2000;
        e3.payload = ve;
        sess->record_event(e3);

        std::string out = (argc>=3)? argv[2] : "demo.chronos";
        auto res = sess->save(out);
        if (!res.ok()) { std::cerr << "save failed: " << res.error().message << "\n"; return 1; }
        std::cout << "Saved recording to " << out << " (" << sess->log().size() << " events)\n";

        // reopen
        auto rr = storage::Reader::read_recording(out);
        if (!rr.ok()) { std::cerr << "read failed: " << rr.error().message << "\n"; return 1; }
        auto rec = rr.value();
        std::cout << "Reopened: " << rec.events.size() << " events, name=" << rec.metadata.name << "\n";
        return 0;
    }
    if (cmd == "info") {
        if (argc < 3) { std::cerr << "Usage: chronos_cli info <recording>\n"; return 1; }
        auto rr = storage::Reader::read_recording(argv[2]);
        if (!rr.ok()) { std::cerr << "read failed: " << rr.error().message << "\n"; return 1; }
        auto rec = rr.value();
        std::cout << "name=" << rec.metadata.name << " events=" << rec.events.size() << " checkpoints=" << rec.checkpoints.size() << " target=" << rec.metadata.target_path << "\n";
        // count by type
        std::map<std::string,int> counts;
        for (auto& e : rec.events) counts[event_type_name(e.type)]++;
        for (auto& kv : counts) std::cout << "  " << kv.first << ": " << kv.second << "\n";
        return 0;
    }
    if (cmd == "maps") {
        if (argc < 3) { std::cerr << "Usage: chronos_cli maps <pid>\n"; return 1; }
        int pid = std::stoi(argv[2]);
        auto r = memory::read_proc_maps(pid);
        if (!r.ok()) { std::cerr << "maps failed: " << r.error().message << "\n"; return 1; }
        for (auto& reg : r.value().regions()) {
            std::cout << std::hex << reg.base << "-" << (reg.base+reg.size) << " "
                      << (reg.readable?"r":"-") << (reg.writable?"w":"-") << (reg.executable?"x":"-")
                      << " " << reg.name << "\n";
        }
        return 0;
    }
    if (cmd == "search") {
        if (argc < 4) { std::cerr << "Usage: chronos_cli search <recording> <text>\n"; return 1; }
        auto rr = storage::Reader::read_recording(argv[2]);
        if (!rr.ok()) { std::cerr << "read failed: " << rr.error().message << "\n"; return 1; }
        auto rec = rr.value();
        timeline::Timeline tl;
        // build an EventLog from events
        EventLog log;
        for (auto& e : rec.events) log.append(e);
        tl.set_log(&log);
        auto ids = tl.find_events(argv[3]);
        std::cout << "matches=" << ids.size() << "\n";
        for (auto id : ids) {
            auto ev = log.by_id(id);
            if (ev) std::cout << "  " << id << " " << event_type_name(ev->type) << "\n";
        }
        return 0;
    }
    if (cmd == "checkpoints") {
        if (argc < 3) { std::cerr << "Usage: chronos_cli checkpoints <recording>\n"; return 1; }
        auto rr = storage::Reader::read_recording(argv[2]);
        if (!rr.ok()) { std::cerr << "read failed: " << rr.error().message << "\n"; return 1; }
        auto rec = rr.value();
        std::cout << "checkpoints=" << rec.checkpoints.size() << "\n";
        for (auto& cp : rec.checkpoints) {
            std::cout << "  id=" << cp.id << " t=" << cp.time_sequence << " label=" << cp.label
                      << " regions=" << cp.regions.size() << "\n";
        }
        return 0;
    }
    if (cmd == "branch") {
        if (argc < 5) { std::cerr << "Usage: chronos_cli branch <src> <dst> <label>\n"; return 1; }
        auto rr = storage::Reader::read_recording(argv[2]);
        if (!rr.ok()) { std::cerr << "read failed: " << rr.error().message << "\n"; return 1; }
        auto rec = rr.value();
        // create a new session copy
        recording::RecordingEngine eng;
        recording::SessionConfig cfg{"branch", rec.metadata.target_path, 1};
        auto sess = eng.create_session(cfg);
        for (auto& e : rec.events) sess->record_event(e);
        for (auto& cp : rec.checkpoints) {
            sess->add_checkpoint(cp.time_sequence, cp.label, cp.regions);
        }
        // add branch marker
        Event m; m.type = EventType::Marker;
        MarkerEvent me; me.label = argv[4]; me.note = "branch from " + std::string(argv[2]);
        m.payload = me;
        sess->record_event(m);
        auto save = sess->save(argv[3]);
        if (!save.ok()) { std::cerr << "save failed: " << save.error().message << "\n"; return 1; }
        std::cout << "Branch saved to " << argv[3] << "\n";
        return 0;
    }
    if (cmd == "reconstruct") {
        if (argc < 4) { std::cerr << "Usage: chronos_cli reconstruct <recording> <checkpointId>\n"; return 1; }
        auto rr = storage::Reader::read_recording(argv[2]);
        if (!rr.ok()) { std::cerr << "read failed: " << rr.error().message << "\n"; return 1; }
        const u64 id = std::stoull(argv[3]);
        const auto& checkpoints = rr.value().checkpoints;
        const auto it = std::find_if(checkpoints.begin(), checkpoints.end(), [id](const auto& cp) { return cp.id == id; });
        if (it == checkpoints.end()) { std::cerr << "checkpoint not found: " << id << "\n"; return 1; }
        auto restored = memory::CheckpointRestorer::restore(*it);
        if (!restored.ok()) { std::cerr << "reconstruction failed: " << restored.error().message << "\n"; return 1; }
        const auto regions = restored.value().regions();
        std::cout << "Checkpoint " << it->id << " (" << it->label << ") at event " << it->time_sequence
                  << ": " << regions.size() << " captured memory regions\n";
        for (const auto& region : regions) {
            std::cout << "  0x" << std::hex << region.base << std::dec << " size=" << region.size
                      << " captured=" << region.data.size() << " name=" << region.name << " bytes=";
            const size_t preview = std::min<size_t>(region.data.size(), 16);
            for (size_t i = 0; i < preview; ++i) {
                std::cout << (i ? " " : "") << std::hex << std::uppercase
                          << static_cast<unsigned>(region.data[i]) << std::nouppercase << std::dec;
            }
            if (region.data.size() > preview) std::cout << " ...";
            std::cout << "\n";
        }
        return 0;
    }
    if (cmd == "memdiff") {
        if (argc < 5) { std::cerr << "Usage: chronos_cli memdiff <recording> <cpA> <cpB>\n"; return 1; }
        auto rr = storage::Reader::read_recording(argv[2]);
        if (!rr.ok()) { std::cerr << "read failed: " << rr.error().message << "\n"; return 1; }
        auto rec = rr.value();
        u64 a = std::stoull(argv[3]), b = std::stoull(argv[4]);
        const storage::CheckpointRecord* cpa = nullptr;
        const storage::CheckpointRecord* cpb = nullptr;
        for (auto& cp : rec.checkpoints) { if (cp.id == a) cpa = &cp; if (cp.id == b) cpb = &cp; }
        if (!cpa || !cpb) { std::cerr << "checkpoint not found\n"; return 1; }
        std::cout << "Comparing checkpoint " << a << " vs " << b << "\n";
        // added/removed regions by base
        for (auto& ra : cpa->regions) {
            const storage::CheckpointRegion* rb = nullptr;
            for (auto& r : cpb->regions) if (r.base == ra.base) { rb = &r; break; }
            if (!rb) { std::cout << "  removed region 0x" << std::hex << ra.base << " (" << std::dec << ra.size << " bytes)\n"; continue; }
            // compare bytes
            size_t n = std::min(ra.bytes.size(), rb->bytes.size());
            size_t changed = 0;
            for (size_t i = 0; i < n; ++i) if (ra.bytes[i] != rb->bytes[i]) changed++;
            if (changed > 0 || ra.bytes.size() != rb->bytes.size()) {
                std::cout << "  changed region 0x" << std::hex << ra.base << std::dec
                          << ": " << changed << " bytes differ"
                          << " (captured " << ra.bytes.size() << " vs " << rb->bytes.size() << ")\n";
            }
        }
        for (auto& rb : cpb->regions) {
            bool found = false;
            for (auto& ra : cpa->regions) if (ra.base == rb.base) { found = true; break; }
            if (!found) std::cout << "  added region 0x" << std::hex << rb.base << " (" << std::dec << rb.size << " bytes)\n";
        }
        return 0;
    }
    if (cmd == "state") {
        if (argc < 4) { std::cerr << "Usage: chronos_cli state <recording> <eventIndex>\n"; return 1; }
        auto rr = storage::Reader::read_recording(argv[2]);
        if (!rr.ok()) { std::cerr << "read failed: " << rr.error().message << "\n"; return 1; }
        auto rec = rr.value();
        EventLog log;
        for (auto& e : rec.events) log.append(e);
        u64 idx = std::stoull(argv[3]);
        if (idx == 0 || idx > rec.events.size()) idx = rec.events.size();
        auto snap = analysis::StateDiffer::build(log, TimePoint{idx});
        std::cout << "State at event " << idx << ": " << snap.variables.size() << " variables\n";
        for (auto& kv : snap.variables) {
            std::cout << "  " << kv.first << " (" << kv.second.type_name << ") = " << kv.second.value << "\n";
        }
        return 0;
    }
    if (cmd == "export-html") {
        if (argc < 4) { std::cerr << "Usage: chronos_cli export-html <recording> <output.html>\n"; return 1; }
        auto rr = storage::Reader::read_recording(argv[2]);
        if (!rr.ok()) { std::cerr << "read failed: " << rr.error().message << "\n"; return 1; }
        auto rec = rr.value();
        std::ofstream out(argv[3]);
        out << "<!DOCTYPE html><html><head><meta charset='utf-8'><title>CHRONOS Report</title>\n";
        out << "<style>body{background:#0e1116;color:#dce0e8;font-family:monospace;margin:24px;}\n";
        out << "h1{color:#4682b4;} table{border-collapse:collapse;width:100%;} th,td{border:1px solid #2a2e36;padding:6px;} th{background:#1e2228;}\n";
        out << ".cp{margin:8px 0;padding:8px;border:1px solid #2a2e36;background:#181a1e;}</style></head><body>\n";
        out << "<h1>CHRONOS Recording Report</h1>\n";
        out << "<p><b>Name:</b> " << rec.metadata.name << " | <b>Events:</b> " << rec.events.size()
            << " | <b>Checkpoints:</b> " << rec.checkpoints.size() << " | <b>Target:</b> " << rec.metadata.target_path << "</p>\n";
        out << "<h2>Checkpoints</h2>\n";
        for (auto& cp : rec.checkpoints) {
            out << "<div class='cp'>Checkpoint " << cp.id << " @ event " << cp.time_sequence
                << " — " << cp.label << " (" << cp.regions.size() << " regions)</div>\n";
        }
        out << "<h2>Event Log</h2>\n";
        out << "<table><tr><th>#</th><th>Type</th><th>PID/TID</th><th>Detail</th></tr>\n";
        for (size_t i=0;i<rec.events.size();++i) {
            const auto& e = rec.events[i];
            std::string detail;
            if (e.type == EventType::SyscallEnter || e.type == EventType::SyscallExit) {
                auto s = std::get<SyscallEvent>(e.payload);
                detail = s.name + " " + s.result;
            } else if (e.type == EventType::VariableChange) {
                auto v = std::get<VariableEvent>(e.payload);
                detail = v.name + " = " + v.new_value;
            } else if (e.type == EventType::Checkpoint) {
                detail = std::get<CheckpointEvent>(e.payload).label;
            } else if (e.type == EventType::FunctionEnter || e.type == EventType::FunctionExit) {
                detail = std::get<FunctionEvent>(e.payload).name;
            }
            out << "<tr><td>" << (i+1) << "</td><td>" << event_type_name(e.type) << "</td><td>"
                << e.pid << "/" << e.tid << "</td><td>" << detail << "</td></tr>\n";
        }
        out << "</table></body></html>\n";
        out.close();
        std::cout << "Exported HTML report to " << argv[3] << "\n";
        return 0;
    }
    if (cmd == "export-json") {
        if (argc < 4) { std::cerr << "Usage: chronos_cli export-json <recording> <output.json>\n"; return 1; }
        auto rr = storage::Reader::read_recording(argv[2]);
        if (!rr.ok()) { std::cerr << "read failed: " << rr.error().message << "\n"; return 1; }
        auto rec = rr.value();
        auto esc = [](const std::string& s) {
            std::string o;
            for (char c : s) {
                switch (c) {
                    case '\"': o += "\\\""; break;
                    case '\\': o += "\\\\"; break;
                    case '\n': o += "\\n"; break;
                    case '\r': o += "\\r"; break;
                    case '\t': o += "\\t"; break;
                    default: o += c; break;
                }
            }
            return o;
        };
        std::ofstream out(argv[3]);
        out << "{\n";
        out << "  \"metadata\": {\"name\": \"" << esc(rec.metadata.name) << "\", \"target\": \"" << esc(rec.metadata.target_path)
            << "\", \"events\": " << rec.events.size() << ", \"checkpoints\": " << rec.checkpoints.size() << "},\n";
        out << "  \"events\": [\n";
        for (size_t i=0;i<rec.events.size();++i) {
            const auto& e = rec.events[i];
            out << "    {\"index\": " << (i+1) << ", \"type\": \"" << esc(event_type_name(e.type)) << "\", \"pid\": " << e.pid
                << ", \"tid\": " << e.tid << ", \"sequence\": " << e.time.sequence;
            if (e.type == EventType::SyscallEnter || e.type == EventType::SyscallExit) {
                auto s = std::get<SyscallEvent>(e.payload);
                out << ", \"syscall\": \"" << esc(s.name) << "\", \"result\": \"" << esc(s.result) << "\"";
            } else if (e.type == EventType::VariableChange) {
                auto v = std::get<VariableEvent>(e.payload);
                out << ", \"variable\": \"" << esc(v.name) << "\", \"value\": \"" << esc(v.new_value) << "\"";
            }
            out << "}";
            if (i+1 < rec.events.size()) out << ",";
            out << "\n";
        }
        out << "  ],\n";
        out << "  \"checkpoints\": [\n";
        for (size_t i=0;i<rec.checkpoints.size();++i) {
            const auto& cp = rec.checkpoints[i];
            out << "    {\"id\": " << cp.id << ", \"time\": " << cp.time_sequence << ", \"label\": \"" << esc(cp.label)
                << "\", \"regions\": " << cp.regions.size() << "}";
            if (i+1 < rec.checkpoints.size()) out << ",";
            out << "\n";
        }
        out << "  ]\n}\n";
        out.close();
        std::cout << "Exported JSON to " << argv[3] << "\n";
        return 0;
    }
    std::cerr << "Unknown command: " << cmd << "\n";
    return 1;
}
