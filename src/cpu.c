#include "cpu.h"

#include "bits.h"
#include "diag.h"
#include "mem.h"
#include "op.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

extern const struct op* ops_data;
extern const struct op* cb_ops_data;

static inline const struct op*
get_op(const struct cpu* cpu, const struct op** cb_op_out) {
  uint16_t pc = cpu->regs.pc;
  size_t rom_size = cpu->rom_size;
  assert(pc < rom_size);
  const uint8_t* rom = cpu->rom;
  uint8_t opcode = rom[pc];
  if (opcode != 0xcb) {
    *cb_op_out = NULL;
  } else {
    pc++;
    assert(pc < rom_size);
    *cb_op_out = cb_ops_data + rom[pc];
  }

  return ops_data + opcode;
}

static inline void
regs_update_pc(struct regs* regs, const struct op* op) {
    regs->pc += op->length;
}

static inline void
regs_mark_all_flags_clean(struct regs* regs) {
  regs->dirty_flags.val = 0;
}

static inline void
regs_mark_all_flags_dirty(struct regs* regs) {
  regs->dirty_flags.val = 0xff;
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

static bool
update_flag_post_op(
    struct regs* regs,
    char flag,
    uint8_t flagcode,
    uint8_t old_val,
    uint8_t val,
    flag_func_t func) {
  bool printed = true;
  uint8_t new_flag_val = 0xff;
  if (flagcode == 0b00) {
    return !printed; // skip
  } else if (flagcode == 0b01) {
    new_flag_val = 1;
  } else if (flagcode == 0b10) {
    new_flag_val = 0;
  } else {
    assert(flagcode == 0b11);
    new_flag_val = func(old_val, val);
  }
  struct flags* flags = &regs->f;
  struct flags* dirty_flags = &regs->dirty_flags;
  uint8_t old_flag_val;
  switch (flag) {
  case 'z':
    if (dirty_flags->z) return !printed;
    old_flag_val = flags->z;
    flags->z = new_flag_val;
    break;
  case 'n':
    if (dirty_flags->n) return !printed;
    old_flag_val = flags->n;
    flags->n = new_flag_val;
    break;
  case 'h':
    if (dirty_flags->h) return !printed;
    old_flag_val = flags->h;
    flags->h = new_flag_val;
    break;
  case 'c':
    if (dirty_flags->c) return !printed;
    old_flag_val = flags->c;
    flags->c = new_flag_val;
    break;
  default:
    fprintf(stderr, "error: unknown flag %c\n", flag);
    assert(false);
  }
  printf("  updating flag %c from %x to %x\n",
      flag, old_flag_val, new_flag_val);
  return printed;
}

static void
update_flags_post_op(
    struct regs* regs, const struct op* op, uint8_t old_val, uint8_t val) {
  char flag_chars[] = { 'z', 'n', 'h', 'c' };
  flag_func_t funcs[] = { calc_z, calc_n, calc_h, calc_c };
  bool printed = false;
  for (size_t i = 0; i < sizeof(flag_chars); i++) {
    char c = flag_chars[i];
    uint8_t flagcode = op_get_flag(op, c);
    printed |=
        update_flag_post_op(regs, c, flagcode, old_val, val, funcs[i]);
  }
  regs_mark_all_flags_dirty(regs);
  if (printed)
    printf("\n");
}

static void
emulate_ld_r_d16(struct regs* regs, const uint8_t* rom, size_t rom_size, uint8_t opcode) {
  // 16-bit load immediate
  assert(rom_size > 0);
  uint8_t regcode = bits_5_4(opcode);
  uint16_t val = *(++rom); // low byte
  val |= (*(++rom) << 8); // high byte
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
    printf("(%s),A\n", reg_name);
  } else {
    regs->a = mem_read(mem, addr);
    printf("A,(%s)\n", reg_name);
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
static void
emulate_bit(struct regs* regs, struct mem* mem, const struct op* op) {
  uint8_t opcode = op->opcode;
  uint8_t bit = bits_5_3(opcode);
  uint8_t regcode = bits_2_0(opcode);
  printf("BIT %x,", bit);
  uint8_t* ptr = regs_get_ptr(regs, mem, regcode, true /* print */);
  uint8_t bitval = bit_n(*ptr, bit);
  printf("\n");
  update_flags_post_op(regs, op, *ptr, bitval);
}

static void
regs_update_n(struct regs* regs, bool set) {
  assert(!regs->dirty_flags.n);
  printf("  updating flag n from %x to %x (manually)\n", regs->f.n, set);
  regs->f.n = set ? 1 : 0;
  regs->dirty_flags.n = 1;
}

static void
regs_update_cy(struct regs* regs, bool set) {
  assert(!regs->dirty_flags.c);
  printf("  updating flag c from %x to %x (manually)\n", regs->f.c, set);
  regs->f.c = set ? 1 : 0;
  regs->dirty_flags.c = 1;
}

static void
emulate_rl(
    struct regs* regs,
    struct mem* mem,
    const struct op* op,
    bool is_cb_op) {
  uint8_t regcode = bits_2_0(op->opcode);
  printf("RL");
  if (is_cb_op)
    printf(" ");
  uint8_t* ptr = regs_get_ptr(regs, mem, regcode, true /* print */);
  uint8_t prev = *ptr;
  printf("\n");
  *ptr = ((*ptr << 1) | regs->f.c);
  regs_update_cy(regs, bit_7(prev));
  update_flags_post_op(regs, op, prev, *ptr);
}

static void
emulate_cb_instruction(
    struct regs* regs,
    struct mem* mem,
    const uint8_t* rom,
    size_t rom_size,
    const struct op* cb_op) {
  assert(rom_size > 1);
  assert(rom[0] == 0xcb);
  uint8_t bitopcode = rom[1];
  bool handled = true;
  //uint8_t prev_a = regs->a;
  switch (bitopcode) {
  case 0x10: case 0x11: case 0x12: case 0x13:
  case 0x14: case 0x15: case 0x16: case 0x17:
    emulate_rl(regs, mem, cb_op, true /* is_cb_op */);
    break;
  default:
    handled = false;
    break;
  }
  if (handled)
    goto post_cb_op;

  handled = true;
  if ((bitopcode & 0b11000000) == 0b01000000) {
    // 01bbbrrr
    emulate_bit(regs, mem, cb_op);
  } else {
    handled = false;
  }

post_cb_op:
  if (!handled) {
    fprintf(stderr, "\nerror: bitopcode %02x not yet implemented\n", bitopcode);
    print_op(cb_op);
    print_regs(regs);
    print_mem(mem);
    assert(false);
  }
}

static void
emulate_jr(
    struct regs* regs,
    const uint8_t* rom,
    size_t rom_size) {
  assert(rom_size > 1);
  printf("JR ");
  int8_t jump_len = (int8_t)(rom[1]) + 2;
  printf("$%04x\n", regs->pc + jump_len);
  regs->pc += jump_len;
}

/**
 * 00 1cc 000: JR cc,r8
 *     ||
 *     condition
 */
static void
emulate_jr_if(
    struct regs* regs,
    const uint8_t* rom,
    size_t rom_size) {
  assert(rom_size > 1);
  uint8_t opcode = rom[0];
  uint8_t condcode = bits_4_3(opcode);
  bool condition;
  printf("JR ");
  switch (condcode) {
  case 0b00:
    condition = !regs->f.z;
    printf("NZ");
    break;
  case 0b01:
    condition = regs->f.z;
    printf("Z");
    break;
  case 0b10:
    condition = !regs->f.c;
    printf("NC");
    break;
  case 0b11:
    condition = regs->f.c;
    printf("C");
    break;
  default:
    fprintf(stderr, "error: unknown condition %02x", condcode);
    assert(false);
  }
  int8_t jump_len = (int8_t)(rom[1]) + 2;
  printf(",$%04x\n", regs->pc + jump_len);
  regs->pc += condition ? jump_len : 2;
  if (!condition)
    printf("break\n");
}

/**
 * 00 xxx 110: LD r,n
 *     \|
 *      regcode
 */
static void
emulate_ld_r_n(
    struct regs* regs,
    struct mem* mem,
    const uint8_t* rom,
    size_t rom_size) {
  assert(rom_size > 1);
  uint8_t opcode = rom[0];
  uint8_t n = rom[1];
  uint8_t regcode = bits_5_3(opcode);
  printf("LD ");
  uint8_t* reg_ptr = regs_get_ptr(regs, mem, regcode, true /* print */);
  *reg_ptr = n;
  printf(",$%02x\n", n);
}

static void
emulate_ld_rc_a_bidi(struct regs* regs, struct mem* mem, uint8_t opcode) {
  uint8_t direction = bit_4(opcode);
  uint16_t addr = 0xff00 + regs->c;
  if (direction == 0) {
    printf("LD ($FF00+C),A\n");
    mem_write(mem, addr, regs->a);
  } else {
    printf("LD A,($FF00+C)\n");
    regs->a = mem_read(mem, addr);
  }
}

static void
emulate_inc(struct regs* regs, struct mem* mem, const struct op* op) {
  uint8_t regcode = bits_5_3(op->opcode);
  printf("INC ");
  uint8_t* ptr = regs_get_ptr(regs, mem, regcode, true /* print */);
  uint8_t prev = *ptr;
  (*ptr)++;
  printf("\n");
  update_flags_post_op(regs, op, prev, *ptr);
}

static void
emulate_ld_r_r(struct regs* regs, struct mem* mem, uint8_t opcode) {
  uint8_t regcode_dst = bits_5_3(opcode);
  uint8_t regcode_src = bits_2_0(opcode);
  printf("LD ");
  uint8_t* ptr_dst = regs_get_ptr(regs, mem, regcode_dst, true /* print */);
  printf(",");
  uint8_t* ptr_src = regs_get_ptr(regs, mem, regcode_src, true /* print */);
  *ptr_dst = *ptr_src;
  printf("\n");
}

static void
emulate_ld_rn_a_bidi(
    struct regs* regs,
    struct mem* mem,
    const uint8_t* rom,
    size_t rom_size) {
  assert(rom_size > 1);
  uint8_t opcode = rom[0];
  uint16_t addr = 0xff00 + rom[1];
  uint8_t direction = bit_4(opcode);
  if (direction == 0) {
    printf("LD ($FF00+$%X),A\n", rom[1]);
    mem_write(mem, addr, regs->a);
  } else {
    printf("LD A,($FF00+$%X)\n", rom[1]);
    regs->a = mem_read(mem, addr);
  }
}

static void
emulate_call(
    struct regs* regs,
    struct mem* mem,
    const uint8_t* rom,
    size_t rom_size) {
  assert(rom_size > 2);
  uint8_t lo = rom[1];
  uint8_t hi = rom[2];
  uint16_t addr = ((hi << 8) | lo);
  printf("CALL $%04X\n", addr);
  regs->pc += 3;
  mem_write(mem, (regs->sp - 1), regs->pc_hi);
  mem_write(mem, (regs->sp - 2), regs->pc_lo);
  regs->sp -= 2;
  regs->pc = addr;
}

static void
emulate_push(struct regs* regs, struct mem* mem, uint8_t opcode) {
  uint8_t regcode = bits_5_4(opcode);
  printf("PUSH ");
  uint8_t* reg_ptr =
      (uint8_t*)regs_get_ptr16_qq(regs, regcode, true /* print */);
  printf("\n");
  // N.B. assumes little-endian
  uint8_t lo = reg_ptr[0];
  uint8_t hi = reg_ptr[1];
  mem_write(mem, (regs->sp - 1), hi);
  mem_write(mem, (regs->sp - 2), lo);
  regs->sp -= 2;
}

static void
emulate_pop(struct regs* regs, struct mem* mem, uint8_t opcode) {
  uint8_t regcode = bits_5_4(opcode);
  printf("POP ");
  uint8_t* ptr = (uint8_t*)regs_get_ptr16_qq(regs, regcode, true /* print */);
  printf("\n");
  // N.B. assumes little-endian
  uint8_t lo = mem_read(mem, regs->sp);
  uint8_t hi = mem_read(mem, (regs->sp + 1));
  ptr[0] = lo;
  ptr[1] = hi;
  regs->sp += 2;
}

static void
emulate_dec(struct regs* regs, struct mem* mem, const struct op* op) {
  uint8_t regcode = bits_5_3(op->opcode);
  printf("DEC ");
  uint8_t* ptr = regs_get_ptr(regs, mem, regcode, true /* print */);
  uint8_t prev = *ptr;
  printf("\n");
  (*ptr)--;
  regs_update_n(regs, 1);
  update_flags_post_op(regs, op, prev, *ptr);
}

static void
emulate_inc16(struct regs* regs, uint8_t opcode) {
  uint8_t regcode = bits_5_4(opcode);
  printf("INC ");
  uint16_t* ptr = regs_get_ptr16_ss(regs, regcode, true /* print */);
  printf("\n");
  (*ptr)++;
}

static void
emulate_ret(struct regs* regs, struct mem* mem) {
  printf("RET\n");
  regs->pc_lo = mem_read(mem, regs->sp++);
  regs->pc_hi = mem_read(mem, regs->sp++);
}

static void
emulate_cp_d8(struct regs* regs, const uint8_t* rom, size_t rom_size) {
  assert(rom_size > 1);
  auto flags = &regs->f;
  uint8_t a = regs->a;
  uint8_t b = rom[1];
  printf("CP $%02x\n", b);
  flags->z = ((a == b) ? 1 : 0);
  flags->n = 1;
  flags->h = ((nibble_lo(b) > nibble_lo(a)) ? 1 : 0);
  flags->c = ((nibble_hi(b) > nibble_hi(a)) ? 1 : 0);
  regs_mark_all_flags_dirty(regs);
}

static void
emulate_sub_r(struct regs* regs, struct mem* mem, uint8_t opcode) {
  auto flags = &regs->f;
  uint8_t regcode = bits_2_0(opcode);
  uint8_t a = regs->a;
  printf("SUB ");
  uint8_t* reg = regs_get_ptr(regs, mem, regcode, true /* print */);
  uint8_t b = *reg;
  regs->a -= b;
  printf("\n");
  flags->z = ((a == b) ? 1 : 0);
  flags->n = 1;
  flags->h = ((nibble_lo(b) > nibble_lo(a)) ? 1 : 0);
  flags->c = ((nibble_hi(b) > nibble_hi(a)) ? 1 : 0);
  regs_mark_all_flags_dirty(regs);
}

static void
emulate_ld_d16_a(
    struct regs* regs,
    struct mem* mem,
    const uint8_t* rom,
    size_t rom_size) {
  assert(rom_size > 2);
  uint16_t addr = ((rom[2] << 8) | rom[1]);
  mem_write(mem, addr, regs->a);
}

void
cpu_execute_instruction(struct cpu* cpu) {
  const struct op* cb_op;
  const struct op* op = get_op(cpu, &cb_op);
  uint8_t opcode = op->opcode;
  struct regs* regs = &cpu->regs;
  struct mem* mem = cpu->mem;
  const uint8_t* rom = cpu->rom + regs->pc;
  size_t rom_size = cpu->rom_size - regs->pc;
  uint8_t prev_a = regs->a;
  bool handled = true;
  bool pc_handled = false;
  switch (opcode) {
  case 0x01: case 0x11: case 0x21: case 0x31:
    emulate_ld_r_d16(regs, rom, rom_size, opcode);
    break;
  case 0x03: case 0x13: case 0x23: case 0x33:
    emulate_inc16(regs, opcode);
    break;
  case 0x05: case 0x0d: case 0x15: case 0x1d:
  case 0x25: case 0x2d: case 0x35: case 0x3d:
    emulate_dec(regs, mem, op);
    break;
  case 0x17: // short-circuit for RL A
    emulate_rl(regs, mem, op, false /* is_cb_op */);
    break;
  case 0x18:
    emulate_jr(regs, rom, rom_size);
    pc_handled = true;
    break;
  case 0xc1: case 0xd1: case 0xe1: case 0xf1:
    emulate_pop(regs, mem, opcode);
    break;
  case 0xc5: case 0xd5: case 0xe5: case 0xf5:
    emulate_push(regs, mem, opcode);
    break;
  case 0xc9:
    emulate_ret(regs, mem);
    pc_handled = true;
    break;
  case 0xcb:
    emulate_cb_instruction(regs, mem, rom, rom_size, cb_op);
    break;
  case 0xcd:
    emulate_call(regs, mem, rom, rom_size);
    pc_handled = true;
    break;
  case 0xea:
    emulate_ld_d16_a(regs, mem, rom, rom_size);
    break;
  case 0xfe:
    emulate_cp_d8(regs, rom, rom_size);
    break;
  default:
    handled = false;
    break;
  }
  if (handled)
    goto post_op;

  handled = true;
  if ((opcode & 0b11000000) == 0b01000000) {
    // 0b11xxxyyy
    emulate_ld_r_r(regs, mem, opcode);
  } else if ((opcode & 0b11000111) == 0b00000010) {
    // 0b00xxx010
    emulate_ld_raddr_a_bidi(regs, mem, opcode);
  } else if ((opcode & 0b11000111) == 0b00000110) {
    // 0b00xxx110
    emulate_ld_r_n(regs, mem, rom, rom_size);
  } else if ((opcode & 0b11101111) == 0b11100010) {
    // 0b111x0010
    emulate_ld_rc_a_bidi(regs, mem, opcode);
  } else if ((opcode & 0b11101111) == 0b11100000) {
    // 0b111x0000
    emulate_ld_rn_a_bidi(regs, mem, rom, rom_size);
  } else if ((opcode & 0b11000111) == 0b00000100) {
    // 0b00xxx100
     emulate_inc(regs, mem, op);
  } else if (bits_7_3(opcode) == 0b10101) {
    // 0b10101xxx
    emulate_xor_r(regs, mem, opcode);
  } else if ((opcode & 0b11100111) == 0b00100000) {
    // 0b001xx000
    emulate_jr_if(regs, rom, rom_size);
    pc_handled = true;
  } else if ((opcode & 0b11111000) == 0b10010000) {
    emulate_sub_r(regs, mem, opcode);
    // 0b10010xxx
  } else {
    handled = false;
  }

post_op:
  if (!handled) {
    fprintf(stderr, "\nerror: opcode %02x not yet implemented\n", opcode);
    print_op(op);
    print_regs(regs);
    print_mem(mem);
    //print_backtrace();
    assert(false);
  }
  if (!pc_handled)
    regs_update_pc(regs, cb_op ? cb_op : op);
  // fallback logic for flags uses accumulator
  update_flags_post_op(regs, cb_op ? cb_op : op, prev_a, regs->a);
  regs_mark_all_flags_clean(regs);
  fflush(stderr);
  fflush(stdout);
}

void
cpu_init(struct cpu* cpu, struct mem* mem, const uint8_t* rom, size_t rom_size) {
  regs_init(&cpu->regs);
  cpu->mem = mem;
  cpu->rom = rom;
  cpu->rom_size = rom_size;
}
