#include "dmg.h"

#include <string.h>

void
dmg_init(
    struct dmg_system* dmg,
    const struct op* ops,
    const struct op* cb_ops,
    const uint8_t* rom,
    size_t rom_size) {
  memset(dmg, 0, sizeof(struct dmg_system));
  regs_init(&dmg->regs);
  mem_init(&dmg->mem, DMG_MEM_SIZE);
  dmg->ops = ops;
  dmg->cb_ops = cb_ops;
  dmg->rom = rom;
  dmg->rom_size = rom_size;
}
