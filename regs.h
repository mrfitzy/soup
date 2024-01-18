#pragma once

#include <stdbool.h>
#include <stdint.h>

struct mem;

// N.B. unions assume little-endian

/**
 * flags register: 7 6 5 4 3 2 1 0
 *                 ---------------
 *                 | | | | | | | |
 *                 z n h c - - - -
 */
struct __attribute__((packed)) flags {
  union {
    struct {
      uint8_t unused: 4;
      uint8_t c: 1;
      uint8_t h: 1;
      uint8_t n: 1;
      uint8_t z: 1;
    };
    uint8_t val;
  };
};

struct regs {
  union {
    struct {
      struct flags f;
      uint8_t a;
    };
    uint16_t af;
  };
  union {
    struct {
      uint8_t c;
      uint8_t b;
    };
    uint16_t bc;
  };
  union {
    struct {
      uint8_t e;
      uint8_t d;
    };
    uint16_t de;
  };
  union {
    struct {
      uint8_t l;
      uint8_t h;
    };
    uint16_t hl;
  };
  uint16_t sp;
  union {
    struct {
      uint8_t pc_lo;
      uint8_t pc_hi;
    };
    uint16_t pc;
  };
  struct flags flags;
  uint8_t pad[3];
};

void regs_init(struct regs* regs);

uint8_t* regs_get_ptr(struct regs* regs, struct mem* mem, uint8_t regcode, bool print);

uint16_t* regs_get_ptr16(struct regs* regs, uint8_t regcode, bool print);

