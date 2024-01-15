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
assert_dmg_equal(const struct dmg_system* a, const struct dmg_system* b) {
  assert_memory_equal(&a->regs, &b->regs, sizeof(struct regs));
  assert_mem_equal(&a->mem, &b->mem);
}

static const struct op* g_ops;
static const struct op* g_cb_ops;
static const uint8_t* g_rom;

static void
test_emulate_boot_rom(void** state) {
  (void)state;
  struct dmg_system actual;
  dmg_init(&actual, g_ops, g_cb_ops, g_rom, DMG_ROM_SIZE);
  struct dmg_system expect;
  dmg_init(&expect, g_ops, g_cb_ops, g_rom, DMG_ROM_SIZE);

  // LD SP,$fffe
  emulate_instruction(&actual);
  expect.regs.sp = 0xfffe;
  expect.regs.pc += 3;

  assert_dmg_equal(&expect, &actual);
}

int main(int argc, char** argv) {
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
