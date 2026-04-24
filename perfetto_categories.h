#ifndef PERFETTO_CATEGORIES
#define PERFETTO_CATEGORIES

#include <perfetto.h>

// https://github.com/google/perfetto/blob/main/examples/sdk/trace_categories.h

PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("perfetto-lua")
        .SetDescription("Lua Tracing")
);

#endif