#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ImDrawList;
struct ImVec2;

void draw_tile(
    const uint8_t* data,
    const float* palette,
    const ImVec2& t0,
    float dot_dim,
    ImDrawList* dl);

#ifdef __cplusplus
} // extern "C"
#endif

