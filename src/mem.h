#pragma once

#include <stdint.h>
#include <unistd.h>

#define DMG_MAX_ADDR (0xffff)

struct mem {
  uint8_t* mem;
  size_t size;
  uint16_t max_addr;
  uint8_t pad[6];
};

void mem_init(struct mem* mem, uint16_t max_addr);

uint8_t mem_read(struct mem* mem, uint16_t addr);

void mem_write(struct mem* mem, uint16_t addr, uint8_t data);

