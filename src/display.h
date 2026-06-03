#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dmg_system;

void display_init(struct dmg_system* dmg);

void display_next_frame(void);

bool display_should_render(void);

void display_render(void);

#ifdef __cplusplus
} // extern "C"
#endif

