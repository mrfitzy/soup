#include "mem.h"

#include "assert.h"
#include "apu.h"
#include "lcdc.h"

#include <stdlib.h>
#include <string.h>

static const uint8_t g_logo[] = {
  0xce,0xed,0x66,0x66,0xcc,0x0d,0x00,0x0b,0x03,0x73,0x00,0x83,0x00,0x0c,0x00,0x0d,
  0x00,0x08,0x11,0x1f,0x88,0x89,0x00,0x0e,0xdc,0xcc,0x6e,0xe6,0xdd,0xdd,0xd9,0x99,
  0xbb,0xbb,0x67,0x63,0x6e,0x0e,0xec,0xcc,0xdd,0xdc,0x99,0x9f,0xbb,0xb9,0x33,0x3e
};

static_assert(sizeof(g_logo) == 0x30, "unexpected size");

void
mem_init(
    struct mem* mem,
    struct apu* apu,
    struct lcdc* lcdc,
    uint16_t max_addr) {
  assert(max_addr == DMG_MAX_ADDR);

  size_t size = (size_t)max_addr + 1;
  uint8_t* buf = malloc(size);
  assert(buf);
  memset(buf, 0xff, size);
  memset(buf + REG_APU_MIN, 0, REG_APU_MAX - REG_APU_MIN + 1);

  mem->apu = apu;
  mem->lcdc = lcdc;
  mem->mem = buf;
  mem->max_addr = max_addr;
  mem->size = size;

  buf[0xff4f] = 0; // VBK

  memcpy(buf + 0x104, g_logo, sizeof(g_logo));
}

void
mem_destroy(struct mem* mem) {
  free(mem->mem);
  mem->mem = NULL;
}

uint8_t
mem_read(const struct mem* mem, uint16_t addr) {
  assert(addr < mem->size);
  return mem->mem[addr];
}

void
mem_write(struct mem* mem, uint16_t addr, uint8_t data) {
  assert(addr < mem->size);
  if ((addr >= REG_APU_MIN) && (addr <= REG_APU_MAX)) {
    apu_reg_write(mem->apu, addr, data);
  } else if (addr == REG_LCDC) {
    lcdc_reg_write(mem->lcdc, data);
  } else {
    mem->mem[addr] = data;
  }
}

uint8_t
dmg_get_logo(uint16_t offset) {
  assert(offset < sizeof(g_logo));
  return g_logo[offset];
}

