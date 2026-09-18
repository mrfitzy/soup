#pragma once

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

int ui_start(void);

int ui_run(int (*work_fn)(void*), void* data);

void ui_update(void* data);

void ui_stop(void);

#ifdef __cplusplus
} // extern "C"
#endif
