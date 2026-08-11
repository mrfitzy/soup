#pragma once

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

int ui_run(int (*work_fn)(void*), void* data);

void ui_update(void* data);

#ifdef __cplusplus
} // extern "C"
#endif
