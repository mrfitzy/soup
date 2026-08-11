#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void profiler_init(void);

void profiler_set_thread_name(const char*);

#ifdef __cplusplus
} // extern "C"
#endif
