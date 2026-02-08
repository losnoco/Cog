/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

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
		uint32_t carry = CheckCarry(st);

		uint32_t result = (st->Registers.A.W & 0x000F) + (Work16 & 0x000F) + carry;
		if (result > 0x0009)
			result += 0x0006;
		carry = result > 0x000F;

		result = (st->Registers.A.W & 0x00F0) + (Work16 & 0x00F0) + (result & 0x000F) + carry * 0x10;
		if (result > 0x009F)
			result += 0x0060;
		carry = result > 0x00FF;

		result = (st->Registers.A.W & 0x0F00) + (Work16 & 0x0F00) + (result & 0x00FF) + carry * 0x100;
		if (result > 0x09FF)
			result += 0x0600;
		carry = result > 0x0FFF;

		result = (st->Registers.A.W & 0xF000) + (Work16 & 0xF000) + (result & 0x0FFF) + carry * 0x1000;

		if ((st->Registers.A.W & 0x8000) == (Work16 & 0x8000) && (st->Registers.A.W & 0x8000) != (result & 0x8000))
			SetOverflow(st);
		else
			ClearOverflow(st);

		if (result > 0x9FFF)
			result += 0x6000;

		if (result > 0xFFFF)
			SetCarry(st);
		else
			ClearCarry(st);

		st->Registers.A.W = result & 0xFFFF;
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
		uint32_t carry = CheckCarry(st);

		uint32_t result = (st->Registers.A.B.l & 0x0F) + (Work8 & 0x0F) + carry;
		if (result > 0x09)
			result += 0x06;
		carry = result > 0x0F;

		result = (st->Registers.A.B.l & 0xF0) + (Work8 & 0xF0) + (result & 0x0F) + (carry * 0x10);

		if ((st->Registers.A.B.l & 0x80) == (Work8 & 0x80) && (st->Registers.A.B.l & 0x80) != (result & 0x80))
			SetOverflow(st);
		else
			ClearOverflow(st);

		if (result > 0x9F)
			result += 0x60;

		if (result > 0xFF)
			SetCarry(st);
		else
			ClearCarry(st);

		st->Registers.A.B.l = result & 0xFF;
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
