#include "ui.h"
#include "tile.h"

#include "bits.h"
#include "debug_args.h"
#include "diag.h"
#include "dmg.h"

#include "imgui.h"
#include <SDL3/SDL.h>

#include <inttypes.h>
#include <stdio.h>

extern const struct op* ops_data;
extern const struct op* cb_ops_data;

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
    for (uint16_t pc = 0; pc < dmg->cpu.rom_size; pc = next_pc) {
      next_pc = rom_to_str(
          ops_data, cb_ops_data, pc, dmg->cpu.rom, dmg->cpu.rom_size, buf, sizeof(buf));
      if (next_pc == 0) {
        break;
      }
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      if (pc == dmg->cpu.regs.pc) {
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
draw_memory_window(const struct mem* mem, const struct regs* regs) {
  bool mem_window = true;
  ImGui::Begin("memory", &mem_window);
  static uint16_t jump_row = 0;
  static int s_jump_highlight = 0;
  const uint8_t lcdc = mem_read(mem, 0xff40);
  if (ImGui::Button("HL")) {
    jump_row = regs->hl / 16;
    s_jump_highlight = 60;
  }
  ImGui::SameLine();
  if (ImGui::Button("< bg tiles")) {
    jump_row = (bit_4(lcdc) ? 0x8000 : 0x8800) / 16;
    s_jump_highlight = 60;
  }
  ImGui::SameLine();
  if (ImGui::Button("bg tiles >")) {
    jump_row = (bit_4(lcdc) ? 0x8fff : 0x97ff) / 16;
    s_jump_highlight = 60;
  }
  ImGui::SameLine();
  if (ImGui::Button("< bg map")) {
    jump_row = (bit_3(lcdc) ? 0x9c00 : 0x9800) / 16;
    s_jump_highlight = 60;
  }
  ImGui::SameLine();
  if (ImGui::Button("bg map >")) {
    jump_row = (bit_3(lcdc) ? 0x9fff : 0x9bff) / 16;
    s_jump_highlight = 60;
  }
  ImGui::SameLine();
  if (ImGui::Button("zero")) {
    jump_row = 0xff00 / 16;
    s_jump_highlight = 60;
  }

  const auto highlight = ImGui::GetColorU32(ImVec4(0.8f, 0.2f, 0.2f, 0.4f));
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
    if (s_jump_highlight) {
      clipper.IncludeItemByIndex(jump_row);
    }
    while (clipper.Step()) {
      for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
        // data row
        mem_copy_row(mem, row, data_buf, sizeof(data_buf));
        ImGui::TableNextRow();
        // data row header column
        ImGui::TableNextColumn();
        if ((s_jump_highlight == 60) && (row == jump_row)) {
          ImGui::SetScrollHereY(0.5f);
        }
        if (s_jump_highlight && (row == jump_row)) {
          ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, highlight);
          if (s_jump_highlight-- == 0) {
            jump_row = 0;
          }
        }
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

static float g_palette[4][3] = {
    {0},
    {160/255.0f, 160/255.0f, 160/255.0f},
    {84/255.0f,  84/255.0f,  84/255.0f},
    {220/255.0f, 220/255.0f, 220/255.0f}
};

static void
draw_display_window(const struct dmg_system* dmg) {
  const uint8_t lcdc = mem_read(&dmg->mem, REG_LCDC);
  const uint8_t ly = mem_read(&dmg->mem, REG_LY);
  const uint8_t scx = mem_read(&dmg->mem, REG_SCX);
  const uint8_t scy = mem_read(&dmg->mem, REG_SCY);
  bool display_window = true;
  ImGui::Begin("display", &display_window);
  ImGui::Text("lcd %s",        bit_7(lcdc) ? "on" : "off");
  ImGui::Text("ly %02x scx %02x scy %02x", (int)ly, (int)scx, (int)scy);
  ImGui::Text("window %s",     bit_5(lcdc) ? "on" : "off");
  ImGui::Text("bg tiles @ %s", bit_4(lcdc) ? "8000-8fff" : "8800-97ff");
  ImGui::Text("bg map @ %s",   bit_3(lcdc) ? "9c00-9fff" : "9800-9bff");
  ImGui::Text("bg %s",         bit_0(lcdc) ? "on" : "off");
  for (int i = 0; i < 4; i++) {
    char text[16];
    snprintf(text, sizeof(text), "color %d", i);
    ImGui::ColorEdit3(text, (float*)g_palette[i]);
  }
  ImGui::End();
}

static void
draw_tiles_window(const struct mem* mem) {
  const uint8_t lcdc = mem_read(mem, 0xff40);
  const uint8_t* tile_data = mem->mem + (bit_4(lcdc) ? 0x8000 : 0x8800);
  bool tiles_window = true;
  ImGui::Begin("tiles", &tiles_window);
  ImVec2 pad = ImGui::GetStyle().CellPadding;
  pad.x = 1.0f;
  pad.y = 1.0f;
  ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, pad);
  if (ImGui::BeginTable("tiles_table", 17 /* columns */)) {
    const float header_col_w = ImGui::CalcTextSize("bg").x + 1.0f;
    const float dim = 40.0f;
    // headers row
    ImGui::TableSetupColumn("bg", ImGuiTableColumnFlags_WidthFixed, header_col_w);
    char text_buf[16];
    for (int i = 0; i < 16; i++) {
      snprintf(text_buf, sizeof(text_buf), "%02X", i);
      ImGui::TableSetupColumn(text_buf, ImGuiTableColumnFlags_WidthFixed, dim);
    }
    ImGui::TableHeadersRow();

    // data rows
    for (int i = 0; i < 16; i++) {
      ImGui::TableNextRow(ImGuiTableRowFlags_None, dim);
      ImGui::TableNextColumn();
      ImGui::Text("%X0", i); // header column
      for (int j = 0; j < 16; j++) {
        ImGui::TableNextColumn();
        draw_tile(
            tile_data + (16 * (i*16 + j)),
            (const float*)g_palette,
            ImGui::GetCursorScreenPos(),
            5.0f,
            ImGui::GetWindowDrawList());
      }
    }
    ImGui::EndTable();
  }
  ImGui::PopStyleVar(); // ImGuiStyleVar_CellPadding
  ImGui::End();
}

static void
draw_bg_window(const struct mem* mem) {
  const uint8_t lcdc = mem_read(mem, 0xff40);
  const uint8_t* bg_map = mem->mem + (bit_3(lcdc) ? 0x9c00 : 0x9800);
  const uint8_t* tile_data = mem->mem + (bit_4(lcdc) ? 0x8000 : 0x8800);
  const uint8_t scy = mem_read(mem, 0xff42);
  const uint8_t scx = mem_read(mem, 0xff43);

  const float dot_dim = 2.0f;
  const float tile_dim = dot_dim * 8.0f;
  const float window_dim = tile_dim * 32.0f;

  ImGui::SetNextWindowSize(ImVec2(window_dim, window_dim), ImGuiCond_Once);
  bool bg_window = true;
  ImGui::Begin("bg", &bg_window);

  // bg tiles
  ImDrawList* dl = ImGui::GetWindowDrawList();
  const ImVec2 w0 = ImGui::GetCursorScreenPos();
  for (int i = 0; i < 32; i++) {
    for (int j = 0; j < 32; j++) {
      const uint8_t tile = bg_map[i*32 + j];
      const auto t0 = ImVec2(w0.x + j*tile_dim, w0.y + i*tile_dim);
      draw_tile(
          tile_data + (16 * tile), (const float*)g_palette, t0, dot_dim, dl);
    }
  }

  // viewport rect
  const auto v0 = ImVec2(w0.x + scx*dot_dim, w0.y + scy*dot_dim);
  const auto v1 = ImVec2(v0.x + 160*dot_dim, v0.y + 144*dot_dim);
  dl->AddRect(v0, v1, IM_COL32(230, 0, 18, 255), 0.0f, 0, dot_dim);

  ImGui::End();
}

void
ui_update(void* data) {
  struct debug_args* debug = (struct debug_args*)data;
  const struct dmg_system* dmg = &debug->dmg;
  const struct regs* regs = &dmg->cpu.regs;
  const struct mem* mem = &dmg->mem;

  draw_regs_window(regs);
  draw_code_window(dmg);
  draw_debug_window(debug);
  draw_memory_window(mem, regs);
  draw_display_window(dmg);
  draw_tiles_window(mem);
  draw_bg_window(mem);
}

