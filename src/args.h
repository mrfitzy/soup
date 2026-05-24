#pragma once

#include "dmg.h"
#include "op.h"

#include <stdbool.h>

#define OPS_BIN_SIZE (OP_SIZE * 256)
#define DMG_ROM_SIZE (256)

struct soup_args {
  struct dmg_system dmg;
  const char* ops_path;
  const char* cb_ops_path;
  const char* rom_path;
};

bool soup_args_from_argv(int argc, char* const argv[], struct soup_args* out);

