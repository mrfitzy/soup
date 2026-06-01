#include "tile.h"

#include "bits.h"
#include "imgui.h"

void
draw_tile(
    const uint8_t* data,
    const float* palette,
    const ImVec2& t0,
    float dim,
    ImDrawList* dl) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      unsigned c = (bit_n(data[2*i], 7-j) << 1) | bit_n(data[2*i + 1], 7-j);
      auto color = ImGui::GetColorU32(
          ImVec4(palette[3*c], palette[3*c + 1], palette[3*c + 2], 1.0f));
      auto p0 = ImVec2(t0.x + j*dim, t0.y + i*dim);
      auto p1 = ImVec2(p0.x + dim, p0.y + dim);
      dl->AddRectFilled(p0, p1, color);
    }
  }
}

