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

#include "snes9x.h"
#include "cpuops.h"

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
	for (;;)
	{
		if (st->CPU.NMILine)
		{
			if (st->Timings.NMITriggerPos <= st->CPU.Cycles)
			{
				st->CPU.NMILine = false;
				st->Timings.NMITriggerPos = 0xffff;
				if (st->CPU.WaitingForInterrupt)
				{
					st->CPU.WaitingForInterrupt = false;
					++st->Registers.PC.W.xPC;
				}

				S9xOpcode_NMI(st);
			}
		}

		if (st->CPU.IRQTransition)
		{
			if (st->CPU.IRQPending)
				--st->CPU.IRQPending;
			else
			{
				if (st->CPU.WaitingForInterrupt)
				{
					st->CPU.WaitingForInterrupt = false;
					++st->Registers.PC.W.xPC;
				}

				st->CPU.IRQTransition = false;
				st->CPU.IRQPending = st->Timings.IRQPendCount;

				if (!CheckFlag(st, IRQ))
					S9xOpcode_IRQ(st);
			}
		}

		if (st->CPU.Flags & SCAN_KEYS_FLAG)
			break;

		uint8_t Op;
		const SOpcodes *Opcodes;

		if (st->CPU.PCBase)
		{
			Op = st->CPU.PCBase[st->Registers.PC.W.xPC];
			st->CPU.PrevCycles = st->CPU.Cycles;
			st->CPU.Cycles += st->CPU.MemSpeed;
			S9xCheckInterrupts(st);
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

			st->CPU.PCBase = S9xGetBasePointer(st, st->ICPU.ShiftedPB + (static_cast<uint16_t>(st->Registers.PC.W.xPC + 4)));
			if (oldPCBase != st->CPU.PCBase || (st->Registers.PC.W.xPC & ~MEMMAP_MASK) == (0xffff & ~MEMMAP_MASK))
				Opcodes = S9xOpcodesSlow;
		}

		++st->Registers.PC.W.xPC;
		(*Opcodes[Op].S9xOpcode)(st);
	}

	S9xPackStatus(st);

	if (st->CPU.Flags & SCAN_KEYS_FLAG)
		st->CPU.Flags &= ~SCAN_KEYS_FLAG;
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
			st->CPU.PrevCycles -= st->Timings.H_Max;
			S9xAPUSetReferenceTime(st, st->CPU.Cycles);

			if ((st->Timings.NMITriggerPos != 0xffff) && (st->Timings.NMITriggerPos >= st->Timings.H_Max))
				st->Timings.NMITriggerPos -= st->Timings.H_Max;

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
				st->CPU.NMILine = false;
				st->Timings.NMITriggerPos = 0xffff;

				st->CPU.Flags |= SCAN_KEYS_FLAG;
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
					st->CPU.NMILine = true;
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
			st->CPU.PrevCycles = st->CPU.Cycles;
			st->CPU.Cycles += SNES_WRAM_REFRESH_CYCLES;
			S9xCheckInterrupts(st);

			S9xReschedule(st);
	}
}
