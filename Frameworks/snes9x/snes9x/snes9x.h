/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

#include <memory>

#ifndef VERSION
#define VERSION "1.60"
#endif

#include <snes9x/port.h>

struct S9xState;

inline constexpr int SNES_WIDTH = 256;
inline constexpr int SNES_HEIGHT = 224;
inline constexpr uint16_t SNES_HEIGHT_EXTENDED = 239;

inline constexpr double NTSC_MASTER_CLOCK = 21477272.727272; // 21477272 + 8/11 exact
inline constexpr double PAL_MASTER_CLOCK = 21281370.0;

inline constexpr int32_t SNES_MAX_NTSC_VCOUNTER = 262;
inline constexpr int32_t SNES_MAX_PAL_VCOUNTER = 312;
inline constexpr int32_t SNES_HCOUNTER_MAX = 341;

inline constexpr int32_t ONE_CYCLE = 6;
inline constexpr int32_t SLOW_ONE_CYCLE = 8;
inline constexpr int32_t TWO_CYCLES = 12;
inline constexpr int32_t ONE_DOT_CYCLE = 4;

inline constexpr int32_t SNES_CYCLES_PER_SCANLINE = SNES_HCOUNTER_MAX * ONE_DOT_CYCLE;

inline constexpr int32_t SNES_WRAM_REFRESH_HC_v1 = 530;
inline constexpr int32_t SNES_WRAM_REFRESH_HC_v2 = 538;
inline constexpr int32_t SNES_WRAM_REFRESH_CYCLES = 40;

inline constexpr int32_t SNES_HBLANK_START_HC = 1096; // H=274
inline constexpr int32_t SNES_HDMA_START_HC = 1106; // FIXME: not true
inline constexpr int32_t SNES_HBLANK_END_HC = 4; // H=1
inline constexpr int32_t SNES_HDMA_INIT_HC = 20; // FIXME: not true
inline constexpr int32_t SNES_RENDER_START_HC = 48 * ONE_DOT_CYCLE; // FIXME: Snes9x renders a line at a time.

inline constexpr uint32_t DEBUG_MODE_FLAG = 1; // debugger
inline constexpr uint32_t TRACE_FLAG = 1 << 1; // debugger
inline constexpr uint32_t SCAN_KEYS_FLAG = 1 << 4; // CPU
inline constexpr uint32_t HALTED_FLAG = 1 << 12; // APU

inline constexpr size_t ROM_NAME_LEN = 23;

struct SCPUState
{
	uint32_t Flags;
	int32_t Cycles;
	int32_t PrevCycles;
	int32_t V_Counter;
	uint8_t *PCBase;
	bool NMIPending;
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

enum
{
	IRQ_NONE = 0x0,
	IRQ_SET_FLAG = 0x1,
	IRQ_CLEAR_FLAG = 0x2,
	IRQ_TRIGGER_NMI = 0x4
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
	int32_t NextIRQTimer;
	int32_t IRQTriggerCycles;
	int32_t WRAMRefreshPos;
	int32_t RenderPos;
	bool InterlaceField;
	int32_t DMACPUSync; // The cycles to synchronize DMA and CPU. Snes9x cannot emulate correctly.
	int32_t NMIDMADelay; // The delay of NMI trigger after DMA transfers. Snes9x cannot emulate correctly.
	int32_t IRQFlagChanging; // This value is just a hack.
	int32_t APUSpeedup;
	bool APUAllowTimeOverflow;
};

struct SSettings
{
	bool SDD1;

	bool ForceNotInterleaved;
	bool PAL;

	bool SoundSync;
	uint32_t SoundPlaybackRate;
	uint32_t SoundInputRate;
	bool Mute;
	int32_t InterpolationMethod;

	bool DisableGameSpecificHacks;
	bool BlockInvalidVRAMAccessMaster;
	bool BlockInvalidVRAMAccess;
	int32_t HDMATimingHack;

	bool TurboMode;

	bool SeparateEchoBuffer;
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
