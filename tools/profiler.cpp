#include "profiler.h"

#include <tracy/Tracy.hpp>

void profiler_start() {
  TracySetProgramName("soup");
}

void profiler_set_thread_name(const char* name) {
  tracy::SetThreadName(name);
}
