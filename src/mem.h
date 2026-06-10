#pragma once

#include <stdint.h>
#include <unistd.h>

#define DMG_MAX_ADDR (0xffff)

// APU
#define REG_NR10 (0xff10)
#define REG_NR11 (0xff11)
#define REG_NR12 (0xff12)
#define REG_NR13 (0xff13)
#define REG_NR14 (0xff14)
#define REG_NR21 (0xff16)
#define REG_NR22 (0xff17)
#define REG_NR23 (0xff18)
#define REG_NR24 (0xff19)
#define REG_NR30 (0xff1a)
#define REG_NR31 (0xff1b)
#define REG_NR32 (0xff1c)
#define REG_NR33 (0xff1d)
#define REG_NR34 (0xff1e)
#define REG_NR41 (0xff20)
#define REG_NR42 (0xff21)
#define REG_NR43 (0xff22)
#define REG_NR44 (0xff23)
#define REG_NR50 (0xff24)
#define REG_NR51 (0xff25)
#define REG_NR52 (0xff26)

#define REG_APU_MIN REG_NR10
#define REG_APU_MAX REG_NR52

// LCDC
#define REG_LCDC (0xff40)
#define REG_SCY  (0xff42)
#define REG_SCX  (0xff43)
#define REG_LY   (0xff44)

#ifdef __cplusplus
extern "C" {
#endif

struct apu;
struct lcdc;

struct mem {
  struct apu* apu;
  struct lcdc* lcdc;
  uint8_t* mem;
  size_t size;
  uint16_t max_addr;
  uint8_t pad[6];
};

void mem_init(struct mem*, struct apu*,  struct lcdc*, uint16_t max_addr);

uint8_t mem_read(const struct mem*, uint16_t addr);

void mem_write(struct mem*, uint16_t addr, uint8_t data);

#ifdef __cplusplus
} // extern "C"
#endif

