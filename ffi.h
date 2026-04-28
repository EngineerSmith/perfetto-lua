typedef enum {
  UNIT_BYTES,
  UNIT_NANOSECONDS,
  UNIT_COUNT,
} pl_counterUnits;

typedef enum {
  TYPE_END,
  TYPE_BOOL,
  TYPE_DOUBLE,
  TYPE_STRING,
} pl_traceType;

void pl_initializePerfetto();

bool pl_startTracing(const char* filename);
void pl_stopTracing();

void pl_setProcessName(const char* name);
void pl_setThreadName(const char* name);

void pl_trace_beginEvent(const char* category, const char* name);
void pl_trace_beginEvent_flowStart(const char* category, const char* name, uint64_t flowID);
void pl_trace_beginEvent_flowEnd(const char* category, const char* name, uint64_t flowID);
void pl_trace_endEvent(const char* category);
void pl_trace_endEvent_double(const char* category, const char* key, double value);
void pl_trace_endEvent_bool(const char* category, const char* key, bool value);
void pl_trace_endEvent_string(const char* category, const char* key, const char* value);
void pl_trace_endEvent_varargs(const char* category, ...);

void pl_trace_counter(const char* category, const char* name, double value);
void pl_trace_counter_unit(const char* category, const char* name, double value, pl_counterUnits unit);
void pl_trace_counter_count(const char* category, const char* name, double value, int64_t multiplier);

void pl_trace_instant(const char* category, const char* name);
void pl_trace_instant_double(const char* category, const char* name, const char* key, double value);
void pl_trace_instant_bool(const char* category, const char* name, const char* key, bool value);
void pl_trace_instant_string(const char* category, const char* name, const char* key, const char* value);
void pl_trace_instant_varargs(const char* category, const char* name, ...);
void pl_trace_instant_flowStart(const char* category, const char* name, uint64_t flowID);
void pl_trace_instant_flowStart_varargs(const char* category, const char* name, uint64_t flowID, ...);
void pl_trace_instant_flowEnd(const char* category, const char* name, uint64_t flowID);
void pl_trace_instant_flowEnd_varargs(const char* category, const char* name, uint64_t flowID, ...);
