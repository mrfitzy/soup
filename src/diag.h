#pragma once

#include <stddef.h>
#include <stdint.h>

struct mem;
struct op;
struct regs;

void print_backtrace(void);

void print_data(const uint8_t* data, size_t length);

void print_mem(const struct mem* mem);

void print_op(const struct op* op);

void print_regs(const struct regs* regs);

void print_rom(
    const struct op* ops,
    const struct op* cb_ops,
    const uint8_t* rom,
    size_t rom_size);

char flag_to_char(const struct op* op, char c);

