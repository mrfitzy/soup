#pragma once

#include "args.h"
#include "dmg.h"

typedef struct SDL_Semaphore SDL_Semaphore;

struct test_args {
  struct dmg_system dmg;
  struct soup_args soup_args;
  SDL_Semaphore* sem;
};
