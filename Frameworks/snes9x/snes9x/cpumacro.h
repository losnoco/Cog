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

template<typename Fa, typename Ff> inline void rOP8(struct S9xState *st, Fa addr, Ff func)
{
	uint8_t val = st->OpenBus = S9xGetByte(st, addr(st, READ));
	func(st, val);
}

template<typename Fa, typename Ff> inline void rOP16(struct S9xState *st, Fa addr, Ff func, s9xwrap_t wrap = WRAP_NONE)
{
	uint16_t val = S9xGetWord(st, addr(st, READ), wrap);
	st->OpenBus = static_cast<uint8_t>(val >> 8);
	func(st, val);
}

template<typename Fa, typename Ff8, typename Ff16> inline void rOPC(struct S9xState *st, bool cond, Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	if (cond)
		rOP8(st, addr, func8);
	else
		rOP16(st, addr, func16, wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void rOPM(struct S9xState *st, Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	rOPC(st, CheckMemory(st), addr, func8, func16, wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void rOPX(struct S9xState *st, Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	rOPC(st, CheckIndex(st), addr, func8, func16, wrap);
}

template<typename Fa, typename Ff> inline void wOP8(struct S9xState *st, Fa addr, Ff func)
{
	func(st, addr(st, WRITE));
}

template<typename Fa, typename Ff> inline void wOP16(struct S9xState *st, Fa addr, Ff func, s9xwrap_t wrap = WRAP_NONE)
{
	func(st, addr(st, WRITE), wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void wOPC(struct S9xState *st, bool cond, Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	if (cond)
		wOP8(st, addr, func8);
	else
		wOP16(st, addr, func16, wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void wOPM(struct S9xState *st, Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	wOPC(st, CheckMemory(st), addr, func8, func16, wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void wOPX(struct S9xState *st, Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	wOPC(st, CheckIndex(st), addr, func8, func16, wrap);
}

template<typename Fa, typename Ff> inline void mOP8(struct S9xState *st, Fa addr, Ff func)
{
	func(st, addr(st, MODIFY));
}

template<typename Fa, typename Ff> inline void mOP16(struct S9xState *st, Fa addr, Ff func, s9xwrap_t wrap = WRAP_NONE)
{
	func(st, addr(st, MODIFY), wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void mOPC(struct S9xState *st, bool cond, Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	if (cond)
		mOP8(st, addr, func8);
	else
		mOP16(st, addr, func16, wrap);
}

template<typename Fa, typename Ff8, typename Ff16> inline void mOPM(struct S9xState *st, Fa addr, Ff8 func8, Ff16 func16, s9xwrap_t wrap = WRAP_NONE)
{
	mOPC(st, CheckMemory(st), addr, func8, func16, wrap);
}

template<typename F> inline void bOP(struct S9xState *st, F rel, bool cond, bool e)
{
	pair newPC;
	newPC.W = rel(st, JUMP);
	if (cond)
	{
		AddCycles(st, ONE_CYCLE);
		if (e && st->Registers.PC.B.xPCh != newPC.B.h)
			AddCycles(st, ONE_CYCLE);
		if ((st->Registers.PC.W.xPC & ~MEMMAP_MASK) != (newPC.W & ~MEMMAP_MASK))
			S9xSetPCBase(st, st->ICPU.ShiftedPB + newPC.W);
		else
			st->Registers.PC.W.xPC = newPC.W;
	}
}

inline void SetZN(struct S9xState *st, uint16_t Work16)
{
	st->ICPU._Zero = !!Work16;
	st->ICPU._Negative = static_cast<uint8_t>(Work16 >> 8);
}

inline void SetZN(struct S9xState *st, uint8_t Work8)
{
	st->ICPU._Zero = Work8;
	st->ICPU._Negative = Work8;
}

inline void ADC16(struct S9xState *st, uint16_t Work16)
{
	if (CheckDecimal(st))
	{
		uint16_t A1 = st->Registers.A.W & 0x000F;
		uint16_t A2 = st->Registers.A.W & 0x00F0;
		uint16_t A3 = st->Registers.A.W & 0x0F00;
		uint32_t A4 = st->Registers.A.W & 0xF000;
		uint16_t W1 = Work16 & 0x000F;
		uint16_t W2 = Work16 & 0x00F0;
		uint16_t W3 = Work16 & 0x0F00;
		uint16_t W4 = Work16 & 0xF000;

		A1 += W1 + CheckCarry(st);
		if (A1 > 0x0009)
		{
			A1 -= 0x000A;
			A1 &= 0x000F;
			A2 += 0x0010;
		}

		A2 += W2;
		if (A2 > 0x0090)
		{
			A2 -= 0x00A0;
			A2 &= 0x00F0;
			A3 += 0x0100;
		}

		A3 += W3;
		if (A3 > 0x0900)
		{
			A3 -= 0x0A00;
			A3 &= 0x0F00;
			A4 += 0x1000;
		}

		A4 += W4;
		if (A4 > 0x9000)
		{
			A4 -= 0xA000;
			A4 &= 0xF000;
			SetCarry(st);
		}
		else
			ClearCarry(st);

		uint16_t Ans16 = A4 | A3 | A2 | A1;

		if (~(st->Registers.A.W ^ Work16) & (Work16 ^ Ans16) & 0x8000)
			SetOverflow(st);
		else
			ClearOverflow(st);

		st->Registers.A.W = Ans16;
		SetZN(st, st->Registers.A.W);
	}
	else
	{
		uint32_t Ans32 = st->Registers.A.W + Work16 + CheckCarry(st);

		st->ICPU._Carry = Ans32 >= 0x10000;

		if (~(st->Registers.A.W ^ Work16) & (Work16 ^ static_cast<uint16_t>(Ans32)) & 0x8000)
			SetOverflow(st);
		else
			ClearOverflow(st);

		st->Registers.A.W = static_cast<uint16_t>(Ans32);
		SetZN(st, st->Registers.A.W);
	}
}

inline void ADC8(struct S9xState *st, uint8_t Work8)
{
	if (CheckDecimal(st))
	{
		uint8_t A1 = st->Registers.A.W & 0x0F;
		uint16_t A2 = st->Registers.A.W & 0xF0;
		uint8_t W1 = Work8 & 0x0F;
		uint8_t W2 = Work8 & 0xF0;

		A1 += W1 + CheckCarry(st);
		if (A1 > 0x09)
		{
			A1 -= 0x0A;
			A1 &= 0x0F;
			A2 += 0x10;
		}

		A2 += W2;
		if (A2 > 0x90)
		{
			A2 -= 0xA0;
			A2 &= 0xF0;
			SetCarry(st);
		}
		else
			ClearCarry(st);

		uint8_t Ans8 = A2 | A1;

		if (~(st->Registers.A.B.l ^ Work8) & (Work8 ^ Ans8) & 0x80)
			SetOverflow(st);
		else
			ClearOverflow(st);

		st->Registers.A.B.l = Ans8;
		SetZN(st, st->Registers.A.B.l);
	}
	else
	{
		uint16_t Ans16 = st->Registers.A.B.l + Work8 + CheckCarry(st);

		st->ICPU._Carry = Ans16 >= 0x100;

		if (~(st->Registers.A.B.l ^ Work8) & (Work8 ^ static_cast<uint8_t>(Ans16)) & 0x80)
			SetOverflow(st);
		else
			ClearOverflow(st);

		st->Registers.A.B.l = static_cast<uint8_t>(Ans16);
		SetZN(st, st->Registers.A.B.l);
	}
}

inline void AND16(struct S9xState *st, uint16_t Work16)
{
	st->Registers.A.W &= Work16;
	SetZN(st, st->Registers.A.W);
}

inline void AND8(struct S9xState *st, uint8_t Work8)
{
	st->Registers.A.B.l &= Work8;
	SetZN(st, st->Registers.A.B.l);
}

inline void ASL16(struct S9xState *st, uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t Work16 = S9xGetWord(st, OpAddress, w);
	st->ICPU._Carry = !!(Work16 & 0x8000);
	Work16 <<= 1;
	AddCycles(st, ONE_CYCLE);
	S9xSetWord(st, Work16, OpAddress, w, WRITE_10);
	st->OpenBus = Work16 & 0xff;
	SetZN(st, Work16);
}

inline void ASL8(struct S9xState *st, uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(st, OpAddress);
	st->ICPU._Carry = !!(Work8 & 0x80);
	Work8 <<= 1;
	AddCycles(st, ONE_CYCLE);
	S9xSetByte(st, Work8, OpAddress);
	st->OpenBus = Work8;
	SetZN(st, Work8);
}

inline void BIT16(struct S9xState *st, uint16_t Work16)
{
	st->ICPU._Overflow = !!(Work16 & 0x4000);
	st->ICPU._Negative = static_cast<uint8_t>(Work16 >> 8);
	st->ICPU._Zero = !!(Work16 & st->Registers.A.W);
}

inline void BIT8(struct S9xState *st, uint8_t Work8)
{
	st->ICPU._Overflow = !!(Work8 & 0x40);
	st->ICPU._Negative = Work8;
	st->ICPU._Zero = !!(Work8 & st->Registers.A.B.l);
}

inline void CMP16(struct S9xState *st, uint16_t val)
{
	int32_t Int32 = static_cast<int32_t>(st->Registers.A.W) - static_cast<int32_t>(val);
	st->ICPU._Carry = Int32 >= 0;
	SetZN(st, static_cast<uint16_t>(Int32));
}

inline void CMP8(struct S9xState *st, uint8_t val)
{
	int16_t Int16 = static_cast<int16_t>(st->Registers.A.B.l) - static_cast<int16_t>(val);
	st->ICPU._Carry = Int16 >= 0;
	SetZN(st, static_cast<uint8_t>(Int16));
}

inline void CPX16(struct S9xState *st, uint16_t val)
{
	int32_t Int32 = static_cast<int32_t>(st->Registers.X.W) - static_cast<int32_t>(val);
	st->ICPU._Carry = Int32 >= 0;
	SetZN(st, static_cast<uint16_t>(Int32));
}

inline void CPX8(struct S9xState *st, uint8_t val)
{
	int16_t Int16 = static_cast<int16_t>(st->Registers.X.B.l) - static_cast<int16_t>(val);
	st->ICPU._Carry = Int16 >= 0;
	SetZN(st, static_cast<uint8_t>(Int16));
}

inline void CPY16(struct S9xState *st, uint16_t val)
{
	int32_t Int32 = static_cast<int32_t>(st->Registers.Y.W) - static_cast<int32_t>(val);
	st->ICPU._Carry = Int32 >= 0;
	SetZN(st, static_cast<uint16_t>(Int32));
}

inline void CPY8(struct S9xState *st, uint8_t val)
{
	int16_t Int16 = static_cast<int16_t>(st->Registers.Y.B.l) - static_cast<int16_t>(val);
	st->ICPU._Carry = Int16 >= 0;
	SetZN(st, static_cast<uint8_t>(Int16));
}

inline void DEC16(struct S9xState *st, uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t Work16 = S9xGetWord(st, OpAddress, w) - 1;
	AddCycles(st, ONE_CYCLE);
	S9xSetWord(st, Work16, OpAddress, w, WRITE_10);
	st->OpenBus = Work16 & 0xff;
	SetZN(st, Work16);
}

inline void DEC8(struct S9xState *st, uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(st, OpAddress) - 1;
	AddCycles(st, ONE_CYCLE);
	S9xSetByte(st, Work8, OpAddress);
	st->OpenBus = Work8;
	SetZN(st, Work8);
}

inline void EOR16(struct S9xState *st, uint16_t val)
{
	st->Registers.A.W ^= val;
	SetZN(st, st->Registers.A.W);
}

inline void EOR8(struct S9xState *st, uint8_t val)
{
	st->Registers.A.B.l ^= val;
	SetZN(st, st->Registers.A.B.l);
}

inline void INC16(struct S9xState *st, uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t Work16 = S9xGetWord(st, OpAddress, w) + 1;
	AddCycles(st, ONE_CYCLE);
	S9xSetWord(st, Work16, OpAddress, w, WRITE_10);
	st->OpenBus = Work16 & 0xff;
	SetZN(st, Work16);
}

inline void INC8(struct S9xState *st, uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(st, OpAddress) + 1;
	AddCycles(st, ONE_CYCLE);
	S9xSetByte(st, Work8, OpAddress);
	st->OpenBus = Work8;
	SetZN(st, Work8);
}

inline void LDA16(struct S9xState *st, uint16_t val)
{
	st->Registers.A.W = val;
	SetZN(st, st->Registers.A.W);
}

inline void LDA8(struct S9xState *st, uint8_t val)
{
	st->Registers.A.B.l = val;
	SetZN(st, st->Registers.A.B.l);
}

inline void LDX16(struct S9xState *st, uint16_t val)
{
	st->Registers.X.W = val;
	SetZN(st, st->Registers.X.W);
}

inline void LDX8(struct S9xState *st, uint8_t val)
{
	st->Registers.X.B.l = val;
	SetZN(st, st->Registers.X.B.l);
}

inline void LDY16(struct S9xState *st, uint16_t val)
{
	st->Registers.Y.W = val;
	SetZN(st, st->Registers.Y.W);
}

inline void LDY8(struct S9xState *st, uint8_t val)
{
	st->Registers.Y.B.l = val;
	SetZN(st, st->Registers.Y.B.l);
}

inline void LSR16(struct S9xState *st, uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t Work16 = S9xGetWord(st, OpAddress, w);
	st->ICPU._Carry = Work16 & 1;
	Work16 >>= 1;
	AddCycles(st, ONE_CYCLE);
	S9xSetWord(st, Work16, OpAddress, w, WRITE_10);
	st->OpenBus = Work16 & 0xff;
	SetZN(st, Work16);
}

inline void LSR8(struct S9xState *st, uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(st, OpAddress);
	st->ICPU._Carry = Work8 & 1;
	Work8 >>= 1;
	AddCycles(st, ONE_CYCLE);
	S9xSetByte(st, Work8, OpAddress);
	st->OpenBus = Work8;
	SetZN(st, Work8);
}

inline void ORA16(struct S9xState *st, uint16_t val)
{
	st->Registers.A.W |= val;
	SetZN(st, st->Registers.A.W);
}

inline void ORA8(struct S9xState *st, uint8_t val)
{
	st->Registers.A.B.l |= val;
	SetZN(st, st->Registers.A.B.l);
}

inline void ROL16(struct S9xState *st, uint32_t OpAddress, s9xwrap_t w)
{
	uint32_t Work32 = (static_cast<uint32_t>(S9xGetWord(st, OpAddress, w)) << 1) | static_cast<uint32_t>(CheckCarry(st));
	st->ICPU._Carry = Work32 >= 0x10000;
	AddCycles(st, ONE_CYCLE);
	S9xSetWord(st, static_cast<uint16_t>(Work32), OpAddress, w, WRITE_10);
	st->OpenBus = Work32 & 0xff;
	SetZN(st, static_cast<uint16_t>(Work32));
}

inline void ROL8(struct S9xState *st, uint32_t OpAddress)
{
	uint16_t Work16 = (static_cast<uint16_t>(S9xGetByte(st, OpAddress)) << 1) | static_cast<uint16_t>(CheckCarry(st));
	st->ICPU._Carry = Work16 >= 0x100;
	AddCycles(st, ONE_CYCLE);
	S9xSetByte(st, static_cast<uint8_t>(Work16), OpAddress);
	st->OpenBus = Work16 & 0xff;
	SetZN(st, static_cast<uint8_t>(Work16));
}

inline void ROR16(struct S9xState *st, uint32_t OpAddress, s9xwrap_t w)
{
	uint32_t Work32 = static_cast<uint32_t>(S9xGetWord(st, OpAddress, w)) | (static_cast<uint32_t>(CheckCarry(st)) << 16);
	st->ICPU._Carry = Work32 & 1;
	Work32 >>= 1;
	AddCycles(st, ONE_CYCLE);
	S9xSetWord(st, static_cast<uint16_t>(Work32), OpAddress, w, WRITE_10);
	st->OpenBus = Work32 & 0xff;
	SetZN(st, static_cast<uint16_t>(Work32));
}

inline void ROR8(struct S9xState *st, uint32_t OpAddress)
{
	uint16_t Work16 = static_cast<uint16_t>(S9xGetByte(st, OpAddress)) | (static_cast<uint16_t>(CheckCarry(st)) << 8);
	st->ICPU._Carry = Work16 & 1;
	Work16 >>= 1;
	AddCycles(st, ONE_CYCLE);
	S9xSetByte(st, static_cast<uint8_t>(Work16), OpAddress);
	st->OpenBus = Work16 & 0xff;
	SetZN(st, static_cast<uint8_t>(Work16));
}

inline void SBC16(struct S9xState *st, uint16_t Work16)
{
	if (CheckDecimal(st))
	{
		uint16_t A1 = st->Registers.A.W & 0x000F;
		uint16_t A2 = st->Registers.A.W & 0x00F0;
		uint16_t A3 = st->Registers.A.W & 0x0F00;
		uint32_t A4 = st->Registers.A.W & 0xF000;
		uint16_t W1 = Work16 & 0x000F;
		uint16_t W2 = Work16 & 0x00F0;
		uint16_t W3 = Work16 & 0x0F00;
		uint16_t W4 = Work16 & 0xF000;

		A1 -= W1 + !CheckCarry(st);
		A2 -= W2;
		A3 -= W3;
		A4 -= W4;

		if (A1 > 0x000F)
		{
			A1 += 0x000A;
			A1 &= 0x000F;
			A2 -= 0x0010;
		}

		if (A2 > 0x00F0)
		{
			A2 += 0x00A0;
			A2 &= 0x00F0;
			A3 -= 0x0100;
		}

		if (A3 > 0x0F00)
		{
			A3 += 0x0A00;
			A3 &= 0x0F00;
			A4 -= 0x1000;
		}

		if (A4 > 0xF000)
		{
			A4 += 0xA000;
			A4 &= 0xF000;
			ClearCarry(st);
		}
		else
			SetCarry(st);

		uint16_t Ans16 = A4 | A3 | A2 | A1;

		if ((st->Registers.A.W ^ Work16) & (st->Registers.A.W ^ Ans16) & 0x8000)
			SetOverflow(st);
		else
			ClearOverflow(st);

		st->Registers.A.W = Ans16;
		SetZN(st, st->Registers.A.W);
	}
	else
	{
		int32_t Int32 = static_cast<int32_t>(st->Registers.A.W) - static_cast<int32_t>(Work16) + static_cast<int32_t>(CheckCarry(st)) - 1;

		st->ICPU._Carry = Int32 >= 0;

		if ((st->Registers.A.W ^ Work16) & (st->Registers.A.W ^ static_cast<uint16_t>(Int32)) & 0x8000)
			SetOverflow(st);
		else
			ClearOverflow(st);

		st->Registers.A.W = static_cast<uint16_t>(Int32);
		SetZN(st, st->Registers.A.W);
	}
}

inline void SBC8(struct S9xState *st, uint8_t Work8)
{
	if (CheckDecimal(st))
	{
		uint8_t A1 = st->Registers.A.W & 0x0F;
		uint16_t A2 = st->Registers.A.W & 0xF0;
		uint8_t W1 = Work8 & 0x0F;
		uint8_t W2 = Work8 & 0xF0;

		A1 -= W1 + !CheckCarry(st);
		A2 -= W2;

		if (A1 > 0x0F)
		{
			A1 += 0x0A;
			A1 &= 0x0F;
			A2 -= 0x10;
		}

		if (A2 > 0xF0)
		{
			A2 += 0xA0;
			A2 &= 0xF0;
			ClearCarry(st);
		}
		else
			SetCarry(st);

		uint8_t Ans8 = A2 | A1;

		if ((st->Registers.A.B.l ^ Work8) & (st->Registers.A.B.l ^ Ans8) & 0x80)
			SetOverflow(st);
		else
			ClearOverflow(st);

		st->Registers.A.B.l = Ans8;
		SetZN(st, st->Registers.A.B.l);
	}
	else
	{
		int16_t Int16 = static_cast<int16_t>(st->Registers.A.B.l) - static_cast<int16_t>(Work8) + static_cast<int16_t>(CheckCarry(st)) - 1;

		st->ICPU._Carry = Int16 >= 0;

		if ((st->Registers.A.B.l ^ Work8) & (st->Registers.A.B.l ^ static_cast<uint8_t>(Int16)) & 0x80)
			SetOverflow(st);
		else
			ClearOverflow(st);

		st->Registers.A.B.l = static_cast<uint8_t>(Int16);
		SetZN(st, st->Registers.A.B.l);
	}
}

inline void STA16(struct S9xState *st, uint32_t OpAddress, s9xwrap_t w)
{
	S9xSetWord(st, st->Registers.A.W, OpAddress, w);
	st->OpenBus = st->Registers.A.B.h;
}

inline void STA8(struct S9xState *st, uint32_t OpAddress)
{
	S9xSetByte(st, st->Registers.A.B.l, OpAddress);
	st->OpenBus = st->Registers.A.B.l;
}

inline void STX16(struct S9xState *st, uint32_t OpAddress, s9xwrap_t w)
{
	S9xSetWord(st, st->Registers.X.W, OpAddress, w);
	st->OpenBus = st->Registers.X.B.h;
}

inline void STX8(struct S9xState *st, uint32_t OpAddress)
{
	S9xSetByte(st, st->Registers.X.B.l, OpAddress);
	st->OpenBus = st->Registers.X.B.l;
}

inline void STY16(struct S9xState *st, uint32_t OpAddress, s9xwrap_t w)
{
	S9xSetWord(st, st->Registers.Y.W, OpAddress, w);
	st->OpenBus = st->Registers.Y.B.h;
}

inline void STY8(struct S9xState *st, uint32_t OpAddress)
{
	S9xSetByte(st, st->Registers.Y.B.l, OpAddress);
	st->OpenBus = st->Registers.Y.B.l;
}

inline void STZ16(struct S9xState *st, uint32_t OpAddress, s9xwrap_t w)
{
	S9xSetWord(st, 0, OpAddress, w);
	st->OpenBus = 0;
}

inline void STZ8(struct S9xState *st, uint32_t OpAddress)
{
	S9xSetByte(st, 0, OpAddress);
	st->OpenBus = 0;
}

inline void TRB16(struct S9xState *st, uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t Work16 = S9xGetWord(st, OpAddress, w);
	st->ICPU._Zero = !!(Work16 & st->Registers.A.W);
	Work16 &= ~st->Registers.A.W;
	AddCycles(st, ONE_CYCLE);
	S9xSetWord(st, Work16, OpAddress, w, WRITE_10);
	st->OpenBus = Work16 & 0xff;
}

inline void TRB8(struct S9xState *st, uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(st, OpAddress);
	st->ICPU._Zero = Work8 & st->Registers.A.B.l;
	Work8 &= ~st->Registers.A.B.l;
	AddCycles(st, ONE_CYCLE);
	S9xSetByte(st, Work8, OpAddress);
	st->OpenBus = Work8;
}

inline void TSB16(struct S9xState *st, uint32_t OpAddress, s9xwrap_t w)
{
	uint16_t Work16 = S9xGetWord(st, OpAddress, w);
	st->ICPU._Zero = !!(Work16 & st->Registers.A.W);
	Work16 |= st->Registers.A.W;
	AddCycles(st, ONE_CYCLE);
	S9xSetWord(st, Work16, OpAddress, w, WRITE_10);
	st->OpenBus = Work16 & 0xff;
}

inline void TSB8(struct S9xState *st, uint32_t OpAddress)
{
	uint8_t Work8 = S9xGetByte(st, OpAddress);
	st->ICPU._Zero = Work8 & st->Registers.A.B.l;
	Work8 |= st->Registers.A.B.l;
	AddCycles(st, ONE_CYCLE);
	S9xSetByte(st, Work8, OpAddress);
	st->OpenBus = Work8;
}
