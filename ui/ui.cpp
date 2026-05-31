#include "ui.h"

#include "bits.h"
#include "debug_args.h"
#include "diag.h"
#include "dmg.h"

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
  static bool s_center = false;
  ImGui::Begin("assembly", &code_window_open);
  if (ImGui::Button("PC")) {
    s_center = true;
  }
  const ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg;
  if (ImGui::BeginTable("code_table", 1 /* columns */, flags)) {
    const float pad = ImGui::GetStyle().CellPadding.x * 2.0f;
    const float width = ImGui::CalcTextSize("LDH A,($ff00+$ff)").x + pad;
    const auto highlight = ImGui::GetColorU32(ImVec4(0.8f, 0.2f, 0.2f, 0.4f));

    char buf[256];
    uint16_t next_pc = 0;
    for (uint16_t pc = 0; pc < dmg->rom_size; pc = next_pc) {
      next_pc = rom_to_str(
          dmg->ops, dmg->cb_ops, pc, dmg->rom, dmg->rom_size, buf, sizeof(buf));
      if (next_pc == 0) {
        break;
      }
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      if (pc == dmg->regs.pc) {
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, highlight);
        if (s_center) {
          ImGui::SetScrollHereY(0.5f);
          s_center = false;
        }
      }
      ImGui::Text("%s", buf);
      ImGui::SameLine(width);
      ImGui::Text("; %04x", pc);
    }
    ImGui::EndTable();
  }
  ImGui::End();
}

static void
draw_debug_window(struct debug_args* debug) {
  bool debug_window = true;
  ImGui::Begin("debug", &debug_window);
  if (ImGui::Button("step")) {
    debug->step = true;
    SDL_SignalSemaphore(debug->sem);
  }
  if (ImGui::Button("continue")) {
    debug->step = false;
    SDL_SignalSemaphore(debug->sem);
  }
  ImGui::End();
}

static void
draw_memory_window(const struct mem* mem) {
  bool mem_window = true;
  ImGui::Begin("memory", &mem_window);
  const ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg;
  ImVec2 pad = ImGui::GetStyle().CellPadding;
  pad.x /= 2.0f;
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, pad);
  if (ImGui::BeginTable("memory_table", 17 /* columns */, flags)) {
    // table column widths
    const float pad = ImGui::GetStyle().CellPadding.x * 2.0f;
    const float col0_w = ImGui::CalcTextSize("[mem]").x + pad;
    const float coln_w = ImGui::CalcTextSize("FF").x + pad;

    // headers row
    ImGui::TableSetupColumn("[mem]", ImGuiTableColumnFlags_WidthFixed, col0_w);
    char text_buf[16];
    for (int i = 0; i < 16; i++) {
      snprintf(text_buf, sizeof(text_buf), "%02X", i);
      ImGui::TableSetupColumn(text_buf, ImGuiTableColumnFlags_WidthFixed, coln_w);
    }
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableHeadersRow();

    // data rows
    uint8_t data_buf[16];
    ImGuiListClipper clipper;
    clipper.Begin(mem->size / sizeof(data_buf));
    while (clipper.Step()) {
      for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
        // data row
        mem_copy_row(mem, row, data_buf, sizeof(data_buf));
        ImGui::TableNextRow();
        // data row header column
        ImGui::TableNextColumn();
        ImGui::Text("%04lX", row * sizeof(data_buf));
        // data row data columns
        for (size_t j = 0; j < sizeof(data_buf); j++) {
          ImGui::TableNextColumn();
          ImGui::Text("%02X", data_buf[j]);
        }
      }
    }
    ImGui::EndTable();
  }
  ImGui::PopStyleVar(); // ImGuiStyleVar_CellPadding
  ImGui::End();
}

static void
draw_display_window(const struct dmg_system* dmg) {
  const uint8_t lcdc = mem_read(&dmg->mem, 0xff40);
  bool display_window = true;
  ImGui::Begin("display", &display_window);
  ImGui::Text("lcd %s",        bit_7(lcdc) ? "on" : "off");
  ImGui::Text("window %s",     bit_5(lcdc) ? "on" : "off");
  ImGui::Text("bg tiles @ %s", bit_4(lcdc) ? "8000-8fff" : "8800-97ff");
  ImGui::Text("bg map @ %s",   bit_3(lcdc) ? "9c00-9fff" : "9800-9bff");
  ImGui::Text("bg %s",         bit_0(lcdc) ? "on" : "off");
  ImGui::End();
}

void
ui_update(void* data) {
  struct debug_args* debug = (struct debug_args*)data;
  const struct dmg_system* dmg = &debug->dmg;
  const struct regs* regs = &dmg->regs;

  draw_regs_window(regs);
  draw_code_window(dmg);
  draw_debug_window(debug);
  draw_memory_window(&dmg->mem);
  draw_display_window(dmg);
}

