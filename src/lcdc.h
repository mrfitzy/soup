#pragma once

struct display;

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct lcdc {
  uint8_t* lcdc;
  uint8_t* ly;
  uint8_t* scx;
  uint8_t* scy;
  struct display* display;
};

void lcdc_init(struct lcdc*, uint8_t* memmap);

void lcdc_reg_write(struct lcdc*, uint8_t data);

#ifdef __cplusplus
} // extern "C"
#endif

