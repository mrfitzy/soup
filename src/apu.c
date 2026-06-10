#include "apu.h"

#include "mem.h"

#include <assert.h>
#include <stdint.h>
#include <string.h>

void
apu_init(struct apu* apu, uint8_t* memmap) {
  memset(apu, 0, sizeof(struct apu));
  // sound 1
  apu->nr10 = memmap + REG_NR10;
  apu->nr11 = memmap + REG_NR11;
  apu->nr12 = memmap + REG_NR12;
  apu->nr13 = memmap + REG_NR13;
  apu->nr14 = memmap + REG_NR14;
  // control
  apu->nr50 = memmap + REG_NR50;
  apu->nr51 = memmap + REG_NR51;
  apu->nr52 = memmap + REG_NR52;
}

void
apu_reg_write(struct apu* apu, uint16_t reg, uint8_t data) {
  switch (reg) {
  case REG_NR10:
    *(apu->nr10) = data;
    break;
  case REG_NR11:
    *(apu->nr11) = data;
    break;
  case REG_NR12:
    *(apu->nr12) = data;
    break;
  case REG_NR13:
    *(apu->nr13) = data;
    break;
  case REG_NR14:
    *(apu->nr14) = data;
    break;
  case REG_NR50:
    *(apu->nr50) = data;
    break;
  case REG_NR51:
    *(apu->nr51) = data;
    break;
  case REG_NR52:
    *(apu->nr52) = data;
    break;
  default:
    assert(false);
  }
}

