#pragma once

#include <stddef.h>
#include <stdint.h>

struct dmg_system;
struct op;

void emulate_rom(
    const struct op* ops,
    const struct op* cb_ops,
    const uint8_t* rom,
    size_t rom_size);

void emulate_instruction(struct dmg_system* dmg);
