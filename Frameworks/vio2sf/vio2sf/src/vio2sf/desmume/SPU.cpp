/*
   Copyright (C) 2006 yopyop
   Copyright (C) 2006 Theo Berkau
   Copyright (C) 2008-2017 DeSmuME team

   Ideas borrowed from Stephane Dallongeville's SCSP core

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

#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_PI
#define M_PI 3.1415926535897932386
#endif

#include <stdlib.h>
#include <string.h>
#include <queue>
#include <vector>

#include "vio2sf.h"
#include "../spu/interpolator.h"

static inline s16 read16(vio2sf_state *st, u32 addr) { return (s16)_MMU_read16<ARMCPU_ARM7,MMU_AT_DEBUG>(st, addr); }
static inline u8 read08(vio2sf_state *st, u32 addr) { return _MMU_read08<ARMCPU_ARM7,MMU_AT_DEBUG>(st, addr); }
static inline s8 read_s8(vio2sf_state *st, u32 addr) { return (s8)_MMU_read08<ARMCPU_ARM7,MMU_AT_DEBUG>(st, addr); }

#define K_ADPCM_LOOPING_RECOVERY_INDEX 99999
#define COSINE_INTERPOLATION_RESOLUTION 8192

static const int format_shift[] = { 2, 1, 3, 0 };
static const u8 volume_shift[] = { 0, 1, 2, 4 };

static const s8 indextbl[8] = { -1, -1, -1, -1, 2, 4, 6, 8 };

static const u16 adpcmtbl[89] = {
  0x0007, 0x0008, 0x0009, 0x000A, 0x000B, 0x000C, 0x000D, 0x000E, 0x0010,
  0x0011, 0x0013, 0x0015, 0x0017, 0x0019, 0x001C, 0x001F, 0x0022, 0x0025,
  0x0029, 0x002D, 0x0032, 0x0037, 0x003C, 0x0042, 0x0049, 0x0050, 0x0058,
  0x0061, 0x006B, 0x0076, 0x0082, 0x008F, 0x009D, 0x00AD, 0x00BE, 0x00D1,
  0x00E6, 0x00FD, 0x0117, 0x0133, 0x0151, 0x0173, 0x0198, 0x01C1, 0x01EE,
  0x0220, 0x0256, 0x0292, 0x02D4, 0x031C, 0x036C, 0x03C3, 0x0424, 0x048E,
  0x0502, 0x0583, 0x0610, 0x06AB, 0x0756, 0x0812, 0x08E0, 0x09C3, 0x0ABD,
  0x0BD0, 0x0CFF, 0x0E4C, 0x0FBA, 0x114C, 0x1307, 0x14EE, 0x1706, 0x1954,
  0x1BDC, 0x1EA5, 0x21B6, 0x2515, 0x28CA, 0x2CDF, 0x315B, 0x364B, 0x3BB9,
  0x41B2, 0x4844, 0x4F7E, 0x5771, 0x602F, 0x69CE, 0x7462, 0x7FFF
};

static const s16 wavedutytbl[8][8] = {
  { -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF },
  { -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF },
  { -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
  { -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
  { -0x7FFF, -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
  { -0x7FFF, -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
  { -0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF },
  { -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF, -0x7FFF }
};

static const double ARM7_CLOCK = 33513982;

void SetDesmumeSampleRate(vio2sf_state *st, double rate) {
  st->DESMUME_SAMPLE_RATE = rate;
  st->sampleLength = st->DESMUME_SAMPLE_RATE / 32728.498;
  st->samples_per_hline = (st->DESMUME_SAMPLE_RATE / 59.8261f) / 263.0f;
  if(st->SPU_core) {
    for (int i = 0; i < 16; i++) {
      channel_struct *chan = &st->SPU_core->channels[i];
    }
  }
}

template<typename T>
static FORCEINLINE T MinMax(T val, T min, T max)
{
  if (val < min)
    return min;
  else if (val > max)
    return max;

  return val;
}

//--------------external spu interface---------------

int SPU_ChangeSoundCore(vio2sf_state *st, int coreid, int buffersize)
{
  int i;

  st->buffersize = buffersize;

  // Make sure the old core is freed
  if (st->SNDCore)
    st->SNDCore->DeInit(st->SNDCoreContext);

  // So which core do we want?
  if (coreid == SNDCORE_DEFAULT)
    coreid = 0; // Assume we want the first one

  st->SPU_currentCoreNum = coreid;

  if (!st->SNDCoreList)
    return -1;

  // Go through core list and find the id
  for (i = 0; st->SNDCoreList[i] != NULL; i++)
  {
    if (st->SNDCoreList[i]->id == coreid)
    {
      // Set to current core
      st->SNDCore = st->SNDCoreList[i];
      break;
    }
  }

  st->SNDCoreId = coreid;

  //If the user picked the dummy core, disable the user spu
  if(st->SNDCore == &SNDDummy)
    return 0;

  //If the core wasnt found in the list for some reason, disable the user spu
  if (st->SNDCore == NULL)
    return -1;

  // Since it failed, instead of it being fatal, disable the user spu
  if (st->SNDCore->Init(st->SNDCoreContext, buffersize * 2) == -1)
  {
    st->SNDCore = 0;
    return -1;
  }

  st->SNDCore->SetVolume(st->SNDCoreContext, st->volume);

  SPU_SetSynchMode(st, st->synchmode, st->synchmethod);

  return 0;
}

SoundInterface_struct *SPU_SoundCore(vio2sf_state *st)
{
  return st->SNDCore;
}

void SPU_ReInit(vio2sf_state *st, bool fakeBoot)
{
  SPU_Init(st, st->SNDCoreId, st->buffersize);

  // Firmware set BIAS to 0x200
  if (fakeBoot)
    SPU_WriteWord(st, 0x04000504, 0x0200);
}

int SPU_Init(vio2sf_state *st, int coreid, int buffersize)
{
  st->SPU_core = new SPU_struct(st, (int)ceil(st->samples_per_hline));
  SPU_Reset(st);

  SPU_SetSynchMode(st, st->synchmode, st->synchmethod);

  return SPU_ChangeSoundCore(st, coreid, buffersize);
}

void SPU_Pause(vio2sf_state *st, int pause)
{
  if (st->SNDCore == NULL) return;

  if(pause)
    st->SNDCore->MuteAudio(st->SNDCoreContext);
  else
    st->SNDCore->UnMuteAudio(st->SNDCoreContext);
}

void SPU_SetSynchMode(vio2sf_state *st, int mode, int method)
{
  st->synchmode = (ESynchMode)mode;
  if(st->synchmethod != (ESynchMethod)method)
  {
    st->synchmethod = (ESynchMethod)method;
    delete st->synchronizer;
    st->synchronizer = metaspu_construct(st->synchmethod);
  }
}

void SPU_ClearOutputBuffer(vio2sf_state *st)
{
  if(st->SNDCore && st->SNDCore->ClearBuffer)
    st->SNDCore->ClearBuffer(st->SNDCoreContext);
}

void SPU_SetVolume(vio2sf_state *st, int volume)
{
  st->volume = volume;
  if (st->SNDCore)
    st->SNDCore->SetVolume(st->SNDCoreContext, st->volume);
}


void SPU_Reset(vio2sf_state *st)
{
  int i;

  st->SPU_core->reset();

  //zero - 09-apr-2010: this concerns me, regarding savestate synch.
  //After 0.9.6, lets experiment with removing it and just properly zapping the spu instead
  // Reset Registers
  for (i = 0x400; i < 0x51D; i++)
    T1WriteByte(st->MMU.ARM7_REG, i, 0);

  st->samples = 0;
}

//------------------------------------------

void SPU_struct::reset()
{
  memset(sndbuf,0,bufsize*2*4);
  memset(outbuf,0,bufsize*2*2);

  memset((void *)channels, 0, sizeof(channel_struct) * 16);

  reconstruct(&regs);

  for(int i = 0; i < 16; i++)
  {
    channels[i].num = i;
  }
}

SPU_struct::SPU_struct(vio2sf_state *_st, int buffersize)
  : st(_st)
  , bufpos(0)
  , buflength(0)
  , sndbuf(0)
  , outbuf(0)
    , bufsize(buffersize)
{
  sndbuf = new s32[buffersize*2];
  outbuf = new s16[buffersize*2];
  reset();
}

SPU_struct::~SPU_struct()
{
  if(sndbuf) delete[] sndbuf;
  if(outbuf) delete[] outbuf;
}

void SPU_DeInit(vio2sf_state *st)
{
  if(st->SNDCore)
    st->SNDCore->DeInit(st->SNDCoreContext);
  st->SNDCore = 0;

  delete st->SPU_core; st->SPU_core=0;
}

//////////////////////////////////////////////////////////////////////////////

void SPU_struct::ShutUp()
{
  for(int i=0;i<16;i++)
    channels[i].status = CHANSTAT_STOPPED;
}

static FORCEINLINE void adjust_channel_timer(vio2sf_state *st, channel_struct *chan)
{
  chan->sampinc = (((double)ARM7_CLOCK) / (st->DESMUME_SAMPLE_RATE * 2)) / (double)(0x10000 - chan->timer);
}

void SPU_struct::KeyProbe(int chan_num)
{
  channel_struct &thischan = channels[chan_num];
  if(thischan.status == CHANSTAT_STOPPED)
  {
    if(thischan.keyon && regs.masteren)
      KeyOn(chan_num);
  }
  else if(thischan.status == CHANSTAT_PLAY)
  {
    if(!thischan.keyon || !regs.masteren)
      KeyOff(chan_num);
  }
}

void SPU_struct::KeyOff(int channel)
{
  channel_struct &thischan = channels[channel];
  thischan.status = CHANSTAT_STOPPED;
}

void SPU_struct::KeyOn(int channel)
{
  channel_struct &thischan = channels[channel];
  thischan.status = CHANSTAT_PLAY;

  thischan.totlength = thischan.length + thischan.loopstart;
  adjust_channel_timer(st, &thischan);

  switch(thischan.format)
  {
    case 0: // 8-bit
      thischan.sampcnt = -3;
      break;
    case 1: // 16-bit
      thischan.sampcnt = -3;
      break;
    case 2: // ADPCM
      {
        thischan.pcm16b = (s16)read16(st, thischan.addr);
        thischan.pcm16b_last = thischan.pcm16b;
        thischan.index = read08(st, thischan.addr + 2) & 0x7F;;
        thischan.lastsampcnt = 7;
        thischan.sampcnt = -3;
        thischan.loop_index = K_ADPCM_LOOPING_RECOVERY_INDEX;
        break;
      }
    case 3: // PSG
      {
        thischan.sampcnt = -1;
        thischan.x = 0x7FFF;
        break;
      }
    default: break;
  }

  thischan.double_totlength_shifted = (double)(thischan.totlength << format_shift[thischan.format]);

  if(thischan.format != 3)
  {
    if(thischan.double_totlength_shifted == 0)
    {
      thischan.status = CHANSTAT_STOPPED;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////

u8 SPU_struct::ReadByte(u32 addr)
{
  //individual channel regs
  if ((addr & 0x0F00) == 0x0400)
  {
    u32 chan_num = (addr >> 4) & 0xF;
    const channel_struct& thischan = channels[chan_num];

    switch (addr & 0xF)
    {
      case 0x0: return thischan.vol;
      case 0x1: return (thischan.volumeDiv | (thischan.hold << 7));
      case 0x2: return thischan.pan;
      case 0x3: return (	thischan.waveduty
                    | (thischan.repeat << 3)
                    | (thischan.format << 5)
                    | ((thischan.status == CHANSTAT_PLAY)?0x80:0)
                    );
      case 0x8: return thischan.timer >> 0;
      case 0x9: return thischan.timer >> 8;
      case 0xA: return thischan.loopstart >> 0;
      case 0xB: return thischan.loopstart >> 8;
    }
    return 0;
  }

  switch(addr)
  {
    //SOUNDCNT
    case 0x500: return regs.mastervol;
    case 0x501: return (regs.ctl_left
                    | (regs.ctl_right << 2)
                    | (regs.ctl_ch1bypass << 4)
                    | (regs.ctl_ch3bypass << 5)
                    | (regs.masteren << 7)
                    );

                //SOUNDBIAS
    case 0x504: return regs.soundbias >> 0;
    case 0x505: return regs.soundbias >> 8;

                //SNDCAP0CNT/SNDCAP1CNT
    case 0x508:
    case 0x509:
                {
                  u32 which = (addr - 0x508);
                  return regs.cap[which].add
                    | (regs.cap[which].source << 1)
                    | (regs.cap[which].oneshot << 2)
                    | (regs.cap[which].bits8 << 3)
                    | (regs.cap[which].runtime.running << 7);
                }

                //SNDCAP0DAD
    case 0x510: return regs.cap[0].dad >> 0;
    case 0x511: return regs.cap[0].dad >> 8;
    case 0x512: return regs.cap[0].dad >> 16;
    case 0x513: return regs.cap[0].dad >> 24;

                //SNDCAP0LEN
    case 0x514: return regs.cap[0].len >> 0;
    case 0x515: return regs.cap[0].len >> 8;

                //SNDCAP1DAD
    case 0x518: return regs.cap[1].dad >> 0;
    case 0x519: return regs.cap[1].dad >> 8;
    case 0x51A: return regs.cap[1].dad >> 16;
    case 0x51B: return regs.cap[1].dad >> 24;

                //SNDCAP1LEN
    case 0x51C: return regs.cap[1].len >> 0;
    case 0x51D: return regs.cap[1].len >> 8;
  } //switch on address

  return 0;
}

u16 SPU_struct::ReadWord(u32 addr)
{
  //individual channel regs
  if ((addr & 0x0F00) == 0x0400)
  {
    u32 chan_num = (addr >> 4) & 0xF;
    const channel_struct& thischan = channels[chan_num];

    switch (addr & 0xF)
    {
      case 0x0: return	(thischan.vol
                    | (thischan.volumeDiv << 8)
                    | (thischan.hold << 15)
                    );
      case 0x2: return	(thischan.pan
                    | (thischan.waveduty << 8)
                    | (thischan.repeat << 11)
                    | (thischan.format << 13)
                    | ((thischan.status == CHANSTAT_PLAY)?(1 << 15):0)
                    );
      case 0x8: return thischan.timer;
      case 0xA: return thischan.loopstart;
    } //switch on individual channel regs
    return 0;
  }

  switch(addr)
  {
    //SOUNDCNT
    case 0x500: return	(regs.mastervol
                    | (regs.ctl_left << 8)
                    | (regs.ctl_right << 10)
                    | (regs.ctl_ch1bypass << 12)
                    | (regs.ctl_ch3bypass << 13)
                    | (regs.masteren << 15)
                    );

                //SOUNDBIAS
    case 0x504: return regs.soundbias;

                //SNDCAP0CNT/SNDCAP1CNT
    case 0x508:
                {
                  u8 val0 =	regs.cap[0].add
                    | (regs.cap[0].source << 1)
                    | (regs.cap[0].oneshot << 2)
                    | (regs.cap[0].bits8 << 3)
                    | (regs.cap[0].runtime.running << 7);
                  u8 val1 =	regs.cap[1].add
                    | (regs.cap[1].source << 1)
                    | (regs.cap[1].oneshot << 2)
                    | (regs.cap[1].bits8 << 3)
                    | (regs.cap[1].runtime.running << 7);
                  return (u16)(val0 | (val1 << 8));
                }

                //SNDCAP0DAD
    case 0x510: return regs.cap[0].dad >> 0;
    case 0x512: return regs.cap[0].dad >> 16;

                //SNDCAP0LEN
    case 0x514: return regs.cap[0].len;

                //SNDCAP1DAD
    case 0x518: return regs.cap[1].dad >> 0;
    case 0x51A: return regs.cap[1].dad >> 16;

                //SNDCAP1LEN
    case 0x51C: return regs.cap[1].len;
  } //switch on address

  return 0;
}

u32 SPU_struct::ReadLong(u32 addr)
{
  //individual channel regs
  if ((addr & 0x0F00) == 0x0400)
  {
    u32 chan_num = (addr >> 4) & 0xF;
    channel_struct &thischan=channels[chan_num];

    switch (addr & 0xF)
    {
      case 0x0: return	(thischan.vol
                    | (thischan.volumeDiv << 8)
                    | (thischan.hold << 15)
                    | (thischan.pan << 16)
                    | (thischan.waveduty << 24)
                    | (thischan.repeat << 27)
                    | (thischan.format << 29)
                    | ((thischan.status == CHANSTAT_PLAY)?(1 << 31):0)
                    );
      case 0x8: return (thischan.timer | (thischan.loopstart << 16));
    } //switch on individual channel regs
    return 0;
  }

  switch(addr)
  {
    //SOUNDCNT
    case 0x500: return	(regs.mastervol
                    | (regs.ctl_left << 8)
                    | (regs.ctl_right << 10)
                    | (regs.ctl_ch1bypass << 12)
                    | (regs.ctl_ch3bypass << 13)
                    | (regs.masteren << 15)
                    );

                //SOUNDBIAS
    case 0x504: return (u32)regs.soundbias;

                //SNDCAP0CNT/SNDCAP1CNT
    case 0x508:
                {
                  u8 val0 =	regs.cap[0].add
                    | (regs.cap[0].source << 1)
                    | (regs.cap[0].oneshot << 2)
                    | (regs.cap[0].bits8 << 3)
                    | (regs.cap[0].runtime.running << 7);
                  u8 val1 =	regs.cap[1].add
                    | (regs.cap[1].source << 1)
                    | (regs.cap[1].oneshot << 2)
                    | (regs.cap[1].bits8 << 3)
                    | (regs.cap[1].runtime.running << 7);
                  return (u32)(val0 | (val1 << 8));
                }

                //SNDCAP0DAD
    case 0x510: return regs.cap[0].dad;

                //SNDCAP0LEN
    case 0x514: return (u32)regs.cap[0].len;

                //SNDCAP1DAD
    case 0x518: return regs.cap[1].dad;

                //SNDCAP1LEN
    case 0x51C: return (u32)regs.cap[1].len;
  } //switch on address

  return 0;
}

SPUFifo::SPUFifo()
{
  reset();
}

void SPUFifo::reset()
{
  head = tail = size = 0;
}

void SPUFifo::enqueue(s16 val)
{
  if(size==16) return;
  buffer[tail] = val;
  tail++;
  tail &= 15;
  size++;
}

s16 SPUFifo::dequeue()
{
  if(size==0) return 0;
  head++;
  head &= 15;
  s16 ret = buffer[head];
  size--;
  return ret;
}

void SPU_struct::ProbeCapture(int which)
{
  //VERY UNTESTED -- HOW MUCH OF THIS RESETS, AND WHEN?

  if(!regs.cap[which].active)
  {
    regs.cap[which].runtime.running = 0;
    return;
  }

  REGS::CAP &cap = regs.cap[which];
  cap.runtime.running = 1;
  cap.runtime.curdad = cap.dad;
  u32 len = cap.len;
  if(len==0) len=1;
  cap.runtime.maxdad = cap.dad + len*4;
  cap.runtime.sampcnt = 0;
  cap.runtime.fifo.reset();
}

void SPU_struct::WriteByte(u32 addr, u8 val)
{
  //individual channel regs
  if ((addr & 0x0F00) == 0x0400)
  {
    u8 chan_num = (addr >> 4) & 0xF;
    channel_struct &thischan = channels[chan_num];

    switch (addr & 0x000F)
    {
      case 0x0: thischan.vol = (val & 0x7F); break;
      case 0x1:
                thischan.volumeDiv = (val & 0x03);
                thischan.hold = (val >> 7) & 0x01;
                break;
      case 0x2: thischan.pan = (val & 0x7F); break;
      case 0x3:
                thischan.waveduty = (val & 0x07);
                thischan.repeat = (val >> 3) & 0x03;
                thischan.format = (val >> 5) & 0x03;
                thischan.keyon = (val >> 7) & 0x01;
                KeyProbe(chan_num);
                break;
      case 0x4: thischan.addr &= 0xFFFFFF00; thischan.addr |= (val & 0xFC); break;
      case 0x5: thischan.addr &= 0xFFFF00FF; thischan.addr |= (val << 8); break;
      case 0x6: thischan.addr &= 0xFF00FFFF; thischan.addr |= (val << 16); break;
      case 0x7: thischan.addr &= 0x00FFFFFF; thischan.addr |= ((val&7) << 24); break; //only 27 bits of this register are used
      case 0x8: thischan.timer &= 0xFF00; thischan.timer |= (val << 0); adjust_channel_timer(st, &thischan); break;
      case 0x9: thischan.timer &= 0x00FF; thischan.timer |= (val << 8); adjust_channel_timer(st, &thischan); break;

      case 0xA: thischan.loopstart &= 0xFF00; thischan.loopstart |= (val << 0); break;
      case 0xB: thischan.loopstart &= 0x00FF; thischan.loopstart |= (val << 8); break;
      case 0xC: thischan.length &= 0xFFFFFF00; thischan.length |= (val << 0); break;
      case 0xD: thischan.length &= 0xFFFF00FF; thischan.length |= (val << 8); break;
      case 0xE: thischan.length &= 0xFF00FFFF; thischan.length |= ((val & 0x3F) << 16); //only 22 bits of this register are used
      case 0xF: break;

    } //switch on individual channel regs

    return;
  }

  switch(addr)
  {
    //SOUNDCNT
    case 0x500: regs.mastervol = (val & 0x7F); break;
    case 0x501:
                regs.ctl_left  = (val >> 0) & 3;
                regs.ctl_right = (val >> 2) & 3;
                regs.ctl_ch1bypass = (val >> 4) & 1;
                regs.ctl_ch3bypass = (val >> 5) & 1;
                regs.masteren = (val >> 7) & 1;
                break;

                //SOUNDBIAS
    case 0x504: regs.soundbias &= 0xFF00; regs.soundbias |= (val << 0); break;
    case 0x505: regs.soundbias &= 0x00FF; regs.soundbias |= ((val&3) << 8); break;

                //SNDCAP0CNT/SNDCAP1CNT
    case 0x508:
    case 0x509:
                {
                  u32 which = (addr - 0x508);
                  regs.cap[which].add = BIT0(val);
                  regs.cap[which].source = BIT1(val);
                  regs.cap[which].oneshot = BIT2(val);
                  regs.cap[which].bits8 = BIT3(val);
                  regs.cap[which].active = BIT7(val);
                  ProbeCapture(which);
                  break;
                }

                //SNDCAP0DAD
    case 0x510: regs.cap[0].dad &= 0xFFFFFF00; regs.cap[0].dad |= (val & 0xFC); break;
    case 0x511: regs.cap[0].dad &= 0xFFFF00FF; regs.cap[0].dad |= (val << 8); break;
    case 0x512: regs.cap[0].dad &= 0xFF00FFFF; regs.cap[0].dad |= (val << 16); break;
    case 0x513: regs.cap[0].dad &= 0x00FFFFFF; regs.cap[0].dad |= ((val&7) << 24); break;

                //SNDCAP0LEN
    case 0x514: regs.cap[0].len &= 0xFF00; regs.cap[0].len |= (val << 0); break;
    case 0x515: regs.cap[0].len &= 0x00FF; regs.cap[0].len |= (val << 8); break;

                //SNDCAP1DAD
    case 0x518: regs.cap[1].dad &= 0xFFFFFF00; regs.cap[1].dad |= (val & 0xFC); break;
    case 0x519: regs.cap[1].dad &= 0xFFFF00FF; regs.cap[1].dad |= (val << 8); break;
    case 0x51A: regs.cap[1].dad &= 0xFF00FFFF; regs.cap[1].dad |= (val << 16); break;
    case 0x51B: regs.cap[1].dad &= 0xFF000000; regs.cap[1].dad |= ((val&7) << 24); break;

                //SNDCAP1LEN
    case 0x51C: regs.cap[1].len &= 0xFF00; regs.cap[1].len |= (val << 0); break;
    case 0x51D: regs.cap[1].len &= 0x00FF; regs.cap[1].len |= (val << 8); break;
  } //switch on address
}

void SPU_struct::WriteWord(u32 addr, u16 val)
{
  //individual channel regs
  if ((addr & 0x0F00) == 0x0400)
  {
    u32 chan_num = (addr >> 4) & 0xF;
    channel_struct &thischan=channels[chan_num];

    switch (addr & 0xF)
    {
      case 0x0:
        thischan.vol = (val & 0x7F);
        thischan.volumeDiv = (val >> 8) & 0x3;
        thischan.hold = (val >> 15) & 0x1;
        break;
      case 0x2:
        thischan.pan = (val & 0x7F);
        thischan.waveduty = (val >> 8) & 0x7;
        thischan.repeat = (val >> 11) & 0x3;
        thischan.format = (val >> 13) & 0x3;
        thischan.keyon = (val >> 15) & 0x1;
        KeyProbe(chan_num);
        break;
      case 0x4: thischan.addr &= 0xFFFF0000; thischan.addr |= (val & 0xFFFC); break;
      case 0x6: thischan.addr &= 0x0000FFFF; thischan.addr |= ((val & 0x07FF) << 16); break;
      case 0x8: thischan.timer = val; adjust_channel_timer(st, &thischan); break;
      case 0xA: thischan.loopstart = val; break;
      case 0xC: thischan.length &= 0xFFFF0000; thischan.length |= (val << 0); break;
      case 0xE: thischan.length &= 0x0000FFFF; thischan.length |= ((val & 0x003F) << 16); break;
    } //switch on individual channel regs
    return;
  }

  switch (addr)
  {
    //SOUNDCNT
    case 0x500:
      regs.mastervol = (val & 0x7F);
      regs.ctl_left  = (val >> 8) & 0x03;
      regs.ctl_right = (val >> 10) & 0x03;
      regs.ctl_ch1bypass = (val >> 12) & 0x01;
      regs.ctl_ch3bypass = (val >> 13) & 0x01;
      regs.masteren = (val >> 15) & 0x01;
      for(u8 i=0; i<16; i++)
        KeyProbe(i);
      break;

      //SOUNDBIAS
    case 0x504: regs.soundbias = (val & 0x3FF); break;

                //SNDCAP0CNT/SNDCAP1CNT
    case 0x508:
                {
                  regs.cap[0].add = BIT0(val);
                  regs.cap[0].source = BIT1(val);
                  regs.cap[0].oneshot = BIT2(val);
                  regs.cap[0].bits8 = BIT3(val);
                  regs.cap[0].active = BIT7(val);
                  ProbeCapture(0);

                  regs.cap[1].add = BIT8(val);
                  regs.cap[1].source = BIT9(val);
                  regs.cap[1].oneshot = BIT10(val);
                  regs.cap[1].bits8 = BIT11(val);
                  regs.cap[1].active = BIT15(val);
                  ProbeCapture(1);
                  break;
                }

                //SNDCAP0DAD
    case 0x510: regs.cap[0].dad &= 0xFFFF0000; regs.cap[0].dad |= (val & 0xFFFC); break;
    case 0x512: regs.cap[0].dad &= 0x0000FFFF; regs.cap[0].dad |= ((val & 0x07FF) << 16); break;

                //SNDCAP0LEN
    case 0x514: regs.cap[0].len = val; break;

                //SNDCAP1DAD
    case 0x518: regs.cap[1].dad &= 0xFFFF0000; regs.cap[1].dad |= (val & 0xFFFC); break;
    case 0x51A: regs.cap[1].dad &= 0x0000FFFF; regs.cap[1].dad |= ((val & 0x07FF) << 16); break;

                //SNDCAP1LEN
    case 0x51C: regs.cap[1].len = val; break;
  } //switch on address
}

void SPU_struct::WriteLong(u32 addr, u32 val)
{
  //individual channel regs
  if ((addr & 0x0F00) == 0x0400)
  {
    u32 chan_num = (addr >> 4) & 0xF;
    channel_struct &thischan=channels[chan_num];

    switch (addr & 0xF)
    {
      case 0x0:
        thischan.vol = val & 0x7F;
        thischan.volumeDiv = (val >> 8) & 0x3;
        thischan.hold = (val >> 15) & 0x1;
        thischan.pan = (val >> 16) & 0x7F;
        thischan.waveduty = (val >> 24) & 0x7;
        thischan.repeat = (val >> 27) & 0x3;
        thischan.format = (val >> 29) & 0x3;
        thischan.keyon = (val >> 31) & 0x1;
        KeyProbe(chan_num);
        break;

      case 0x4: thischan.addr = (val & 0x07FFFFFC); break;
      case 0x8:
                thischan.timer = (val & 0xFFFF);
                thischan.loopstart = ((val >> 16) & 0xFFFF);
                adjust_channel_timer(st, &thischan);
                break;

      case 0xC: thischan.length = (val & 0x003FFFFF); break; //only 22 bits of this register are used
    } //switch on individual channel regs
    return;
  }

  switch(addr)
  {
    //SOUNDCNT
    case 0x500:
      regs.mastervol = (val & 0x7F);
      regs.ctl_left  = ((val >> 8) & 3);
      regs.ctl_right = ((val>>10) & 3);
      regs.ctl_ch1bypass = ((val >> 12) & 1);
      regs.ctl_ch3bypass = ((val >> 13) & 1);
      regs.masteren = ((val >> 15) & 1);
      for(u8 i=0; i<16; i++)
        KeyProbe(i);
      break;

      //SOUNDBIAS
    case 0x504: regs.soundbias = (val & 0x3FF);

                //SNDCAP0CNT/SNDCAP1CNT
    case 0x508:
                regs.cap[0].add = BIT0(val);
                regs.cap[0].source = BIT1(val);
                regs.cap[0].oneshot = BIT2(val);
                regs.cap[0].bits8 = BIT3(val);
                regs.cap[0].active = BIT7(val);
                ProbeCapture(0);

                regs.cap[1].add = BIT8(val);
                regs.cap[1].source = BIT9(val);
                regs.cap[1].oneshot = BIT10(val);
                regs.cap[1].bits8 = BIT11(val);
                regs.cap[1].active = BIT15(val);
                ProbeCapture(1);
                break;

                //SNDCAP0DAD
    case 0x510: regs.cap[0].dad = (val & 0x07FFFFFC); break;

                //SNDCAP0LEN
    case 0x514: regs.cap[0].len = (val & 0xFFFF); break;

                //SNDCAP1DAD
    case 0x518: regs.cap[1].dad = (val & 0x07FFFFFC); break;

                //SNDCAP1LEN
    case 0x51C: regs.cap[1].len = (val & 0xFFFF); break;
  } //switch on address
}

//////////////////////////////////////////////////////////////////////////////

static FORCEINLINE void FetchPSGData(channel_struct *chan, s32 *data)
{
  if (chan->sampcnt < 0)
  {
    *data = 0;
    return;
  }

  if(chan->num < 8)
  {
    *data = 0;
  }
  else if(chan->num < 14)
  {
    *data = (s32)wavedutytbl[chan->waveduty][(sputrunc(chan->sampcnt)) & 0x7];
  }
  else
  {
    if(chan->lastsampcnt == sputrunc(chan->sampcnt))
    {
      *data = (s32)chan->psgnoise_last;
      return;
    }

    u32 max = sputrunc(chan->sampcnt);
    for(u32 i = chan->lastsampcnt; i < max; i++)
    {
      if(chan->x & 0x1)
      {
        chan->x = (chan->x >> 1) ^ 0x6000;
        chan->psgnoise_last = -0x7FFF;
      }
      else
      {
        chan->x >>= 1;
        chan->psgnoise_last = 0x7FFF;
      }
    }

    chan->lastsampcnt = sputrunc(chan->sampcnt);

    *data = (s32)chan->psgnoise_last;
  }
}

//////////////////////////////////////////////////////////////////////////////

static FORCEINLINE void MixL(SPU_struct* SPU, channel_struct *chan, s32 data)
{
  data = spumuldiv7(data, chan->vol) >> volume_shift[chan->volumeDiv];
  SPU->sndbuf[SPU->bufpos<<1] += data;
}

static FORCEINLINE void MixR(SPU_struct* SPU, channel_struct *chan, s32 data)
{
  data = spumuldiv7(data, chan->vol) >> volume_shift[chan->volumeDiv];
  SPU->sndbuf[(SPU->bufpos<<1)+1] += data;
}

static FORCEINLINE void MixLR(SPU_struct* SPU, channel_struct *chan, s32 data)
{
  data = spumuldiv7(data, chan->vol) >> volume_shift[chan->volumeDiv];
  SPU->sndbuf[SPU->bufpos<<1] += spumuldiv7(data, 127 - chan->pan);
  SPU->sndbuf[(SPU->bufpos<<1)+1] += spumuldiv7(data, chan->pan);
}

//////////////////////////////////////////////////////////////////////////////

template<int FORMAT> static FORCEINLINE void TestForLoop(SPU_struct *SPU, channel_struct *chan)
{
  const int shift = (FORMAT == 0 ? 2 : 1);

  chan->sampcnt += chan->sampinc;

  if (chan->sampcnt > chan->double_totlength_shifted)
  {
    // Do we loop? Or are we done?
    if (chan->repeat == 1)
    {
      while (chan->sampcnt > chan->double_totlength_shifted) {
        chan->sampcnt -= chan->double_totlength_shifted - (double)(chan->loopstart << shift);
      }
    }
    else
    {
      SPU->KeyOff(chan->num);
      SPU->bufpos = SPU->buflength;
    }
  }
}

static FORCEINLINE void TestForLoop2(SPU_struct *SPU, channel_struct *chan)
{
  // Minimum length (the sum of PNT+LEN) is 4 words (16 bytes),
  // smaller values (0..3 words) are causing hang-ups
  // (busy bit remains set infinite, but no sound output occurs).
  // fix: 7th Dragon (JP) - http://sourceforge.net/p/desmume/bugs/1357/
  if (chan->totlength < 4) return;

  chan->sampcnt += chan->sampinc;

  if (chan->sampcnt > chan->double_totlength_shifted)
  {
    // Do we loop? Or are we done?
    if (chan->repeat == 1)
    {
      double step = (chan->double_totlength_shifted - (double)(chan->loopstart << 3));

      while (chan->sampcnt > chan->double_totlength_shifted) chan->sampcnt -= step;

      if(chan->loop_index == K_ADPCM_LOOPING_RECOVERY_INDEX)
      {
        chan->pcm16b = (s16)read16(SPU->st, chan->addr);
        chan->index = read08(SPU->st, chan->addr+2) & 0x7F;
        chan->lastsampcnt = 7;
      }
      else
      {
        chan->pcm16b = chan->loop_pcm16b;
        chan->index = chan->loop_index;
        chan->lastsampcnt = (chan->loopstart << 3);
      }
    }
    else
    {
      chan->status = CHANSTAT_STOPPED;
      SPU->KeyOff(chan->num);
      SPU->bufpos = SPU->buflength;
    }
  }
}

template<int CHANNELS> FORCEINLINE static void SPU_Mix(SPU_struct* SPU, channel_struct *chan, s32 data)
{
  switch(CHANNELS)
  {
    case -1: break;
    case 0: MixL(SPU, chan, data); break;
    case 1: MixLR(SPU, chan, data); break;
    case 2: MixR(SPU, chan, data); break;
  }
  SPU->lastdata = data;
}

//WORK
  template<int FORMAT, int CHANNELS>
FORCEINLINE static void ____SPU_ChanUpdate(SPU_struct* const SPU, channel_struct* const chan)
{
  vio2sf_state *st = SPU->st;
  for (; SPU->bufpos < SPU->buflength; SPU->bufpos++)
  {
    if(CHANNELS != -1)
    {
      s32 data;
      if (chan->sampcnt < 0) {
        data = 0;
      } else if (FORMAT == 3) {
        FetchPSGData(chan, &data);
      } else {
        const SampleData& sample = st->sampleCache.getSample(SPU->st, chan->addr, chan->loopstart, chan->length, SampleData::Format(FORMAT));
        data = sample.sampleAt(chan->sampcnt, IInterpolator::allInterpolators[st->CommonSettings.spuInterpolationMode]);
      }
      SPU_Mix<CHANNELS>(SPU, chan, data);
    }

    switch(FORMAT) {
      case 0: case 1: TestForLoop<FORMAT>(SPU, chan); break;
      case 2: TestForLoop2(SPU, chan); break;
      case 3: chan->sampcnt += chan->sampinc; break;
    }
  }
}

template<int FORMAT>
FORCEINLINE static void ___SPU_ChanUpdate(const bool actuallyMix, SPU_struct* const SPU, channel_struct* const chan)
{
  if(!actuallyMix)
    ____SPU_ChanUpdate<FORMAT,-1>(SPU,chan);
  else if (chan->pan == 0)
    ____SPU_ChanUpdate<FORMAT,0>(SPU,chan);
  else if (chan->pan == 127)
    ____SPU_ChanUpdate<FORMAT,2>(SPU,chan);
  else
    ____SPU_ChanUpdate<FORMAT,1>(SPU,chan);
}

FORCEINLINE static void _SPU_ChanUpdate(const bool actuallyMix, SPU_struct* const SPU, channel_struct* const chan)
{
  switch(chan->format)
  {
    case 0: ___SPU_ChanUpdate<0>(actuallyMix, SPU, chan); break;
    case 1: ___SPU_ChanUpdate<1>(actuallyMix, SPU, chan); break;
    case 2: ___SPU_ChanUpdate<2>(actuallyMix, SPU, chan); break;
    case 3: ___SPU_ChanUpdate<3>(actuallyMix, SPU, chan); break;
    default: assert(false);
  }
}

//ENTERNEW
static void SPU_MixAudio_Advanced(bool actuallyMix, SPU_struct *SPU, int length)
{
  //the advanced spu function correctly handles all sound control mixing options, as well as capture
  //this code is not entirely optimal, as it relies on sort of manhandling the core mixing functions
  //in order to get the results it needs.

  //THIS IS MAX HACKS!!!!
  //AND NEEDS TO BE REWRITTEN ALONG WITH THE DEEPEST PARTS OF THE SPU
  //ONCE WE KNOW THAT IT WORKS

  //BIAS gets ignored since our spu is still not bit perfect,
  //and it doesnt matter for purposes of capture

  //-----------DEBUG CODE
  bool skipcap = false;
  //-----------------

  vio2sf_state *st = SPU->st;

  s32 samp0[2] = {0,0};

  //believe it or not, we are going to do this one sample at a time.
  //like i said, it is slower.
  for (int samp = 0; samp < length; samp++)
  {
    SPU->sndbuf[0] = 0;
    SPU->sndbuf[1] = 0;
    SPU->buflength = 1;

    s32 capmix[2] = {0,0};
    s32 mix[2] = {0,0};
    s32 chanout[16];
    s32 submix[32];
    static int tsamp = 0;
    ++tsamp;

    //generate each channel, and helpfully mix it at the same time
    for (int i = 0; i < 16; i++)
    {
      channel_struct *chan = &SPU->channels[i];

      if (chan->status == CHANSTAT_PLAY)
      {
        SPU->bufpos = 0;

        bool bypass = false;
        if (i==1 && SPU->regs.ctl_ch1bypass) bypass=true;
        if (i==3 && SPU->regs.ctl_ch3bypass) bypass=true;


        //output to mixer unless we are bypassed.
        //dont output to mixer if the user muted us
        bool outputToMix = true;
        if (st->CommonSettings.spu_muteChannels[i]) outputToMix = false;
        if (bypass) outputToMix = false;
        bool outputToCap = outputToMix;
        if (st->CommonSettings.spu_captureMuted && !bypass) outputToCap = true;

        //channels 1 and 3 should probably always generate their audio
        //internally at least, just in case they get used by the spu output
        bool domix = outputToCap || outputToMix || i==1 || i==3;

        //clear the output buffer since this is where _SPU_ChanUpdate wants to accumulate things
        SPU->sndbuf[0] = SPU->sndbuf[1] = 0;

        //get channel's next output sample.
        _SPU_ChanUpdate(domix, SPU, chan);
        chanout[i] = SPU->lastdata >> volume_shift[chan->volumeDiv];

        //save the panned results
        submix[i*2] = SPU->sndbuf[0];
        submix[i*2+1] = SPU->sndbuf[1];

        //send sample to our capture mix
        if (outputToCap)
        {
          capmix[0] += submix[i*2];
          capmix[1] += submix[i*2+1];
        }

        //send sample to our main mixer
        if (outputToMix)
        {
          mix[0] += submix[i*2];
          mix[1] += submix[i*2+1];
        }
      }
      else
      {
        chanout[i] = 0;
        submix[i*2] = 0;
        submix[i*2+1] = 0;
      }
    } //foreach channel

    s32 mixout[2] = {mix[0],mix[1]};
    s32 capmixout[2] = {capmix[0],capmix[1]};
    s32 sndout[2];
    s32 capout[2];

    //create SPU output
    switch (SPU->regs.ctl_left)
    {
      case SPU_struct::REGS::LOM_LEFT_MIXER: sndout[0] = mixout[0]; break;
      case SPU_struct::REGS::LOM_CH1: sndout[0] = submix[1*2+0]; break;
      case SPU_struct::REGS::LOM_CH3: sndout[0] = submix[3*2+0]; break;
      case SPU_struct::REGS::LOM_CH1_PLUS_CH3: sndout[0] = submix[1*2+0] + submix[3*2+0]; break;
    }
    switch (SPU->regs.ctl_right)
    {
      case SPU_struct::REGS::ROM_RIGHT_MIXER: sndout[1] = mixout[1]; break;
      case SPU_struct::REGS::ROM_CH1: sndout[1] = submix[1*2+1]; break;
      case SPU_struct::REGS::ROM_CH3: sndout[1] = submix[3*2+1]; break;
      case SPU_struct::REGS::ROM_CH1_PLUS_CH3: sndout[1] = submix[1*2+1] + submix[3*2+1]; break;
    }


    //generate capture output ("capture bugs" from gbatek are not emulated)
    if (SPU->regs.cap[0].source == 0)
      capout[0] = capmixout[0]; //cap0 = L-mix
    else if (SPU->regs.cap[0].add)
      capout[0] = chanout[0] + chanout[1]; //cap0 = ch0+ch1
    else capout[0] = chanout[0]; //cap0 = ch0

    if (SPU->regs.cap[1].source == 0)
      capout[1] = capmixout[1]; //cap1 = R-mix
    else if (SPU->regs.cap[1].add)
      capout[1] = chanout[2] + chanout[3]; //cap1 = ch2+ch3
    else capout[1] = chanout[2]; //cap1 = ch2

    capout[0] = MinMax(capout[0],-0x8000,0x7FFF);
    capout[1] = MinMax(capout[1],-0x8000,0x7FFF);

    //write the output sample where it is supposed to go
    if (samp == 0)
    {
      samp0[0] = sndout[0];
      samp0[1] = sndout[1];
    }
    else
    {
      SPU->sndbuf[samp*2+0] = sndout[0];
      SPU->sndbuf[samp*2+1] = sndout[1];
    }

    for (int capchan = 0; capchan < 2; capchan++)
    {
      if (SPU->regs.cap[capchan].runtime.running)
      {
        SPU_struct::REGS::CAP& cap = SPU->regs.cap[capchan];
        u32 last = sputrunc(cap.runtime.sampcnt);
        cap.runtime.sampcnt += SPU->channels[1+2*capchan].sampinc;
        u32 curr = sputrunc(cap.runtime.sampcnt);
        for (u32 j = last; j < curr; j++)
        {
          //so, this is a little strange. why go through a fifo?
          //it seems that some games will set up a reverb effect by capturing
          //to the nearly same address as playback, but ahead by a couple.
          //So, playback will always end up being what was captured a couple of samples ago.
          //This system counts on playback always having read ahead 16 samples.
          //In that case, playback will end up being what was processed at one entire buffer length ago,
          //since the 16 samples would have read ahead before they got captured over

          //It's actually the source channels which should have a fifo, but we are
          //not going to take the hit in speed and complexity. Save it for a future rewrite.
          //Instead, what we do here is delay the capture by 16 samples to create a similar effect.
          //Subjectively, it seems to be working.

          //Don't do anything until the fifo is filled, so as to delay it
          if (cap.runtime.fifo.size < 16)
          {
            cap.runtime.fifo.enqueue(capout[capchan]);
            continue;
          }

          //(actually capture sample from fifo instead of most recently generated)
          u32 multiplier;
          s32 sample = cap.runtime.fifo.dequeue();
          cap.runtime.fifo.enqueue(capout[capchan]);

          if (cap.bits8)
          {
            s8 sample8 = sample >> 8;
            if (skipcap) _MMU_write08<1,MMU_AT_DMA>(st, cap.runtime.curdad,0);
            else _MMU_write08<1,MMU_AT_DMA>(st, cap.runtime.curdad,sample8);
            cap.runtime.curdad++;
            multiplier = 4;
          }
          else
          {
            s16 sample16 = sample;
            if (skipcap) _MMU_write16<1,MMU_AT_DMA>(st, cap.runtime.curdad,0);
            else _MMU_write16<1,MMU_AT_DMA>(st, cap.runtime.curdad,sample16);
            cap.runtime.curdad+=2;
            multiplier = 2;
          }

          if (cap.runtime.curdad >= cap.runtime.maxdad)
          {
            cap.runtime.curdad = cap.dad;
            cap.runtime.sampcnt -= cap.len*multiplier;
          }
        } //sampinc loop
      } //if capchan running
    } //capchan loop
  } //main sample loop

  SPU->sndbuf[0] = samp0[0];
  SPU->sndbuf[1] = samp0[1];
}

//ENTER
static void SPU_MixAudio(bool actuallyMix, SPU_struct *SPU, int length)
{
  if (actuallyMix)
  {
    memset(SPU->sndbuf, 0, length*4*2);
    memset(SPU->outbuf, 0, length*2*2);
  }

  SPU_MixAudio_Advanced(actuallyMix, SPU, length);

  //we used to bail out if speakers were disabled.
  //this is technically wrong. sound may still be captured, or something.
  //in all likelihood, any game doing this probably master disabled the SPU also
  //so, optimization of this case is probably not necessary.
  //later, we'll just silence the output
  bool speakers = T1ReadWord(SPU->st->MMU.ARM7_REG, 0x304) & 0x01;

  u8 vol = SPU->regs.mastervol;

  // convert from 32-bit->16-bit
  if (actuallyMix && speakers) {
    for (int i = 0; i < length*2; i++)
    {
      // Apply Master Volume
      SPU->sndbuf[i] = spumuldiv7(SPU->sndbuf[i], vol);
      s16 outsample = MinMax(SPU->sndbuf[i],-0x8000,0x7FFF);
      SPU->outbuf[i] = outsample;
    }
  }
}

//////////////////////////////////////////////////////////////////////////////


//emulates one hline of the cpu core.
//this will produce a variable number of samples, calculated to keep a 44100hz output
//in sync with the emulator framerate
void SPU_Emulate_core(vio2sf_state *st)
{
  bool needToMix = true;
  SoundInterface_struct *soundProcessor = SPU_SoundCore(st);
  void *context = st->SNDCoreContext;

  st->samples += st->samples_per_hline;
  st->spu_core_samples = (int)(st->samples);
  st->samples -= st->spu_core_samples;

  SPU_MixAudio(needToMix, st->SPU_core, st->spu_core_samples);

  if (soundProcessor == NULL)
  {
    return;
  }

  if (soundProcessor->FetchSamples != NULL)
  {
    soundProcessor->FetchSamples(context, st->SPU_core->outbuf, st->spu_core_samples, st->synchmode, st->synchronizer);
  }
  else
  {
    SPU_DefaultFetchSamples(st->SPU_core->outbuf, st->spu_core_samples, st->synchmode, st->synchronizer);
  }
}

void SPU_Emulate_user(vio2sf_state *st, bool mix)
{
  size_t freeSampleCount = 0;
  size_t processedSampleCount = 0;
  SoundInterface_struct *soundProcessor = SPU_SoundCore(st);
  void *context = st->SNDCoreContext;

  if (soundProcessor == NULL)
  {
    return;
  }

  // Check to see how many free samples are available.
  // If there are some, fill up the output buffer.
  freeSampleCount = soundProcessor->GetAudioSpace(context);
  if (freeSampleCount == 0)
  {
    return;
  }

  if (freeSampleCount > st->buffersize)
  {
    freeSampleCount = st->buffersize;
  }

  // If needed, resize the post-process buffer to guarantee that
  // we can store all the sound data.
  if (st->postProcessBufferSize < freeSampleCount * 2 * sizeof(s16))
  {
    st->postProcessBufferSize = freeSampleCount * 2 * sizeof(s16);
    st->postProcessBuffer = (s16 *)realloc(st->postProcessBuffer, st->postProcessBufferSize);
  }

  if (soundProcessor->PostProcessSamples != NULL)
  {
    processedSampleCount = soundProcessor->PostProcessSamples(context, st->postProcessBuffer, freeSampleCount, st->synchmode, st->synchronizer);
  }
  else
  {
    processedSampleCount = SPU_DefaultPostProcessSamples(st->postProcessBuffer, freeSampleCount, st->synchmode, st->synchronizer);
  }

  soundProcessor->UpdateAudio(context, st->postProcessBuffer, processedSampleCount);
}

void SPU_DefaultFetchSamples(s16 *sampleBuffer, size_t sampleCount, ESynchMode synchMode, ISynchronizingAudioBuffer *theSynchronizer)
{
  theSynchronizer->enqueue_samples(sampleBuffer, sampleCount);
}

size_t SPU_DefaultPostProcessSamples(s16 *postProcessBuffer, size_t requestedSampleCount, ESynchMode synchMode, ISynchronizingAudioBuffer *theSynchronizer)
{
  return theSynchronizer->output_samples(postProcessBuffer, requestedSampleCount);
}

//////////////////////////////////////////////////////////////////////////////
// Dummy Sound Interface
//////////////////////////////////////////////////////////////////////////////

int SNDDummyInit(void *, int buffersize);
void SNDDummyDeInit(void *);
void SNDDummyUpdateAudio(void *, s16 *buffer, u32 num_samples);
u32 SNDDummyGetAudioSpace(void *);
void SNDDummyMuteAudio(void *);
void SNDDummyUnMuteAudio(void *);
void SNDDummySetVolume(void *, int volume);
void SNDDummyClearBuffer(void *);
void SNDDummyFetchSamples(void *, s16 *sampleBuffer, size_t sampleCount, ESynchMode synchMode, ISynchronizingAudioBuffer *theSynchronizer);
size_t SNDDummyPostProcessSamples(void *, s16 *postProcessBuffer, size_t requestedSampleCount, ESynchMode synchMode, ISynchronizingAudioBuffer *theSynchronizer);

SoundInterface_struct SNDDummy = {
  SNDCORE_DUMMY,
  "Dummy Sound Interface",
  SNDDummyInit,
  SNDDummyDeInit,
  SNDDummyUpdateAudio,
  SNDDummyGetAudioSpace,
  SNDDummyMuteAudio,
  SNDDummyUnMuteAudio,
  SNDDummySetVolume,
  SNDDummyClearBuffer,
  SNDDummyFetchSamples,
  SNDDummyPostProcessSamples
};

int SNDDummyInit(void *, int buffersize) { return 0; }
void SNDDummyDeInit(void *) {}
void SNDDummyUpdateAudio(void *, s16 *buffer, u32 num_samples) { }
u32 SNDDummyGetAudioSpace(void *) { return /*DESMUME_SAMPLE_RATE*/ 48000/60 + 5; }
void SNDDummyMuteAudio(void *) {}
void SNDDummyUnMuteAudio(void *) {}
void SNDDummySetVolume(void *, int volume) {}
void SNDDummyClearBuffer(void *) {}
void SNDDummyFetchSamples(void *, s16 *sampleBuffer, size_t sampleCount, ESynchMode synchMode, ISynchronizingAudioBuffer *theSynchronizer) {}
size_t SNDDummyPostProcessSamples(void *, s16 *postProcessBuffer, size_t requestedSampleCount, ESynchMode synchMode, ISynchronizingAudioBuffer *theSynchronizer) { return 0; }

void SPU_WriteByte(vio2sf_state *st, u32 addr, u8 val)
{
	addr &= 0xFFF;

	st->SPU_core->WriteByte(addr,val);
}
void SPU_WriteWord(vio2sf_state *st, u32 addr, u16 val)
{
	addr &= 0xFFF;

	st->SPU_core->WriteWord(addr,val);
}
void SPU_WriteLong(vio2sf_state *st, u32 addr, u32 val)
{
	addr &= 0xFFF;

	st->SPU_core->WriteLong(addr,val);
}
u8 SPU_ReadByte(vio2sf_state *st, u32 addr) { return st->SPU_core->ReadByte(addr & 0x0FFF); }
u16 SPU_ReadWord(vio2sf_state *st, u32 addr) { return st->SPU_core->ReadWord(addr & 0x0FFF); }
u32 SPU_ReadLong(vio2sf_state *st, u32 addr) { return st->SPU_core->ReadLong(addr & 0x0FFF); }
