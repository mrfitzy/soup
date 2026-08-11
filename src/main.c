#include "dmg.h"
#include "profiler.h"
#include "ui.h"

#include <SDL3/SDL.h>
#include <stdio.h>

static int
run_dmg(void* data) {
  profiler_set_thread_name("dmg");
  dmg_on((struct dmg_system*)data);
  return 0;
}

int
main(void) {
  profiler_init();
  profiler_set_thread_name("ui");
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    fprintf(stderr, "error: SDL_Init(): %s\n", SDL_GetError());
    return 1;
  }

  struct dmg_system dmg;
  dmg_init(&dmg);
  int rc = ui_run(run_dmg, &dmg);
  dmg_destroy(&dmg);

  SDL_Quit();
  return rc;
}
