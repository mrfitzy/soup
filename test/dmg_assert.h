#pragma once

#include "assert.h"
#include "dmg.h"

#include <stdint.h>

static inline void
assert_flag_z_equal(uint8_t a, uint8_t b) {
  assert_int_equal(a, b);
}

static inline void
assert_flag_n_equal(uint8_t a, uint8_t b) {
  assert_int_equal(a, b);
}

static inline void
assert_flag_h_equal(uint8_t a, uint8_t b) {
  assert_int_equal(a, b);
}

static inline void
assert_flag_cy_equal(uint8_t a, uint8_t b) {
  assert_int_equal(a, b);
}

void assert_flags_equal(const struct flags* a, const struct flags* b);

static inline void
assert_a_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->a, &b->a, 1);
}

static inline void
assert_b_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->b, &b->b, 1);
}

static inline void
assert_c_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->c, &b->c, 1);
}

static inline void
assert_d_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->d, &b->d, 1);
}

static inline void
assert_e_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->e, &b->e, 1);
}

static inline void
assert_h_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->h, &b->h, 1);
}

static inline void
assert_l_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->l, &b->l, 1);
}

static inline void
assert_sp_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->sp, &b->sp, 2);
}

static inline void
assert_pc_equal(const struct regs* a, const struct regs* b) {
  assert_memory_equal(&a->pc, &b->pc, 2);
}

void assert_regs_equal(const struct regs* a, const struct regs* b);

void assert_mem_equal(const struct mem* a, const struct mem* b);

void assert_dmg_equal(const struct dmg_system* a, const struct dmg_system* b);
