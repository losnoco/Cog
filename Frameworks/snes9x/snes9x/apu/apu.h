/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

struct S9xState;

namespace SNES {
struct CPU;
struct SMP;
struct DSP;
}

#include <snes9x/resampler.h>

struct S9xSPC
{
  bool sound_in_sync;
  bool sound_enabled;

  std::unique_ptr<SNES::CPU> cpu;
  std::unique_ptr<SNES::SMP> smp;
  std::unique_ptr<SNES::DSP> dsp;
  std::unique_ptr<Resampler> resampler;

  int32_t reference_time;
  uint32_t remainder;

  static constexpr int timing_hack_numerator = 256;
  int timing_hack_denominator;
  uint32_t ratio_numerator;
  uint32_t ratio_denominator;

  S9xSPC();
};

bool S9xInitAPU(struct S9xState *st);
void S9xDeinitAPU(struct S9xState *st);
void S9xResetAPU(struct S9xState *st);
uint8_t S9xAPUReadPort(struct S9xState *st, int);
void S9xAPUWritePort(struct S9xState *st, int, uint8_t);
void S9xAPUEndScanline(struct S9xState *st);
void S9xAPUSetReferenceTime(struct S9xState *st, int32_t);
void S9xAPUTimingSetSpeedup(struct S9xState *st, int);

bool S9xInitSound(struct S9xState *st, int);
bool S9xOpenSoundDevice(struct S9xState *st);

void S9xClearSamples(struct S9xState *st);
bool S9xSyncSound(struct S9xState *st);
int S9xGetSampleCount(struct S9xState *st);
void S9xSetSoundControl(struct S9xState *st, uint8_t);
void S9xSetSoundMute(struct S9xState *st, bool);
bool S9xMixSamples(struct S9xState *st, uint8_t *, int);
