#include "args.h"
#include "bits.h"
#include "diag.h"
#include "dmg.h"
#include "emulate.h"
#include "signal_handler.h"
#include "test.h"
#include "debug_args.h"
#include "test_shims.h"
#include "ui/ui.h"

#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <SDL3/SDL.h>

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

static inline void
assert_flag_z_equal(uint8_t a, uint8_t b) {
  assert_int_equal(a, b);
}

static inline void
assert_flag_n_equal(uint8_t a, uint8_t b) {
  assert_int_equal(a, b);
}

static inline void
assert_flag_h_equal(uint8_t a, uint8_t b) {
  assert_int_equal(a, b);
}

static inline void
assert_flag_cy_equal(uint8_t a, uint8_t b) {
  assert_int_equal(a, b);
}

static void
assert_flags_equal(const struct flags* a, const struct flags* b) {
  assert_flag_z_equal(a->z, b->z);
  assert_flag_n_equal(a->n, b->n);
  assert_flag_h_equal(a->h, b->h);
  assert_flag_cy_equal(a->c, b->c);
  int a_unused = a->unused;
  int b_unused = b->unused;
  assert_int_equal(a_unused, b_unused);
}

static inline void
assert_a_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->a, &b->a, 1);
}

static inline void
assert_b_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->b, &b->b, 1);
}

static inline void
assert_c_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->c, &b->c, 1);
}

static inline void
assert_d_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->d, &b->d, 1);
}

static inline void
assert_e_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->e, &b->e, 1);
}

static inline void
assert_h_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->h, &b->h, 1);
}

static inline void
assert_l_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->l, &b->l, 1);
}

static inline void
assert_sp_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->sp, &b->sp, 2);
}

static inline void
assert_pc_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->pc, &b->pc, 2);
}

static void
assert_regs_equal(const struct regs* a, const struct regs* b) {
  assert_a_equal(a, b);
  assert_b_equal(a, b);
  assert_c_equal(a, b);
  assert_d_equal(a, b);
  assert_e_equal(a, b);
  assert_h_equal(a, b);
  assert_l_equal(a, b);
  assert_sp_equal(a, b);
  assert_pc_equal(a, b);
  assert_flags_equal(&a->f, &b->f);
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
static SDL_Semaphore* g_sem = NULL;

static void
emulate_instruction_for_test(struct dmg_system* dmg) {
  while (!SDL_WaitSemaphoreTimeout(g_sem, 100 /* ms */)) {
    if (signal_handler_should_quit()) {
      fprintf(stderr, "signal received, quitting...\n");
      exit(0);
    }
  }
  emulate_instruction(dmg);
}

static void
emulate_instruction_and_assert_dmg_equal(
    const struct dmg_system* expect,
    struct dmg_system* actual) {
  emulate_instruction_for_test(actual);
  assert_dmg_equal(expect, actual);
}

#define hi (true)
#define lo (false)

static uint8_t convert_logo(uint8_t logo, bool nibble) {
  uint8_t bit = (nibble == hi) ? 7 : 3;
  uint8_t val = 0;
  for (int i = 0; i < 4; i++) {
    val <<= 1;
    val |= bit_n(logo, bit);
    val <<= 1;
    val |= bit_n(logo, bit);
    bit--;
  }
  return val;
}

/**
 * af: 80 22
 * bc: 01 70
 * de: 01 04
 * hl: 80 10
 * sp: ff fc
 * pc: 00 99
 *
 * z: 0
 * n: 1
 * h: 0
 * c: 1
 */
static void
test_emulate_boot_rom(void** state) {
  struct debug_args* debug_args = (struct debug_args*)(*state);

  struct dmg_system* actual = &debug_args->dmg;
  dmg_init(actual, g_ops, g_cb_ops, g_rom, DMG_ROM_SIZE);

  struct dmg_system e;
  struct dmg_system* expect = &e;
  dmg_init(expect, g_ops, g_cb_ops, g_rom, DMG_ROM_SIZE);
  struct regs* regs = &expect->regs;
  struct flags* flags = &regs->f;
  uint8_t* mem = expect->mem.mem;

  // LD SP,$fffe
  regs->sp = 0xfffe;
  regs->pc += 3;
  emulate_instruction_and_assert_dmg_equal(expect, actual);
  
  // XOR A
  regs->a = 0;
  flags->val = 0;
  flags->z = 1;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD HL,$9fff
  regs->hl = 0x9fff;
  regs->pc += 3;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // Addr_0007: loop
  uint16_t iters = (0x9fff - 0x8000) + 1;
  uint16_t i;
  for (i = 0; i < iters; i++) {
    // LD (HL-),A
    mem[0x9fff - i] = 0;
    regs->hl--;
    regs->pc += 1;
    emulate_instruction_and_assert_dmg_equal(expect, actual);

    // BIT 7,H
    flags->z = (i == iters - 1) ? 1 : 0; // h: 9f
    flags->n = 0;
    flags->h = 1;
    regs->pc += 2;
    emulate_instruction_and_assert_dmg_equal(expect, actual);

    // JR NZ,Addr_0007
    regs->pc += (i == iters - 1) ? 2 : -3;
    emulate_instruction_and_assert_dmg_equal(expect, actual);
  }

  // LD HL,$ff26
  regs->hl = 0xff26;
  regs->pc += 3;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD C,$11
  regs->c = 0x11;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD A,$80
  regs->a = 0x80;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD (HL-),A
  mem[0xff26] = 0x80;
  regs->hl--;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD ($FF00+C),A
  mem[0xff11] = 0x80;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // INC C
  regs->c = 0x12;
  flags->z = 0;
  flags->n = 0;
  flags->h = 0;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD A,$f3
  regs->a = 0xf3;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD ($FF00+C),A
  mem[0xff12] = 0xf3;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD (HL-),A
  mem[0xff25] = 0xf3;
  regs->hl--;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD A,$77
  regs->a = 0x77;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD (HL),A
  mem[0xff24] = 0x77;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD A,$fc
  regs->a = 0xfc;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD ($FF00+$47),A
  mem[0xff47] = 0xfc;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD DE,$0104
  regs->de = 0x0104;
  regs->pc += 3;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD HL,$8010
  regs->hl = 0x8010;
  regs->pc += 3;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // Addr_0027: loop
  // LD A,(DE)
  // "Nintendo" Character Data (0104H~0133H)
  regs->a = 0xce;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // CALL $0095
  mem[0xfffd] = 0x00;
  mem[0xfffc] = 0x2b;
  regs->sp = 0xfffc;
  regs->pc = 0x0095;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // Graphic routine: $0095
  // LD C,A
  regs->c = 0xce;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD B,$04
  regs->b = 0x04;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // Addr_00098: loop
  // PUSH BC
  mem[0xfffb] = 0x04;
  mem[0xfffa] = 0xce;
  regs->sp = 0xfffa;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // RL C
  regs->c = 0x9c;
  flags->z = 0;
  flags->n = 0;
  flags->h = 0;
  flags->c = 1;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // RLA
  regs->a = 0x9d;
  flags->z = 0;
  flags->n = 0;
  flags->h = 0;
  flags->c = 1;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // POP BC
  regs->bc = 0x04ce;
  regs->sp = 0xfffc;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // RL C
  regs->c = 0x9d;
  flags->z = 0;
  flags->n = 0;
  flags->h = 0;
  flags->c = 1;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // RLA
  regs->a = 0x3b;
  flags->z = 0;
  flags->n = 0;
  flags->h = 0;
  flags->c = 1;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // DEC B
  regs->b = 0x03;
  flags->z = 0;
  flags->n = 1;
  flags->h = 0;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // JR NZ,Addr_0098
  regs->pc = 0x0098;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  for (int i = 0; i < 8 * 3; i++)
  emulate_instruction_for_test(actual);
  regs->a = 0xf0;
  regs->bc = 0x00eb;
  flags->z = 1;
  flags->c = 0;
  mem[0xfffb] = 0x01;
  mem[0xfffa] = 0x75;
  regs->pc = 0x00a3;
  assert_dmg_equal(expect, actual);

  // LD (HL+),A
  mem[0x8010] = 0xf0;
  regs->hl = 0x8011;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // INC HL
  regs->hl = 0x8012;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD (HL+),A
  mem[0x8012] = 0xf0;
  regs->hl = 0x8013;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // INC HL
  regs->hl = 0x8014;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // RET
  regs->sp = 0xfffe;
  regs->pc = 0x002b;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // CALL $0096
  mem[0xfffd] = 0x00;
  mem[0xfffc] = 0x2e;
  regs->sp = 0xfffc;
  regs->pc = 0x0096;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD B,$04
  regs->b = 0x04;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // Addr_00098: loop
  // fast-forward past last iter
  printf("--- [test] fast-forward begin ---\n");
  for (i = 0; i < (4 * 8); i++)
    emulate_instruction_for_test(actual);
  printf("--- [test] fast-forward end ---\n\n");
  regs->a = 0xfc;
  regs->bc = 0x00bc;
  flags->z = 1;
  flags->n = 1;
  flags->h = 0;
  flags->c = 0;
  mem[0xfffb] = 0x01;
  mem[0xfffa] = 0x5e;
  regs->pc = 0x00a3;
  assert_dmg_equal(expect, actual);

  // LD (HL+),A
  mem[0x8014] = 0xfc;
  regs->hl = 0x8015;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // INC HL
  regs->hl = 0x8016;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD (HL+),A
  mem[0x8016] = 0xfc;
  regs->hl = 0x8017;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // INC HL
  regs->hl = 0x8018;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // RET
  regs->sp = 0xfffe;
  regs->pc = 0x002e;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // INC DE
  regs->de = 0x0105;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD A,E
  regs->a = 0x05;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // CP $34
  flags->z = 0;
  flags->n = 1;
  flags->h = 0;
  flags->c = 1;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // JR NZ, Addr_0027
  regs->pc = 0x0027;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // smoke-test convert_logo
  assert_int_equal(0xf0, convert_logo(0xce, hi));
  assert_int_equal(0xfc, convert_logo(0xce, lo));
  assert_int_equal(0xfc, convert_logo(0xed, hi));
  assert_int_equal(0xf3, convert_logo(0xed, lo));

  // fast-forward through remaining iters
  // after iter n:
  // mem[0x8010+8n], mem[0x8012+8n] = mem[0x0104+n]_77665544
  // mem[0x8014+8n], mem[0x8016+8n] = mem[0x0104+n]_33221100
  const int addr_0095_func = 2 + (4 * 8 /* addr_0098_loop */) + 5;
  const int addr_0027_loop =
      (2 + addr_0095_func + 1 + (addr_0095_func - 1) + 4);
  const int n = 0x2f; // 0x0134 - 0x104 + 1 - 1
  for (int i = 0; i < n; i++) {
    printf("%04x\n", actual->regs.de);
    for (int j = 0; j < addr_0027_loop; j++) {
      emulate_instruction_for_test(actual);
    }

    regs->de = (0x0104 + (i + 2));
    regs->hl = (0x8018 + (8 * (i + 1)));
    if (i == (n - 1))
      regs->pc = 0x0034;
    mem[0x8010 + (8 * (i + 1))] = convert_logo(dmg_get_logo(i + 1), hi);
    mem[0x8012 + (8 * (i + 1))] = convert_logo(dmg_get_logo(i + 1), hi);
    mem[0x8014 + (8 * (i + 1))] = convert_logo(dmg_get_logo(i + 1), lo);
    mem[0x8016 + (8 * (i + 1))] = convert_logo(dmg_get_logo(i + 1), lo);

    // not tested (loop/function local variables):
    regs->af = actual->regs.af;
    regs->bc = actual->regs.bc;
    mem[0xfffb] = actual->mem.mem[0xfffb];
    mem[0xfffa] = actual->mem.mem[0xfffa];

    assert_dmg_equal(expect, actual);
  }
  print_regs(&actual->regs);

  // LD DE,$00d8
  regs->de = 0x00d8;
  regs->pc += 3;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD B,$08
  regs->b = 0x08;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // Addr_0039: loop
  // LD A,(DE)
  regs->a = 0x3c;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // INC DE
  regs->de = 0x00d9;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD (HL+),A
  mem[0x8190] = 0x3c;
  regs->hl = 0x8191;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // INC HL
  regs->hl = 0x8192;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // DEC B
  regs->b = 0x07;
  flags->z = 0;
  flags->n = 1;
  flags->h = 1;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // JR NZ,$0039
  regs->pc = 0x0039;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // (Addr_0039: loop) fast-forward through remaining iters
  const uint8_t video_data[] = { 0x3c,0x42,0xb9,0xa5,0xb9,0xa5,0x42,0x3c };
  for (int i = 0; i < 7; i++) {
    for (int j = 0; j < 6; j++)
      emulate_instruction_for_test(actual);
    mem[0x8190 + (2 * (i + 1))] = video_data[i + 1];
  }
  regs->a = video_data[7];
  regs->b = 0;
  regs->de = 0xe0;
  regs->hl = 0x81a0;
  flags->z = 1;
  flags->n = 1;
  flags->h = 0;
  regs->pc = 0x0040;
  assert_dmg_equal(expect, actual);

  // LD A,$19
  regs->a = 0x19;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD ($9910),A
  mem[0x9910] = 0x19;
  regs->pc += 3;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD HL,$992f
  regs->hl = 0x992f;
  regs->pc += 3;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // Addr_0048
  // LD C,$0c
  regs->c = 0x0c;
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // Addr_004a: loop
  // DEC A
  regs->a = 0x18;
  flags->z = 0;
  flags->n = 1;
  flags->h = 0;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // JR Z, $0055
  regs->pc += 2;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // LD (HL-),A
  mem[0x992f] = 0x18;
  regs->hl = 0x992e;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // DEC C
  regs->c = 0x0b;
  flags->z = 0;
  flags->n = 1;
  flags->h = 0;
  regs->pc += 1;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // JR NZ, $004a
  regs->pc = 0x004a;
  emulate_instruction_and_assert_dmg_equal(expect, actual);

  // fast-forward through remaining iters
  print_mem(&actual->mem);
  print_regs(&actual->regs);
}

static int
test_thread(void* data) {
  struct debug_args* debug_args = data;
  const struct soup_args* soup_args = &debug_args->soup_args;
  g_sem = debug_args->sem;
  g_ops = map_file(soup_args->ops_path, OPS_BIN_SIZE);
  g_cb_ops = map_file(soup_args->cb_ops_path, OPS_BIN_SIZE);
  g_rom = map_file(soup_args->rom_path, DMG_ROM_SIZE);
  int rc = test_run(test_emulate_boot_rom, (void**)data);
  return rc;
}

int
main(int argc, char** argv) {
  argc--;
  argv++;
  struct debug_args args;
  if (!soup_args_from_argv(argc, argv, &args.soup_args)) {
    fprintf(stderr, "error: unexpected arg count (%d)\n", argc);
    return 1;
  }

  args.sem = SDL_CreateSemaphore(0);
  if (args.sem == NULL) {
    fprintf(stderr, "error: SDL_CreateSemaphore failed: %s\n", SDL_GetError());
    return 1;
  }

  signal_handler_run();

  int rc = ui_run(test_thread, &args);
  return rc;
}
