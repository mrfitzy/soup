#pragma once

#include <stdint.h>
#include <unistd.h>

#define DMG_MAX_ADDR (0xffff)

#define REG_LCDC (0xff40)
#define REG_LY   (0xff44)
#define REG_SCX  (0xff43)
#define REG_SCY  (0xff42)

#ifdef __cplusplus
extern "C" {
#endif

struct lcdc;

struct mem {
  struct lcdc* lcdc;
  uint8_t* mem;
  size_t size;
  uint16_t max_addr;
  uint8_t pad[6];
};

void mem_init(struct mem* mem, struct lcdc* lcdc, uint16_t max_addr);

uint8_t mem_read(const struct mem* mem, uint16_t addr);

void mem_write(struct mem* mem, uint16_t addr, uint8_t data);

#ifdef __cplusplus
} // extern "C"
#endif

