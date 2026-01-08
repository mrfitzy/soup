#include "regs.h"

#include "mem.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

_Static_assert(sizeof(struct flags) == 1, "unexpected size");
_Static_assert(sizeof(struct regs) == 14, "unexpected size");

void
regs_init(struct regs* regs) {
  regs->af = 0x1100;
  regs->bc = 0x2233;
  regs->de = 0x4455;
  regs->hl = 0x6677;
  regs->sp = 0x8899;
  regs->pc = 0x0000;

  regs->dirty_flags.val = 0;
  memset(regs->pad, 0xff, sizeof(regs->pad));
}

uint8_t*
regs_get_ptr(struct regs* regs, struct mem* mem, uint8_t regcode, bool print) {
  uint8_t* ptr;
  char* reg_name;
  switch (regcode) {
    case 0b000:
      ptr = &regs->b;
      reg_name = "B";
      break;
    case 0b001:
      ptr = &regs->c;
      reg_name = "C";
      break;
    case 0b010:
      ptr = &regs->d;
      reg_name = "D";
      break;
    case 0b011:
      ptr = &regs->e;
      reg_name = "E";
      break;
    case 0b100:
      ptr = &regs->h;
      reg_name = "H";
      break;
    case 0b101:
      ptr = &regs->l;
      reg_name = "L";
      break;
    case 0b110:
      ptr = mem->mem + regs->hl;
      reg_name = "(HL)";
      break;
    case 0b111:
      ptr = &regs->a;
      reg_name = "A";
      break;
    default:
      fprintf(stderr, "error: unknown regcode %x\n", regcode);
      assert(false);
  }
  if (print)
    printf("%s", reg_name);
  return ptr;
}

static uint16_t*
get_ptr16(struct regs* regs, uint8_t regcode, bool print, bool qq) {
  uint16_t* ptr;
  char* reg_name;
  switch (regcode) {
    case 0b00:
      ptr = &regs->bc;
      reg_name = "BC";
      break;
    case 0b01:
      ptr = &regs->de;
      reg_name = "DE";
      break;
    case 0b10:
      ptr = &regs->hl;
      reg_name = "HL";
      break;
    case 0b11:
      ptr = qq ? &regs->af : &regs->sp;
      reg_name = qq ? "AF" : "SP";
      break;
    default:
      fprintf(stderr, "error: unknown regcode %x\n", regcode);
      assert(false);
  }
  if (print)
    printf("%s", reg_name);
  return ptr;
}

uint16_t*
regs_get_ptr16_qq(struct regs* regs, uint8_t regcode, bool print) {
  return get_ptr16(regs, regcode, print, true);
}

uint16_t*
regs_get_ptr16_ss(struct regs* regs, uint8_t regcode, bool print) {
  return get_ptr16(regs, regcode, print, false);
}

