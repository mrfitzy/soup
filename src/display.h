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

bool display_should_render(struct display*);

void display_render(struct display*);

#ifdef __cplusplus
} // extern "C"
#endif

