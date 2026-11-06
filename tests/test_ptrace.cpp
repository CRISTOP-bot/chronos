#include <gtest/gtest.h>
#include "chronos/instrumentation/ptrace_tracer.h"
#include "chronos/recording/session.h"
#include "chronos/recording/recording_engine.h"

using namespace chronos;

TEST(PtraceTracer, TracesEcho) {
    recording::RecordingEngine eng;
    recording::SessionConfig cfg{"t", "/bin/echo", 1};
    auto sess = eng.create_session(cfg);
    instrumentation::TraceConfig tcfg{"/bin/echo", {"hi"}, "t.chronos"};
    instrumentation::PtraceTracer tracer;
    auto res = tracer.trace(tcfg, *sess);
    ASSERT_TRUE(res.ok()) << (res.ok()? "" : res.error().message);
    EXPECT_GT(res.value(), 0u);
    std::remove("t.chronos");
}
