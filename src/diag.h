#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mem;
struct op;
struct regs;

void print_backtrace(void);

void print_mem(const struct mem* mem);

void mem_copy_row(const struct mem* mem, int row, uint8_t* buf, size_t buf_size);

void print_op(const struct op* op);

void print_regs(const struct regs* regs);

// @returns next pc for iterating rom; 0 for EOF
uint16_t rom_to_str(
    const struct op* ops,
    const struct op* cb_ops,
    uint16_t pc,
    const uint8_t* rom,
    size_t rom_size,
    char* buf,
    size_t buf_size);

char flag_to_char(const struct op* op, char c);

#ifdef __cplusplus
} // extern "C"
#endif

