#include "diag.h"

#include "mem.h"
#include "op.h"
#include "regs.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

void
print_data(const uint8_t* data, size_t length) {
  printf("%02X", *data);
  for (size_t i = 1; i < length; i++)
    printf("%c%02X", (((i % 16 == 0) && (i != 0)) ? '\n' : ' '), data[i]);
  printf("\n");
}

_Static_assert(sizeof(size_t) == 8, "uhoh");

typedef uint64_t half_row_size_t;

void print_mem(const struct mem* mem) {
  half_row_size_t lo;
  half_row_size_t ffs = 0xffffffffffffffff;
  size_t half_word_size = sizeof(half_row_size_t);
  size_t word_size = half_word_size * 2;
  assert(mem->size % word_size == 0);
  const half_row_size_t* ptr = (const half_row_size_t*)mem->mem;
  half_row_size_t val_lo, val_hi;
  size_t n = mem->size / half_word_size;
  half_row_size_t last_print = 0;
  printf("[mem] ");
  for (size_t i = 0; i < word_size; i++) {
    printf("%02zX ", i);
  }
  printf("\n");
  for (size_t i = 0; i < (5 + (3 * word_size)); i++) {
    putchar('-');
  }
  printf("\n");
  for (lo = 0; lo < n; lo += 2) {
    val_lo = ptr[lo];
    val_hi = ptr[lo + 1];
    if ((lo < 4) || (lo > n - 5) || (val_lo != ffs) || (val_hi != ffs)) {
      if (lo - last_print > 2)
        printf("...\n");
      last_print = lo;
      const uint8_t* iter = (const uint8_t*)(ptr + lo);
      printf("%04X ", (uint16_t)(iter - mem->mem));
      for (size_t i = 0; i < word_size; i++) {
        printf(" %02X", *(iter++));
      }
      printf("\n");
    }
  }
}

void
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

void
print_regs(const struct regs* regs) {
  const struct flags* f = &regs->f;
  printf("AF: %04x A: %02x F: %02x\n", regs->af, regs->a, f->val);
  printf("BC: %04x B: %02x C: %02x\n", regs->bc, regs->b, regs->c);
  printf("DE: %04x D: %02x E: %02x\n", regs->de, regs->d, regs->e);
  printf("HL: %04x H: %02x L: %02x\n", regs->hl, regs->h, regs->l);
  printf("PC: %04x\n", regs->pc);
  printf("SP: %04x\n", regs->sp);
  printf("z:%d n:%d h:%d c:%d (%x)\n", f->z, f->n, f->h, f->c, f->val);
}

void
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

char
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
