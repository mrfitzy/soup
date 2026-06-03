#include "ui.h"

#include "display.h"

#include <stdio.h>
#include <SDL3/SDL.h>

int
ui_run(int (*work_fn)(void*), void* data) {
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    fprintf(stderr, "error: SDL_Init(): %s\n", SDL_GetError());
    return 1;
  }

  display_init((struct dmg_system*)data);

  SDL_Thread* thread = SDL_CreateThread(work_fn, "emulator", data);

  bool done = false;
  while (!done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT ||
          event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
        done = true;
      }
    }

    if (display_should_render()) {
      display_render();
    }
  }

  int rc;
  SDL_WaitThread(thread, &rc);
  SDL_Quit();

  return rc;
}
