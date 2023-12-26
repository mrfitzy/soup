#pragma once

#include <stddef.h>
#include <stdint.h>

struct op;

void print_data(const uint8_t* data, size_t length);

void print_rom(
    const struct op* ops,
    const struct op* cb_ops,
    const uint8_t* rom,
    size_t rom_size);

char flag_to_char(const struct op* op, char c);
