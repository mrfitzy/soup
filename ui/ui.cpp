#include "ui.h"

#include "dmg.h"
#include "test_args.h"

#include "imgui.h"
#include <SDL3/SDL.h>

static void
draw_flags_window(const struct regs* regs) {
    const struct flags* f = &regs->f;
    bool flags_window_open = true;

    ImGui::Begin("flags", &flags_window_open);

    ImGui::Text("AF\t%04x", regs->af);
    ImGui::Text("BC\t%04x", regs->bc);
    ImGui::Text("DE\t%04x", regs->de);
    ImGui::Text("HL\t%04x", regs->hl);
    ImGui::Text("PC\t%04x", regs->pc);
    ImGui::Text("SP\t%04x", regs->sp);
    ImGui::Text("z:%d n:%d h:%d c:%d", f->z, f->n, f->h, f->c);

    ImGui::End();
}

void
ui_update(void* data) {
    struct test_args* args = (struct test_args*)data;
    const struct dmg_system* dmg = &args->dmg;
    const struct regs* regs = &dmg->regs;

    bool show_another_window = false;

    draw_flags_window(regs);

    {
        ImGui::Begin("sem", &show_another_window);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("step")) {
            SDL_SignalSemaphore(args->sem);
        }
        ImGui::End();
    }
}
