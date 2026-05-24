#include "ui.h"

#include "dmg.h"
#include "imgui.h"

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
    const struct dmg_system* dmg = (struct dmg_system*)data;
    const struct regs* regs = &dmg->regs;

    bool show_another_window = false;

    draw_flags_window(regs);

    {
        ImGui::Begin("Another Window", &show_another_window);
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
}
