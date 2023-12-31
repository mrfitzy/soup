#pragma once

#include <stdint.h>

struct __attribute__((packed)) op {
  uint8_t opcode;
  uint8_t length;
  /// (duration_hi << 4) | duration_lo
  uint8_t duration;
  /**
   * flags: 7 6 5 4 3 2 1 0  for each flag:
   *        ---------------    00: unaffected
   *        | | | | | | | |    01: set after execution
   *        Z Z N N H H C C    10: cleared after execution
   *                           11: depends on result of execution
   */
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

