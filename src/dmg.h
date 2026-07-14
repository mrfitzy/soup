#pragma once

#include "apu.h"
#include "cpu.h"
#include "display.h"
#include "lcdc.h"
#include "mem.h"

#include <unistd.h>

struct dmg_system {
  struct cpu cpu;
  struct mem mem;
  struct lcdc lcdc;
  struct apu apu;
  struct display* display;
};

enum dmg_init_options {
  DMG_INIT_NO_DISPLAY = 0x1,
};

void dmg_init(struct dmg_system*);

void dmg_init_with_options(struct dmg_system*, enum dmg_init_options);

void dmg_destroy(struct dmg_system*);

void dmg_on(struct dmg_system*);

uint8_t dmg_get_logo(uint16_t offset);
