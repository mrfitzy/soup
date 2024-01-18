#include "args.h"
#include "dmg.h"
#include "emulate.h"

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

static const void*
map_file(const char* path, size_t size) {
  int fd = open(path, O_RDONLY);
  assert_true(fd != -1);

  const void* data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0 /* offset */);
  if (data == MAP_FAILED) {
    perror(NULL);
    assert_true(false);
  }

  return data;
}

static void
assert_mem_equal(const struct mem* a, const struct mem* b) {
  assert_int_equal(a->size, b->size);
  assert_int_equal(a->max_addr, b->max_addr);
  assert_memory_equal(a->mem, b->mem, a->size);
}

static void
assert_flags_equal(const struct flags* a, const struct flags* b) {
  int a_z = a->z;
  int b_z = b->z;
  assert_int_equal(a_z, b_z);
  int a_n = a->n;
  int b_n = b->n;
  assert_int_equal(a_n, b_n);
  int a_h = a->h;
  int b_h = b->h;
  assert_int_equal(a_h, b_h);
  int a_c = a->c;
  int b_c = b->c;
  assert_int_equal(a_c, b_c);
  int a_unused = a->unused;
  int b_unused = b->unused;
  assert_int_equal(a_unused, b_unused);
}

static void
assert_regs_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->a, &b->a, 1);
  assert_flags_equal(&a->f, &b->f);
  assert_memory_equal(&a->b, &b->b, 1);
  assert_memory_equal(&a->c, &b->c, 1);
  assert_memory_equal(&a->d, &b->d, 1);
  assert_memory_equal(&a->e, &b->e, 1);
  assert_memory_equal(&a->h, &b->h, 1);
  assert_memory_equal(&a->l, &b->l, 1);
  assert_memory_equal(&a->sp, &b->sp, 2);
  assert_memory_equal(&a->pc, &b->pc, 2);
  assert_memory_equal(a, b, sizeof(struct regs));
}

static void
assert_dmg_equal(const struct dmg_system* a, const struct dmg_system* b) {
  assert_regs_equal(&a->regs, &b->regs);
  assert_mem_equal(&a->mem, &b->mem);
}

static const struct op* g_ops;
static const struct op* g_cb_ops;
static const uint8_t* g_rom;

static void
emulate_instruction_and_assert_dmg_equal(
    const struct dmg_system* expect,
    struct dmg_system* actual) {
  emulate_instruction(actual);
  assert_dmg_equal(expect, actual);
}

/**
 * af: ce 22
 * bc: 04 9c
 * de: 01 04
 * hl: 80 10
 * sp: ff fa
 * pc: 00 99
 *
 * z: 0
 * n: 0
 * h: 0
 * c: 0
 */
static void
test_emulate_boot_rom(void** state) {
  (void)state;
  struct dmg_system actual;
  dmg_init(&actual, g_ops, g_cb_ops, g_rom, DMG_ROM_SIZE);
  struct dmg_system expect;
  dmg_init(&expect, g_ops, g_cb_ops, g_rom, DMG_ROM_SIZE);
  struct regs* regs = &expect.regs;
  struct flags* flags = &regs->f;
  uint8_t* mem = expect.mem.mem;

  // LD SP,$fffe
  regs->sp = 0xfffe;
  regs->pc += 3;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);
  
  // XOR A
  regs->a = 0;
  flags->val = 0;
  flags->z = 1;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD HL,$9fff
  regs->hl = 0x9fff;
  regs->pc += 3;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // Addr_0007: loop
  int iters = (0x9fff - 0x8000) + 1;
  for (int i = 0; i < iters; i++) {
    // LD (HL-),A
    mem[0x9fff - i] = 0;
    regs->hl--;
    regs->pc += 1;
    emulate_instruction_and_assert_dmg_equal(&expect, &actual);

    // BIT 7,H
    flags->z = (i == iters - 1) ? 1 : 0; // h: 9f
    flags->n = 0;
    flags->h = 1;
    regs->pc += 2;
    emulate_instruction_and_assert_dmg_equal(&expect, &actual);

    // JR NZ, Addr_0007
    regs->pc += (i == iters - 1) ? 2 : -3;
    emulate_instruction_and_assert_dmg_equal(&expect, &actual);
  }

  // LD HL,$ff26
  regs->hl = 0xff26;
  regs->pc += 3;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD C,$11
  regs->c = 0x11;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD A,$80
  regs->a = 0x80;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD (HL-),A
  mem[0xff26] = 0x80;
  regs->hl--;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD ($FF00+C),A
  mem[0xff11] = 0x80;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // INC C
  regs->c = 0x12;
  flags->z = 0;
  flags->n = 0;
  flags->h = 0;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD A,$f3
  regs->a = 0xf3;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD ($FF00+C),A
  mem[0xff12] = 0xf3;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD (HL-),A
  mem[0xff25] = 0xf3;
  regs->hl--;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD A,$77
  regs->a = 0x77;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD (HL),A
  mem[0xff24] = 0x77;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD A,$fc
  regs->a = 0xfc;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD ($FF00+$47),A
  mem[0xff47] = 0xfc;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD DE,$0104
  regs->de = 0x0104;
  regs->pc += 3;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD HL,$8010
  regs->hl = 0x8010;
  regs->pc += 3;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD A,(DE)
  // "Nintendo" Character Data (0104H~0133H)
  regs->a = mem[0x0104];
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // CALL $0095
  mem[0xfffd] = 0x00;
  mem[0xfffc] = 0x2b;
  regs->sp = 0xfffc;
  regs->pc = 0x0095;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // Graphic routine: $0095
  // LD C,A
  regs->c = 0xce;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // LD B,$04
  regs->b = 0x04;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // PUSH BC
  mem[0xfffb] = 0x04;
  mem[0xfffa] = 0xce;
  regs->sp = 0xfffa;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);

  // RL C
  regs->c = 0x9c; // 0xce << 1
  flags->z = 0;
  flags->n = 0;
  flags->h = 0;
  flags->c = 1; // bit_7(0xce)
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(&expect, &actual);
}

int
main(int argc, char** argv) {
  argc--;
  argv++;
  struct soup_args args;
  if (!soup_args_from_argv(argc, argv, &args)) {
    fprintf(stderr, "error: unexpected arg count (%d)\n", argc);
    return 1;
  }
  g_ops = map_file(args.ops_path, OPS_BIN_SIZE);
  g_cb_ops = map_file(args.cb_ops_path, OPS_BIN_SIZE);
  g_rom = map_file(args.rom_path, DMG_ROM_SIZE);
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_emulate_boot_rom),
  };
  return cmocka_run_group_tests(tests, NULL, NULL);
}
