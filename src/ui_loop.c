#include "ui.h"

#include "display.h"
#include "dmg.h"

#include <stdio.h>
#include <SDL3/SDL.h>

int
ui_run(int (*work_fn)(void*), void* data) {

  SDL_Thread* thread = SDL_CreateThread(work_fn, "emulator", data);

  auto dmg = (struct dmg_system*)data;

  bool done = false;
  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT ||
          event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        done = true;
      }
    }

    if (display_should_render(dmg->display)) {
      display_render(dmg->display);
    }
  }

  int rc;
  SDL_WaitThread(thread, &rc);

  return rc;
}
