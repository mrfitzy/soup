#include "diag.h"

#include "mem.h"
#include "op.h"
#include "regs.h"

#include <assert.h>
#include <execinfo.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
print_backtrace(void) {
  void* callstack[16];
  int frames = backtrace(callstack, 16);
  char** strs = backtrace_symbols(callstack, frames);
  printf("---backtrace begin---\n");
  for (int i = 0; i < frames; i++) {
    printf("%s\n", strs[i]);
  }
  free(strs);
  printf("---backtrace end---\n");
}

void
print_data(const uint8_t* data, size_t length) {
  printf("%02X", *data);
  for (size_t i = 1; i < length; i++)
    printf("%c%02X", (((i % 16 == 0) && (i != 0)) ? '\n' : ' '), data[i]);
  printf("\n");
}

typedef uint64_t half_row_size_t;

void print_mem(const struct mem* mem) {
  size_t half_word_size = sizeof(half_row_size_t);
  size_t word_size = half_word_size * 2;
  assert(mem->size % word_size == 0);
  const half_row_size_t* ptr = (const half_row_size_t*)mem->mem;
  size_t n = mem->size / half_word_size;
  half_row_size_t last_print = 0;
  half_row_size_t last_print_val_lo = 0x7766554433221100;
  half_row_size_t last_print_val_hi = 0xffeeddccbbaa9988;
  printf("[mem] ");
  for (size_t i = 0; i < word_size; i++) {
    printf("%02zX ", i);
  }
  printf("\n");
  for (size_t i = 0; i < (5 + (3 * word_size)); i++) {
    putchar('-');
  }
  printf("\n");
  for (half_row_size_t lo = 0; lo < n; lo += 2) {
    half_row_size_t val_lo = ptr[lo];
    half_row_size_t val_hi = ptr[lo + 1];
    bool print = (val_lo != last_print_val_lo)
              || (val_hi != last_print_val_hi)
              || (lo == (n - 2));
    if (!print)
      continue;
    if (lo - last_print > 2)
      printf("...\n");
    last_print = lo;
    last_print_val_lo = val_lo;
    last_print_val_hi = val_hi;
    const uint8_t* iter = (const uint8_t*)(ptr + lo);
    printf("%04X ", (uint16_t)(iter - mem->mem));
    for (size_t i = 0; i < word_size; i++) {
      printf(" %02X", *(iter++));
    }
    printf("\n");
  }
}

void
mem_copy_row(const struct mem* mem, int row, uint8_t* buf, size_t buf_size) {
  assert(row >= 0);
  const size_t row_size = sizeof(half_row_size_t) * 2;
  assert(mem->size % row_size == 0);
  assert(buf_size >= row_size);
  const size_t rows = mem->size / row_size;
  if ((size_t)row >= rows) {
    return;
  }
  const half_row_size_t* ptr = (const half_row_size_t*)mem->mem + (row * 2);
  memcpy(buf, (const uint8_t*)ptr, row_size);
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
  printf("AF: %04x\n", regs->af);
  printf("BC: %04x\n", regs->bc);
  printf("DE: %04x\n", regs->de);
  printf("HL: %04x\n", regs->hl);
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

static inline size_t
rom_to_str_arg(
    const struct op* op,
    uint16_t ppc,
    const uint8_t* pc,
    size_t rom_size,
    char* buf,
    size_t buf_size,
    const char** it) {
  const char* iter = *it;
  assert(rom_size >= op->length);
  assert(iter[0] != 0); assert(iter[1] != 0);
  // {d8, d16, a8, a16, r8}
  size_t len = 0;
  if ((iter[0] == 'd') && (iter[1] == '8')) {
    len = snprintf(buf, buf_size, "$%02x", pc[1]);
    *it += 2;
  } else if ((iter[0] == 'd') && (iter[1] == '1')) {
    assert(iter[2] == '6');
    len = snprintf(buf, buf_size, "$%04x", ((uint16_t)pc[2] << 8) | pc[1]);
    *it += 3;
  } else if ((iter[0] == 'a') && (iter[1] == '8')) {
    len = snprintf(buf, buf_size, "$ff00+$%02x", pc[1]);
    *it += 2;
  } else if ((iter[0] == 'a') && (iter[1] == '1')) {
    assert(iter[2] == '6');
    len = snprintf(buf, buf_size, "%04x", ((uint16_t)pc[2] << 8) | pc[1]);
    *it += 3;
  } else if ((iter[0] == 'r') && (iter[1] == '8')) {
    len = snprintf(buf, buf_size, "%04x", ppc + (int8_t)(pc[1]) + 2);
    *it += 2;
  } else {
    assert(false);
  }
  return len;
}

// @returns next pc for iterating rom; 0 for EOF
uint16_t rom_to_str(
    const struct op* ops,
    const struct op* cb_ops,
    uint16_t ppc,
    const uint8_t* rom,
    size_t rom_size,
    char* buf,
    size_t buf_size) {
  const uint8_t* pc = rom + ppc;
  if (pc >= rom + rom_size) {
    return 0;
  }
  const uint8_t opcode = *pc;
  const struct op* op = (opcode == 0xcb ? cb_ops + *(pc + 1) : ops + opcode);
  if (op->length == 0) {
    return 0;
  }
  assert(buf_size > 0);
  // find and replace {d8, d16, a8, a16, r8} while copying
  size_t len = 0, ilen = 0;
  const char* iter = op->text;
  while ((len < buf_size) && (*iter != 0)) {
    if ((*iter == 'd') || (*iter == 'a') || (*iter == 'r')) {
      ilen = rom_to_str_arg(op, ppc, pc, rom_size - (pc - rom), buf, buf_size - len, &iter);
    } else {
      *buf = *iter;
      ilen = 1;
      iter++;
    }
    buf += ilen;
    len += ilen;
  }
  assert(len < buf_size);
  *buf = 0;
  return (ppc + op->length);
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
