#include "profiler.h"

#ifdef TRACY_ENABLE
#error "building stubs with TRACY_ENABLE defined"
#endif

void profiler_start() {}

void profiler_set_thread_name(const char*) {}

void profiler_plot(const char*, int) {}
