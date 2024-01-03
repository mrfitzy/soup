#pragma once

#include "mem.h"
#include "op.h"
#include "regs.h"

#include <unistd.h>

struct dmg_system {
  struct regs regs;
  struct mem mem;
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
