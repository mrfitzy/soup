#pragma once

#include "regs.h"

#include <unistd.h>

struct mem;

struct cpu {
  struct regs regs;
  struct mem* mem;
  const uint8_t* rom;
  size_t rom_size;
};

void cpu_init(struct cpu*, struct mem* mem, const uint8_t* rom, size_t rom_size);

void cpu_execute_instruction(struct cpu*);
