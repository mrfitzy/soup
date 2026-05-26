#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void signal_handler_run(void);

void signal_handler_quit(void);

bool signal_handler_should_quit(void);

#ifdef __cplusplus
} // extern "C"
#endif
