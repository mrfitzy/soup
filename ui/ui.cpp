#include "ui.h"

#include "diag.h"
#include "dmg.h"
#include "test_args.h"

#include "imgui.h"
#include <SDL3/SDL.h>

static void
draw_regs_window(const struct regs* regs) {
    const struct flags* f = &regs->f;
    bool regs_window_open = true;
    ImGui::Begin("registers", &regs_window_open);

    ImGui::Text("AF\t%04x", regs->af);
    ImGui::Text("BC\t%04x", regs->bc);
    ImGui::Text("DE\t%04x", regs->de);
    ImGui::Text("HL\t%04x", regs->hl);
    ImGui::Text("PC\t%04x", regs->pc);
    ImGui::Text("SP\t%04x", regs->sp);
    ImGui::Text("z:%d n:%d h:%d c:%d", f->z, f->n, f->h, f->c);

    ImGui::End();
}

static void
draw_code_window(const struct dmg_system* dmg) {
  bool code_window_open = true;
  ImGui::Begin("assembly", &code_window_open);
  char buf[256];
  uint16_t pc = dmg->regs.pc;
  for (int i = 0; i < 10 && pc < dmg->rom_size; i++) {
    uint16_t next_pc = rom_to_str(
        dmg->ops, dmg->cb_ops, pc, dmg->rom, dmg->rom_size, buf, sizeof(buf));
    if (next_pc == 0) {
      break;
    }
    ImGui::Text("%s", buf);
    ImGui::SameLine(100.0f);
    ImGui::Text("; %04x", pc);
    pc = next_pc;
  }
  ImGui::End();
}

static void
draw_control_window(SDL_Semaphore* sem) {
  bool control_window = true;
  ImGui::Begin("control", &control_window);
  if (ImGui::Button("step")) {
    SDL_SignalSemaphore(sem);
  }
  ImGui::End();
}

void
ui_update(void* data) {
  struct test_args* args = (struct test_args*)data;
  const struct dmg_system* dmg = &args->dmg;
  const struct regs* regs = &dmg->regs;

  draw_regs_window(regs);
  draw_code_window(dmg);
  draw_control_window(args->sem);
}
