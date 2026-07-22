#include "apu.h"

#include "assert.h"
#include "bits.h"
#include "mem.h"

#include <stdint.h>
#include <string.h>

#include <SDL3/SDL.h>

#define APU_AUDIOSPEC_MAKE(_freq) \
    { .format = SDL_AUDIO_U8, .channels = 1, .freq = _freq }

static const uint8_t s_samples[4][8] = {
    { 1, 0, 0, 0, 0, 0, 0, 0 }, // 12.5%
    { 1, 1, 0, 0, 0, 0, 0, 0 }, // 25%
    { 1, 1, 1, 1, 0, 0, 0, 0 }, // 50%
    { 1, 1, 1, 1, 1, 1, 0, 0 }, // 75%
};

static int sound1_thread(void*);

void
apu_init(struct apu* apu, uint8_t* memmap) {
  memset(apu, 0, sizeof(struct apu));
  // sound 1
  apu->nr10 = memmap + REG_NR10;
  apu->nr11 = memmap + REG_NR11;
  apu->nr12 = memmap + REG_NR12;
  apu->nr13 = memmap + REG_NR13;
  apu->nr14 = memmap + REG_NR14;
  // control
  apu->nr50 = memmap + REG_NR50;
  apu->nr51 = memmap + REG_NR51;
  apu->nr52 = memmap + REG_NR52;

  bool ret = SDL_Init(SDL_INIT_AUDIO);
  assert(ret);

  apu->stream1 = SDL_CreateAudioStream(NULL, NULL);
  apu->sem1 = SDL_CreateSemaphore(0);
  apu->device_id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
  assert(apu->device_id);

  auto thread = SDL_CreateThread(sound1_thread, "sound1", apu);
  assert(thread);
}

static void
sound1_queue_samples(struct apu* apu) {
  if (!SDL_GetAudioStreamDevice(apu->stream1))
    return; // deactivated

  // sample rate
  SDL_AudioSpec spec;
  bool ret = SDL_GetAudioStreamFormat(apu->stream1, &spec, NULL);
  assert(ret);
  const int sample_rate = spec.freq;

  // duty
  const uint8_t nr11 = *(apu->nr11);
  const uint8_t duty = bits_7_6(nr11);

  // duration
  const uint8_t t1 = bits_5_0(nr11);
  const float duration_s = (64 - t1) * (1 / 256.0f);
  const int num_samples = sample_rate * duration_s;

  // envelope
  const uint8_t nr12 = *(apu->nr12);
  uint8_t amp = bits_7_4(nr12);
  const uint8_t n = bits_2_0(nr12);
  const float step_s = n * (1 / 64.0f);
  const int samples_per_step = step_s * sample_rate;
  const int dir = bit_3(nr12);

  // samples
  int sampled = 0;
  uint8_t pd_samples[8];
  const int zero = SDL_GetSilenceValueForFormat(SDL_AUDIO_U8);
  while (sampled < num_samples) {
    for (size_t i = 0; i < sizeof(pd_samples); i++) {
      pd_samples[i] = s_samples[duty][i] ? zero + ((amp+1) * 8) - 1 : zero;
    }
    SDL_PutAudioStreamData(apu->stream1, pd_samples, sizeof(pd_samples));

    sampled += sizeof(pd_samples);
    if (sampled % samples_per_step == 0) {
      amp += dir ? 1 : -1;
    }
  }
}

static int
sound1_thread(void* data) {
  auto apu = (struct apu*)data;
  while (true) {
    SDL_WaitSemaphore(apu->sem1);
    sound1_queue_samples(apu);
  }
  return 0;
}

static inline void
apu_nr14_write(struct apu* apu, uint8_t data) {
  const uint8_t prev = *(apu->nr14);
  *(apu->nr14) = data;

  if (bit_7(data) && bit_7(prev)) {
    SDL_UnbindAudioStream(apu->stream1); // stop sound 1
  }
  if (!bit_7(data)) {
    return;
  }

  // frequency
  const uint16_t x = ((uint16_t)bits_2_0(data) << 8) | *(apu->nr13);
  const int freq = 1048576 / (2048 - x);

  // apply changes, (re)activate sound 1
  SDL_AudioSpec spec = APU_AUDIOSPEC_MAKE(freq);
  SDL_SetAudioStreamFormat(apu->stream1, &spec, NULL);
  SDL_ClearAudioStream(apu->stream1);
  bool ret = SDL_BindAudioStream(apu->device_id, apu->stream1);
  assert(ret);
  // XXX check counter bit
  SDL_SignalSemaphore(apu->sem1);
}

void
apu_reg_write(struct apu* apu, uint16_t reg, uint8_t data) {
  switch (reg) {
  case REG_NR10:
    *(apu->nr10) = data;
    break;
  case REG_NR11:
    *(apu->nr11) = data;
    break;
  case REG_NR12:
    *(apu->nr12) = data;
    break;
  case REG_NR13:
    *(apu->nr13) = data;
    break;
  case REG_NR14:
    apu_nr14_write(apu, data);
    break;
  case REG_NR50:
    *(apu->nr50) = data;
    break;
  case REG_NR51:
    *(apu->nr51) = data;
    break;
  case REG_NR52:
    *(apu->nr52) = data;
    break;
  default:
    assert(false);
  }
}

