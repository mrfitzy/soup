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

void dmg_init(struct dmg_system*);

void dmg_create_display(struct dmg_system*);

void dmg_destroy_display(struct dmg_system*);

void dmg_on(struct dmg_system*);

uint8_t dmg_get_logo(uint16_t offset);
