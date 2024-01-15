#include "args.h"

bool
soup_args_from_argv(int argc, char* const argv[], struct soup_args* out) {
  if (argc < 3)
    return false;
  out->ops_path = argv[0];
  out->cb_ops_path = argv[1];
  out->rom_path = argv[2];
  return true;
}

