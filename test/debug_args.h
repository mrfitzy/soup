#pragma once

#include "dmg.h"

typedef struct SDL_Semaphore SDL_Semaphore;

struct debug_args {
  struct dmg_system dmg;
  SDL_Semaphore* sem;
  bool step;
};
