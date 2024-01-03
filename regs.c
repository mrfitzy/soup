#include "regs.h"

_Static_assert(sizeof(struct flags) == 1, "unexpected size");
_Static_assert(sizeof(struct regs) == 16, "unexpected size");

void
regs_init(struct regs* regs) {
  regs->af = 0x1122;
  regs->bc = 0x3344;
  regs->de = 0x5566;
  regs->hl = 0x7788;
  regs->sp = 0x99aa;
  regs->pc = 0x0000;
  regs->flags.val = 0b10101111;
}

