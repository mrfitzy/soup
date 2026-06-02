#include "dmg.h"

#include <assert.h>
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
  mem_init(&dmg->mem, &dmg->lcdc, DMG_MAX_ADDR);
  lcdc_init(&dmg->lcdc, dmg->mem.mem);
  dmg->ops = ops;
  dmg->cb_ops = cb_ops;
  dmg->rom = rom;
  dmg->rom_size = rom_size;

  // TODO: rename rom -> boot_rom
  assert(dmg->mem.size >= rom_size);
  memcpy(dmg->mem.mem, rom, rom_size);
}

const struct op*
dmg_get_op(const struct dmg_system* dmg, const struct op** cb_op_out) {
  uint16_t pc = dmg->regs.pc;
  size_t rom_size = dmg->rom_size;
  assert(pc < rom_size);
  const uint8_t* rom = dmg->rom;
  uint8_t opcode = rom[pc];
  if (opcode != 0xcb) {
    *cb_op_out = NULL;
  } else {
    pc++;
    assert(pc < rom_size);
    *cb_op_out = dmg->cb_ops + rom[pc];
  }

  return dmg->ops + opcode;
}

