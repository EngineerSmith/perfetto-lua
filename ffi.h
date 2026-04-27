typedef enum {
  // Memory
  UNIT_BYTES,
  UNIT_KILOBYTES,
  UNIT_MEGABYTES,
  UNIT_GIGABYTES,
  // Time
  UNIT_NANOSECONDS,
  UNIT_MICROSECONDS,
  UNIT_MILLISECONDS,
  UNIT_SECONDS,
  UNIT_MINUTES,
  UNIT_HOURS,
  // Generic
  UNIT_COUNT,
} pl_counterUnits;

void pl_initializePerfetto();

bool pl_startTracing(const char* filename);
void pl_stopTracing();

void pl_setProcessName(const char* name);
void pl_setThreadName(const char* name);

void pl_trace_beginEvent(const char* category, const char* name);
void pl_trace_beginEvent_flowStart(const char* cat, const char* name, uint64_t flowID);
void pl_trace_beginEvent_flowEnd(const char* cat, const char* name, uint64_t flowID);
void pl_trace_endEvent(const char* category);

void pl_trace_counter(const char* category, const char* name, double value);
void pl_trace_counter_unit(const char* category, const char* name, double value, pl_counterUnits unit);
void pl_trace_counter_count(const char* category, const char* name, double value, int64_t multiplier);

void pl_trace_instant(const char* category, const char* name);
