#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ImDrawList;
typedef unsigned int ImU32;

void draw_tile(
    const uint8_t* data,
    const float* palette,
    struct ImDrawList* dl,
    float dim);

#ifdef __cplusplus
} // extern "C"
#endif

