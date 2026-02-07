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

#pragma once

#include <memory>

#ifndef VERSION
#define VERSION "1.53"
#endif

#include <snes9x/port.h>

struct S9xState;

const int SNES_WIDTH = 256;
const int SNES_HEIGHT = 224;
const uint16_t SNES_HEIGHT_EXTENDED = 239;

const int32_t SNES_MAX_NTSC_VCOUNTER = 262;
const int32_t SNES_MAX_PAL_VCOUNTER = 312;
const int32_t SNES_HCOUNTER_MAX = 341;

const int32_t ONE_CYCLE = 6;
const int32_t SLOW_ONE_CYCLE = 8;
const int32_t TWO_CYCLES = 12;
const int32_t ONE_DOT_CYCLE = 4;

const int32_t SNES_CYCLES_PER_SCANLINE = SNES_HCOUNTER_MAX * ONE_DOT_CYCLE;

const int32_t SNES_WRAM_REFRESH_HC_v1 = 530;
const int32_t SNES_WRAM_REFRESH_HC_v2 = 538;
const int32_t SNES_WRAM_REFRESH_CYCLES = 40;

const int32_t SNES_HBLANK_START_HC = 1096; // H=274
const int32_t SNES_HDMA_START_HC = 1106; // FIXME: not true
const int32_t SNES_HBLANK_END_HC = 4; // H=1
const int32_t SNES_HDMA_INIT_HC = 20; // FIXME: not true
const int32_t SNES_RENDER_START_HC = 48 * ONE_DOT_CYCLE; // FIXME: Snes9x renders a line at a time.

const uint32_t DEBUG_MODE_FLAG = 1; // debugger
const uint32_t TRACE_FLAG = 1 << 1; // debugger
const uint32_t SCAN_KEYS_FLAG = 1 << 4; // CPU
const uint32_t HALTED_FLAG = 1 << 12; // APU

const size_t ROM_NAME_LEN = 23;

struct SCPUState
{
	uint32_t Flags;
	int32_t Cycles;
	int32_t PrevCycles;
	int32_t V_Counter;
	uint8_t *PCBase;
	bool NMILine;
	bool IRQLine;
	bool IRQTransition;
	bool IRQLastState;
	int32_t IRQPending;
	int32_t MemSpeed;
	int32_t MemSpeedx2;
	int32_t FastROMSpeed;
	bool InDMA;
	bool InHDMA;
	bool InDMAorHDMA;
	bool InWRAMDMAorHDMA;
	uint8_t HDMARanInDMA;
	int32_t CurrentDMAorHDMAChannel;
	uint8_t WhichEvent;
	int32_t NextEvent;
	bool WaitingForInterrupt;
};

enum
{
	HC_HBLANK_START_EVENT = 1,
	HC_HDMA_START_EVENT = 2,
	HC_HCOUNTER_MAX_EVENT = 3,
	HC_HDMA_INIT_EVENT = 4,
	HC_RENDER_EVENT = 5,
	HC_WRAM_REFRESH_EVENT = 6
};

struct STimings
{
	int32_t H_Max_Master;
	int32_t H_Max;
	int32_t V_Max_Master;
	int32_t V_Max;
	int32_t HBlankStart;
	int32_t HBlankEnd;
	int32_t HDMAInit;
	int32_t HDMAStart;
	int32_t NMITriggerPos;
	int32_t IRQTriggerCycles;
	int32_t WRAMRefreshPos;
	int32_t RenderPos;
	bool InterlaceField;
	int32_t DMACPUSync; // The cycles to synchronize DMA and CPU. Snes9x cannot emulate correctly.
	int32_t NMIDMADelay; // The delay of NMI trigger after DMA transfers. Snes9x cannot emulate correctly.
	int32_t IRQPendCount; // This value is just a hack.
	int32_t APUSpeedup;
	bool APUAllowTimeOverflow;
};

struct SSettings
{
	bool SDD1;

	bool ForceNotInterleaved;
	bool PAL;

	bool SoundSync;
	bool SixteenBitSound;
	uint32_t SoundPlaybackRate;
	uint32_t SoundInputRate;
	bool Stereo;
	bool ReverseStereo;
	bool Mute;

	bool DisableGameSpecificHacks;
	bool BlockInvalidVRAMAccessMaster;
	bool BlockInvalidVRAMAccess;
	int32_t HDMATimingHack;

	bool TurboMode;
};

struct SSNESGameFixes
{
	bool Uniracers;
};

// struct CMemory
#include <snes9x/memmap.h>

// struct SRegisters
#include <snes9x/65c816.h>

// struct SOpcodes
// struct SICPU
#include <snes9x/cpuexec.h>

// struct SDMA
#include <snes9x/dma.h>

// struct InternalPPU
// struct SPPU
// struct SnesModel
#include <snes9x/ppu.h>

// struct S9xSPC
#include <snes9x/apu.h>

// struct SNES_SPC
#include <snes9x/SNES_SPC.h>

struct S9xState
{
  SCPUState CPU;
  SICPU ICPU;
  SRegisters Registers;
  SPPU PPU;
  InternalPPU IPPU;
  SDMA DMA[8];
  STimings Timings;
  SSettings Settings;
  SSNESGameFixes SNESGameFixes;
  CMemory Memory;

  uint8_t OpenBus;
  uint8_t *HDMAMemPointers[8];

  SnesModel M1SNES;
  SnesModel *Model;

  S9xSPC SPC;
  std::unique_ptr<SNES_SPC> spc_core;

  uint8_t sdd1_decode_buffer[0x10000];

  S9xState()
  {
    CPU = { 0 };
    ICPU = { 0 };
    Registers = { 0 };
    PPU = { 0 };
    IPPU = { 0 };
    for(size_t i = 0; i < 8; ++i) {
      DMA[i] = { 0 };
    }
    DMA[0] = { 0 };
    Timings = { 0 };
    Settings = { 0 };
    SNESGameFixes = { 0 };
    OpenBus = 0;
    for(size_t i = 0; i < 8; ++i) {
      HDMAMemPointers[i] = nullptr;
    }
    M1SNES = { 1, 3, 2 };
    Model = &M1SNES;
  }
};
