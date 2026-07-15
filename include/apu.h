#pragma once

#include <stdint.h>

typedef struct SDL_AudioStream SDL_AudioStream;
typedef struct SDL_Semaphore SDL_Semaphore;

struct apu {
  // sound 1
  uint8_t* nr10; // sweep
  uint8_t* nr11; // duration
  uint8_t* nr12; // envelope
  uint8_t* nr13; // freq lo
  uint8_t* nr14; // freq hi / control
  // sound 2
  uint8_t* nr21; // duration
  uint8_t* nr22; // envelope
  uint8_t* nr23; // freq lo
  uint8_t* nr24; // freq hi / control
  // sound 3
  uint8_t* nr30; // enable
  uint8_t* nr31; // duration
  uint8_t* nr32; // shift
  uint8_t* nr33; // freq lo
  uint8_t* nr34; // control
  // sound 4
  uint8_t* nr41; // duration
  uint8_t* nr42; // envelope
  uint8_t* nr43; // freq
  uint8_t* nr44; // control
  // control
  uint8_t* nr50; // volume
  uint8_t* nr51; // mixer
  uint8_t* nr52; // enable

  SDL_AudioStream* stream1;
  SDL_Semaphore* sem1;
  int device_id;
  int pad;
};

void apu_init(struct apu*, uint8_t* memmap);

void apu_reg_write(struct apu*, uint16_t reg, uint8_t data);

