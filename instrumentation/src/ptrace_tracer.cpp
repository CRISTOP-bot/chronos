#include "chronos/instrumentation/ptrace_tracer.h"
#include "chronos/memory/proc_maps.h"
#include "chronos/storage/format.h"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>

namespace chronos {
namespace instrumentation {

static std::string syscall_name(long nr) {
    // minimal map
    static const std::map<long,std::string> m = {
        {0,"read"},{1,"write"},{2,"open"},{3,"close"},{9,"mmap"},{10,"mprotect"},
        {11,"munmap"},{12,"brk"},{39,"getpid"},{57,"fork"},{59,"execve"},
        {60,"exit"},{61,"wait4"},{62,"kill"},{231,"exit_group"},{257,"openat"},
        {318,"getrandom"},{332,"statx"}
    };
    auto it = m.find(nr);
    if (it != m.end()) return it->second;
    return "syscall_"+std::to_string(nr);
}

Result<u64> PtraceTracer::trace(TraceConfig cfg, recording::Session& session) {
    if (cfg.program.empty())
        return Result<u64>(Error::make(ErrorCode::InvalidArgument,"empty program"));

    pid_t pid = fork();
    if (pid < 0) return Result<u64>(Error::make(ErrorCode::IoError,"fork failed"));
    if (pid == 0) {
        // child
        if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) {
            _exit(127);
        }
        raise(SIGSTOP);
        // exec
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(cfg.program.c_str()));
        for (auto& a : cfg.args) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);
        execvp(cfg.program.c_str(), argv.data());
        _exit(127);
    }

    // parent
    int status = 0;
    waitpid(pid, &status, 0);
    // set options
    ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK);

    u64 event_count = 0;
    u64 syscall_count = 0;
    bool in_syscall = false;
    long current_nr = 0;
    std::vector<std::string> current_args;

    // initial continue
    ptrace(PTRACE_SYSCALL, pid, nullptr, nullptr);

    while (true) {
        if (waitpid(pid, &status, 0) < 0) break;

        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            Event ev; ev.type = EventType::ProcessExit; ev.pid = pid; ev.tid = pid;
            ProcessEvent pe; pe.pid = pid; pe.tid = pid; pe.detail = "exited";
            ev.payload = pe;
            session.record_event(ev); event_count++;
            break;
        }
        if (!WIFSTOPPED(status)) { ptrace(PTRACE_SYSCALL, pid, nullptr, nullptr); continue; }

        int sig = WSTOPSIG(status);
        if (sig == (SIGTRAP | 0x80)) {
            struct user_regs_struct regs{};
            if (ptrace(PTRACE_GETREGS, pid, nullptr, &regs) == 0) {
                if (!in_syscall) {
                    in_syscall = true;
                    current_nr = regs.orig_rax;
                    current_args.clear();
                    auto hex = [](unsigned long v){ std::ostringstream o; o<<"0x"<<std::hex<<v; return o.str(); };
                    current_args.push_back(hex(regs.rdi));
                    current_args.push_back(hex(regs.rsi));
                    current_args.push_back(hex(regs.rdx));
                    Event ev; ev.type = EventType::SyscallEnter; ev.pid = pid; ev.tid = pid;
                    SyscallEvent se; se.name = syscall_name(current_nr); se.number = current_nr; se.args = current_args;
                    ev.payload = se;
                    session.record_event(ev); event_count++;
                } else {
                    in_syscall = false;
                    Event ev; ev.type = EventType::SyscallExit; ev.pid = pid; ev.tid = pid;
                    SyscallEvent se; se.name = syscall_name(current_nr); se.number = current_nr;
                    std::ostringstream o; o << "0x" << std::hex << regs.rax; se.result = o.str();
                    ev.payload = se;
                    session.record_event(ev); event_count++;
                    syscall_count++;
                    if (syscall_count % 10 == 0) {
                        auto mr = memory::read_proc_maps(pid);
                        if (mr.ok()) {
                            std::vector<storage::CheckpointRegion> regions;
                            for (const auto& r : mr.value().regions()) {
                                storage::CheckpointRegion cr;
                                cr.base = r.base; cr.size = r.size; cr.name = r.name;
                                cr.readable = r.readable; cr.writable = r.writable; cr.executable = r.executable;
                                if (r.readable && r.size > 0) {
                                    // Capture the full mapped region (bounded to avoid OOM on huge mappings).
                                    const u64 MAX_REGION_BYTES = 16 * 1024 * 1024; // 16 MiB per region
                                    u64 toread = r.size;
                                    if (toread > MAX_REGION_BYTES) toread = MAX_REGION_BYTES;
                                    auto br = memory::read_proc_mem(pid, r.base, toread);
                                    if (br.ok()) cr.bytes = br.value();
                                }
                                regions.push_back(cr);
                            }
                            session.add_checkpoint(session.current_time().sequence, "auto", regions);
                        }
                    }
                }
            }
            ptrace(PTRACE_SYSCALL, pid, nullptr, nullptr);
        } else if (sig == SIGTRAP) {
            ptrace(PTRACE_SYSCALL, pid, nullptr, nullptr);
        } else {
            ptrace(PTRACE_SYSCALL, pid, nullptr, (void*)(long)sig);
        }
    }

    return Result<u64>(event_count);
}

} // namespace instrumentation
} // namespace chronos
