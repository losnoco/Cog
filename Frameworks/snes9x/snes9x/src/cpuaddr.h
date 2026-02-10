/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

enum AccessMode
{
	NONE = 0,
	READ = 1,
	WRITE = 2,
	MODIFY = 3,
	JUMP = 5,
	JSR = 8
};

inline uint8_t Immediate8Slow(struct S9xState *st, AccessMode a)
{
	uint8_t val = S9xGetByte(st, st->Registers.PC.xPBPC);
	if (a & READ)
		st->OpenBus = val;
	++st->Registers.PC.W.xPC;

	return val;
}

inline uint8_t Immediate8(struct S9xState *st, AccessMode a)
{
	uint8_t val = st->CPU.PCBase[st->Registers.PC.W.xPC];
	if (a & READ)
		st->OpenBus = val;
	AddCycles(st, st->CPU.MemSpeed);
	++st->Registers.PC.W.xPC;

	return val;
}

inline uint16_t Immediate16Slow(struct S9xState *st, AccessMode a)
{
	uint16_t val = S9xGetWord(st, st->Registers.PC.xPBPC, WRAP_BANK);
	if (a & READ)
		st->OpenBus = static_cast<uint8_t>(val >> 8);
	st->Registers.PC.W.xPC += 2;

	return val;
}

inline uint16_t Immediate16(struct S9xState *st, AccessMode a)
{
	uint16_t val = READ_WORD(st->CPU.PCBase + st->Registers.PC.W.xPC);
	if (a & READ)
		st->OpenBus = static_cast<uint8_t>(val >> 8);
	AddCycles(st, st->CPU.MemSpeedx2);
	st->Registers.PC.W.xPC += 2;

	return val;
}

inline uint32_t RelativeSlow(struct S9xState *st, AccessMode a) // branch $xx
{
	int8_t offset = Immediate8Slow(st, a);

	return (static_cast<int16_t>(st->Registers.PC.W.xPC) + offset) & 0xffff;
}

inline uint32_t Relative(struct S9xState *st, AccessMode a) // branch $xx
{
	int8_t offset = Immediate8(st, a);

	return (static_cast<int16_t>(st->Registers.PC.W.xPC) + offset) & 0xffff;
}

inline uint32_t RelativeLongSlow(struct S9xState *st, AccessMode a) // BRL $xxxx
{
	int16_t offset = Immediate16Slow(st, a);

	return (static_cast<int32_t>(st->Registers.PC.W.xPC) + offset) & 0xffff;
}

inline uint32_t RelativeLong(struct S9xState *st, AccessMode a) // BRL $xxxx
{
	int16_t offset = Immediate16(st, a);

	return (static_cast<int32_t>(st->Registers.PC.W.xPC) + offset) & 0xffff;
}

inline uint32_t AbsoluteIndexedIndirectSlow(struct S9xState *st, AccessMode a) // (a,X)
{
	uint16_t addr;

	if (a & JSR)
	{
		// JSR (a,X) pushes the old address in the middle of loading the new.
		// OpenBus needs to be set to account for this.
		addr = Immediate8Slow(st, READ);
		if (a == JSR)
			st->OpenBus = st->Registers.PC.B.xPCl;
		addr |= Immediate8Slow(st, READ) << 8;
	}
	else
		addr = Immediate16Slow(st, READ);

	AddCycles(st, ONE_CYCLE);
	addr += st->Registers.X.W;

	// Address load wraps within the bank
	uint16_t addr2 = S9xGetWord(st, st->ICPU.ShiftedPB | addr, WRAP_BANK);
	st->OpenBus = addr2 >> 8;

	return addr2;
}

inline uint32_t AbsoluteIndexedIndirect(struct S9xState *st, AccessMode) // (a,X)
{
	uint16_t addr = Immediate16Slow(st, READ);

	AddCycles(st, ONE_CYCLE);
	addr += st->Registers.X.W;

	// Address load wraps within the bank
	uint16_t addr2 = S9xGetWord(st, st->ICPU.ShiftedPB | addr, WRAP_BANK);
	st->OpenBus = addr2 >> 8;

	return addr2;
}

template<typename F> inline uint32_t AbsoluteIndirectLongWrapper(struct S9xState *st, F f, AccessMode)
{
	uint16_t addr = f(st, READ);

	// No info on wrapping, but it doesn't matter anyway due to mirroring
	uint32_t addr2 = S9xGetWord(st, addr);
	st->OpenBus = addr2 >> 8;
	addr2 |= (st->OpenBus = S9xGetByte(st, addr + 2)) << 16;

	return addr2;
}

inline uint32_t AbsoluteIndirectLongSlow(struct S9xState *st, AccessMode a) // [a]
{
	return AbsoluteIndirectLongWrapper(st, Immediate16Slow, a);
}

inline uint32_t AbsoluteIndirectLong(struct S9xState *st, AccessMode a) // [a]
{
	return AbsoluteIndirectLongWrapper(st, Immediate16, a);
}

template<typename F> inline uint32_t AbsoluteIndirectWrapper(struct S9xState *st, F f, AccessMode)
{
	// No info on wrapping, but it doesn't matter anyway due to mirroring
	uint16_t addr2 = S9xGetWord(st, f(st, READ));
	st->OpenBus = addr2 >> 8;

	return addr2;
}

inline uint32_t AbsoluteIndirectSlow(struct S9xState *st, AccessMode a) // (a)
{
	return AbsoluteIndirectWrapper(st, Immediate16Slow, a);
}

inline uint32_t AbsoluteIndirect(struct S9xState *st, AccessMode a) // (a)
{
	return AbsoluteIndirectWrapper(st, Immediate16, a);
}

template<typename F> inline uint32_t AbsoluteWrapper(struct S9xState *st, F f, AccessMode a)
{
	return st->ICPU.ShiftedDB | f(st, a);
}

inline uint32_t AbsoluteSlow(struct S9xState *st, AccessMode a) // a
{
	return AbsoluteWrapper(st, Immediate16Slow, a);
}

inline uint32_t Absolute(struct S9xState *st, AccessMode a) // a
{
	return AbsoluteWrapper(st, Immediate16, a);
}

inline uint32_t AbsoluteLongSlow(struct S9xState *st, AccessMode a) // l
{
	uint32_t addr = Immediate16Slow(st, READ);

	// JSR l pushes the old bank in the middle of loading the new.
	// OpenBus needs to be set to account for this.
	if (a == JSR)
		st->OpenBus = st->Registers.PC.B.xPB;

	addr |= Immediate8Slow(st, a) << 16;

	return addr;
}

inline uint32_t AbsoluteLong(struct S9xState *st, AccessMode a) // l
{
	uint32_t addr = READ_3WORD(st->CPU.PCBase + st->Registers.PC.W.xPC);
	AddCycles(st, st->CPU.MemSpeedx2 + st->CPU.MemSpeed);
	if (a & READ)
		st->OpenBus = addr >> 16;
	st->Registers.PC.W.xPC += 3;

	return addr;
}

template<typename F> inline uint32_t DirectWrapper(struct S9xState *st, F f, AccessMode a)
{
	uint16_t addr = f(st, a) + st->Registers.D.W;
	if (st->Registers.D.B.l)
		AddCycles(st, ONE_CYCLE);

	return addr;
}

inline uint32_t DirectSlow(struct S9xState *st, AccessMode a) // d
{
	return DirectWrapper(st, Immediate8Slow, a);
}

inline uint32_t Direct(struct S9xState *st, AccessMode a) // d
{
	return DirectWrapper(st, Immediate8, a);
}

inline uint32_t DirectIndirectSlow(struct S9xState *st, AccessMode a) // (d)
{
	uint32_t addr = S9xGetWord(st, DirectSlow(st, READ), !CheckEmulation(st) || st->Registers.D.B.l ? WRAP_BANK : WRAP_PAGE);
	if (a & READ)
		st->OpenBus = static_cast<uint8_t>(addr >> 8);
	addr |= st->ICPU.ShiftedDB;

	return addr;
}

inline uint32_t DirectIndirectE0(struct S9xState *st, AccessMode a) // (d)
{
	uint32_t addr = S9xGetWord(st, Direct(st, READ));
	if (a & READ)
		st->OpenBus = static_cast<uint8_t>(addr >> 8);
	addr |= st->ICPU.ShiftedDB;

	return addr;
}

inline uint32_t DirectIndirectE1(struct S9xState *st, AccessMode a) // (d)
{
	uint32_t addr = S9xGetWord(st, DirectSlow(st, READ), st->Registers.D.B.l ? WRAP_BANK : WRAP_PAGE);
	if (a & READ)
		st->OpenBus = static_cast<uint8_t>(addr >> 8);
	addr |= st->ICPU.ShiftedDB;

	return addr;
}

inline uint32_t DirectIndirectIndexedSlow(struct S9xState *st, AccessMode a) // (d),Y
{
	uint32_t addr = DirectIndirectSlow(st, a);
	if ((a & WRITE) || !CheckIndex(st) || (addr & 0xff) + st->Registers.Y.B.l >= 0x100)
		AddCycles(st, ONE_CYCLE);

	return addr + st->Registers.Y.W;
}

inline uint32_t DirectIndirectIndexedE0X0(struct S9xState *st, AccessMode a) // (d),Y
{
	uint32_t addr = DirectIndirectE0(st, a);
	AddCycles(st, ONE_CYCLE);

	return addr + st->Registers.Y.W;
}

inline uint32_t DirectIndirectIndexedE0X1(struct S9xState *st, AccessMode a) // (d),Y
{
	uint32_t addr = DirectIndirectE0(st, a);
	if ((a & WRITE) || (addr & 0xff) + st->Registers.Y.B.l >= 0x100)
		AddCycles(st, ONE_CYCLE);

	return addr + st->Registers.Y.W;
}

inline uint32_t DirectIndirectIndexedE1(struct S9xState *st, AccessMode a) // (d),Y
{
	uint32_t addr = DirectIndirectE1(st, a);
	if ((a & WRITE) || (addr & 0xff) + st->Registers.Y.B.l >= 0x100)
		AddCycles(st, ONE_CYCLE);

	return addr + st->Registers.Y.W;
}

template<typename F> inline uint32_t DirectIndirectLongWrapper(struct S9xState *st, F f, AccessMode)
{
	uint16_t addr = f(st, READ);
	uint32_t addr2 = S9xGetWord(st, addr);
	st->OpenBus = addr2 >> 8;
	addr2 |= (st->OpenBus = S9xGetByte(st, addr + 2)) << 16;

	return addr2;
}

inline uint32_t DirectIndirectLongSlow(struct S9xState *st, AccessMode a) // [d]
{
	return DirectIndirectLongWrapper(st, DirectSlow, a);
}

inline uint32_t DirectIndirectLong(struct S9xState *st, AccessMode a) // [d]
{
	return DirectIndirectLongWrapper(st, Direct, a);
}

template<typename F> inline uint32_t DirectIndirectIndexedLongWrapper(struct S9xState *st, F f, AccessMode a)
{
	return f(st, a) + st->Registers.Y.W;
}

inline uint32_t DirectIndirectIndexedLongSlow(struct S9xState *st, AccessMode a) // [d],Y
{
	return DirectIndirectIndexedLongWrapper(st, DirectIndirectLongSlow, a);
}

inline uint32_t DirectIndirectIndexedLong(struct S9xState *st, AccessMode a) // [d],Y
{
	return DirectIndirectIndexedLongWrapper(st, DirectIndirectLong, a);
}

inline uint32_t DirectIndexedXSlow(struct S9xState *st, AccessMode a) // d,X
{
	pair addr;
	addr.W = DirectSlow(st, a);
	if (!CheckEmulation(st) || st->Registers.D.B.l)
		addr.W += st->Registers.X.W;
	else
		addr.B.l += st->Registers.X.B.l;

	AddCycles(st, ONE_CYCLE);

	return addr.W;
}

inline uint32_t DirectIndexedXE0(struct S9xState *st, AccessMode a) // d,X
{
	uint16_t addr = Direct(st, a) + st->Registers.X.W;
	AddCycles(st, ONE_CYCLE);

	return addr;
}

inline uint32_t DirectIndexedXE1(struct S9xState *st, AccessMode a) // d,X
{
	if (st->Registers.D.B.l)
		return DirectIndexedXE0(st, a);
	else
	{
		pair addr;
		addr.W = Direct(st, a);
		addr.B.l += st->Registers.X.B.l;
		AddCycles(st, ONE_CYCLE);

		return addr.W;
	}
}

inline uint32_t DirectIndexedYSlow(struct S9xState *st, AccessMode a) // d,Y
{
	pair addr;
	addr.W = DirectSlow(st, a);
	if (!CheckEmulation(st) || st->Registers.D.B.l)
		addr.W += st->Registers.Y.W;
	else
		addr.B.l += st->Registers.Y.B.l;

	AddCycles(st, ONE_CYCLE);

	return addr.W;
}

inline uint32_t DirectIndexedYE0(struct S9xState *st, AccessMode a) // d,Y
{
	uint16_t addr = Direct(st, a) + st->Registers.Y.W;
	AddCycles(st, ONE_CYCLE);

	return addr;
}

inline uint32_t DirectIndexedYE1(struct S9xState *st, AccessMode a) // d,Y
{
	if (st->Registers.D.B.l)
		return DirectIndexedYE0(st, a);
	else
	{
		pair addr;
		addr.W = Direct(st, a);
		addr.B.l += st->Registers.Y.B.l;
		AddCycles(st, ONE_CYCLE);

		return addr.W;
	}
}

inline uint32_t DirectIndexedIndirectSlow(struct S9xState *st, AccessMode a) // (d,X)
{
	uint32_t addr = S9xGetWord(st, DirectIndexedXSlow(st, READ), !CheckEmulation(st) || st->Registers.D.B.l ? WRAP_BANK : WRAP_PAGE);
	if (a & READ)
		st->OpenBus = static_cast<uint8_t>(addr >> 8);

	return st->ICPU.ShiftedDB | addr;
}

inline uint32_t DirectIndexedIndirectE0(struct S9xState *st, AccessMode a) // (d,X)
{
	uint32_t addr = S9xGetWord(st, DirectIndexedXE0(st, READ));
	if (a & READ)
		st->OpenBus = static_cast<uint8_t>(addr >> 8);

	return st->ICPU.ShiftedDB | addr;
}

inline uint32_t DirectIndexedIndirectE1(struct S9xState *st, AccessMode a) // (d,X)
{
	uint32_t addr = S9xGetWord(st, DirectIndexedXE1(st, READ), st->Registers.D.B.l ? WRAP_BANK : WRAP_PAGE);
	if (a & READ)
		st->OpenBus = static_cast<uint8_t>(addr >> 8);

	return st->ICPU.ShiftedDB | addr;
}

inline uint32_t AbsoluteIndexedXSlow(struct S9xState *st, AccessMode a) // a,X
{
	uint32_t addr = AbsoluteSlow(st, a);
	if ((a & WRITE) || !CheckIndex(st) || (addr & 0xff) + st->Registers.X.B.l >= 0x100)
		AddCycles(st, ONE_CYCLE);

	return addr + st->Registers.X.W;
}

inline uint32_t AbsoluteIndexedXX0(struct S9xState *st, AccessMode a) // a,X
{
	uint32_t addr = Absolute(st, a);
	AddCycles(st, ONE_CYCLE);

	return addr + st->Registers.X.W;
}

inline uint32_t AbsoluteIndexedXX1(struct S9xState *st, AccessMode a) // a,X
{
	uint32_t addr = Absolute(st, a);
	if ((a & WRITE) || (addr & 0xff) + st->Registers.X.B.l >= 0x100)
		AddCycles(st, ONE_CYCLE);

	return addr + st->Registers.X.W;
}

inline uint32_t AbsoluteIndexedYSlow(struct S9xState *st, AccessMode a) // a,Y
{
	uint32_t addr = AbsoluteSlow(st, a);
	if ((a & WRITE) || !CheckIndex(st) || (addr & 0xff) + st->Registers.Y.B.l >= 0x100)
		AddCycles(st, ONE_CYCLE);

	return addr + st->Registers.Y.W;
}

inline uint32_t AbsoluteIndexedYX0(struct S9xState *st, AccessMode a) // a,Y
{
	uint32_t addr = Absolute(st, a);
	AddCycles(st, ONE_CYCLE);

	return addr + st->Registers.Y.W;
}

inline uint32_t AbsoluteIndexedYX1(struct S9xState *st, AccessMode a) // a,Y
{
	uint32_t addr = Absolute(st, a);
	if ((a & WRITE) || (addr & 0xff) + st->Registers.Y.B.l >= 0x100)
		AddCycles(st, ONE_CYCLE);

	return addr + st->Registers.Y.W;
}

template<typename F> inline uint32_t AbsoluteLongIndexedXWrapper(struct S9xState *st, F f, AccessMode a)
{
	return f(st, a) + st->Registers.X.W;
}

inline uint32_t AbsoluteLongIndexedXSlow(struct S9xState *st, AccessMode a) // l,X
{
	return AbsoluteLongIndexedXWrapper(st, AbsoluteLongSlow, a);
}

inline uint32_t AbsoluteLongIndexedX(struct S9xState *st, AccessMode a) // l,X
{
	return AbsoluteLongIndexedXWrapper(st, AbsoluteLong, a);
}

template<typename F> inline uint32_t StackRelativeWrapper(struct S9xState *st, F f, AccessMode a)
{
	uint16_t addr = f(st, a) + st->Registers.S.W;
	AddCycles(st, ONE_CYCLE);

	return addr;
}

inline uint32_t StackRelativeSlow(struct S9xState *st, AccessMode a) // d,S
{
	return StackRelativeWrapper(st, Immediate8Slow, a);
}

inline uint32_t StackRelative(struct S9xState *st, AccessMode a) // d,S
{
	return StackRelativeWrapper(st, Immediate8, a);
}

template<typename F> inline uint32_t StackRelativeIndirectIndexedWrapper(struct S9xState *st, F f, AccessMode a)
{
	uint32_t addr = S9xGetWord(st, f(st, READ));
	if (a & READ)
		st->OpenBus = static_cast<uint8_t>(addr >> 8);
	addr = (addr + st->Registers.Y.W + st->ICPU.ShiftedDB) & 0xffffff;
	AddCycles(st, ONE_CYCLE);

	return addr;
}

inline uint32_t StackRelativeIndirectIndexedSlow(struct S9xState *st, AccessMode a) // (d,S),Y
{
	return StackRelativeIndirectIndexedWrapper(st, StackRelativeSlow, a);
}

inline uint32_t StackRelativeIndirectIndexed(struct S9xState *st, AccessMode a) // (d,S),Y
{
	return StackRelativeIndirectIndexedWrapper(st, StackRelative, a);
}
