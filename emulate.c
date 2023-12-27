#include "emulate.h"

#include "diag.h"
#include "regs.h"

struct regs regs = { 0 };

void
emulate_rom(
    const struct op* ops,
    const struct op* cb_ops,
    const uint8_t* rom,
    size_t rom_size) {
  (void)ops;
  (void)cb_ops;
  (void)rom;
  (void)rom_size;
  regs.af = 0x1122;
  regs.bc = 0xabcd;
  regs.de = 0x3344;
  regs.hl = 0x5566;
  regs.sp = 0xffef;
  regs.pc = 0x9898;
  print_regs(&regs);
}
