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
static int g_fd = -1;

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
    g_fd = _open(filename, _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY, _S_IREAD | _S_IWRITE);
#else
    g_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0600);
#endif

    if (g_fd >= 0) {
        g_tracing_session->Setup(cfg, g_fd);
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

    if (g_fd != -1) {
#if defined(_WIN32)
        _close(g_fd);
#else
        close(g_fd);
#endif
        g_fd = -1;
    }
}

PL_EXPORT void pl_setProcessName(const char* name) {
    auto desc = perfetto::ProcessTrack::Current().Serialize();
    desc.mutable_process()->set_process_name(name);
    perfetto::TrackEvent::SetTrackDescriptor(
        perfetto::ProcessTrack::Current(), desc
    );
}

PL_EXPORT void pl_setThreadName(const char* name) {
    auto desc = perfetto::ThreadTrack::Current().Serialize();
    desc.mutable_thread()->set_thread_name(name);
    perfetto::TrackEvent::SetTrackDescriptor(
        perfetto::ThreadTrack::Current(), desc
    );
}

enum pl_counterUnits {
    // Memory
    bytes,
    kilobytes,
    megabytes,
    gigabytes,

    // Time
    nanoseconds,
    microseconds,
    milliseconds,
    seconds,
    minutes,
    hours,

    // Generic Counter
    count,
};

PL_EXPORT void pl_trace_beginEvent(const char* cat, const char* name) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_BEGIN(dCat, perfetto::DynamicString(name));
}

PL_EXPORT void pl_trace_beginEvent_flowStart(const char* cat, const char* name, uint64_t flowID) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_BEGIN(dCat, perfetto::DynamicString(name),
        perfetto::Flow::ProcessScoped(flowID)
    );
}

PL_EXPORT void pl_trace_beginEvent_flowEnd(const char* cat, const char* name, uint64_t flowID) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_BEGIN(dCat, perfetto::DynamicString(name),
        perfetto::TerminatingFlow::ProcessScoped(flowID)
    );
}

PL_EXPORT void pl_trace_endEvent(const char* cat) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_END(dCat);
}

PL_EXPORT void pl_trace_counter(const char* cat, const char* name, double value) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_COUNTER(dCat, perfetto::DynamicString(name), value);
}

PL_EXPORT void pl_trace_counter_unit(const char* cat, const char* name, double value, pl_counterUnits units) {
    perfetto::DynamicCategory dCat{ cat };

    auto track = perfetto::CounterTrack(perfetto::DynamicString(name));

    switch(units) {
        // Memory
        case pl_counterUnits::bytes:
            track.set_unit(perfetto::CounterTrack::Unit::UNIT_SIZE_BYTES);
            break;
        case pl_counterUnits::kilobytes:
            track.set_unit(perfetto::CounterTrack::Unit::UNIT_SIZE_BYTES);
            track.set_unit_multiplier(1024LL);
            break;
        case pl_counterUnits::megabytes:
            track.set_unit(perfetto::CounterTrack::Unit::UNIT_SIZE_BYTES);
            track.set_unit_multiplier(1024LL * 1024);
            break;
        case pl_counterUnits::gigabytes:
            track.set_unit(perfetto::CounterTrack::Unit::UNIT_SIZE_BYTES);
            track.set_unit_multiplier(1024LL * 1024 * 1024);
            break;

        // Time
        case pl_counterUnits::nanoseconds:
            track.set_unit(perfetto::CounterTrack::Unit::UNIT_TIME_NS);
            break;
        case pl_counterUnits::microseconds:
            track.set_unit(perfetto::CounterTrack::Unit::UNIT_TIME_NS);
            track.set_unit_multiplier(1000LL);
            break;
        case pl_counterUnits::milliseconds:
            track.set_unit(perfetto::CounterTrack::Unit::UNIT_TIME_NS);
            track.set_unit_multiplier(1000LL * 1000);
            break;
        case pl_counterUnits::seconds:
            track.set_unit(perfetto::CounterTrack::Unit::UNIT_TIME_NS);
            track.set_unit_multiplier(1000LL * 1000 * 1000);
            break;
        case pl_counterUnits::minutes:
            track.set_unit(perfetto::CounterTrack::Unit::UNIT_TIME_NS);
            track.set_unit_multiplier(60LL * 1000 * 1000 * 1000);
            break;
        case pl_counterUnits::hours:
            track.set_unit(perfetto::CounterTrack::Unit::UNIT_TIME_NS);
            track.set_unit_multiplier(3600LL * 1000 * 1000 * 1000);
            break;

        // Generic Count
        case pl_counterUnits::count:
        default:
            track.set_unit(perfetto::CounterTrack::Unit::UNIT_COUNT);
    }

    TRACE_COUNTER(dCat, track, value);
}

PL_EXPORT void pl_trace_counter_count(const char* cat, const char* name, double value, int64_t mul) {
    perfetto::DynamicCategory dCat{ cat };

    auto track = perfetto::CounterTrack(perfetto::DynamicString(name));
    track.set_unit(perfetto::CounterTrack::Unit::UNIT_COUNT);
    track.set_unit_multiplier(mul);

    TRACE_COUNTER(dCat, track, value);
}

PL_EXPORT void pl_trace_instant(const char* cat, const char* name) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_INSTANT(dCat, perfetto::DynamicString(name));
}



}