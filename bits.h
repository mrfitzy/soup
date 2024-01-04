#pragma once

static inline uint8_t bit_3(uint8_t byte) {
  return (byte & 0b00001000);
}

static inline uint8_t bit_7(uint8_t byte) {
  return (byte & 0b10000000);
}

static inline uint8_t bits_2_0(uint8_t byte) {
  return (byte & 0b00000111);
}

static inline uint8_t bits_5_3(uint8_t byte) {
  return (byte & 0b00111000) >> 3;
}

static inline uint8_t bits_5_4(uint8_t byte) {
  return (byte & 0b00110000) >> 4;
}

static inline uint8_t bits_7_3(uint8_t byte) {
  return (byte & 0b11111000) >> 3;
}

