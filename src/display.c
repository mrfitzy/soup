#include "display.h"

#include "bits.h"
#include "dmg.h"

#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdlib.h>

#define SCALE (3)

static const uint8_t g_pal[4][3] = {
    {0},
    {160, 160, 160},
    {84,  84,  84},
    {255, 255, 255}
};

struct display {
  struct dmg_system* dmg;
  SDL_Window* window;
  SDL_Renderer* r;
  SDL_Texture* texture;
  SDL_Semaphore* frame_ready;
};

struct display*
display_create(struct dmg_system* dmg) {
  struct display* display = malloc(sizeof(struct display));
  display->dmg = dmg;
  display->window = SDL_CreateWindow("soup", 160*SCALE, 144*SCALE, 0);
  display->r = SDL_CreateRenderer(display->window, NULL);
  display->texture = SDL_CreateTexture(
      display->r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 256, 256);
  display->frame_ready = SDL_CreateSemaphore(0);
  SDL_RaiseWindow(display->window);
  return display;
}

void
display_destroy(struct display* display) {
  SDL_SignalSemaphore(display->frame_ready);
  assert(!SDL_GetSemaphoreValue(display->frame_ready));
  SDL_DestroySemaphore(display->frame_ready);
  SDL_DestroyTexture(display->texture);
  SDL_DestroyRenderer(display->r);
  SDL_DestroyWindow(display->window);
  free(display);
}

static void
draw_tile(SDL_Renderer* r, const uint8_t* data, int x, int y) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      unsigned c = (bit_n(data[2*i], 7-j) << 1) | bit_n(data[2*i + 1], 7-j);
      SDL_SetRenderDrawColor(r, g_pal[c][0], g_pal[c][1], g_pal[c][2], 255);
      SDL_RenderPoint(r, (float)x + (float)j, (float)y + (float)i);
    }
  }
}

static void
draw_bg(struct display* display) {
  struct dmg_system* dmg = display->dmg;
  const struct lcdc* lcdc = &dmg->lcdc;
  const uint8_t reg_lcdc = *(lcdc->lcdc);
  const uint8_t* bg_map = dmg->mem.mem + (bit_3(reg_lcdc) ? 0x9c00 : 0x9800);
  const uint8_t* tile_data = dmg->mem.mem + (bit_4(reg_lcdc) ? 0x8000 : 0x8800);
  for (int i = 0; i < 32; i++) {
    for (int j = 0; j < 32; j++) {
      const uint8_t tile = bg_map[i*32 + j];
      draw_tile(display->r, (tile_data + (16 * tile)), j*8, i*8);
    }
  }
}

void
display_next_frame(struct display* display) {
  SDL_SignalSemaphore(display->frame_ready);
}

bool
display_should_render(struct display* display) {
  return SDL_TryWaitSemaphore(display->frame_ready);
}

void
display_render(struct display* display) {
  // render bg surface
  const struct lcdc* lcdc = &display->dmg->lcdc;
  SDL_SetRenderTarget(display->r, display->texture);
  SDL_SetRenderDrawColor(display->r, 0, 0, 0, 255);
  SDL_RenderClear(display->r);
  draw_bg(display);
  SDL_SetRenderTarget(display->r, NULL);

  // render window
  SDL_SetRenderDrawColor(display->r, 0, 0, 0, 255);
  SDL_RenderClear(display->r);

  // render viewport
  SDL_FRect src = { *(lcdc->scx), *(lcdc->scy), 160, 144 };
  SDL_FRect viewport = { 0, 0, 160*SCALE, 144*SCALE };
  SDL_SetTextureScaleMode(display->texture, SDL_SCALEMODE_NEAREST);
  SDL_RenderTexture(display->r, display->texture, &src, &viewport);

  // present
  SDL_RenderPresent(display->r);
}

