#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/mman.h>

#define OP_SIZE (16)
#define OPS_BIN_SIZE (OP_SIZE * 256)

struct op {
  uint8_t opcode;
  uint8_t length;
  uint8_t duration;
  uint8_t flags;
  char text[12];
};
_Static_assert(sizeof(struct op) == OP_SIZE, "unexpected size");

static const struct op* ops;
static const struct op* cb_ops;

static const void*
map_file(const char* path) {
  int fd = open(path, O_RDONLY);
  assert(fd != -1);

  const void* data = mmap(NULL, OPS_BIN_SIZE, PROT_READ, MAP_PRIVATE, fd, 0 /* offset */);
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

int
main(int argc, char** argv) {
  if (argc < 3) {
    fprintf(stderr, "error: missing arguments\n");
    fprintf(stderr, "%s ops.bin cb_ops.bin\n", argv[0]);
    return 1;
  }
  ops = map_file(argv[1]);
  cb_ops = map_file(argv[2]);
  print_op(ops + 0x35);
  print_op(cb_ops + 0x7a);
  return 0;
}
