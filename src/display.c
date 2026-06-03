#include "display.h"

#include "bits.h"
#include "dmg.h"

#include <SDL3/SDL.h>
#include <stdint.h>

#define SCALE (3)

static const uint8_t g_pal[4][3] = {
    {0},
    {160, 160, 160},
    {84,  84,  84},
    {255, 255, 255}
};

static struct dmg_system* g_dmg = NULL;
static SDL_Window* g_window = NULL;
static SDL_Renderer* g_r = NULL;
static SDL_Texture* g_target = NULL;
static SDL_Semaphore* g_frame_sem = NULL;

static void
draw_tile(const uint8_t* data, int x, int y) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      unsigned c = (bit_n(data[2*i], 7-j) << 1) | bit_n(data[2*i + 1], 7-j);
      SDL_SetRenderDrawColor(g_r, g_pal[c][0], g_pal[c][1], g_pal[c][2], 255);
      SDL_RenderPoint(g_r, (float)x + (float)j, (float)y + (float)i);
    }
  }
}

static void
draw_bg(void) {
  const struct lcdc* lcdc = &g_dmg->lcdc;
  const uint8_t reg_lcdc = *(lcdc->lcdc);
  const uint8_t* bg_map = g_dmg->mem.mem + (bit_3(reg_lcdc) ? 0x9c00 : 0x9800);
  const uint8_t* tile_data = g_dmg->mem.mem + (bit_4(reg_lcdc) ? 0x8000 : 0x8800);
  for (int i = 0; i < 32; i++) {
    for (int j = 0; j < 32; j++) {
      const uint8_t tile = bg_map[i*32 + j];
      draw_tile(tile_data + (16 * tile), j*8, i*8);
    }
  }
}

void
display_init(struct dmg_system* dmg) {
  g_dmg = dmg;
  g_window = SDL_CreateWindow("soup_disp", 160*SCALE, 144*SCALE, 0);
  g_r = SDL_CreateRenderer(g_window, NULL);
  g_target = SDL_CreateTexture(
      g_r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 256, 256);
  g_frame_sem = SDL_CreateSemaphore(0);
}

void
display_next_frame(void) {
  SDL_SignalSemaphore(g_frame_sem);
}

bool
display_should_render(void) {
  return SDL_TryWaitSemaphore(g_frame_sem);
}

void
display_render(void) {
  // bg surface
  const struct lcdc* lcdc = &g_dmg->lcdc;
  SDL_SetRenderTarget(g_r, g_target);
  SDL_SetRenderDrawColor(g_r, 0, 0, 0, 255);
  SDL_RenderClear(g_r);
  draw_bg();
  SDL_SetRenderTarget(g_r, NULL);

  // window
  SDL_SetRenderDrawColor(g_r, 0, 0, 0, 255);
  SDL_RenderClear(g_r);

  // viewport
  SDL_FRect src0 = { *(lcdc->scx), *(lcdc->scy), 160, 144 };
  SDL_FRect viewport = { 0, 0, 160*SCALE, 144*SCALE };
  SDL_SetTextureScaleMode(g_target, SDL_SCALEMODE_NEAREST);
  SDL_RenderTexture(g_r, g_target, &src0, &viewport);

  SDL_RenderPresent(g_r);
  (void)lcdc;
}

