#include "profiler.h"

#include <tracy/Tracy.hpp>

void
profiler_init() {
  TracySetProgramName("soup");
}

void
profiler_set_thread_name(const char* name) {
  tracy::SetThreadName(name);
}
