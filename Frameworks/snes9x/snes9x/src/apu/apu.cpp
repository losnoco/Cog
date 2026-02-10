/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include <memory>
#include <snes9x/snes9x.h>
#include <snes9x/resampler.h>

#include <snes9x/snes.hpp>
#include <snes9x/smp.hpp>
#include <snes9x/sdsp.hpp>

static constexpr int APU_DEFAULT_INPUT_RATE = 31950; // ~59.94Hz
static constexpr int APU_SAMPLE_BLOCK = 48;
static constexpr int APU_NUMERATOR_NTSC = 15664;
static constexpr int APU_DENOMINATOR_NTSC = 328125;
static constexpr int APU_NUMERATOR_PAL = 34176;
static constexpr int APU_DENOMINATOR_PAL = 709379;
// Max number of samples we'll ever generate before call to port API and
// moving the samples to the resampler.
// This is 535 sample frames, which corresponds to 1 video frame + some leeway
// for use with SoundSync, multiplied by 2, for left and right samples.
static constexpr int MINIMUM_BUFFER_SIZE = 550 * 2;

void S9xClearSamples(struct S9xState *st)
{
	st->SPC.resampler->clear();
}

bool S9xMixSamples(struct S9xState *st, uint8_t *dest, int sample_count)
{
	int16_t *out = reinterpret_cast<int16_t *>(dest);

	if (st->Settings.Mute)
	{
		std::fill_n(&out[0], sample_count, 0);
		S9xClearSamples(st);
	}
	else
	{
		if (st->SPC.resampler->avail() >= sample_count)
			st->SPC.resampler->read(reinterpret_cast<short *>(out), sample_count);
		else
		{
			std::fill_n(&out[0], sample_count, 0);
		}
	}

	st->SPC.sound_in_sync = st->SPC.resampler->space_empty() >= 535 * 2 || !st->Settings.SoundSync || st->Settings.TurboMode || st->Settings.Mute;

	return true;
}

int S9xGetSampleCount(struct S9xState *st)
{
	return st->SPC.resampler->avail();
}

void S9xLandSamples(struct S9xState *st)
{
	st->SPC.sound_in_sync = st->SPC.resampler->space_empty() >= 535 * 2 || !st->Settings.SoundSync || st->Settings.TurboMode || st->Settings.Mute;
}

bool S9xSyncSound(struct S9xState *st)
{
	if (!st->Settings.SoundSync || st->SPC.sound_in_sync)
		return true;

	S9xLandSamples(st);

	return st->SPC.sound_in_sync;
}

static void UpdatePlaybackRate(struct S9xState *st)
{
	if (!st->Settings.SoundInputRate)
		st->Settings.SoundInputRate = APU_DEFAULT_INPUT_RATE;

	double time_ratio = static_cast<double>(st->Settings.SoundInputRate) * st->SPC.timing_hack_numerator / (st->Settings.SoundPlaybackRate * st->SPC.timing_hack_denominator);
	st->SPC.resampler->time_ratio(time_ratio);
}

bool S9xInitSound(struct S9xState *st, int buffer_ms)
{
	// The resampler and spc unit use samples (16-bit short) as arguments.
	int buffer_size_samples = MINIMUM_BUFFER_SIZE;
	int requested_buffer_size_samples = st->Settings.SoundPlaybackRate * buffer_ms * 2 / 1000;

	if (requested_buffer_size_samples > buffer_size_samples)
		buffer_size_samples = requested_buffer_size_samples;

	st->SPC.resampler.reset(new Resampler(buffer_size_samples));
	if (!st->SPC.resampler)
		return false;

	st->SPC.cpu.reset(new SNES::CPU);
	st->SPC.smp.reset(new SNES::SMP);
	st->SPC.dsp.reset(new SNES::DSP);
	if (!st->SPC.cpu || !st->SPC.smp || !st->SPC.dsp)
		return false;

	st->SPC.dsp->spc_dsp.set_output(st->SPC.resampler.get());

	UpdatePlaybackRate(st);

	st->SPC.sound_enabled = S9xOpenSoundDevice(st);

	return st->SPC.sound_enabled;
}

void S9xSetSoundControl(struct S9xState *st, uint8_t voice_switch)
{
	st->SPC.dsp->spc_dsp.set_stereo_switch((voice_switch << 8) | voice_switch);
}

void S9xSetSoundMute(struct S9xState *st, bool mute)
{
	st->Settings.Mute = mute;
	if (!st->SPC.sound_enabled)
		st->Settings.Mute = true;
}

S9xSPC::S9xSPC()
{
	sound_in_sync = true;
	sound_enabled = false;

	timing_hack_denominator = 256;
	/* Set these to NTSC for now. Will change to PAL in S9xAPUTimingSetSpeedup
	   if necessary on game load. */
	ratio_numerator = APU_NUMERATOR_NTSC;
	ratio_denominator = APU_DENOMINATOR_NTSC;
}

bool S9xInitAPU(struct S9xState *st)
{
	st->SPC.resampler.reset();

	return true;
}

void S9xDeinitAPU(struct S9xState *st)
{
	st->SPC.resampler.reset();
}

static inline int S9xAPUGetClock(struct S9xState *st, int32_t cpucycles)
{
	return (st->SPC.ratio_numerator * (cpucycles - st->SPC.reference_time) + st->SPC.remainder) / st->SPC.ratio_denominator;
}

static inline int S9xAPUGetClockRemainder(struct S9xState *st, int32_t cpucycles)
{
	return (st->SPC.ratio_numerator * (cpucycles - st->SPC.reference_time) + st->SPC.remainder) % st->SPC.ratio_denominator;
}

void S9xAPUExecute(struct S9xState *st)
{
	st->SPC.smp->clock -= S9xAPUGetClock(st, st->CPU.Cycles);
	st->SPC.smp->enter();

	st->SPC.remainder = S9xAPUGetClockRemainder(st, st->CPU.Cycles);

	S9xAPUSetReferenceTime(st, st->CPU.Cycles);
}

uint8_t S9xAPUReadPort(struct S9xState *st, int port)
{
	S9xAPUExecute(st);
	return static_cast<uint8_t>(st->SPC.smp->port_read(port & 3));
}

void S9xAPUWritePort(struct S9xState *st, int port, uint8_t byte)
{
	S9xAPUExecute(st);
	st->SPC.cpu->port_write(port & 3, byte);
}

void S9xAPUSetReferenceTime(struct S9xState *st, int32_t cpucycles)
{
	st->SPC.reference_time = cpucycles;
}

void S9xAPUEndScanline(struct S9xState *st)
{
	S9xAPUExecute(st);
	st->SPC.dsp->synchronize();

	if (st->SPC.resampler->space_filled() >= APU_SAMPLE_BLOCK || !st->SPC.sound_in_sync)
		S9xLandSamples(st);
}

void S9xAPUTimingSetSpeedup(struct S9xState *st, int ticks)
{
	st->SPC.timing_hack_denominator = 256 - ticks;

	st->SPC.ratio_numerator = st->Settings.PAL ? APU_NUMERATOR_PAL : APU_NUMERATOR_NTSC;
	st->SPC.ratio_denominator = (st->Settings.PAL ? APU_DENOMINATOR_PAL : APU_DENOMINATOR_NTSC) * st->SPC.timing_hack_denominator / st->SPC.timing_hack_numerator;

	UpdatePlaybackRate(st);
}

void S9xResetAPU(struct S9xState *st)
{
	st->SPC.reference_time = 0;
	st->SPC.remainder = 0;
	st->SPC.cpu->reset();
	st->SPC.smp->power(&st->SPC);
	st->SPC.dsp->power(st);

	S9xClearSamples(st);
}

bool S9xOpenSoundDevice(struct S9xState *st)
{
	return true;
}
