#include "args.h"
#include "diag.h"
#include "emulate.h"
#include "op.h"
#include "ui.h"

#include <SDL3/SDL.h>
#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/mman.h>
#include <unistd.h>

#define ARG_COUNT (3)

static const void*
map_file(const char* path, size_t size) {
  int fd = open(path, O_RDONLY);
  assert(fd != -1);

  const void* data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0 /* offset */);
  if (data == MAP_FAILED) {
    perror(NULL);
    assert(false);
  }

  return data;
}

static void
print_usage(const char* progname) {
  fprintf(stderr, "%s [-p] ops.bin cb_ops.bin dmg_rom.bin\n", progname);
}

static int
emulate_rom(void* data) {
  dmg_emulate_rom((struct dmg_system*)data);
  return 0;
}

int
main(int argc, char** argv) {
  if (argc < ARG_COUNT + 1) {
    fprintf(stderr, "error: missing arguments\n");
    print_usage(argv[0]);
    return 1;
  }
  bool print = false;
  int c;
  while ((c = getopt(argc, argv, "p")) != -1) {
    switch (c) {
      case 'p':
        print = true;
        break;
      case '?':
        fprintf(stderr, "error: invalid argument '%c'", optopt);
        print_usage(argv[0]);
        return 1;
    }
  }
  argc -= optind;
  argv += optind;
  struct soup_args args;
  if (!soup_args_from_argv(argc, argv, &args)) {
    fprintf(stderr, "error: unexpected arg count (%d)\n", argc);
    return 1;
  }
  const struct op* ops = map_file(args.ops_path, OPS_BIN_SIZE);
  const struct op* cb_ops = map_file(args.cb_ops_path, OPS_BIN_SIZE);
  const uint8_t* rom = map_file(args.rom_path, DMG_ROM_SIZE);
  if (print) {
    print_rom(ops, cb_ops, rom, 0xa8);
    print_data(rom + 0xa8, 0xe0 - 0xa8);
    print_rom(ops, cb_ops, rom + 0xe0, DMG_ROM_SIZE - 0xe0);
    return 0;
  }

  struct dmg_system dmg;
  dmg_init(&dmg, ops, cb_ops, rom, DMG_ROM_SIZE);

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

