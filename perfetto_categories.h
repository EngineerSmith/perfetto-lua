#ifndef PERFETTO_CATEGORIES
#define PERFETTO_CATEGORIES

#include <perfetto.h>

PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("perfetto-lua")
        .SetDescription("LuaJIT Tracing")
);

#endif