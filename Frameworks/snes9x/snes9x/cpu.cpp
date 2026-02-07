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

#include <algorithm>
#include "snes9x.h"

void SetCarry(struct S9xState *st) { st->ICPU._Carry = 1; }
void ClearCarry(struct S9xState *st) { st->ICPU._Carry = 0; }
void SetIRQ(struct S9xState *st) { st->Registers.P.B.l |= IRQ; }
void ClearIRQ(struct S9xState *st) { st->Registers.P.B.l &= ~IRQ; }
void SetDecimal(struct S9xState *st) { st->Registers.P.B.l |= Decimal; }
void ClearDecimal(struct S9xState *st) { st->Registers.P.B.l &= ~Decimal; }
void SetOverflow(struct S9xState *st) { st->ICPU._Overflow = 1; }
void ClearOverflow(struct S9xState *st) { st->ICPU._Overflow = 0; }

bool CheckCarry(struct S9xState *st) { return !!st->ICPU._Carry; }
bool CheckZero(struct S9xState *st) { return !st->ICPU._Zero; }
bool CheckDecimal(struct S9xState *st) { return !!(st->Registers.P.B.l & Decimal); }
bool CheckIndex(struct S9xState *st) { return !!(st->Registers.P.B.l & IndexFlag); }
bool CheckMemory(struct S9xState *st) { return !!(st->Registers.P.B.l & MemoryFlag); }
bool CheckOverflow(struct S9xState *st) { return !!(st->ICPU._Overflow); }
bool CheckNegative(struct S9xState *st) { return !!(st->ICPU._Negative & 0x80); }
bool CheckEmulation(struct S9xState *st) { return !!(st->Registers.P.W & Emulation); }

void SetFlags(struct S9xState *st, uint16_t f) { st->Registers.P.W |= f; }
void ClearFlags(struct S9xState *st, uint16_t f) { st->Registers.P.W &= ~f; }
bool CheckFlag(struct S9xState *st, uint16_t f) { return !!(st->Registers.P.W & f); }

static void S9xSoftResetCPU(struct S9xState *st)
{
	st->CPU.Cycles = 182; // Or 188. This is the cycle count just after the jump to the Reset Vector.
	st->CPU.PrevCycles = st->CPU.Cycles;
	st->CPU.V_Counter = 0;
	st->CPU.Flags &= DEBUG_MODE_FLAG | TRACE_FLAG;
	st->CPU.PCBase = nullptr;
	st->CPU.NMILine = st->CPU.IRQLine = st->CPU.IRQTransition = st->CPU.IRQLastState = false;
	st->CPU.IRQPending = st->Timings.IRQPendCount;
	st->CPU.MemSpeed = SLOW_ONE_CYCLE;
	st->CPU.MemSpeedx2 = SLOW_ONE_CYCLE * 2;
	st->CPU.FastROMSpeed = SLOW_ONE_CYCLE;
	st->CPU.InDMA = st->CPU.InHDMA = st->CPU.InDMAorHDMA = st->CPU.InWRAMDMAorHDMA = false;
	st->CPU.HDMARanInDMA = 0;
	st->CPU.CurrentDMAorHDMAChannel = -1;
	st->CPU.WhichEvent = HC_RENDER_EVENT;
	st->CPU.NextEvent  = st->Timings.RenderPos;
	st->CPU.WaitingForInterrupt = false;

	st->Registers.PC.xPBPC = 0;
	st->Registers.PC.B.xPB = 0;
	st->Registers.PC.W.xPC = S9xGetWord(st, 0xfffc);
	st->OpenBus = st->Registers.PC.B.xPCh;
	st->Registers.D.W = 0;
	st->Registers.DB = 0;
	st->Registers.S.B.h = 1;
	st->Registers.S.B.l -= 3;
	st->Registers.X.B.h = st->Registers.Y.B.h = 0;

	st->ICPU.ShiftedPB = st->ICPU.ShiftedDB = 0;
	SetFlags(st, MemoryFlag | IndexFlag | IRQ | Emulation);
	ClearFlags(st, Decimal);

	st->Timings.InterlaceField = false;
	st->Timings.H_Max = st->Timings.H_Max_Master;
	st->Timings.V_Max = st->Timings.V_Max_Master;
	st->Timings.NMITriggerPos = 0xffff;
	st->Timings.WRAMRefreshPos = st->Model->_5A22 == 2 ? SNES_WRAM_REFRESH_HC_v2 : SNES_WRAM_REFRESH_HC_v1;

	S9xSetPCBase(st, st->Registers.PC.xPBPC);

	st->ICPU.S9xOpcodes = S9xOpcodesE1;
	st->ICPU.S9xOpLengths = S9xOpLengthsM1X1;

	S9xUnpackStatus(st);
}

static void S9xResetCPU(struct S9xState *st)
{
	S9xSoftResetCPU(st);
	st->Registers.S.B.l = 0xff;
	st->Registers.P.W = st->Registers.A.W = st->Registers.X.W = st->Registers.Y.W = 0;
	SetFlags(st, MemoryFlag | IndexFlag | IRQ | Emulation);
	ClearFlags(st, Decimal);
}

void S9xReset(struct S9xState *st)
{
    //printf("1Mem MAP: %08X rom %08X size %d\n",st->Memory.Map[15],st->Memory.ROM,st->Memory.CalculatedSize);
	std::fill_n(&st->Memory.RAM[0], 0x20000, 0x55);
	std::fill_n(&st->Memory.VRAM[0], 0x10000, 0);
	std::fill_n(&st->Memory.FillRAM[0], 0x8000, 0);

	S9xResetCPU(st);
	S9xResetPPU(st);
	S9xResetDMA(st);
	S9xResetAPU(st);
}
