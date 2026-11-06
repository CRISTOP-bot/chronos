#include "chronos/memory/proc_maps.h"
#include <fstream>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>

namespace chronos {
namespace memory {

Result<MemoryMap> read_proc_maps(int pid) {
    MemoryMap mm;
    std::ifstream in("/proc/" + std::to_string(pid) + "/maps");
    if (!in) return Result<MemoryMap>(Error::make(ErrorCode::IoError,"cannot open maps"));
    std::string line;
    while (std::getline(in, line)) {
        // format: start-end perms offset dev inode pathname
        std::istringstream ss(line);
        std::string range, perms, off, dev, inode, path;
        ss >> range >> perms >> off >> dev >> inode;
        std::getline(ss, path); // rest
        // trim
        while (!path.empty() && (path[0]==' ')) path.erase(0,1);
        size_t dash = range.find('-');
        if (dash == std::string::npos) continue;
        Address start = std::stoull(range.substr(0,dash), nullptr, 16);
        Address end = std::stoull(range.substr(dash+1), nullptr, 16);
        MemoryRegion r;
        r.base = start;
        r.size = end - start;
        r.name = path;
        r.readable = perms.size()>0 && perms[0]=='r';
        r.writable = perms.size()>1 && perms[1]=='w';
        r.executable = perms.size()>2 && perms[2]=='x';
        mm.add_region(r);
    }
    return Result<MemoryMap>(mm);
}

Result<std::vector<u8>> read_proc_mem(int pid, Address addr, u64 size) {
    std::vector<u8> buf(size);
    int fd = open(("/proc/" + std::to_string(pid) + "/mem").c_str(), O_RDONLY);
    if (fd < 0) return Result<std::vector<u8>>(Error::make(ErrorCode::IoError,"cannot open mem"));
    ssize_t n = pread(fd, buf.data(), size, (off_t)addr);
    close(fd);
    if (n < 0) return Result<std::vector<u8>>(Error::make(ErrorCode::IoError,"pread failed"));
    buf.resize(n);
    return Result<std::vector<u8>>(buf);
}

} }
