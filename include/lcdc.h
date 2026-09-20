#pragma once

struct display;

#include <stdint.h>

#define LCDC_M_CYCLES_PER_LINE (114)

#ifdef __cplusplus
extern "C" {
#endif

struct lcdc {
  uint8_t* lcdc;
  uint8_t* ly;
  uint8_t* scx;
  uint8_t* scy;
};

void lcdc_init(struct lcdc*, uint8_t* memmap);

/// @return true if completed line concludes vblank; false otherwise
bool lcdc_line_complete(struct lcdc*);

void lcdc_reg_write(struct lcdc*, uint8_t data);

#ifdef __cplusplus
} // extern "C"
#endif

