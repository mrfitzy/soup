#include "emulate.h"

#include "diag.h"
#include "op.h"
#include "regs.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

struct dmg_system {
  struct regs regs;
  const struct op* ops;
  const struct op* cb_ops;
  const uint8_t* rom;
  size_t rom_size;
};

static uint8_t nibble_5_4(uint8_t byte) {
  return (byte & 0b00110000) >> 4;
}

static void
regs_init(struct regs* regs) {
  regs->af = 0x1122;
  regs->bc = 0x3344;
  regs->de = 0x5566;
  regs->hl = 0x7788;
  regs->sp = 0x99aa;
  regs->pc = 0x0000;
}

static const struct op*
get_op(const struct dmg_system* dmg) {
  assert(dmg->regs.pc < dmg->rom_size);
  const uint8_t opcode = dmg->rom[dmg->regs.pc];
  return (opcode == 0xcb) ? &dmg->cb_ops[opcode] : &dmg->ops[opcode];
}

static void
emulate_instruction(struct dmg_system* dmg) {
  const struct op* op = get_op(dmg);
  const uint8_t opcode = op->opcode;
  const uint8_t* rom = dmg->rom + dmg->regs.pc;
  struct regs* regs = &dmg->regs;
  bool handled = true;
  switch (opcode) {
  case 0x01:
  case 0x11:
  case 0x21:
  case 0x31: {
    // 16-bit load immediate
    uint8_t regcode = nibble_5_4(opcode);
    uint16_t val = *((uint16_t*)(rom + 1));
    printf("LD ");
    if (regcode == 0b00) {
      regs->bc = val;
      printf("BC,");
    } else if (regcode == 0b01) {
      regs->de = val;
      printf("DE,");
    } else if (regcode == 0b10) {
      regs->hl = val;
      printf("HL,");
    } else {
      assert(regcode == 0b11);
      regs->sp = val;
      printf("SP,");
    }
    printf("$%04x\n", val);
  }
    break;
  default:
    handled = false;
    break;
  }
  if (handled) {
    // update pc, flags
    dmg->regs.pc += op->length;
  } else {
    fprintf(stderr, "\nerror: opcode %02x not yet implemented\n", opcode);
    print_op(op);
    print_regs(regs);
    assert(false);
  }
}

void
emulate_rom(
    const struct op* ops,
    const struct op* cb_ops,
    const uint8_t* rom,
    size_t rom_size) {
  struct dmg_system dmg = {
    .regs = { 0 },
    .ops = ops,
    .cb_ops = cb_ops,
    .rom = rom,
    .rom_size = rom_size,
  };
  struct regs* regs = &dmg.regs;
  regs_init(regs);
  print_regs(regs);
  printf("\n");
  while (regs->pc < rom_size) {
    emulate_instruction(&dmg);
  }
  fprintf(stderr, "\nerror: pc overload\n");
  print_regs(regs);
  assert(false);
}
