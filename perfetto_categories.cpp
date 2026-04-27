#include "perfetto_categories.h"

// https://github.com/google/perfetto/blob/main/examples/sdk/trace_categories.cc
// V54-52 have a bug on MSVC, see my issue https://github.com/google/perfetto/issues/5591

PERFETTO_TRACK_EVENT_STATIC_STORAGE();