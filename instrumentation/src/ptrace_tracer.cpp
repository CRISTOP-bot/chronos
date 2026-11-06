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
        // common x86_64 syscalls (Linux 6.x)
        {0,"read"},{1,"write"},{2,"open"},{3,"close"},{4,"stat"},{5,"fstat"},
        {6,"lstat"},{7,"poll"},{8,"lseek"},{9,"mmap"},{10,"mprotect"},
        {11,"munmap"},{12,"brk"},{13,"rt_sigaction"},{14,"rt_sigprocmask"},
        {16,"ioctl"},{17,"pread64"},{18,"pwrite64"},{19,"readv"},{20,"writev"},
        {21,"access"},{22,"pipe"},{23,"select"},{24,"sched_yield"},
        {25,"mremap"},{26,"msync"},{28,"madvise"},{32,"dup"},{33,"dup2"},
        {35,"nanosleep"},{37,"alarm"},{39,"getpid"},{40,"sendfile"},
        {41,"socket"},{42,"connect"},{43,"accept"},{44,"sendto"},{45,"recvfrom"},
        {46,"sendmsg"},{47,"recvmsg"},{48,"shutdown"},{49,"bind"},{50,"listen"},
        {51,"getsockname"},{52,"getpeername"},{53,"socketpair"},{54,"setsockopt"},
        {55,"getsockopt"},{56,"clone"},{57,"fork"},{58,"vfork"},{59,"execve"},
        {60,"exit"},{61,"wait4"},{62,"kill"},{63,"uname"},{72,"fcntl"},
        {74,"fsync"},{75,"fdatasync"},{76,"truncate"},{77,"ftruncate"},
        {78,"getdents"},{79,"getcwd"},{80,"chdir"},{81,"fchdir"},{82,"rename"},
        {83,"mkdir"},{84,"rmdir"},{85,"creat"},{86,"link"},{87,"unlink"},
        {88,"symlink"},{89,"readlink"},{90,"chmod"},{91,"fchmod"},{92,"chown"},
        {95,"umask"},{96,"gettimeofday"},{97,"getrlimit"},{98,"getrusage"},
        {99,"sysinfo"},{100,"times"},{101,"ptrace"},{102,"getuid"},{104,"getgid"},
        {105,"setuid"},{106,"setgid"},{107,"geteuid"},{108,"getegid"},
        {110,"getppid"},{111,"getpgrp"},{112,"setsid"},{118,"getresuid"},
        {120,"getresgid"},{131,"sigaltstack"},{158,"arch_prctl"},{186,"gettid"},
        {202,"futex"},{218,"set_tid_address"},{228,"clock_gettime"},
        {229,"clock_getres"},{230,"clock_nanosleep"},{231,"exit_group"},
        {232,"epoll_wait"},{233,"epoll_ctl"},{234,"tgkill"},{257,"openat"},
        {258,"mkdirat"},{259,"mknodat"},{260,"fchownat"},{261,"futimesat"},
        {262,"newfstatat"},{263,"unlinkat"},{264,"renameat"},{265,"linkat"},
        {266,"symlinkat"},{267,"readlinkat"},{268,"fchmodat"},{269,"faccessat"},
        {270,"pselect6"},{271,"ppoll"},{273,"set_robust_list"},
        {275,"splice"},{276,"tee"},{277,"sync_file_range"},
        {285,"fallocate"},{288,"timerfd_create"},{290,"eventfd2"},
        {291,"epoll_create1"},{292,"dup3"},{293,"pipe2"},{295,"inotify_init1"},
        {302,"prlimit64"},{318,"getrandom"},{319,"memfd_create"},
        {332,"statx"},{334,"rseq"},{435,"clone3"}
    };
    auto it = m.find(nr);
    if (it != m.end()) return it->second;
    return "syscall_"+std::to_string(nr);
}

// Capture a memory snapshot of the given process and add it as a checkpoint to the session.
// Returns the number of regions captured (0 if maps can't be read).
static size_t capture_memory_checkpoint(pid_t pid, recording::Session& session, const std::string& label) {
    constexpr u64 MAX_REGION_BYTES = 16ULL * 1024 * 1024;
    auto mr = memory::read_proc_maps(pid);
    if (!mr.ok()) return 0;
    std::vector<storage::CheckpointRegion> regions;
    regions.reserve(mr.value().regions().size());
    for (const auto& r : mr.value().regions()) {
        storage::CheckpointRegion cr;
        cr.base       = r.base;
        cr.size       = r.size;
        cr.name       = r.name;
        cr.readable   = r.readable;
        cr.writable   = r.writable;
        cr.executable = r.executable;
        if (r.readable && r.size > 0) {
            const u64 toread = std::min(r.size, MAX_REGION_BYTES);
            auto br = memory::read_proc_mem(pid, r.base, toread);
            if (br.ok()) cr.bytes = std::move(br.value());
        }
        regions.push_back(std::move(cr));
    }
    session.add_checkpoint(session.current_time().sequence, label, regions);
    return regions.size();
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
            // capture final memory checkpoint for the main process
            if (w == pid) {
                capture_memory_checkpoint(w, session, "final");
            }
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
                    // capture checkpoint before process exits
                    if (w == pid && (current_nr[w] == 60 || current_nr[w] == 231)) {
                        capture_memory_checkpoint(w, session, "pre-exit");
                    }
                } else {
                    in_syscall[w] = false;
                    Event ev; ev.type = EventType::SyscallExit; ev.pid = w; ev.tid = w; ev.timestamp = now_ts();
                    SyscallEvent se; se.name = syscall_name(current_nr[w]); se.number = current_nr[w];
                    std::ostringstream o; o << "0x" << std::hex << regs.rax; se.result = o.str();
                    ev.payload = se;
                    session.record_event(ev); event_count++;
                    syscall_count++;
                    const bool is_exit = (current_nr[w] == 60 || current_nr[w] == 231);
                    if (w == pid && (syscall_count % 10 == 0 || is_exit)) {
                        capture_memory_checkpoint(w, session, is_exit ? "final" : "auto");
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
