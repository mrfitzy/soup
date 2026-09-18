#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void profiler_start(void);

void profiler_set_thread_name(const char*);

#ifdef __cplusplus
} // extern "C"
#endif
