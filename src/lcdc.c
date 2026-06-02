#include "lcdc.h"

#include "bits.h"
#include "mem.h"

#include <SDL3/SDL.h>

void
lcdc_init(struct lcdc* lcdc, uint8_t* memmap) {
  lcdc->lcdc = memmap + REG_LCDC;
  lcdc->ly   = memmap + REG_LY;
  lcdc->scx  = memmap + REG_SCX;
  lcdc->scy  = memmap + REG_SCY;

  *(lcdc->lcdc) = 0x11; // lcd off, low bg, bg on
  *(lcdc->ly)   = 0;
  *(lcdc->scx)  = 0;
  *(lcdc->scy)  = 0;
}

static uint64_t
line_complete_cb(void* data, SDL_TimerID id, uint64_t interval) {
  struct lcdc* lcdc = data;
  (void)id;

  bool off = !bit_7(*(lcdc->lcdc));
  if (off) {
    *(lcdc->ly) = 0;
    return 0;
  }

  if (*(lcdc->ly) == 153) {
    *(lcdc->ly) = 0;
  } else {
    *(lcdc->ly) += 1;
  }
  return interval;
}

void
lcdc_reg_write(struct lcdc* lcdc, uint8_t data) {
  const uint8_t prev = *(lcdc->lcdc);
  *(lcdc->lcdc) = data;
  if (bit_7(data) && !bit_7(prev)) {
    // turn on
    SDL_AddTimerNS(SDL_US_TO_NS(109), line_complete_cb, lcdc);
  } else if (!bit_7(data) && bit_7(prev)) {
    // turn off (callback cancels timer)
    *(lcdc->ly) = 0;
  }
}

