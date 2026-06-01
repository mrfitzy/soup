#include "tile.h"

#include "bits.h"

#include "imgui.h"

void
draw_tile(
    const uint8_t* data,
    const float* palette,
    struct ImDrawList* dl,
    float dim) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      unsigned c = (bit_n(data[2*i], 7-j) << 1) | bit_n(data[2*i + 1], 7-j);
      ImU32 color = ImGui::GetColorU32(
          ImVec4(palette[3*c], palette[3*c + 1], palette[3*c + 2], 1.0f));
      ImVec2 p0 = ImGui::GetCursorScreenPos();
      p0.x += j * dim;
      p0.y += i * dim;
      ImVec2 p1 = ImVec2(p0.x + dim, p0.y + dim);
      dl->AddRectFilled(p0, p1, color);
    }
  }
}

