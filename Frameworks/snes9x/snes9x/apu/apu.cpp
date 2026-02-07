/***********************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.

  (c) Copyright 1996 - 2002  Gary Henderson (gary.henderson@ntlworld.com),
                             Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2002 - 2004  Matthew Kendora

  (c) Copyright 2002 - 2005  Peter Bortas (peter@bortas.org)

  (c) Copyright 2004 - 2005  Joel Yliluoma (http://iki.fi/bisqwit/)

  (c) Copyright 2001 - 2006  John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2006  funkyass (funkyass@spam.shaw.ca),
                             Kris Bleakley (codeviolation@hotmail.com)

  (c) Copyright 2002 - 2010  Brad Jorsch (anomie@users.sourceforge.net),
                             Nach (n-a-c-h@users.sourceforge.net),

  (c) Copyright 2002 - 2011  zones (kasumitokoduck@yahoo.com)

  (c) Copyright 2006 - 2007  nitsuja

  (c) Copyright 2009 - 2011  BearOso,
                             OV2


  BS-X C emulator code
  (c) Copyright 2005 - 2006  Dreamer Nom,
                             zones

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003  _Demo_ (_demo_@zsnes.com),
                             Nach,
                             zsKnight (zsknight@zsnes.com)

  C4 C++ code
  (c) Copyright 2003 - 2006  Brad Jorsch,
                             Nach

  DSP-1 emulator code
  (c) Copyright 1998 - 2006  _Demo_,
                             Andreas Naive (andreasnaive@gmail.com),
                             Gary Henderson,
                             Ivar (ivar@snes9x.com),
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora,
                             Nach,
                             neviksti (neviksti@hotmail.com)

  DSP-2 emulator code
  (c) Copyright 2003         John Weidman,
                             Kris Bleakley,
                             Lord Nightmare (lord_nightmare@users.sourceforge.net),
                             Matthew Kendora,
                             neviksti

  DSP-3 emulator code
  (c) Copyright 2003 - 2006  John Weidman,
                             Kris Bleakley,
                             Lancer,
                             z80 gaiden

  DSP-4 emulator code
  (c) Copyright 2004 - 2006  Dreamer Nom,
                             John Weidman,
                             Kris Bleakley,
                             Nach,
                             z80 gaiden

  OBC1 emulator code
  (c) Copyright 2001 - 2004  zsKnight,
                             pagefault (pagefault@zsnes.com),
                             Kris Bleakley
                             Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code used in 1.39-1.51
  (c) Copyright 2002         Matthew Kendora with research by
                             zsKnight,
                             John Weidman,
                             Dark Force

  SPC7110 and RTC C++ emulator code used in 1.52+
  (c) Copyright 2009         byuu,
                             neviksti

  S-DD1 C emulator code
  (c) Copyright 2003         Brad Jorsch with research by
                             Andreas Naive,
                             John Weidman

  S-RTC C emulator code
  (c) Copyright 2001 - 2006  byuu,
                             John Weidman

  ST010 C++ emulator code
  (c) Copyright 2003         Feather,
                             John Weidman,
                             Kris Bleakley,
                             Matthew Kendora

  Super FX x86 assembler emulator code
  (c) Copyright 1998 - 2003  _Demo_,
                             pagefault,
                             zsKnight

  Super FX C emulator code
  (c) Copyright 1997 - 1999  Ivar,
                             Gary Henderson,
                             John Weidman

  Sound emulator code used in 1.5-1.51
  (c) Copyright 1998 - 2003  Brad Martin
  (c) Copyright 1998 - 2006  Charles Bilyue'

  Sound emulator code used in 1.52+
  (c) Copyright 2004 - 2007  Shay Green (gblargg@gmail.com)

  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004  Marcus Comstedt (marcus@mc.pp.se)

  2xSaI filter
  (c) Copyright 1999 - 2001  Derek Liauw Kie Fa

  HQ2x, HQ3x, HQ4x filters
  (c) Copyright 2003         Maxim Stepin (maxim@hiend3d.com)

  NTSC filter
  (c) Copyright 2006 - 2007  Shay Green

  GTK+ GUI code
  (c) Copyright 2004 - 2011  BearOso

  Win32 GUI code
  (c) Copyright 2003 - 2006  blip,
                             funkyass,
                             Matthew Kendora,
                             Nach,
                             nitsuja
  (c) Copyright 2009 - 2011  OV2

  Mac OS GUI code
  (c) Copyright 1998 - 2001  John Stiles
  (c) Copyright 2001 - 2011  zones


  Specific ports contains the works of other authors. See headers in
  individual files.


  Snes9x homepage: http://www.snes9x.com/

  Permission to use, copy, modify and/or distribute Snes9x in both binary
  and source form, for non-commercial purposes, is hereby granted without
  fee, providing that this license information and copyright notice appear
  with all copies and any derived work.

  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software or it's derivatives.

  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes,
  but is not limited to, charging money for Snes9x or software derived from
  Snes9x, including Snes9x or derivatives in commercial game bundles, and/or
  using Snes9x as a promotion for your commercial product.

  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.

  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
 ***********************************************************************************/

#include <memory>
#include "../snes9x.h"
#include "linear_resampler.h"
#include "hermite_resampler.h"
#include "bspline_resampler.h"
#include "osculating_resampler.h"
#include "sinc_resampler.h"

bool SincResampler::initializedLUTs = false;
double SincResampler::sinc_lut[SincResampler::SINC_SAMPLES + 1];

static const uint32_t APU_DEFAULT_INPUT_RATE = 32000;
static const int APU_MINIMUM_SAMPLE_COUNT = 512;
static const int APU_MINIMUM_SAMPLE_BLOCK = 128;
static const uint32_t APU_NUMERATOR_NTSC = 15664;
static const uint32_t APU_DENOMINATOR_NTSC = 328125;
static const uint32_t APU_NUMERATOR_PAL = 34176;
static const uint32_t APU_DENOMINATOR_PAL = 709379;

static const uint8_t APUROM[] =
{
	0xCD, 0xEF, 0xBD, 0xE8, 0x00, 0xC6, 0x1D, 0xD0,
	0xFC, 0x8F, 0xAA, 0xF4, 0x8F, 0xBB, 0xF5, 0x78,
	0xCC, 0xF4, 0xD0, 0xFB, 0x2F, 0x19, 0xEB, 0xF4,
	0xD0, 0xFC, 0x7E, 0xF4, 0xD0, 0x0B, 0xE4, 0xF5,
	0xCB, 0xF4, 0xD7, 0x00, 0xFC, 0xD0, 0xF3, 0xAB,
	0x01, 0x10, 0xEF, 0x7E, 0xF4, 0x10, 0xEB, 0xBA,
	0xF6, 0xDA, 0x00, 0xBA, 0xF4, 0xC4, 0xF4, 0xDD,
	0x5D, 0xD0, 0xDB, 0x1F, 0x00, 0x00, 0xC0, 0xFF
};

static void EightBitize(uint8_t *buffer, int sample_count)
{
	int16_t *buf16 = reinterpret_cast<int16_t *>(buffer);

	for (int i = 0; i < sample_count; ++i)
		buffer[i] = static_cast<uint8_t>((buf16[i] / 256) + 128);
}

static void DeStereo(uint8_t *buffer, int sample_count)
{
	int16_t *buf = reinterpret_cast<int16_t *>(buffer);

	for (int i = 0; i < sample_count >> 1; ++i)
	{
		int32_t s1 = static_cast<int32_t>(buf[2 * i]);
		int32_t s2 = static_cast<int32_t>(buf[2 * i + 1]);
		buf[i] = static_cast<int16_t>((s1 + s2) >> 1);
	}
}

static void ReverseStereo(uint8_t *src_buffer, int sample_count)
{
	int16_t *buffer = reinterpret_cast<int16_t *>(src_buffer);

	for (int i = 0; i < sample_count; i += 2)
		std::swap(buffer[i], buffer[i + 1]);
}

bool S9xMixSamples(struct S9xState *st, uint8_t *buffer, int sample_count)
{
	static int shrink_buffer_size = -1;
	uint8_t *dest;

	if (!st->Settings.SixteenBitSound || !st->Settings.Stereo)
	{
		/* We still need both stereo samples for generating the mono sample */
		if (!st->Settings.Stereo)
			sample_count <<= 1;

		/* We still have to generate 16-bit samples for bit-dropping, too */
		if (shrink_buffer_size < (sample_count << 1))
		{
			st->SPC.shrink_buffer.reset(new uint8_t[sample_count << 1]);
			shrink_buffer_size = sample_count << 1;
		}

		dest = st->SPC.shrink_buffer.get();
	}
	else
		dest = buffer;

	if (st->Settings.Mute)
	{
		std::fill_n(&dest[0], sample_count << 1, 0);
		st->SPC.resampler->clear();

		return false;
	}
	else
	{
		if (st->SPC.resampler->avail() >= sample_count + st->SPC.lag)
		{
			st->SPC.resampler->read(reinterpret_cast<short *>(dest), sample_count);
			if (st->SPC.lag == st->SPC.lag_master)
				st->SPC.lag = 0;
		}
		else
		{
			std::fill_n(&buffer[0], (sample_count << (st->Settings.SixteenBitSound ? 1 : 0)) >> (st->Settings.Stereo ? 0 : 1), st->Settings.SixteenBitSound ? 0 : 128);
			if (!st->SPC.lag)
				st->SPC.lag = st->SPC.lag_master;

			return false;
		}
	}

	if (st->Settings.ReverseStereo && st->Settings.Stereo)
		ReverseStereo(dest, sample_count);

	if (!st->Settings.Stereo || !st->Settings.SixteenBitSound)
	{
		if (!st->Settings.Stereo)
		{
			DeStereo(dest, sample_count);
			sample_count >>= 1;
		}

		if (!st->Settings.SixteenBitSound)
			EightBitize(dest, sample_count);

		std::copy_n(&dest[0], sample_count << (st->Settings.SixteenBitSound ? 1 : 0), &buffer[0]);
	}

	return true;
}

int S9xGetSampleCount(struct S9xState *st)
{
	return st->SPC.resampler->avail() >> (st->Settings.Stereo ? 0 : 1);
}

void S9xFinalizeSamples(struct S9xState *st)
{
	if (!st->Settings.Mute)
	{
		if (!st->SPC.resampler->push(reinterpret_cast<short *>(st->SPC.landing_buffer.get()), st->spc_core->sample_count()))
		{
			/* We weren't able to process the entire buffer. Potential overrun. */
			st->SPC.sound_in_sync = false;

			if (st->Settings.SoundSync && !st->Settings.TurboMode)
				return;
		}
	}

	st->SPC.sound_in_sync = !st->Settings.SoundSync || st->Settings.TurboMode || st->Settings.Mute || st->SPC.resampler->space_empty() >= st->SPC.resampler->space_filled();

	st->spc_core->set_output(reinterpret_cast<SNES_SPC::sample_t *>(st->SPC.landing_buffer.get()), st->SPC.buffer_size >> 1);
}

void S9xLandSamples(struct S9xState *st)
{
	S9xFinalizeSamples(st);
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

template<class ResamplerClass> bool S9xInitSound(struct S9xState *st, int buffer_ms, int lag_ms)
{
	// buffer_ms : buffer size given in millisecond
	// lag_ms    : allowable time-lag given in millisecond

	int sample_count = buffer_ms * 32000 / 1000;
	int lag_sample_count = lag_ms * 32000 / 1000;

	st->SPC.lag_master = lag_sample_count;
	if (st->Settings.Stereo)
		st->SPC.lag_master <<= 1;
	st->SPC.lag = st->SPC.lag_master;

	if (sample_count < APU_MINIMUM_SAMPLE_COUNT)
		sample_count = APU_MINIMUM_SAMPLE_COUNT;

	st->SPC.buffer_size = sample_count;
	if (st->Settings.Stereo)
		st->SPC.buffer_size <<= 1;
	if (st->Settings.SixteenBitSound)
		st->SPC.buffer_size <<= 1;

	st->SPC.landing_buffer.reset(new uint8_t[st->SPC.buffer_size * 2]);
	if (!st->SPC.landing_buffer)
		return false;

	/* The resampler and spc unit use samples (16-bit short) as
	 *   arguments. Use 2x in the resampler for buffer leveling with SoundSync */
	st->SPC.resampler.reset(new ResamplerClass(st->SPC.buffer_size >> (st->Settings.SoundSync ? 0 : 1)));
	if (!st->SPC.resampler)
	{
		st->SPC.landing_buffer.reset();
		return false;
	}

	st->spc_core->set_output(reinterpret_cast<SNES_SPC::sample_t *>(st->SPC.landing_buffer.get()), st->SPC.buffer_size >> 1);

	UpdatePlaybackRate(st);

	st->SPC.sound_enabled = S9xOpenSoundDevice(st);

	return st->SPC.sound_enabled;
}

template bool S9xInitSound<LinearResampler>(struct S9xState *st, int, int);
template bool S9xInitSound<HermiteResampler>(struct S9xState *st, int, int);
template bool S9xInitSound<BsplineResampler>(struct S9xState *st, int, int);
template bool S9xInitSound<OsculatingResampler>(struct S9xState *st, int, int);
template bool S9xInitSound<SincResampler>(struct S9xState *st, int, int);

void S9xSetSoundControl(struct S9xState *st, uint8_t voice_switch)
{
	st->spc_core->dsp_set_stereo_switch((voice_switch << 8) | voice_switch);
}

void S9xSetSoundMute(struct S9xState *st, bool mute)
{
	st->Settings.Mute = mute;
	if (!st->SPC.sound_enabled)
		st->Settings.Mute = true;
}

S9xSPC::S9xSPC()
	: timing_hack_numerator(SNES_SPC::tempo_unit)
{
	sound_in_sync = true;
	sound_enabled = false;

	lag_master = 0;
	lag = 0;

	timing_hack_denominator = SNES_SPC::tempo_unit;
	/* Set these to NTSC for now. Will change to PAL in S9xAPUTimingSetSpeedup
	   if necessary on game load. */
	ratio_numerator = APU_NUMERATOR_NTSC;
	ratio_denominator = APU_DENOMINATOR_NTSC;
}

bool S9xInitAPU(struct S9xState *st)
{
	st->spc_core.reset(new SNES_SPC);
	if (!st->spc_core)
		return false;

	st->spc_core->init();
	st->spc_core->init_rom(APUROM);

	st->SPC.landing_buffer.reset();
	st->SPC.shrink_buffer.reset();
	st->SPC.resampler.reset();

	return true;
}

void S9xDeinitAPU(struct S9xState *st)
{
	st->spc_core.reset();
	st->SPC.resampler.reset();
	st->SPC.landing_buffer.reset();
	st->SPC.shrink_buffer.reset();
}

static inline int S9xAPUGetClock(struct S9xState *st, int32_t cpucycles)
{
	return (st->SPC.ratio_numerator * (cpucycles - st->SPC.reference_time) + st->SPC.remainder) / st->SPC.ratio_denominator;
}

static inline int S9xAPUGetClockRemainder(struct S9xState *st, int32_t cpucycles)
{
	return (st->SPC.ratio_numerator * (cpucycles - st->SPC.reference_time) + st->SPC.remainder) % st->SPC.ratio_denominator;
}

uint8_t S9xAPUReadPort(struct S9xState *st, int port)
{
	return static_cast<uint8_t>(st->spc_core->read_port(S9xAPUGetClock(st, st->CPU.Cycles), port));
}

void S9xAPUWritePort(struct S9xState *st, int port, uint8_t byte)
{
	st->spc_core->write_port(S9xAPUGetClock(st, st->CPU.Cycles), port, byte);
}

void S9xAPUSetReferenceTime(struct S9xState *st, int32_t cpucycles)
{
	st->SPC.reference_time = cpucycles;
}

void S9xAPUExecute(struct S9xState *st)
{
	/* Accumulate partial APU cycles */
	st->spc_core->end_frame(S9xAPUGetClock(st, st->CPU.Cycles));

	st->SPC.remainder = S9xAPUGetClockRemainder(st, st->CPU.Cycles);

	S9xAPUSetReferenceTime(st, st->CPU.Cycles);
}

void S9xAPUEndScanline(struct S9xState *st)
{
	S9xAPUExecute(st);

	if (st->spc_core->sample_count() >= APU_MINIMUM_SAMPLE_BLOCK || !st->SPC.sound_in_sync)
		S9xLandSamples(st);
}

void S9xAPUTimingSetSpeedup(struct S9xState *st, int ticks)
{
	st->SPC.timing_hack_denominator = SNES_SPC::tempo_unit - ticks;
	st->spc_core->set_tempo(st->SPC.timing_hack_denominator);

	st->SPC.ratio_numerator = st->Settings.PAL ? APU_NUMERATOR_PAL : APU_NUMERATOR_NTSC;
	st->SPC.ratio_denominator = (st->Settings.PAL ? APU_DENOMINATOR_PAL : APU_DENOMINATOR_NTSC) * st->SPC.timing_hack_denominator / st->SPC.timing_hack_numerator;

	UpdatePlaybackRate(st);
}

void S9xAPUAllowTimeOverflow(struct S9xState *st, bool allow)
{
	st->spc_core->spc_allow_time_overflow(allow);
}

void S9xResetAPU(struct S9xState *st)
{
	st->SPC.reference_time = 0;
	st->SPC.remainder = 0;
	st->spc_core->reset();
	st->spc_core->set_output(reinterpret_cast<SNES_SPC::sample_t *>(st->SPC.landing_buffer.get()), st->SPC.buffer_size >> 1);

	st->SPC.resampler->clear();
}

bool S9xOpenSoundDevice(struct S9xState *st)
{
	return true;
}
