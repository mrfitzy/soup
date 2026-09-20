#include "ui.h"

#include "display.h"
#include "dmg.h"
#include "log.h"

#include <stdio.h>
#include <SDL3/SDL.h>

struct work_args {
  int (*fn)(void*);
  void* data;
};

static int
work_thread(void* data) {
  bool success = SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_TIME_CRITICAL);
  if (!success) {
    log_error("failed to set thread priority: %s", SDL_GetError());
  }
  auto work = (struct work_args*)data;
  return work->fn(work->data);
}

int
ui_run(int (*work_fn)(void*), void* data) {
  struct work_args work = { .fn = work_fn, .data = data };
  SDL_Thread* thread = SDL_CreateThread(work_thread, "emulator", &work);
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

    display_wait_for_frame(dmg->display);
    display_render(dmg->display);
  }

  int rc;
  SDL_WaitThread(thread, &rc);
  return rc;
}
