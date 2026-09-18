#include "ui.h"

#include <SDL3/SDL.h>

int
ui_start(void) {
    bool success = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
    if (!success) {
        fprintf(stderr, "error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }
    return 0;
}

void
ui_stop(void) {
    SDL_Quit();
}