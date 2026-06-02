#pragma once

#include "lcdc.h"
#include "mem.h"
#include "op.h"
#include "regs.h"

#include <unistd.h>

#pragma clang diagnostic ignored "-Wpadded"
struct dmg_system {
  struct regs regs;
  struct mem mem;
  struct lcdc lcdc;
  const struct op* ops;
  const struct op* cb_ops;
  const uint8_t* rom;
  size_t rom_size;
};

void dmg_init(
  struct dmg_system* dmg,
  const struct op* ops,
  const struct op* cb_ops,
  const uint8_t* rom,
  size_t rom_size);

const struct op* dmg_get_op(
  const struct dmg_system* dmg,
  const struct op** cb_op_out);

uint8_t dmg_get_logo(uint16_t offset);

