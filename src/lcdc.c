#include "lcdc.h"

#include "bits.h"
#include "mem.h"

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

bool
lcdc_line_complete(struct lcdc* lcdc) {
  bool off = !bit_7(*(lcdc->lcdc));
  if (off) {
    *(lcdc->ly) = 0;
    return false;
  }

  if (*(lcdc->ly) == 153) {
    *(lcdc->ly) = 0;
    return true;
  }

  *(lcdc->ly) += 1;
  return false;
}

void
lcdc_reg_write(struct lcdc* lcdc, uint8_t data) {
  const uint8_t prev = *(lcdc->lcdc);
  *(lcdc->lcdc) = data;
  if (!bit_7(data) && bit_7(prev)) {
    *(lcdc->ly) = 0; // turn off
  }
}
