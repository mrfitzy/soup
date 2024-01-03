#include "mem.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void
mem_init(struct mem* mem, uint16_t max_addr) {
  assert(max_addr == DMG_MAX_ADDR);

  size_t size = (size_t)max_addr + 1;
  uint8_t* buf = malloc(size);
  assert(buf);

  memset(buf, 0xff, size);
  mem->mem = buf;
  mem->max_addr = max_addr;
  mem->size = size;
}

uint8_t
mem_read(struct mem* mem, uint16_t addr) {
  assert(addr < mem->size);
  return mem->mem[addr];
}

void
mem_write(struct mem* mem, uint16_t addr, uint8_t data) {
  assert(addr < mem->size);
  mem->mem[addr] = data;
}

