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

struct op {
  uint8_t opcode;
  uint8_t length;
  uint8_t duration;
  uint8_t flags;
  char text[12];
};
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

static inline uint8_t
op_get_duration_hi(const struct op* op) {
  return op->duration >> 4;
}

static inline uint8_t
op_get_duration_lo(const struct op* op) {
  return op->duration & 0x0f;
}

static inline uint8_t
op_get_flag(const struct op* op, char c) {
  uint8_t flag = op->flags;
  if (c == 'Z')
    flag = flag >> 6;
  else if (c == 'N')
    flag = (flag >> 4) & 3;
  else if (c == 'H')
    flag = (flag >> 2) & 3;
  else {
    assert(c == 'C');
    flag &= 3;
  }
  return flag;
}

static char
flag_to_char(const struct op* op, char c) {
  uint8_t flag = op_get_flag(op, c);
  if (flag == 0)
    return '-';
  if (flag == 1)
    return '1';
  if (flag == 2)
    return '0';
  assert(flag == 3);
  return c;
}

static void
print_op(const struct op* op) {
  printf("%02x: %s\n%d ", op->opcode, op->text, op->length);
  uint8_t duration_hi = op_get_duration_hi(op);
  if (duration_hi != 0) {
    printf("%d/", duration_hi * 4);
  }
  printf("%d\n", op_get_duration_lo(op) * 4);
  printf(
      "%c %c %c %c\n",
      flag_to_char(op, 'Z'),
      flag_to_char(op, 'N'),
      flag_to_char(op, 'H'),
      flag_to_char(op, 'C'));
}

static void
print_rom(
    const struct op* ops,
    const struct op* cb_ops,
    const uint8_t* rom,
    size_t rom_size) {
  const uint8_t* pc = rom;
  while (pc < rom + rom_size) {
    const uint8_t opcode = *pc;
    const struct op* op = (opcode == 0xcb ? cb_ops + *(pc + 1) : ops + opcode);
    if (op->length == 0) {
      fprintf(stderr, "unsupported opcode:\n");
      print_op(op);
      assert(false);
    }
    printf("%s (len %d)\n", op->text, op->length);
    pc += op->length;
  }
}

static void print_data(const uint8_t* data, size_t length) {
  printf("%02X", *data);
  for (size_t i = 1; i < length; i++)
    printf("%c%02X", (((i % 16 == 0) && (i != 0)) ? '\n' : ' '), data[i]);
  printf("\n");
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
