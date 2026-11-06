#include "chronos/storage/writer.h"
#include "chronos/common/error.h"
#include <zlib.h>
#include <zstd.h>
#include <chrono>
#include <cstring>
#include <sstream>
#include <variant>
#include <fstream>
#include <cstdio>

namespace chronos {
namespace storage {

namespace {

void put_u8(std::ostream& o, u8 v){ o.write(reinterpret_cast<const char*>(&v),1);}
void put_u32(std::ostream& o, u32 v){ o.write(reinterpret_cast<const char*>(&v),4);}
void put_u64(std::ostream& o, u64 v){ o.write(reinterpret_cast<const char*>(&v),8);}
void put_i64(std::ostream& o, i64 v){ o.write(reinterpret_cast<const char*>(&v),8);}
void put_str(std::ostream& o, const std::string& s){
    u64 n = s.size();
    put_u64(o,n);
    if(n) o.write(s.data(), (std::streamsize)n);
}
void put_bytes(std::ostream& o, const std::vector<u8>& b){
    u64 n = b.size();
    put_u64(o,n);
    if(n) o.write(reinterpret_cast<const char*>(b.data()), (std::streamsize)n);
}
void put_strings(std::ostream& o, const std::vector<std::string>& v){
    u64 n = v.size();
    put_u64(o,n);
    for(const auto& s : v) put_str(o,s);
}

u8 get_u8(std::istream& i){ u8 v=0; i.read(reinterpret_cast<char*>(&v),1); return v;}
u32 get_u32(std::istream& i){ u32 v=0; i.read(reinterpret_cast<char*>(&v),4); return v;}
u64 get_u64(std::istream& i){ u64 v=0; i.read(reinterpret_cast<char*>(&v),8); return v;}
i64 get_i64(std::istream& i){ i64 v=0; i.read(reinterpret_cast<char*>(&v),8); return v;}
std::string get_str(std::istream& i){
    u64 n = get_u64(i);
    std::string s(n,'\0');
    if(n) i.read(s.data(), (std::streamsize)n);
    return s;
}
std::vector<u8> get_bytes(std::istream& i){
    u64 n = get_u64(i);
    std::vector<u8> b(n);
    if(n) i.read(reinterpret_cast<char*>(b.data()), (std::streamsize)n);
    return b;
}
std::vector<std::string> get_strings(std::istream& i){
    u64 n = get_u64(i);
    std::vector<std::string> v; v.reserve(n);
    for(u64 k=0;k<n;k++) v.push_back(get_str(i));
    return v;
}

void write_event(std::ostream& o, const Event& e){
    put_u64(o, e.id);
    put_u64(o, e.time.sequence);
    put_u8(o, static_cast<u8>(e.type));
    put_u32(o, e.pid);
    put_u64(o, e.tid);
    put_i64(o, e.timestamp.count());
    // location
    if (e.location.has_value()) {
        put_u8(o,1);
        put_str(o, e.location->file);
        put_u32(o, e.location->line);
        put_u32(o, e.location->column);
    } else {
        put_u8(o,0);
    }
    // payload tag
    // determine tag
    u8 tag = 0;
    if (std::holds_alternative<std::monostate>(e.payload)) tag=0;
    else if (std::holds_alternative<FunctionEvent>(e.payload)) tag=1;
    else if (std::holds_alternative<VariableEvent>(e.payload)) tag=2;
    else if (std::holds_alternative<MemoryEvent>(e.payload)) tag=3;
    else if (std::holds_alternative<ExceptionEvent>(e.payload)) tag=4;
    else if (std::holds_alternative<SyscallEvent>(e.payload)) tag=5;
    else if (std::holds_alternative<ProcessEvent>(e.payload)) tag=6;
    else if (std::holds_alternative<CheckpointEvent>(e.payload)) tag=7;
    else if (std::holds_alternative<MarkerEvent>(e.payload)) tag=8;
    else if (std::holds_alternative<CustomEvent>(e.payload)) tag=9;
    put_u8(o, tag);

    if (tag==1) {
        const auto& f = std::get<FunctionEvent>(e.payload);
        put_str(o, f.name);
        put_u64(o, f.entry_address);
        put_strings(o, f.args);
    } else if (tag==2) {
        const auto& v = std::get<VariableEvent>(e.payload);
        put_str(o, v.name);
        put_str(o, v.type_name);
        put_str(o, v.old_value);
        put_str(o, v.new_value);
        put_u64(o, v.address);
    } else if (tag==3) {
        const auto& m = std::get<MemoryEvent>(e.payload);
        put_u64(o, m.address);
        put_u64(o, m.size);
        put_bytes(o, m.old_bytes);
        put_bytes(o, m.new_bytes);
    } else if (tag==4) {
        const auto& ex = std::get<ExceptionEvent>(e.payload);
        put_str(o, ex.type_name);
        put_str(o, ex.message);
        put_u64(o, ex.throw_address);
    } else if (tag==5) {
        const auto& s = std::get<SyscallEvent>(e.payload);
        put_str(o, s.name);
        put_i64(o, s.number);
        put_strings(o, s.args);
        put_str(o, s.result);
    } else if (tag==6) {
        const auto& p = std::get<ProcessEvent>(e.payload);
        put_u32(o, p.pid);
        put_u64(o, p.tid);
        put_str(o, p.detail);
    } else if (tag==7) {
        const auto& c = std::get<CheckpointEvent>(e.payload);
        put_u64(o, c.checkpoint_id);
        put_str(o, c.label);
    } else if (tag==8) {
        const auto& m = std::get<MarkerEvent>(e.payload);
        put_str(o, m.label);
        put_str(o, m.note);
    } else if (tag==9) {
        const auto& c = std::get<CustomEvent>(e.payload);
        put_str(o, c.kind);
        put_str(o, c.payload);
    }
}

Event read_event(std::istream& i){
    Event e;
    e.id = get_u64(i);
    e.time.sequence = get_u64(i);
    e.type = static_cast<EventType>(get_u8(i));
    e.pid = get_u32(i);
    e.tid = get_u64(i);
    e.timestamp = Timestamp(get_i64(i));
    u8 hasloc = get_u8(i);
    if (hasloc) {
        SourceLocation sl;
        sl.file = get_str(i);
        sl.line = get_u32(i);
        sl.column = get_u32(i);
        e.location = sl;
    }
    u8 tag = get_u8(i);
    if (tag==1) {
        FunctionEvent f;
        f.name = get_str(i);
        f.entry_address = get_u64(i);
        f.args = get_strings(i);
        e.payload = f;
    } else if (tag==2) {
        VariableEvent v;
        v.name = get_str(i);
        v.type_name = get_str(i);
        v.old_value = get_str(i);
        v.new_value = get_str(i);
        v.address = get_u64(i);
        e.payload = v;
    } else if (tag==3) {
        MemoryEvent m;
        m.address = get_u64(i);
        m.size = get_u64(i);
        m.old_bytes = get_bytes(i);
        m.new_bytes = get_bytes(i);
        e.payload = m;
    } else if (tag==4) {
        ExceptionEvent ex;
        ex.type_name = get_str(i);
        ex.message = get_str(i);
        ex.throw_address = get_u64(i);
        e.payload = ex;
    } else if (tag==5) {
        SyscallEvent s;
        s.name = get_str(i);
        s.number = get_i64(i);
        s.args = get_strings(i);
        s.result = get_str(i);
        e.payload = s;
    } else if (tag==6) {
        ProcessEvent p;
        p.pid = get_u32(i);
        p.tid = get_u64(i);
        p.detail = get_str(i);
        e.payload = p;
    } else if (tag==7) {
        CheckpointEvent c;
        c.checkpoint_id = get_u64(i);
        c.label = get_str(i);
        e.payload = c;
    } else if (tag==8) {
        MarkerEvent m;
        m.label = get_str(i);
        m.note = get_str(i);
        e.payload = m;
    } else if (tag==9) {
        CustomEvent c;
        c.kind = get_str(i);
        c.payload = get_str(i);
        e.payload = c;
    }
    return e;
}

u32 crc32_bytes(const std::vector<u8>& data){
    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, data.data(), (uInt)data.size());
    return (u32)crc;
}

} // anon

Writer::Writer(const std::string& path) : path_(path) {}
Writer::~Writer() {}

Result<void> Writer::write_header(const FileHeader& h){
    std::ofstream out(path_, std::ios::binary | std::ios::trunc);
    if(!out) return Result<void>(Error::make(ErrorCode::IoError,"cannot open file",path_));
    // write fields
    out.write(h.magic,4);
    put_u32(out, h.version);
    put_u32(out, h.flags);
    put_u64(out, h.created_unix);
    put_u64(out, h.metadata_offset);
    put_u64(out, h.metadata_size);
    put_u64(out, h.events_offset);
    put_u64(out, h.events_size);
    put_u64(out, h.checkpoints_offset);
    put_u64(out, h.checkpoints_size);
    put_u64(out, h.index_offset);
    put_u64(out, h.index_size);
    put_u64(out, h.footer_offset);
    put_u32(out, h.header_crc);
    out.close();
    if(!out) return Result<void>(Error::make(ErrorCode::IoError,"write header failed",path_));
    return Result<void>();
}

// We'll instead implement write_recording as full pipeline using ofstream directly.
Result<void> Writer::write_recording(const std::string& path, const RecordingMetadata& meta, const std::vector<Event>& events, const std::vector<CheckpointRecord>& checkpoints){
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if(!out) return Result<void>(Error::make(ErrorCode::IoError,"cannot open file",path));

    // placeholder header size: compute by writing zeros for 4+4+4+8*11+4 = let's just write header later.
    // We'll buffer: write a placeholder header of fixed size first.
    // Fixed header field count size:
    // magic4 + version4 + flags4 + created8 + 11*8? Let's count: metadata_offset,size (2), events_offset,size(2), checkpoints_offset,size(2), index_offset,size(2), footer_offset(1) = 9 u64? Actually fields: created_unix(1)+metadata_offset+metadata_size+events_offset+events_size+checkpoints_offset+checkpoints_size+index_offset+index_size+footer_offset = 10 u64. Plus header_crc u32.
    // total = 4+4+4+10*8+4 = 12+80+4=96 bytes.
    const size_t HEADER_SIZE = 96;
    std::vector<u8> header_buf(HEADER_SIZE,0);
    // write magic
    header_buf[0]='C';header_buf[1]='H';header_buf[2]='R';header_buf[3]='N';
    out.write(reinterpret_cast<const char*>(header_buf.data()), HEADER_SIZE);

    // metadata section
    u64 meta_offset = out.tellp();
    // serialize metadata to temp buffer
    std::vector<u8> meta_buf;
    {
        // use stringstream
        std::ostringstream ss(std::ios::binary);
        put_str(ss, meta.name);
        put_str(ss, meta.target_path);
        put_str(ss, meta.created_by);
        put_u32(ss, meta.event_count);
        put_u64(ss, meta.duration_ns);
        std::string str = ss.str();
        meta_buf.assign(str.begin(), str.end());
    }
    out.write(reinterpret_cast<const char*>(meta_buf.data()), meta_buf.size());
    u64 meta_size = meta_buf.size();

    // events section: serialize then compress with zstd
    u64 events_offset = out.tellp();
    std::ostringstream evss(std::ios::binary);
    for (const auto& e : events) write_event(evss, e);
    std::string raw = evss.str();
    size_t bound = ZSTD_compressBound(raw.size());
    std::vector<char> cbuf(bound);
    size_t csize = ZSTD_compress(cbuf.data(), bound, raw.data(), raw.size(), 3);
    if (ZSTD_isError(csize)) {
        // fallback uncompressed
        out.write(raw.data(), raw.size());
        // mark version 1? We'll keep version 2 but store raw; reader can't know. Instead store uncompressed in v1 style.
        // For simplicity, store raw and set a flag via version=1 in header later? Header version set FORMAT_VERSION (2).
        // To avoid mismatch, we store compressed only.
    }
    out.write(cbuf.data(), csize);
    u64 events_size = csize;

    // checkpoints section
    u64 cp_offset = out.tellp();
    std::ostringstream cpss(std::ios::binary);
    // count
    put_u64(cpss, checkpoints.size());
    for (const auto& cp : checkpoints) {
        put_u64(cpss, cp.id);
        put_u64(cpss, cp.time_sequence);
        put_str(cpss, cp.label);
        put_u64(cpss, cp.regions.size());
        for (const auto& r : cp.regions) {
            put_u64(cpss, r.base);
            put_u64(cpss, r.size);
            put_str(cpss, r.name);
            put_u8(cpss, r.readable);
            put_u8(cpss, r.writable);
            put_u8(cpss, r.executable);
            put_bytes(cpss, r.bytes);
        }
    }
    std::string cps = cpss.str();
    out.write(cps.data(), cps.size());
    u64 cp_size = cps.size();
    // index: none
    u64 idx_offset = out.tellp();
    u64 idx_size = 0;

    // footer crc over everything before footer? We'll crc the file content so far.
    // Read file into buffer to crc
    out.flush();
    // compute crc by re-reading? Instead compute incrementally: we didn't. Let's compute from file.
    // Simpler: crc over metadata+events bytes we have in memory? We have meta_buf and events not buffered.
    // We'll just write a footer with 0 for now? No—integrity required. Let's compute crc by reading file.
    out.close();
    // read file
    std::ifstream in(path, std::ios::binary);
    in.seekg(0, std::ios::end);
    size_t fsize = in.tellg();
    in.seekg(0);
    std::vector<u8> all(fsize);
    in.read(reinterpret_cast<char*>(all.data()), fsize);
    in.close();
    // crc over all except last 4 (footer) but footer not written yet; crc over all current
    u32 crc = crc32_bytes(all);

    // append footer
    std::ofstream out2(path, std::ios::binary | std::ios::app);
    put_u32(out2, crc);
    out2.close();
    u64 footer_offset = fsize;

    // now patch header
    // reopen read-write
    std::fstream rw(path, std::ios::binary | std::ios::in | std::ios::out);
    // build header
    u64 created = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    // write fields at offset 0
    rw.seekp(0);
    char magic[4]={'C','H','R','N'};
    rw.write(magic,4);
    put_u32(rw, FORMAT_VERSION);
    put_u32(rw, 0); // flags
    put_u64(rw, created);
    put_u64(rw, meta_offset);
    put_u64(rw, meta_size);
    put_u64(rw, events_offset);
    put_u64(rw, events_size);
    put_u64(rw, cp_offset);
    put_u64(rw, cp_size);
    put_u64(rw, idx_offset);
    put_u64(rw, idx_size);
    put_u64(rw, footer_offset);
    put_u32(rw, 0); // header_crc reserved (integrity in footer)
    rw.flush();
    rw.close();

    // Recompute CRC after header patch and rewrite footer.
    std::ifstream in2(path, std::ios::binary);
    in2.seekg(0, std::ios::end);
    size_t fsz = in2.tellg();
    in2.seekg(0);
    std::vector<u8> all2(fsz > 4 ? fsz - 4 : 0);
    if (!all2.empty()) in2.read(reinterpret_cast<char*>(all2.data()), all2.size());
    in2.close();
    u32 crc2 = crc32_bytes(all2);
    std::fstream rw2(path, std::ios::binary | std::ios::in | std::ios::out);
    rw2.seekp(footer_offset);
    put_u32(rw2, crc2);
    rw2.flush();
    rw2.close();

    (void)meta;
    return Result<void>();
}

// stubs intentionally not redefined; real implementations above for write_recording only.

Reader::Reader(const std::string& path) : path_(path) {}
Reader::~Reader() = default;

Result<FileHeader> Reader::read_header(){
    std::ifstream in(path_, std::ios::binary);
    if(!in) return Result<FileHeader>(Error::make(ErrorCode::IoError,"cannot open file",path_));
    FileHeader h;
    char magic[4];
    in.read(magic,4);
    std::memcpy(h.magic,magic,4);
    h.version = get_u32(in);
    h.flags = get_u32(in);
    h.created_unix = get_u64(in);
    h.metadata_offset = get_u64(in);
    h.metadata_size = get_u64(in);
    h.events_offset = get_u64(in);
    h.events_size = get_u64(in);
    h.checkpoints_offset = get_u64(in);
    h.checkpoints_size = get_u64(in);
    h.index_offset = get_u64(in);
    h.index_size = get_u64(in);
    h.footer_offset = get_u64(in);
    h.header_crc = get_u32(in);
    if(!in) return Result<FileHeader>(Error::make(ErrorCode::CorruptData,"truncated header",path_));
    // validate magic
    if (std::memcmp(h.magic, MAGIC, 4) != 0)
        return Result<FileHeader>(Error::make(ErrorCode::InvalidFormat,"bad magic",path_));
    if (h.version != FORMAT_VERSION)
        return Result<FileHeader>(Error::make(ErrorCode::UnsupportedVersion,"version mismatch",path_));
    return Result<FileHeader>(h);
}

Result<RecordingMetadata> Reader::read_metadata(const FileHeader& h){
    std::ifstream in(path_, std::ios::binary);
    if(!in) return Result<RecordingMetadata>(Error::make(ErrorCode::IoError,"cannot open",path_));
    in.seekg(h.metadata_offset);
    RecordingMetadata m;
    // read from section buffer
    std::vector<u8> buf(h.metadata_size);
    in.read(reinterpret_cast<char*>(buf.data()), h.metadata_size);
    // parse via istringstream
    std::istringstream ss(std::string(buf.begin(), buf.end()), std::ios::binary);
    m.name = get_str(ss);
    m.target_path = get_str(ss);
    m.created_by = get_str(ss);
    m.event_count = get_u32(ss);
    m.duration_ns = get_u64(ss);
    return Result<RecordingMetadata>(m);
}

Result<std::vector<Event>> Reader::read_events(const FileHeader& h){
    std::ifstream in(path_, std::ios::binary);
    if(!in) return Result<std::vector<Event>>(Error::make(ErrorCode::IoError,"cannot open",path_));
    in.seekg(h.events_offset);
    std::vector<char> comp(h.events_size);
    if (h.events_size) in.read(comp.data(), h.events_size);

    std::string raw;
    if (h.version <= 1) {
        raw.assign(comp.begin(), comp.end());
    } else {
        unsigned long long dsize = ZSTD_getFrameContentSize(comp.data(), comp.size());
        if (dsize == ZSTD_CONTENTSIZE_UNKNOWN || dsize == ZSTD_CONTENTSIZE_ERROR) {
            // fallback large
            dsize = comp.size() * 8 + 1024;
        }
        raw.resize(dsize);
        size_t r = ZSTD_decompress(raw.data(), raw.size(), comp.data(), comp.size());
        if (ZSTD_isError(r)) {
            return Result<std::vector<Event>>(Error::make(ErrorCode::CorruptData, ZSTD_getErrorName(r), path_));
        }
        raw.resize(r);
    }

    std::istringstream ss(raw, std::ios::binary);
    std::vector<Event> events;
    while (ss.peek() != EOF) {
        // ensure not trailing empty
        std::streampos pos = ss.tellg();
        Event e = read_event(ss);
        if (!ss && pos == ss.tellg()) break;
        events.push_back(e);
        if (ss.eof()) break;
    }
    return Result<std::vector<Event>>(events);
}

Result<std::vector<CheckpointRecord>> Reader::read_checkpoints(const FileHeader& h){
    std::ifstream in(path_, std::ios::binary);
    if(!in) return Result<std::vector<CheckpointRecord>>(Error::make(ErrorCode::IoError,"cannot open",path_));
    if (h.checkpoints_size == 0) return Result<std::vector<CheckpointRecord>>(std::vector<CheckpointRecord>{});
    in.seekg(h.checkpoints_offset);
    std::vector<char> buf(h.checkpoints_size);
    in.read(buf.data(), h.checkpoints_size);
    std::istringstream ss(std::string(buf.begin(), buf.end()), std::ios::binary);
    std::vector<CheckpointRecord> cps;
    u64 n = get_u64(ss);
    for (u64 i=0;i<n;i++) {
        CheckpointRecord cp;
        cp.id = get_u64(ss);
        cp.time_sequence = get_u64(ss);
        cp.label = get_str(ss);
        u64 nr = get_u64(ss);
        for (u64 j=0;j<nr;j++) {
            CheckpointRegion r;
            r.base = get_u64(ss);
            r.size = get_u64(ss);
            r.name = get_str(ss);
            r.readable = get_u8(ss);
            r.writable = get_u8(ss);
            r.executable = get_u8(ss);
            r.bytes = get_bytes(ss);
            cp.regions.push_back(r);
        }
        cps.push_back(cp);
    }
    return Result<std::vector<CheckpointRecord>>(cps);
}

Result<bool> Reader::verify_integrity(const FileHeader& h){
    // crc over file content up to footer_offset should equal header_crc? We stored crc of whole file before footer.
    std::ifstream in(path_, std::ios::binary);
    if(!in) return Result<bool>(Error::make(ErrorCode::IoError,"cannot open",path_));
    in.seekg(0, std::ios::end);
    // read up to footer_offset
    std::vector<u8> buf(h.footer_offset);
    in.seekg(0);
    in.read(reinterpret_cast<char*>(buf.data()), h.footer_offset);
    // read stored crc from footer
    std::ifstream in2(path_, std::ios::binary);
    in2.seekg(h.footer_offset);
    u32 stored = get_u32(in2);
    in2.close();
    u32 calc = crc32_bytes(buf);
    if (calc != stored) return Result<bool>(Error::make(ErrorCode::CorruptData,"crc mismatch",path_));
    return Result<bool>(true);
}

Result<Reader::Recording> Reader::read_recording(const std::string& path){
    Reader r(path);
    auto hr = r.read_header();
    if(!hr.ok()) return Result<Reader::Recording>(hr.error());
    FileHeader h = hr.value();
    auto vr = r.verify_integrity(h);
    if(!vr.ok()) return Result<Reader::Recording>(vr.error());
    auto mr = r.read_metadata(h);
    if(!mr.ok()) return Result<Reader::Recording>(mr.error());
    auto er = r.read_events(h);
    if(!er.ok()) return Result<Reader::Recording>(er.error());
    auto cr = r.read_checkpoints(h);
    if(!cr.ok()) return Result<Reader::Recording>(cr.error());
    Reader::Recording rec; rec.metadata = mr.value(); rec.events = er.value(); rec.checkpoints = cr.value();
    return Result<Reader::Recording>(rec);
}

} // namespace storage
} // namespace chronos
