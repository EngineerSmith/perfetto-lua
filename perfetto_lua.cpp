#include <perfetto.h>
#include <fcntl.h>

#if defined(_WIN32)
    #include <io.h>

    #define PL_EXPORT __declspec(dllexport)
#else
    #include <unistd.h>

    #define PL_EXPORT __attribute__((visibility("default")))
#endif

#include "perfetto_categories.h"

// References
// https://github.com/google/perfetto/blob/main/examples/sdk/example.cc
// https://perfetto.dev/docs/instrumentation/tracing-sdk
// https://perfetto.dev/docs/instrumentation/track-events

extern "C" {

static std::unique_ptr<perfetto::TracingSession> g_tracing_session;

PL_EXPORT void pl_initializePerfetto() { // Ran once per process
    perfetto::TracingInitArgs args;
    args.backends |= perfetto::kInProcessBackend;
    perfetto::Tracing::Initialize(args);
    perfetto::TrackEvent::Register();
}

// Bool = success
PL_EXPORT bool pl_startTracing(const char* filename) {
    perfetto::TraceConfig cfg;
    cfg.add_buffers()->set_size_kb(1024 * 64); // 64MB

    auto* ds_cfg = cfg.add_data_sources()->mutable_config();
    ds_cfg->set_name("track_event");

    g_tracing_session = perfetto::Tracing::NewTrace();

#if defined(_WIN32)
    int fd = _open(filename, _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY, _S_IREAD | S_IWRITE);
#else
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0600);
#endif

    if (fd > 0) {
        g_tracing_session->Setup(cfg, fd);
        g_tracing_session->StartBlocking();
        return true;
    }

    return false;
}

PL_EXPORT void pl_stopTracing() {
    if (g_tracing_session) {
        perfetto::TrackEvent::Flush();
        g_tracing_session->StopBlocking();
        g_tracing_session.reset();
    }
}

PL_EXPORT void pl_trace_beginEvent(const char* cat, const char* name) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_BEGIN(dCat, perfetto::DynamicString(name));
}

PL_EXPORT void pl_trace_endEvent(const char* cat) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_END(dCat);
}

PL_EXPORT void pl_trace_counter(const char* cat, const char* name, double unit) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_COUNTER(dCat, perfetto::DynamicString(name), unit);
}

PL_EXPORT void pl_trace_instant(const char* cat, const char* name) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_INSTANT(dCat, perfetto::DynamicString(name));
}

PL_EXPORT void pl_test() {
    TRACE_EVENT_BEGIN("perfetto-lua", "test");
    TRACE_EVENT_END("perfetto-lua");
}

}