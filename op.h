#pragma once

#include <stdint.h>

struct op {
  uint8_t opcode;
  uint8_t length;
  uint8_t duration;
  uint8_t flags;
  char text[12];
};

static inline uint8_t
op_get_duration_hi(const struct op* op) {
  return op->duration >> 4;
}

static inline uint8_t
op_get_duration_lo(const struct op* op) {
  return op->duration & 0x0f;
}

uint8_t op_get_flag(const struct op* op, char c);

