#include "mem.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void
mem_init(struct mem* mem, uint16_t size) {
  assert(size == DMG_MEM_SIZE);
  mem->mem = malloc(size);
  assert(mem->mem);
  memset(mem->mem, 0xff, size);
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

