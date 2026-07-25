#include "dmg_assert.h"

void
assert_flags_equal(const struct flags* a, const struct flags* b) {
  assert_flag_z_equal(a->z, b->z);
  assert_flag_n_equal(a->n, b->n);
  assert_flag_h_equal(a->h, b->h);
  assert_flag_cy_equal(a->c, b->c);
  int a_unused = a->unused;
  int b_unused = b->unused;
  assert_int_equal(a_unused, b_unused);
}

void
assert_regs_equal(const struct regs* a, const struct regs* b) {
  assert_a_equal(a, b);
  assert_b_equal(a, b);
  assert_c_equal(a, b);
  assert_d_equal(a, b);
  assert_e_equal(a, b);
  assert_h_equal(a, b);
  assert_l_equal(a, b);
  assert_sp_equal(a, b);
  assert_pc_equal(a, b);
  assert_flags_equal(&a->f, &b->f);
  assert_memory_equal(a, b, sizeof(struct regs));
}

void
assert_mem_equal(const struct mem* a, const struct mem* b) {
  assert_int_equal(a->size, b->size);
  assert_int_equal(a->max_addr, b->max_addr);
  assert_memory_equal(a->mem, b->mem, REG_LY);
  assert_memory_equal(a->mem + REG_LY + 1, b->mem + REG_LY + 1, a->size - REG_LY - 1);
}

void
assert_dmg_equal(const struct dmg_system* a, const struct dmg_system* b) {
  assert_regs_equal(&a->cpu.regs, &b->cpu.regs);
  assert_mem_equal(&a->mem, &b->mem);
}
