#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dmg_system;

struct display;

struct display* display_create(struct dmg_system* dmg);

void display_destroy(struct display*);

void display_next_frame(struct display*);

void display_wait_for_frame(struct display*);

void display_render(struct display*);

#ifdef __cplusplus
} // extern "C"
#endif

