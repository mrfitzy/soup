#include "emulate.h"

#include "bits.h"
#include "diag.h"
#include "dmg.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static inline void
regs_update_pc(struct regs* regs, const struct op* op) {
    regs->pc += op->length;
}

typedef uint8_t (*flag_func_t)(uint8_t old_val, uint8_t val);

static uint8_t
calc_z(uint8_t old_val, uint8_t val) {
  (void)old_val;
  return val ? 0 : 1;
}

static uint8_t
calc_n(uint8_t old_val, uint8_t val) {
  (void)old_val;
  (void)val;
  // all operations either set or clear n consistently
  fprintf(stderr, "error: not expecting to need to calculate n\n");
  assert(false);
}

static uint8_t
calc_h(uint8_t old_val, uint8_t val) {
  return (bit_3(old_val) ^ bit_3(val)) ? 1 : 0;
}

static uint8_t
calc_c(uint8_t old_val, uint8_t val) {
  return (bit_7(old_val) ^ bit_7(val)) ? 1 : 0;
}

#define OLD_FLAG_FMT "%x to "

static bool
flags_update_flag_post_op(
    struct flags* flags,
    char flag,
    uint8_t flagcode,
    uint8_t old_val,
    uint8_t val,
    flag_func_t func) {
  uint8_t new_flag_val = 0xff;
  bool skipped = true;
  if (flagcode == 0b00) {
    return skipped;
  } else if (flagcode == 0b01) {
    new_flag_val = 1;
  } else if (flagcode == 0b10) {
    new_flag_val = 0;
  } else {
    assert(flagcode == 0b11);
    new_flag_val = func(old_val, val);
  }
  printf("  updating flag %c from ", flag);
  uint8_t old_flag_val;
  switch (flag) {
  case 'z':
    old_flag_val = flags->z;
    flags->z = new_flag_val;
    break;
  case 'n':
    old_flag_val = flags->n;
    flags->n = new_flag_val;
    break;
  case 'h':
    old_flag_val = flags->h;
    flags->h = new_flag_val;
    break;
  case 'c':
    old_flag_val = flags->c;
    flags->c = new_flag_val;
    break;
  default:
    fprintf(stderr, "error: unknown flag %c\n", flag);
    assert(false);
  }
  printf(OLD_FLAG_FMT "%x\n", old_flag_val, new_flag_val);
  return !skipped;
}

static void
flags_update_post_op(
    struct flags* flags, const struct op* op, uint8_t old_val, uint8_t val) {
  char flag_chars[] = { 'z', 'n', 'h', 'c' };
  flag_func_t funcs[] = { calc_z, calc_n, calc_h, calc_c };
  bool all_skipped = false;
  for (size_t i = 0; i < sizeof(flag_chars); i++) {
    char c = flag_chars[i];
    uint8_t flagcode = op_get_flag(op, c);
    all_skipped |=
        flags_update_flag_post_op(flags, c, flagcode, old_val, val, funcs[i]);
  }
  if (!all_skipped)
    printf("\n");
}

static void
emulate_ld_r_d16(struct regs* regs, const uint8_t* rom, size_t rom_size, uint8_t opcode) {
  // 16-bit load immediate
  assert(rom_size > 0);
  uint8_t regcode = bits_5_4(opcode);
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

/**
 * 00 xxy 010: LD A <-> raddr
 *    ||\
 *     \ direction
 *      regcode
 */
static void
emulate_ld_raddr_a_bidi(struct regs* regs, struct mem* mem, uint8_t opcode) {
  uint8_t regcode   = bits_5_4(opcode);
  uint8_t direction = bit_3(opcode);
  uint16_t addr;
  const char* reg_name;
  printf("LD ");
  switch (regcode) {
  case 0b00:
    addr = regs->bc;
    reg_name = "BC";
    break;
  case 0b01:
    addr = regs->de;
    reg_name = "DE";
    break;
  case 0b10:
    addr = regs->hl++;
    reg_name = "HL+";
    break;
  case 0b11:
    addr = regs->hl--;
    reg_name = "HL-";
    break;
  default:
    fprintf(stderr, "error: unknown regcode %x\n", regcode);
    assert(false);
  }

  if (direction == 0) {
    mem_write(mem, addr, regs->a);
    printf("(%s), A\n", reg_name);
  } else {
    regs->a = mem_read(mem, addr);
    printf("A, (%s)\n", reg_name);
  }
}

static void
emulate_xor_r(struct regs* regs, struct mem* mem, uint8_t opcode) {
  uint8_t regcode = (opcode & 0b111);
  printf("XOR ");
  uint8_t* reg = regs_get_ptr(regs, mem, regcode, true /* print */);
  regs->a ^= *reg;
  printf("\n");
}

/**
 * 01 bbb rrr: BIT b, r
 *    \ | \ |
 *      |   regcode
 *      bit
 */
static uint8_t
emulate_bit(struct regs* regs, struct mem* mem, uint8_t bitopcode) {
  uint8_t bit = bits_5_3(bitopcode);
  uint8_t regcode = bits_2_0(bitopcode);
  printf("BIT %x,", bit);
  uint8_t* reg_ptr = regs_get_ptr(regs, mem, regcode, true /* print */);
  uint8_t res = (*reg_ptr & (1 << bit));
  printf("\n");
  return res;
}

static uint8_t
emulate_cb_instruction(
    struct regs* regs,
    struct mem* mem,
    const uint8_t* rom,
    size_t rom_size,
    const struct op* cb_op) {
  (void)regs;
  assert(rom_size > 1);
  assert(rom[0] == 0xcb);
  uint8_t bitopcode = rom[1];
  bool handled = true;
  bool has_result = false;
  uint8_t res;
  if ((bitopcode & 0b11000000) == 0b01000000) {
    // 01bbbrrr
    res = emulate_bit(regs, mem, bitopcode);
    has_result = true;
  } else {
    handled = false;
  }

  if (!handled) {
    fprintf(stderr, "\nerror: bitopcode %02x not yet implemented\n", bitopcode);
    print_op(cb_op);
    print_regs(regs);
    print_mem(mem);
    assert(false);
  }
  return has_result ? res : regs->a;
}

static void
emulate_instruction(struct dmg_system* dmg) {
  const struct op* cb_op;
  const struct op* op = dmg_get_op(dmg, &cb_op);
  uint8_t opcode = op->opcode;
  struct regs* regs = &dmg->regs;
  struct mem* mem = &dmg->mem;
  const uint8_t* rom = dmg->rom + regs->pc;
  size_t rom_size = dmg->rom_size - regs->pc;
  uint8_t prev_a = regs->a;
  bool handled = true;
  uint8_t res;
  bool has_result = false;
  switch (opcode) {
  case 0x01:
  case 0x11:
  case 0x21:
  case 0x31:
    emulate_ld_r_d16(regs, rom, rom_size, opcode);
    break;
  case 0xcb:
    res = emulate_cb_instruction(regs, mem, rom, rom_size, cb_op);
    has_result = true;
    break;
  default:
    handled = false;
    break;
  }
  if (handled) {
    goto post_op;
  }

  handled = true;
  if ((opcode & 0b11000111) == 0b00000010) {
    // 0b00xxx010
    emulate_ld_raddr_a_bidi(regs, mem, opcode);
  } else if (bits_7_3(opcode) == 0b10101) {
    // 0b10101xxx
    emulate_xor_r(regs, mem, opcode);
  } else {
    handled = false;
  }

post_op:
  if (!handled) {
    fprintf(stderr, "\nerror: opcode %02x not yet implemented\n", opcode);
    print_op(op);
    print_regs(regs);
    print_mem(mem);
    assert(false);
  }
  if (!has_result)
    res = regs->a;
  regs_update_pc(regs, cb_op ? cb_op : op);
  flags_update_post_op(&regs->flags, cb_op ? cb_op : op, prev_a /* fix */, res);
  fflush(stderr);
  fflush(stdout);
}

void
emulate_rom(
    const struct op* ops,
    const struct op* cb_ops,
    const uint8_t* rom,
    size_t rom_size) {
  struct dmg_system dmg;
  dmg_init(&dmg, ops, cb_ops, rom, rom_size);
  struct regs* regs = &dmg.regs;
  regs_init(regs);
  print_regs(regs);
  print_mem(&dmg.mem);
  printf("\n");
  while (regs->pc < rom_size) {
    emulate_instruction(&dmg);
  }
  fprintf(stderr, "\nerror: pc overload\n");
  print_regs(regs);
  assert(false);
}

