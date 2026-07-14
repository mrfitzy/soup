#include "dmg.h"
#include "emulate.h"
#include "op.h"
#include "ui.h"

#include <SDL3/SDL.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

extern const struct op* ops_data;
extern const struct op* cb_ops_data;
extern const uint8_t* boot_rom_data;
extern const size_t boot_rom_size;

static int
emulate_rom(void* data) {
  dmg_emulate_rom((struct dmg_system*)data);
  return 0;
}

int
main(void) {
  const struct op* ops = ops_data;
  const struct op* cb_ops = cb_ops_data;
  const uint8_t* rom = boot_rom_data;

  struct dmg_system dmg;
  dmg_init(&dmg, ops, cb_ops, rom, boot_rom_size);

  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
    fprintf(stderr, "error: SDL_Init(): %s\n", SDL_GetError());
    return 1;
  }
  dmg_create_display(&dmg);
  int rc = ui_run(emulate_rom, &dmg);
  dmg_destroy_display(&dmg);
  SDL_Quit();

  return rc;
}

