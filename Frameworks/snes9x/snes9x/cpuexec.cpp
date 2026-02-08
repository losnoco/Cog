/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include <snes9x/snes9x.h>
#include "cpuops.h"

#include <snes9x/snes.hpp>
#include <snes9x/smp.hpp>
#include <snes9x/sdsp.hpp>

void S9xUnpackStatus(struct S9xState *st)
{
	st->ICPU._Zero = !(st->Registers.P.B.l & Zero);
	st->ICPU._Negative = st->Registers.P.B.l & Negative;
	st->ICPU._Carry = st->Registers.P.B.l & Carry;
	st->ICPU._Overflow = (st->Registers.P.B.l & Overflow) >> 6;
}

void S9xPackStatus(struct S9xState *st)
{
	st->Registers.P.B.l &= ~(Zero | Negative | Carry | Overflow);
	st->Registers.P.B.l |= st->ICPU._Carry | ((!st->ICPU._Zero) << 1) | (st->ICPU._Negative & 0x80) | (st->ICPU._Overflow << 6);
}

void S9xFixCycles(struct S9xState *st)
{
	if (CheckEmulation(st))
	{
		st->ICPU.S9xOpcodes = S9xOpcodesE1;
		st->ICPU.S9xOpLengths = S9xOpLengthsM1X1;
	}
	else if (CheckMemory(st))
	{
		if (CheckIndex(st))
		{
			st->ICPU.S9xOpcodes = S9xOpcodesM1X1;
			st->ICPU.S9xOpLengths = S9xOpLengthsM1X1;
		}
		else
		{
			st->ICPU.S9xOpcodes = S9xOpcodesM1X0;
			st->ICPU.S9xOpLengths = S9xOpLengthsM1X0;
		}
	}
	else
	{
		if (CheckIndex(st))
		{
			st->ICPU.S9xOpcodes = S9xOpcodesM0X1;
			st->ICPU.S9xOpLengths = S9xOpLengthsM0X1;
		}
		else
		{
			st->ICPU.S9xOpcodes = S9xOpcodesM0X0;
			st->ICPU.S9xOpLengths = S9xOpLengthsM0X0;
		}
	}
}

void S9xCheckInterrupts(struct S9xState *st)
{
	bool thisIRQ = st->PPU.HTimerEnabled || st->PPU.VTimerEnabled;

	if (st->CPU.IRQLine && thisIRQ)
		st->CPU.IRQTransition = true;

	if (st->PPU.HTimerEnabled)
	{
		int32_t htimepos = st->PPU.HTimerPosition;
		if (st->CPU.Cycles >= st->Timings.H_Max)
			htimepos += st->Timings.H_Max;

		if (st->CPU.PrevCycles >= htimepos || st->CPU.Cycles < htimepos)
			thisIRQ = false;
	}

	if (st->PPU.VTimerEnabled)
	{
		int32_t vcounter = st->CPU.V_Counter;
		if (st->CPU.Cycles >= st->Timings.H_Max)
			++vcounter;

		if (vcounter != st->PPU.VTimerPosition)
			thisIRQ = false;
	}

	if (!st->CPU.IRQLastState && thisIRQ)
		st->CPU.IRQLine = true;

	st->CPU.IRQLastState = thisIRQ;
}

static inline void S9xReschedule(struct S9xState *st)
{
	switch (st->CPU.WhichEvent)
	{
		case HC_HBLANK_START_EVENT:
			st->CPU.WhichEvent = HC_HDMA_START_EVENT;
			st->CPU.NextEvent = st->Timings.HDMAStart;
			break;

		case HC_HDMA_START_EVENT:
			st->CPU.WhichEvent = HC_HCOUNTER_MAX_EVENT;
			st->CPU.NextEvent = st->Timings.H_Max;
			break;

		case HC_HCOUNTER_MAX_EVENT:
			st->CPU.WhichEvent = HC_HDMA_INIT_EVENT;
			st->CPU.NextEvent = st->Timings.HDMAInit;
			break;

		case HC_HDMA_INIT_EVENT:
			st->CPU.WhichEvent = HC_RENDER_EVENT;
			st->CPU.NextEvent = st->Timings.RenderPos;
			break;

		case HC_RENDER_EVENT:
			st->CPU.WhichEvent = HC_WRAM_REFRESH_EVENT;
			st->CPU.NextEvent = st->Timings.WRAMRefreshPos;
			break;

		case HC_WRAM_REFRESH_EVENT:
			st->CPU.WhichEvent = HC_HBLANK_START_EVENT;
			st->CPU.NextEvent = st->Timings.HBlankStart;
	}
}

void S9xMainLoop(struct S9xState *st)
{
	if (st->CPU.Flags & SCAN_KEYS_FLAG)
		st->CPU.Flags &= ~SCAN_KEYS_FLAG;

	for (;;)
	{
		if (st->CPU.NMIPending)
		{
			if (st->Timings.NMITriggerPos <= st->CPU.Cycles)
			{
				st->CPU.NMIPending = false;
				st->Timings.NMITriggerPos = 0xffff;
				if (st->CPU.WaitingForInterrupt)
				{
					st->CPU.WaitingForInterrupt = false;
					++st->Registers.PC.W.xPC;
					st->CPU.Cycles += TWO_CYCLES + ONE_DOT_CYCLE / 2;
					while (st->CPU.Cycles >= st->CPU.NextEvent)
						S9xDoHEventProcessing(st);
				}

				CHECK_FOR_IRQ_CHANGE(st);
				S9xOpcode_NMI(st);
			}
		}

		if (st->CPU.Cycles >= st->Timings.NextIRQTimer)
		{
			S9xUpdateIRQPositions(st, false);
			st->CPU.IRQLine = true;
		}

		if (st->CPU.IRQLine)
		{
			if (st->CPU.WaitingForInterrupt)
			{
				st->CPU.WaitingForInterrupt = false;
				++st->Registers.PC.W.xPC;
				st->CPU.Cycles += TWO_CYCLES + ONE_DOT_CYCLE / 2;
				while (st->CPU.Cycles >= st->CPU.NextEvent)
					S9xDoHEventProcessing(st);
			}

			if (!CheckFlag(st, IRQ))
			{
				/* The flag pushed onto the stack is the new value */
				CHECK_FOR_IRQ_CHANGE(st);
				S9xOpcode_IRQ(st);
			}
		}

		CHECK_FOR_IRQ_CHANGE(st);

		if (st->CPU.Flags & SCAN_KEYS_FLAG)
			break;

		uint8_t Op;
		const SOpcodes *Opcodes;

		if (st->CPU.PCBase)
		{
			Op = st->CPU.PCBase[st->Registers.PC.W.xPC];
			st->CPU.Cycles += st->CPU.MemSpeed;
			Opcodes = st->ICPU.S9xOpcodes;
		}
		else
		{
			Op = S9xGetByte(st, st->Registers.PC.xPBPC);
			st->OpenBus = Op;
			Opcodes = S9xOpcodesSlow;
		}

		if ((st->Registers.PC.W.xPC & MEMMAP_MASK) + st->ICPU.S9xOpLengths[Op] >= MEMMAP_BLOCK_SIZE)
		{
			uint8_t *oldPCBase = st->CPU.PCBase;

			st->CPU.PCBase = S9xGetBasePointer(st, st->ICPU.ShiftedPB + static_cast<uint16_t>(st->Registers.PC.W.xPC + 4));
			if (oldPCBase != st->CPU.PCBase || (st->Registers.PC.W.xPC & ~MEMMAP_MASK) == (0xffff & ~MEMMAP_MASK))
				Opcodes = S9xOpcodesSlow;
		}

		++st->Registers.PC.W.xPC;
		(*Opcodes[Op].S9xOpcode)(st);
	}

	S9xPackStatus(st);
}

void S9xDoHEventProcessing(struct S9xState *st)
{
	switch (st->CPU.WhichEvent)
	{
		case HC_HBLANK_START_EVENT:
			S9xReschedule(st);
			break;

		case HC_HDMA_START_EVENT:
			S9xReschedule(st);

			if (st->PPU.HDMA && st->CPU.V_Counter <= st->PPU.ScreenHeight)
				st->PPU.HDMA = S9xDoHDMA(st, st->PPU.HDMA);

			break;

		case HC_HCOUNTER_MAX_EVENT:
			S9xAPUEndScanline(st);
			st->CPU.Cycles -= st->Timings.H_Max;
			if (st->Timings.NMITriggerPos != 0xffff)
				st->Timings.NMITriggerPos -= st->Timings.H_Max;
			if (st->Timings.NextIRQTimer != 0x0fffffff)
				st->Timings.NextIRQTimer -= st->Timings.H_Max;
			S9xAPUSetReferenceTime(st, st->CPU.Cycles);

			++st->CPU.V_Counter;
			if (st->CPU.V_Counter >= st->Timings.V_Max) // V ranges from 0 to Timings.V_Max - 1
			{
				st->CPU.V_Counter = 0;
				st->Timings.InterlaceField = !st->Timings.InterlaceField;

				// From byuu:
				// [NTSC]
				// interlace mode has 525 scanlines: 263 on the even frame, and 262 on the odd.
				// non-interlace mode has 524 scanlines: 262 scanlines on both even and odd frames.
				// [PAL] <PAL info is unverified on hardware>
				// interlace mode has 625 scanlines: 313 on the even frame, and 312 on the odd.
				// non-interlace mode has 624 scanlines: 312 scanlines on both even and odd frames.
				if (st->IPPU.Interlace && !st->Timings.InterlaceField)
					st->Timings.V_Max = st->Timings.V_Max_Master + 1; // 263 (NTSC), 313?(PAL)
				else
					st->Timings.V_Max = st->Timings.V_Max_Master; // 262 (NTSC), 312?(PAL)

				st->Memory.FillRAM[0x213F] ^= 0x80;

				// FIXME: reading $4210 will wait 2 cycles, then perform reading, then wait 4 more cycles.
				st->Memory.FillRAM[0x4210] = st->Model->_5A22;
			}

			// From byuu:
			// In non-interlace mode, there are 341 dots per scanline, and 262 scanlines per frame.
			// On odd frames, scanline 240 is one dot short.
			// In interlace mode, there are always 341 dots per scanline. Even frames have 263 scanlines,
			// and odd frames have 262 scanlines.
			// Interlace mode scanline 240 on odd frames is not missing a dot.
			if (st->CPU.V_Counter == 240 && !st->IPPU.Interlace && st->Timings.InterlaceField) // V=240
				st->Timings.H_Max = st->Timings.H_Max_Master - ONE_DOT_CYCLE; // HC=1360
			else
				st->Timings.H_Max = st->Timings.H_Max_Master; // HC=1364

			if (st->Model->_5A22 == 2)
			{
				if (st->CPU.V_Counter != 240 || st->IPPU.Interlace || !st->Timings.InterlaceField)	// V=240
				{
					if (st->Timings.WRAMRefreshPos == SNES_WRAM_REFRESH_HC_v2 - ONE_DOT_CYCLE) // HC=534
						st->Timings.WRAMRefreshPos = SNES_WRAM_REFRESH_HC_v2; // HC=538
					else
						st->Timings.WRAMRefreshPos = SNES_WRAM_REFRESH_HC_v2 - ONE_DOT_CYCLE; // HC=534
				}
			}
			else
				st->Timings.WRAMRefreshPos = SNES_WRAM_REFRESH_HC_v1;

			if (st->CPU.V_Counter == st->PPU.ScreenHeight + FIRST_VISIBLE_LINE) // VBlank starts from V=225(240).
			{
				st->CPU.Flags |= SCAN_KEYS_FLAG;

				st->PPU.HDMA = 0;
				// Bits 7 and 6 of $4212 are computed when read in S9xGetPPU.
				st->PPU.ForcedBlanking = !!((st->Memory.FillRAM[0x2100] >> 7) & 1);

				if (!st->PPU.ForcedBlanking)
				{
					st->PPU.OAMAddr = st->PPU.SavedOAMAddr;

					uint8_t tmp = 0;

					if (st->PPU.OAMPriorityRotation)
						tmp = (st->PPU.OAMAddr & 0xFE) >> 1;
					if ((st->PPU.OAMFlip & 1) || st->PPU.FirstSprite != tmp)
						st->PPU.FirstSprite = tmp;

					st->PPU.OAMFlip = 0;
				}

				// FIXME: writing to $4210 will wait 6 cycles.
				st->Memory.FillRAM[0x4210] = 0x80 | st->Model->_5A22;
				if (st->Memory.FillRAM[0x4200] & 0x80)
				{
					// FIXME: triggered at HC=6, checked just before the final CPU cycle,
					// then, when to call S9xOpcode_NMI()?
					st->CPU.NMIPending = true;
					st->Timings.NMITriggerPos = 6 + 6;
				}
			}

			S9xReschedule(st);

			break;

		case HC_HDMA_INIT_EVENT:
			S9xReschedule(st);

			if (!st->CPU.V_Counter)
				S9xStartHDMA(st);

			break;

		case HC_RENDER_EVENT:
			S9xReschedule(st);

			break;

		case HC_WRAM_REFRESH_EVENT:
			st->CPU.Cycles += SNES_WRAM_REFRESH_CYCLES;

			S9xReschedule(st);
	}
}

void CHECK_FOR_IRQ_CHANGE(struct S9xState *st)
{
	if (st->Timings.IRQFlagChanging)
	{
		if (st->Timings.IRQFlagChanging & IRQ_TRIGGER_NMI)
		{
			st->CPU.NMIPending = true;
			st->Timings.NMITriggerPos = st->CPU.Cycles + 6;
		}
		if (st->Timings.IRQFlagChanging & IRQ_CLEAR_FLAG)
			ClearIRQ(st);
		else if (st->Timings.IRQFlagChanging & IRQ_SET_FLAG)
			SetIRQ(st);
		st->Timings.IRQFlagChanging = IRQ_NONE;
	}
}
