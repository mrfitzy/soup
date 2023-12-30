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

static inline uint8_t bits_7_3(uint8_t byte) {
  return (byte & 0b11111000) >> 3;
}

static inline uint8_t nibble_5_4(uint8_t byte) {
  return (byte & 0b00110000) >> 4;
}

static inline uint8_t deref(uint16_t addr) {
  (void)addr;
  fprintf(stderr, "error: requires memory map (not yet implemented)\n");
  assert(false);
}

static inline void regs_update_pc(struct regs* regs, const struct op* op) {
    regs->pc += op->length;
}

static inline void regs_update_flags(struct regs* regs, const struct op* op) {
  (void)regs;
  if (op->flags != 0) {
    fprintf(stderr, "error: flags not yet implemented\n");
    print_regs(regs);
    assert(false);
  }
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
emulate_16_ld_immediate(struct regs* regs, const uint8_t* rom, uint8_t opcode) {
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

static void
emulate_xor_r(struct regs* regs, uint8_t opcode) {
  uint8_t regcode = (opcode & 0b111);
  printf("XOR ");
  switch (regcode) {
    case 0b000:
      regs->a ^= regs->b;
      printf("B");
      break;
    case 0b001:
      regs->a ^= regs->c;
      printf("C");
      break;
    case 0b010:
      regs->a ^= regs->d;
      printf("D");
      break;
    case 0b011:
      regs->a ^= regs->e;
      printf("E");
      break;
    case 0b100:
      regs->a ^= regs->h;
      printf("H");
      break;
    case 0b101:
      regs->a ^= regs->l;
      printf("L");
      break;
    case 0b110:
      regs->a ^= deref(regs->hl);
      printf("(HL)");
      break;
    case 0b111:
      regs->a ^= regs->a;
      printf("A");
      break;
    default:
      fprintf(stderr, "error: unknown regcode %x\n", regcode);
      assert(false);
  }
  printf("\n");
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
  case 0x31:
    emulate_16_ld_immediate(regs, rom, opcode);
    break;
  default:
    handled = false;
    break;
  }
  if (handled) {
    goto post_op;
  }

  handled = true;
  if (bits_7_3(opcode) == 0b10101) {
    emulate_xor_r(regs, opcode);
  } else {
    handled = false;
  }

post_op:
  if (handled) {
    regs_update_pc(regs, op);
    regs_update_flags(regs, op);
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
