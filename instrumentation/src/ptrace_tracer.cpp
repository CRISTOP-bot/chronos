#include "chronos/instrumentation/ptrace_tracer.h"
#include "chronos/memory/proc_maps.h"
#include "chronos/storage/format.h"
#include "chronos/common/types.h"
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <chrono>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace chronos {
namespace instrumentation {

static std::string syscall_name(long nr) {
    static const std::map<long,std::string> m = {
        {0,"read"},{1,"write"},{2,"open"},{3,"close"},{9,"mmap"},{10,"mprotect"},
        {11,"munmap"},{12,"brk"},{39,"getpid"},{56,"clone"},{57,"fork"},{58,"vfork"},
        {59,"execve"},{60,"exit"},{61,"wait4"},{62,"kill"},{231,"exit_group"},
        {257,"openat"},{318,"getrandom"},{332,"statx"}
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
        if (ptrace(PTRACE_TRACEME, 0, nullptr, nullptr) < 0) _exit(127);
        raise(SIGSTOP);
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(cfg.program.c_str()));
        for (auto& a : cfg.args) argv.push_back(const_cast<char*>(a.c_str()));
        argv.push_back(nullptr);
        execvp(cfg.program.c_str(), argv.data());
        _exit(127);
    }

    int status = 0;
    waitpid(pid, &status, 0);

    const int opts = PTRACE_O_TRACESYSGOOD | PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK | PTRACE_O_TRACEVFORK | PTRACE_O_TRACEEXEC;
    ptrace(PTRACE_SETOPTIONS, pid, nullptr, opts);

    std::set<pid_t> traced;
    traced.insert(pid);
    std::map<pid_t, bool> in_syscall;
    std::map<pid_t, long> current_nr;
    std::map<pid_t, std::vector<std::string>> current_args;
    u64 event_count = 0;
    u64 syscall_count = 0;

    auto now_ts = []() {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch());
    };

    // initial continue
    ptrace(PTRACE_SYSCALL, pid, nullptr, nullptr);

    while (!traced.empty()) {
        int st = 0;
        pid_t w = waitpid(-1, &st, __WALL);
        if (w < 0) break;

        if (WIFEXITED(st) || WIFSIGNALED(st)) {
            traced.erase(w);
            in_syscall.erase(w);
            // record exit
            Event ev; ev.type = EventType::ThreadExit; ev.pid = w; ev.tid = w; ev.timestamp = now_ts();
            ProcessEvent pe; pe.pid = w; pe.tid = w; pe.detail = "exited";
            ev.payload = pe;
            session.record_event(ev); event_count++;
            if (traced.empty()) {
                Event ev2; ev2.type = EventType::ProcessExit; ev2.pid = pid; ev2.tid = pid; ev2.timestamp = now_ts();
                ProcessEvent pe2; pe2.pid = pid; pe2.tid = pid; pe2.detail = "process exited";
                ev2.payload = pe2;
                session.record_event(ev2); event_count++;
            }
            continue;
        }
        if (!WIFSTOPPED(st)) {
            ptrace(PTRACE_SYSCALL, w, nullptr, nullptr);
            continue;
        }

        int event = (st >> 16) & 0xff;
        int sig = WSTOPSIG(st);

        // New thread/process creation
        if (event == PTRACE_EVENT_CLONE || event == PTRACE_EVENT_FORK || event == PTRACE_EVENT_VFORK) {
            unsigned long newpid = 0;
            ptrace(PTRACE_GETEVENTMSG, w, nullptr, &newpid);
            if (newpid != 0 && traced.count((pid_t)newpid) == 0) {
                traced.insert((pid_t)newpid);
                ptrace(PTRACE_SETOPTIONS, (pid_t)newpid, nullptr, opts);
                Event ev; ev.type = EventType::ThreadCreate; ev.pid = (pid_t)newpid; ev.tid = (pid_t)newpid; ev.timestamp = now_ts();
                ProcessEvent pe; pe.pid = (pid_t)newpid; pe.tid = (pid_t)newpid; pe.detail = "thread created";
                ev.payload = pe;
                session.record_event(ev); event_count++;
                ptrace(PTRACE_SYSCALL, (pid_t)newpid, nullptr, nullptr);
            }
            ptrace(PTRACE_SYSCALL, w, nullptr, nullptr);
            continue;
        }

        if (event == PTRACE_EVENT_EXEC) {
            ptrace(PTRACE_SYSCALL, w, nullptr, nullptr);
            continue;
        }

        if (sig == (SIGTRAP | 0x80)) {
            struct user_regs_struct regs{};
            if (ptrace(PTRACE_GETREGS, w, nullptr, &regs) == 0) {
                if (!in_syscall[w]) {
                    in_syscall[w] = true;
                    current_nr[w] = regs.orig_rax;
                    current_args[w].clear();
                    auto hex = [](unsigned long v){ std::ostringstream o; o<<"0x"<<std::hex<<v; return o.str(); };
                    current_args[w].push_back(hex(regs.rdi));
                    current_args[w].push_back(hex(regs.rsi));
                    current_args[w].push_back(hex(regs.rdx));
                    Event ev; ev.type = EventType::SyscallEnter; ev.pid = w; ev.tid = w; ev.timestamp = now_ts();
                    SyscallEvent se; se.name = syscall_name(current_nr[w]); se.number = current_nr[w]; se.args = current_args[w];
                    ev.payload = se;
                    session.record_event(ev); event_count++;
                } else {
                    in_syscall[w] = false;
                    Event ev; ev.type = EventType::SyscallExit; ev.pid = w; ev.tid = w; ev.timestamp = now_ts();
                    SyscallEvent se; se.name = syscall_name(current_nr[w]); se.number = current_nr[w];
                    std::ostringstream o; o << "0x" << std::hex << regs.rax; se.result = o.str();
                    ev.payload = se;
                    session.record_event(ev); event_count++;
                    syscall_count++;
                    if (w == pid && syscall_count % 10 == 0) {
                        auto mr = memory::read_proc_maps(w);
                        if (mr.ok()) {
                            std::vector<storage::CheckpointRegion> regions;
                            for (const auto& r : mr.value().regions()) {
                                storage::CheckpointRegion cr;
                                cr.base = r.base; cr.size = r.size; cr.name = r.name;
                                cr.readable = r.readable; cr.writable = r.writable; cr.executable = r.executable;
                                if (r.readable && r.size > 0) {
                                    const u64 MAX_REGION_BYTES = 16 * 1024 * 1024;
                                    u64 toread = r.size > MAX_REGION_BYTES ? MAX_REGION_BYTES : r.size;
                                    auto br = memory::read_proc_mem(w, r.base, toread);
                                    if (br.ok()) cr.bytes = br.value();
                                }
                                regions.push_back(cr);
                            }
                            session.add_checkpoint(session.current_time().sequence, "auto", regions);
                        }
                    }
                }
            }
            ptrace(PTRACE_SYSCALL, w, nullptr, nullptr);
        } else if (sig == SIGTRAP) {
            ptrace(PTRACE_SYSCALL, w, nullptr, nullptr);
        } else {
            ptrace(PTRACE_SYSCALL, w, nullptr, (void*)(long)sig);
        }
    }

    return Result<u64>(event_count);
}

} // namespace instrumentation
} // namespace chronos
