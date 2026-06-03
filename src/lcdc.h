#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct lcdc {
  uint8_t* lcdc;
  uint8_t* ly;
  uint8_t* scx;
  uint8_t* scy;
  int* palette; // (r,g,b)*4
};

void lcdc_init(struct lcdc* lcdc, uint8_t* memmap);

void lcdc_reg_write(struct lcdc* lcdc, uint8_t data);

#ifdef __cplusplus
} // extern "C"
#endif

