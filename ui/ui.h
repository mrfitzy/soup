#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

int ui_run(int (*work_fn)(void*), void* data);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // UI_H

