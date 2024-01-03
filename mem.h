#pragma once

#include <stdint.h>

#define DMG_MEM_SIZE (0xffff)

struct mem {
  uint8_t* mem;
  uint16_t size;
  uint8_t pad[6];
};

void mem_init(struct mem* mem, uint16_t size);

uint8_t mem_read(struct mem* mem, uint16_t addr);

void mem_write(struct mem* mem, uint16_t addr, uint8_t data);

