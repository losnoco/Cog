/*
	Copyright 2006 Theo Berkau
	Copyright (C) 2006-2015 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SPU_H
#define SPU_H

#include <iosfwd>
#include <string>
#include <assert.h>
#include <stdio.h>
#include <memory>

#include <vio2sf/types.h>
#include <vio2sf/matrix.h>
#include <vio2sf/metaspu.h>

class EMUFILE;

#define SNDCORE_DEFAULT         -1
#define SNDCORE_DUMMY           0

#define CHANSTAT_STOPPED          0
#define CHANSTAT_PLAY             1


//who made these static? theyre used in multiple places.
FORCEINLINE u32 sputrunc(float f) { return u32floor(f); }
FORCEINLINE u32 sputrunc(double d) { return u32floor(d); }
FORCEINLINE s32 spumuldiv7(s32 val, u8 multiplier) {
	assert(multiplier <= 127);
	return (multiplier == 127) ? val : ((val * multiplier) >> 7);
}

enum SPUInterpolationMode
{
	SPUInterpolation_None   = 0,
	SPUInterpolation_Linear = 1,
	SPUInterpolation_Cosine = 2,
	SPUInterpolation_Sharp  = 3
};

struct SoundInterface_struct
{
   int id;
   const char *Name;
   int (*Init)(void *context, int buffersize);
   void (*DeInit)(void *context);
   void (*UpdateAudio)(void *context, s16 *buffer, u32 num_samples);
   u32 (*GetAudioSpace)(void *context);
   void (*MuteAudio)(void *context);
   void (*UnMuteAudio)(void *context);
   void (*SetVolume)(void *context, int volume);
   void (*ClearBuffer)(void *context);
   void (*FetchSamples)(void *context, s16 *sampleBuffer, size_t sampleCount, ESynchMode synchMode, ISynchronizingAudioBuffer *theSynchronizer);
   size_t (*PostProcessSamples)(void *context, s16 *postProcessBuffer, size_t requestedSampleCount, ESynchMode synchMode, ISynchronizingAudioBuffer *theSynchronizer);
};

extern SoundInterface_struct SNDDummy;

struct channel_struct
{
	channel_struct() :	num(0),
						vol(0),
						volumeDiv(0),
						hold(0),
						pan(0),
						waveduty(0),
						repeat(0),
						format(0),
						keyon(0),
						status(0),
						addr(0),
						timer(0),
						loopstart(0),
						length(0),
						totlength(0),
						double_totlength_shifted(0.0),
						sampcnt(0.0),
						sampinc(0.0),
						lastsampcnt(0),
						pcm16b(0),
						pcm16b_last(0),
						loop_pcm16b(0),
						index(0),
						loop_index(0),
						x(0),
						psgnoise_last(0)
	{}
	u32 num;
   u8 vol;
   u8 volumeDiv;
   u8 hold;
   u8 pan;
   u8 waveduty;
   u8 repeat;
   u8 format;
   u8 keyon;
   u8 status;
   u32 addr;
   u16 timer;
   u16 loopstart;
   u32 length;
   u32 totlength;
   double double_totlength_shifted;
   double sampcnt;
   double sampinc;
   // ADPCM specific
   u32 lastsampcnt;
   s16 pcm16b, pcm16b_last;
   s16 loop_pcm16b;
   s32 index;
   int loop_index;
   u16 x;
   s16 psgnoise_last;
};

class SPUFifo
{
public:
	SPUFifo();
	void enqueue(s16 val);
	s16 dequeue();
	s16 buffer[16];
	s32 head,tail,size;
	void reset();
};

class SPU_struct
{
public:
   SPU_struct(vio2sf_state *_st, int buffersize);
   u32 bufpos;
   u32 buflength;
   s32 *sndbuf;
   s32 lastdata; //the last sample that a channel generated
   s16 *outbuf;
   u32 bufsize;
   channel_struct channels[16];
   vio2sf_state *st;

   //registers
   struct REGS {
	   REGS()
			: mastervol(0)
			, ctl_left(0)
			, ctl_right(0)
			, ctl_ch1bypass(0)
			, ctl_ch3bypass(0)
			, masteren(0)
			, soundbias(0)
	   {}

	   u8 mastervol;
	   u8 ctl_left, ctl_right;
	   u8 ctl_ch1bypass, ctl_ch3bypass;
	   u8 masteren;
	   u16 soundbias;

	   enum LeftOutputMode
	   {
		   LOM_LEFT_MIXER=0, LOM_CH1=1, LOM_CH3=2, LOM_CH1_PLUS_CH3=3
	   };

	   enum RightOutputMode
	   {
		   ROM_RIGHT_MIXER=0, ROM_CH1=1, ROM_CH3=2, ROM_CH1_PLUS_CH3=3
	   };

	   struct CAP {
		   CAP()
			   : add(0), source(0), oneshot(0), bits8(0), active(0), dad(0), len(0)
		   {}
		   u8 add, source, oneshot, bits8, active;
		   u32 dad;
		   u16 len;
		   struct Runtime {
			   Runtime()
				   : running(0), curdad(0), maxdad(0)
			   {}
			   u8 running;
			   u32 curdad;
			   u32 maxdad;
			   double sampcnt;
			   SPUFifo fifo;
		   } runtime;
	   } cap[2];
   } regs;

   void reset();
   ~SPU_struct();
   void KeyOff(int channel);
   void KeyOn(int channel);
   void KeyProbe(int channel);
   void ProbeCapture(int which);
   void WriteByte(u32 addr, u8 val);
   void WriteWord(u32 addr, u16 val);
   void WriteLong(u32 addr, u32 val);
   u8 ReadByte(u32 addr);
   u16 ReadWord(u32 addr);
   u32 ReadLong(u32 addr);
   bool isSPU(u32 addr) { return ((addr >= 0x04000400) && (addr < 0x04000520)); }

   //kills all channels but leaves SPU otherwise running normally
   void ShutUp();
};

void SPU_ReInit(vio2sf_state *st, bool fakeBoot = false);
int SPU_Init(vio2sf_state *st, int coreid, int buffersize);
void SPU_Pause(vio2sf_state *st, int pause);
void SPU_SetVolume(vio2sf_state *st, int volume);
void SPU_SetSynchMode(vio2sf_state *st, int mode, int method);
void SPU_ClearOutputBuffer(vio2sf_state *st);
void SPU_Reset(vio2sf_state *st);
void SPU_DeInit(vio2sf_state *st);
void SPU_KeyOn(vio2sf_state *st, int channel);
void SPU_WriteByte(vio2sf_state *st, u32 addr, u8 val);
void SPU_WriteWord(vio2sf_state *st, u32 addr, u16 val);
void SPU_WriteLong(vio2sf_state *st, u32 addr, u32 val);
u8 SPU_ReadByte(vio2sf_state *st, u32 addr);
u16 SPU_ReadWord(vio2sf_state *st, u32 addr);
u32 SPU_ReadLong(vio2sf_state *st, u32 addr);
void SPU_Emulate_core(vio2sf_state *st);
void SPU_Emulate_user(vio2sf_state *st, bool mix = true);
void SPU_DefaultFetchSamples(s16 *sampleBuffer, size_t sampleCount, ESynchMode synchMode, ISynchronizingAudioBuffer *theSynchronizer);
size_t SPU_DefaultPostProcessSamples(s16 *postProcessBuffer, size_t requestedSampleCount, ESynchMode synchMode, ISynchronizingAudioBuffer *theSynchronizer);

void SetDesmumeSampleRate(vio2sf_state *st, double rate);

#endif
