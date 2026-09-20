#include "dmg.h"

#include "assert.h"
#include "diag.h"
#include "log.h"
#include "op.h"

#include <SDL3/SDL.h>
#include <string.h>

extern const uint8_t* boot_rom_data;
extern const size_t boot_rom_size;

static void
create_display(struct dmg_system* dmg) {
  assert(dmg->display == NULL);
  dmg->display = display_create(dmg);
  dmg->lcdc.display = dmg->display;
}

void
dmg_init(struct dmg_system* dmg) {
  dmg_init_with_options(dmg, 0);
}

void
dmg_init_with_options(struct dmg_system* dmg, enum dmg_init_options options) {
  memset(dmg, 0, sizeof(struct dmg_system));
  mem_init(&dmg->mem, &dmg->apu, &dmg->lcdc, DMG_MAX_ADDR);
  cpu_init(&dmg->cpu, &dmg->mem, boot_rom_data, boot_rom_size);
  lcdc_init(&dmg->lcdc, dmg->mem.mem);
  apu_init(&dmg->apu, dmg->mem.mem);
  dmg->display = NULL;

  assert(boot_rom_size <= dmg->mem.size);
  memcpy(dmg->mem.mem, boot_rom_data, boot_rom_size);

  if (!(options & DMG_INIT_NO_DISPLAY)) {
    create_display(dmg);
  }
}

void
dmg_destroy(struct dmg_system* dmg) {
  if (dmg->display) {
    dmg->lcdc.display = NULL;
    display_destroy(dmg->display);
    dmg->display = NULL;
  }
}

static void
dmg_step(struct dmg_system* dmg) {
  (void)cpu_execute_instruction(&dmg->cpu);
}

void
dmg_on(struct dmg_system* dmg) {
  struct cpu* cpu = &dmg->cpu;
  const struct regs* regs = &cpu->regs;
  while (regs->pc < cpu->rom_size) {
    dmg_step(dmg);
  }
  print_regs(regs);
  fatal("pc overload: $%04x >= $%04x", regs->pc, cpu->rom_size);
}
