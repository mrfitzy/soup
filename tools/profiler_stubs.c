#include "profiler.h"

#ifdef TRACY_ENABLE
#error "building stubs with TRACY_ENABLE defined"
#endif

void profiler_init() {}

void profiler_set_thread_name(const char* name) {
  (void)name;
}
