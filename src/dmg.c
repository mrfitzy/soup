#include "dmg.h"

#include "assert.h"
#include "diag.h"
#include "log.h"
#include "op.h"
#include "profiler.h"

#include <SDL3/SDL.h>
#include <string.h>

extern const uint8_t* boot_rom_data;
extern const size_t boot_rom_size;

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
  dmg->epoch = 0;
  dmg->frame_deadline_ns = 0;
  dmg->display = NULL;

  assert(boot_rom_size <= dmg->mem.size);
  memcpy(dmg->mem.mem, boot_rom_data, boot_rom_size);

  if (!(options & DMG_INIT_NO_DISPLAY)) {
    dmg->display = display_create(dmg);
  }
}

void
dmg_destroy(struct dmg_system* dmg) {
  if (dmg->display) {
    display_destroy(dmg->display);
    dmg->display = NULL;
  }
}

// 154 lines/frame * 114 cycles/line * 1000 ns/1.048576 cycle
#define DMG_FRAME_NS (16742706)

static void
dmg_frame_complete(struct dmg_system* dmg) {
  if (dmg->display) {
    display_next_frame(dmg->display);
  }

  const uint64_t now = SDL_GetTicksNS();
  if (dmg->frame_deadline_ns == 0) {
    dmg->frame_deadline_ns = now;
  }

  // pace at frame granularity to manage wait overhead
  dmg->frame_deadline_ns += DMG_FRAME_NS;
  if (dmg->frame_deadline_ns > now) {
    SDL_DelayPrecise(dmg->frame_deadline_ns - now);
    return;
  }

  dmg->frame_deadline_ns = now;
  profiler_plot("slow_frame", (int)(now - dmg->frame_deadline_ns));
}

void
dmg_step(struct dmg_system* dmg) {
  // step cpu
  const uint16_t pc_prev = dmg->cpu.regs.pc;
  const auto op = cpu_execute_instruction(&dmg->cpu);

  // compute m-cycles elapsed
  uint8_t m_cycles = op_get_duration_lo(op);
  const uint8_t hi = op_get_duration_hi(op);
  if (hi && (dmg->cpu.regs.pc != (pc_prev + op->length))) {
    m_cycles = hi;
  }
  dmg->epoch += m_cycles;

  // step lcdc based on m-cycles elapsed
  bool vblank_complete = false;
  if (dmg->epoch >= LCDC_M_CYCLES_PER_LINE) {
    vblank_complete = lcdc_line_complete(&dmg->lcdc);
    dmg->epoch -= LCDC_M_CYCLES_PER_LINE;
    assert(dmg->epoch < LCDC_M_CYCLES_PER_LINE);
  }

  // complete frame (blocks)
  if (vblank_complete) {
    dmg_frame_complete(dmg);
  }
}

void
dmg_run(struct dmg_system* dmg) {
  struct cpu* cpu = &dmg->cpu;
  const struct regs* regs = &cpu->regs;
  while (regs->pc < cpu->rom_size) {
    dmg_step(dmg);
  }
  print_regs(regs);
  fatal("pc overload: $%04x >= $%04x", regs->pc, cpu->rom_size);
}
