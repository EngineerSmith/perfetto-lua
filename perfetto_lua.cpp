#include <perfetto.h>
#include <fcntl.h>
#include <cstdarg>

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

#define GLOBAL_TRACK_ID 471353371973

extern "C" {

static std::unique_ptr<perfetto::TracingSession> g_tracing_session;
static int g_fd = -1;

PL_EXPORT void pl_initializePerfetto() { // Ran once per process
    perfetto::TracingInitArgs args;
    args.backends |= perfetto::kInProcessBackend;
    perfetto::Tracing::Initialize(args);
    perfetto::TrackEvent::Register();

    auto desc = perfetto::Track(GLOBAL_TRACK_ID).Serialize();
    desc.set_name("Global Events");
    perfetto::TrackEvent::SetTrackDescriptor(perfetto::Track(GLOBAL_TRACK_ID), desc);
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

enum class pl_counterUnits {
    bytes,
    nanoseconds,
    count,
};

enum class pl_traceType {
    None = 0,
    typeBool,
    typeDouble,
    typeString,
};

enum class pl_instantScope {
    thread,
    process,
    global,
};

inline void pl_parse_varargs(perfetto::EventContext& ctx, va_list* args) {
    while (true) {
        pl_traceType type = static_cast<pl_traceType>(va_arg(args, int));
        if (type == pl_traceType::None)
            break;

        const char* key = va_arg(args, const char*);
        if (!key) break;

        auto* debug = ctx.event()->add_debug_annotations();
        debug->set_name(key);

        switch(type) {
            case pl_traceType::typeBool:
                debug->set_bool_value(va_arg(args, int) != 0);
                break;
            case pl_traceType::typeDouble:
                debug->set_double_value(va_arg(args, double));
                break;
            case pl_traceType::typeString:
                debug->set_string_value(va_arg(args, const char*));
                break;
        }
    }
}

inline perfetto::Track pl_getInstantTrack(pl_instantScope scope) {
    switch(scope) {
        default:
        case pl_instantScope::thread:
            return perfetto::ThreadTrack::Current();
        case pl_instantScope::process:
            return perfetto::ProcessTrack::Current();
        case pl_instantScope::global:
            return perfetto::Track(GLOBAL_TRACK_ID);
    }
}

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

PL_EXPORT void pl_trace_endEvent_double(const char* cat, const char* key, double value) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_END(dCat, [&](perfetto::EventContext ctx) {
        auto* debug = ctx.event()->add_debug_annotations();
        debug->set_name(key);
        debug->set_double_value(value);
    });
}

PL_EXPORT void pl_trace_endEvent_bool(const char* cat, const char* key, bool value) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_END(dCat, [&](perfetto::EventContext ctx) {
        auto* debug = ctx.event()->add_debug_annotations();
        debug->set_name(key);
        debug->set_bool_value(value);
    });
}

PL_EXPORT void pl_trace_endEvent_string(const char* cat, const char* key, const char* value) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_END(dCat, [&](perfetto::EventContext ctx) {
        auto* debug = ctx.event()->add_debug_annotations();
        debug->set_name(key);
        debug->set_string_value(value);
    });
}

PL_EXPORT void pl_trace_endEvent_varargs(const char* cat, ...) {
    perfetto::DynamicCategory dCat{ cat };

    va_list args;
    va_start(args, cat);

    TRACE_EVENT_END(dCat, [&](perfetto::EventContext ctx) {
        pl_parse_varargs(ctx, &args);
    });

    va_end(args);
}

PL_EXPORT void pl_trace_counter(const char* cat, const char* name, double value) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_COUNTER(dCat, perfetto::DynamicString(name), value);
}

PL_EXPORT void pl_trace_counter_unit(const char* cat, const char* name, double value, pl_counterUnits units) {
    perfetto::DynamicCategory dCat{ cat };

    auto track = perfetto::CounterTrack(perfetto::DynamicString(name));

    switch(units) {
        case pl_counterUnits::bytes:
            track.set_unit(perfetto::CounterTrack::Unit::UNIT_SIZE_BYTES);
            break;
        case pl_counterUnits::nanoseconds:
            track.set_unit(perfetto::CounterTrack::Unit::UNIT_TIME_NS);
            break;
        default:
        case pl_counterUnits::count:
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

PL_EXPORT void pl_trace_instant(const char* cat, const char* name, pl_instantScope scope) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_INSTANT(dCat, perfetto::DynamicString(name), pl_getInstantTrack(scope));
}

PL_EXPORT void pl_trace_instant_double(const char* cat, const char* name, pl_instantScope scope, const char* key, double value) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_INSTANT(dCat, perfetto::DynamicString(name), pl_getInstantTrack(scope),
        [&](perfetto::EventContext ctx) {
            auto* debug = ctx.event()->add_debug_annotations();
            debug->set_name(key);
            debug->set_double_value(value);
        }
    );
}

PL_EXPORT void pl_trace_instant_bool(const char* cat, const char* name, pl_instantScope scope, const char* key, bool value) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_INSTANT(dCat, perfetto::DynamicString(name), pl_getInstantTrack(scope),
        [&](perfetto::EventContext ctx) {
            auto* debug = ctx.event()->add_debug_annotations();
            debug->set_name(key);
            debug->set_bool_value(value);
        }
    );
}

PL_EXPORT void pl_trace_instant_string(const char* cat, const char* name, pl_instantScope scope, const char* key, const char* value) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_INSTANT(dCat, perfetto::DynamicString(name), pl_getInstantTrack(scope),
        [&](perfetto::EventContext ctx) {
            auto* debug = ctx.event()->add_debug_annotations();
            debug->set_name(key);
            debug->set_string_value(value);
        }
    );
}

PL_EXPORT void pl_trace_instant_varargs(const char* cat, const char* name, pl_instantScope scope, ...) {
    perfetto::DynamicCategory dCat{ cat };

    va_list args;
    va_start(args, scope);

    TRACE_EVENT_INSTANT(dCat, perfetto::DynamicString(name), pl_getInstantTrack(scope),
        [&](perfetto::EventContext ctx) {
            pl_parse_varargs(ctx, &args);
        }
    );

    va_end(args);
}

PL_EXPORT void pl_trace_instant_flowStart(const char* cat, const char* name, pl_instantScope scope, uint64_t flowID) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_INSTANT(dCat, perfetto::DynamicString(name), pl_getInstantTrack(scope),
        perfetto::Flow::ProcessScoped(flowID)
    );
}

PL_EXPORT void pl_trace_instant_flowStart_varargs(const char* cat, const char* name, pl_instantScope scope, uint64_t flowID, ...) {
    perfetto::DynamicCategory dCat{ cat };

    va_list args;
    va_start(args, flowID);

    TRACE_EVENT_INSTANT(dCat, perfetto::DynamicString(name), pl_getInstantTrack(scope),
        perfetto::Flow::ProcessScoped(flowID),
        [&](perfetto::EventContext ctx) {
            pl_parse_varargs(ctx, &args);
        }
    );

    va_end(args);
}

PL_EXPORT void pl_trace_instant_flowEnd(const char* cat, const char* name, pl_instantScope scope, uint64_t flowID) {
    perfetto::DynamicCategory dCat{ cat };
    TRACE_EVENT_INSTANT(dCat, perfetto::DynamicString(name), pl_getInstantTrack(scope),
        perfetto::TerminatingFlow::ProcessScoped(flowID)
    );
}

PL_EXPORT void pl_trace_instant_flowEnd_varargs(const char* cat, const char* name, pl_instantScope scope, uint64_t flowID, ...) {
    perfetto::DynamicCategory dCat{ cat };

    va_list args;
    va_start(args, flowID);

    TRACE_EVENT_INSTANT(dCat, perfetto::DynamicString(name), pl_getInstantTrack(scope),
        perfetto::TerminatingFlow::ProcessScoped(flowID),
        [&](perfetto::EventContext ctx) {
            pl_parse_varargs(ctx, &args);
        }
    );

    va_end(args);
}

}