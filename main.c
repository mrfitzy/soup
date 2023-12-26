#include "diag.h"
#include "op.h"

#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/mman.h>

#define OP_SIZE (16)
#define OPS_BIN_SIZE (OP_SIZE * 256)
#define DMG_ROM_SIZE (256)

_Static_assert(sizeof(struct op) == OP_SIZE, "unexpected size");

static const void*
map_file(const char* path, size_t size) {
  int fd = open(path, O_RDONLY);
  assert(fd != -1);

  const void* data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0 /* offset */);
  if (data == MAP_FAILED) {
    perror(NULL);
    assert(false);
  }

  return data;
}

int
main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "error: missing arguments\n");
    fprintf(stderr, "%s ops.bin cb_ops.bin dmg_rom.bin\n", argv[0]);
    return 1;
  }
  const struct op* ops = map_file(argv[1], OPS_BIN_SIZE);
  const struct op* cb_ops = map_file(argv[2], OPS_BIN_SIZE);
  const uint8_t* rom = map_file(argv[3], DMG_ROM_SIZE);
  print_rom(ops, cb_ops, rom, 0xa8);
  print_data(rom + 0xa8, 0xe0 - 0xa8);
  print_rom(ops, cb_ops, rom + 0xe0, DMG_ROM_SIZE - 0xe0);
  return 0;
}

