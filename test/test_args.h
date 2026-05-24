#pragma once

#include "args.h"
#include "dmg.h"

#include <semaphore.h>

struct test_args {
  struct dmg_system dmg;
  struct soup_args soup_args;
  sem_t* sem;
};
