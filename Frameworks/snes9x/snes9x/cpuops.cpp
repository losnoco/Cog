/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include <snes9x/snes9x.h>

#include <snes9x/snes.hpp>
#include <snes9x/smp.hpp>
#include <snes9x/sdsp.hpp>

static inline void AddCycles(struct S9xState *st, int32_t n) { st->CPU.Cycles += n; while (st->CPU.Cycles >= st->CPU.NextEvent) S9xDoHEventProcessing(st); }

#include "cpuaddr.h"
#include "cpuops.h"
#include "cpumacro.h"

/* ADC ********************************************************************* */

template<typename T> static inline void ADC(struct S9xState *, T)
{
}

template<> inline void ADC<uint16_t>(struct S9xState *st, uint16_t Work16)
{
	ADC16(st, Work16);
}

template<> inline void ADC<uint8_t>(struct S9xState *st, uint8_t Work8)
{
	ADC8(st, Work8);
}

template<typename F> static inline void Op69Wrapper(struct S9xState *st, F f)
{
	ADC(st, f(st, READ));
}

static void Op69M1(struct S9xState *st)
{
	Op69Wrapper(st, Immediate8);
}

static void Op69M0(struct S9xState *st)
{
	Op69Wrapper(st, Immediate16);
}

static void Op69Slow(struct S9xState *st)
{
	if (CheckMemory(st))
		Op69Wrapper(st, Immediate8Slow);
	else
		Op69Wrapper(st, Immediate16Slow);
}

static void Op65M1(struct S9xState *st) { rOP8(st, Direct, ADC8); }
static void Op65M0(struct S9xState *st) { rOP16(st, Direct, ADC16, WRAP_BANK); }
static void Op65Slow(struct S9xState *st) { rOPM(st, DirectSlow, ADC8, ADC16, WRAP_BANK); }

static void Op75E1(struct S9xState *st) { rOP8(st, DirectIndexedXE1, ADC8); }
static void Op75E0M1(struct S9xState *st) { rOP8(st, DirectIndexedXE0, ADC8); }
static void Op75E0M0(struct S9xState *st) { rOP16(st, DirectIndexedXE0, ADC16, WRAP_BANK); }
static void Op75Slow(struct S9xState *st) { rOPM(st, DirectIndexedXSlow, ADC8, ADC16, WRAP_BANK); }

static void Op72E1(struct S9xState *st) { rOP8(st, DirectIndirectE1, ADC8); }
static void Op72E0M1(struct S9xState *st) { rOP8(st, DirectIndirectE0, ADC8); }
static void Op72E0M0(struct S9xState *st) { rOP16(st, DirectIndirectE0, ADC16); }
static void Op72Slow(struct S9xState *st) { rOPM(st, DirectIndirectSlow, ADC8, ADC16); }

static void Op61E1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE1, ADC8); }
static void Op61E0M1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE0, ADC8); }
static void Op61E0M0(struct S9xState *st) { rOP16(st, DirectIndexedIndirectE0, ADC16); }
static void Op61Slow(struct S9xState *st) { rOPM(st, DirectIndexedIndirectSlow, ADC8, ADC16); }

static void Op71E1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE1, ADC8); }
static void Op71E0M1X1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X1, ADC8); }
static void Op71E0M0X1(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X1, ADC16); }
static void Op71E0M1X0(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X0, ADC8); }
static void Op71E0M0X0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X0, ADC16); }
static void Op71Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedSlow, ADC8, ADC16); }

static void Op67M1(struct S9xState *st) { rOP8(st, DirectIndirectLong, ADC8); }
static void Op67M0(struct S9xState *st) { rOP16(st, DirectIndirectLong, ADC16); }
static void Op67Slow(struct S9xState *st) { rOPM(st, DirectIndirectLongSlow, ADC8, ADC16); }

static void Op77M1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedLong, ADC8); }
static void Op77M0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedLong, ADC16); }
static void Op77Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedLongSlow, ADC8, ADC16); }

static void Op6DM1(struct S9xState *st) { rOP8(st, Absolute, ADC8); }
static void Op6DM0(struct S9xState *st) { rOP16(st, Absolute, ADC16); }
static void Op6DSlow(struct S9xState *st) { rOPM(st, AbsoluteSlow, ADC8, ADC16); }

static void Op7DM1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX1, ADC8); }
static void Op7DM0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX1, ADC16); }
static void Op7DM1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX0, ADC8); }
static void Op7DM0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX0, ADC16); }
static void Op7DSlow(struct S9xState *st) { rOPM(st, AbsoluteIndexedXSlow, ADC8, ADC16); }

static void Op79M1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX1, ADC8); }
static void Op79M0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX1, ADC16); }
static void Op79M1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX0, ADC8); }
static void Op79M0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX0, ADC16); }
static void Op79Slow(struct S9xState *st) { rOPM(st, AbsoluteIndexedYSlow, ADC8, ADC16); }

static void Op6FM1(struct S9xState *st) { rOP8(st, AbsoluteLong, ADC8); }
static void Op6FM0(struct S9xState *st) { rOP16(st, AbsoluteLong, ADC16); }
static void Op6FSlow(struct S9xState *st) { rOPM(st, AbsoluteLongSlow, ADC8, ADC16); }

static void Op7FM1(struct S9xState *st) { rOP8(st, AbsoluteLongIndexedX, ADC8); }
static void Op7FM0(struct S9xState *st) { rOP16(st, AbsoluteLongIndexedX, ADC16); }
static void Op7FSlow(struct S9xState *st) { rOPM(st, AbsoluteLongIndexedXSlow, ADC8, ADC16); }

static void Op63M1(struct S9xState *st) { rOP8(st, StackRelative, ADC8); }
static void Op63M0(struct S9xState *st) { rOP16(st, StackRelative, ADC16); }
static void Op63Slow(struct S9xState *st) { rOPM(st, StackRelativeSlow, ADC8, ADC16); }

static void Op73M1(struct S9xState *st) { rOP8(st, StackRelativeIndirectIndexed, ADC8); }
static void Op73M0(struct S9xState *st) { rOP16(st, StackRelativeIndirectIndexed, ADC16); }
static void Op73Slow(struct S9xState *st) { rOPM(st, StackRelativeIndirectIndexedSlow, ADC8, ADC16); }

/* AND ********************************************************************* */

template<typename T, typename F> static inline void Op29Wrapper(struct S9xState *st, T &reg, F f)
{
	reg &= f(st, READ);
	SetZN(st, reg);
}

static void Op29M1(struct S9xState *st)
{
	Op29Wrapper(st, st->Registers.A.B.l, Immediate8);
}

static void Op29M0(struct S9xState *st)
{
	Op29Wrapper(st, st->Registers.A.W, Immediate16);
}

static void Op29Slow(struct S9xState *st)
{
	if (CheckMemory(st))
		Op29Wrapper(st, st->Registers.A.B.l, Immediate8Slow);
	else
		Op29Wrapper(st, st->Registers.A.W, Immediate16Slow);
}

static void Op25M1(struct S9xState *st) { rOP8(st, Direct, AND8); }
static void Op25M0(struct S9xState *st) { rOP16(st, Direct, AND16, WRAP_BANK); }
static void Op25Slow(struct S9xState *st) { rOPM(st, DirectSlow, AND8, AND16, WRAP_BANK); }

static void Op35E1(struct S9xState *st) { rOP8(st, DirectIndexedXE1, AND8); }
static void Op35E0M1(struct S9xState *st) { rOP8(st, DirectIndexedXE0, AND8); }
static void Op35E0M0(struct S9xState *st) { rOP16(st, DirectIndexedXE0, AND16, WRAP_BANK); }
static void Op35Slow(struct S9xState *st) { rOPM(st, DirectIndexedXSlow, AND8, AND16, WRAP_BANK); }

static void Op32E1(struct S9xState *st) { rOP8(st, DirectIndirectE1, AND8); }
static void Op32E0M1(struct S9xState *st) { rOP8(st, DirectIndirectE0, AND8); }
static void Op32E0M0(struct S9xState *st) { rOP16(st, DirectIndirectE0, AND16); }
static void Op32Slow(struct S9xState *st) { rOPM(st, DirectIndirectSlow, AND8, AND16); }

static void Op21E1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE1, AND8); }
static void Op21E0M1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE0, AND8); }
static void Op21E0M0(struct S9xState *st) { rOP16(st, DirectIndexedIndirectE0, AND16); }
static void Op21Slow(struct S9xState *st) { rOPM(st, DirectIndexedIndirectSlow, AND8, AND16); }

static void Op31E1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE1, AND8); }
static void Op31E0M1X1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X1, AND8); }
static void Op31E0M0X1(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X1, AND16); }
static void Op31E0M1X0(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X0, AND8); }
static void Op31E0M0X0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X0, AND16); }
static void Op31Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedSlow, AND8, AND16); }

static void Op27M1(struct S9xState *st) { rOP8(st, DirectIndirectLong, AND8); }
static void Op27M0(struct S9xState *st) { rOP16(st, DirectIndirectLong, AND16); }
static void Op27Slow(struct S9xState *st) { rOPM(st, DirectIndirectLongSlow, AND8, AND16); }

static void Op37M1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedLong, AND8); }
static void Op37M0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedLong, AND16); }
static void Op37Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedLongSlow, AND8, AND16); }

static void Op2DM1(struct S9xState *st) { rOP8(st, Absolute, AND8); }
static void Op2DM0(struct S9xState *st) { rOP16(st, Absolute, AND16); }
static void Op2DSlow(struct S9xState *st) { rOPM(st, AbsoluteSlow, AND8, AND16); }

static void Op3DM1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX1, AND8); }
static void Op3DM0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX1, AND16); }
static void Op3DM1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX0, AND8); }
static void Op3DM0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX0, AND16); }
static void Op3DSlow(struct S9xState *st) { rOPM(st, AbsoluteIndexedXSlow, AND8, AND16); }

static void Op39M1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX1, AND8); }
static void Op39M0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX1, AND16); }
static void Op39M1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX0, AND8); }
static void Op39M0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX0, AND16); }
static void Op39Slow(struct S9xState *st) { rOPM(st, AbsoluteIndexedYSlow, AND8, AND16); }

static void Op2FM1(struct S9xState *st) { rOP8(st, AbsoluteLong, AND8); }
static void Op2FM0(struct S9xState *st) { rOP16(st, AbsoluteLong, AND16); }
static void Op2FSlow(struct S9xState *st) { rOPM(st, AbsoluteLongSlow, AND8, AND16); }

static void Op3FM1(struct S9xState *st) { rOP8(st, AbsoluteLongIndexedX, AND8); }
static void Op3FM0(struct S9xState *st) { rOP16(st, AbsoluteLongIndexedX, AND16); }
static void Op3FSlow(struct S9xState *st) { rOPM(st, AbsoluteLongIndexedXSlow, AND8, AND16); }

static void Op23M1(struct S9xState *st) { rOP8(st, StackRelative, AND8); }
static void Op23M0(struct S9xState *st) { rOP16(st, StackRelative, AND16); }
static void Op23Slow(struct S9xState *st) { rOPM(st, StackRelativeSlow, AND8, AND16); }

static void Op33M1(struct S9xState *st) { rOP8(st, StackRelativeIndirectIndexed, AND8); }
static void Op33M0(struct S9xState *st) { rOP16(st, StackRelativeIndirectIndexed, AND16); }
static void Op33Slow(struct S9xState *st) { rOPM(st, StackRelativeIndirectIndexedSlow, AND8, AND16); }

/* ASL ********************************************************************* */

static inline void Op0AM1(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	st->ICPU._Carry = !!(st->Registers.A.B.l & 0x80);
	st->Registers.A.B.l <<= 1;
	SetZN(st, st->Registers.A.B.l);
}

static inline void Op0AM0(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	st->ICPU._Carry = !!(st->Registers.A.B.h & 0x80);
	st->Registers.A.W <<= 1;
	SetZN(st, st->Registers.A.W);
}

static void Op0ASlow(struct S9xState *st)
{
	if (CheckMemory(st))
		Op0AM1(st);
	else
		Op0AM0(st);
}

static void Op06M1(struct S9xState *st) { mOP8(st, Direct, ASL8); }
static void Op06M0(struct S9xState *st) { mOP16(st, Direct, ASL16, WRAP_BANK); }
static void Op06Slow(struct S9xState *st) { mOPM(st, DirectSlow, ASL8, ASL16, WRAP_BANK); }

static void Op16E1(struct S9xState *st) { mOP8(st, DirectIndexedXE1, ASL8); }
static void Op16E0M1(struct S9xState *st) { mOP8(st, DirectIndexedXE0, ASL8); }
static void Op16E0M0(struct S9xState *st) { mOP16(st, DirectIndexedXE0, ASL16, WRAP_BANK); }
static void Op16Slow(struct S9xState *st) { mOPM(st, DirectIndexedXSlow, ASL8, ASL16, WRAP_BANK); }

static void Op0EM1(struct S9xState *st) { mOP8(st, Absolute, ASL8); }
static void Op0EM0(struct S9xState *st) { mOP16(st, Absolute, ASL16); }
static void Op0ESlow(struct S9xState *st) { mOPM(st, AbsoluteSlow, ASL8, ASL16); }

static void Op1EM1X1(struct S9xState *st) { mOP8(st, AbsoluteIndexedXX1, ASL8); }
static void Op1EM0X1(struct S9xState *st) { mOP16(st, AbsoluteIndexedXX1, ASL16); }
static void Op1EM1X0(struct S9xState *st) { mOP8(st, AbsoluteIndexedXX0, ASL8); }
static void Op1EM0X0(struct S9xState *st) { mOP16(st, AbsoluteIndexedXX0, ASL16); }
static void Op1ESlow(struct S9xState *st) { mOPM(st, AbsoluteIndexedXSlow, ASL8, ASL16); }

/* BIT ********************************************************************* */

template<typename T, typename F> static inline void Op89Wrapper(struct S9xState *st, const T &reg, F f)
{
	st->ICPU._Zero = !!(reg & f(st, READ));
}

static void Op89M1(struct S9xState *st)
{
	Op89Wrapper(st, st->Registers.A.B.l, Immediate8);
}

static void Op89M0(struct S9xState *st)
{
	Op89Wrapper(st, st->Registers.A.W, Immediate16);
}

static void Op89Slow(struct S9xState *st)
{
	if (CheckMemory(st))
		Op89Wrapper(st, st->Registers.A.B.l, Immediate8Slow);
	else
		Op89Wrapper(st, st->Registers.A.W, Immediate16Slow);
}

static void Op24M1(struct S9xState *st) { rOP8(st, Direct, BIT8); }
static void Op24M0(struct S9xState *st) { rOP16(st, Direct, BIT16, WRAP_BANK); }
static void Op24Slow(struct S9xState *st) { rOPM(st, DirectSlow, BIT8, BIT16, WRAP_BANK); }

static void Op34E1(struct S9xState *st) { rOP8(st, DirectIndexedXE1, BIT8); }
static void Op34E0M1(struct S9xState *st) { rOP8(st, DirectIndexedXE0, BIT8); }
static void Op34E0M0(struct S9xState *st) { rOP16(st, DirectIndexedXE0, BIT16, WRAP_BANK); }
static void Op34Slow(struct S9xState *st) { rOPM(st, DirectIndexedXSlow, BIT8, BIT16, WRAP_BANK); }

static void Op2CM1(struct S9xState *st) { rOP8(st, Absolute, BIT8); }
static void Op2CM0(struct S9xState *st) { rOP16(st, Absolute, BIT16); }
static void Op2CSlow(struct S9xState *st) { rOPM(st, AbsoluteSlow, BIT8, BIT16); }

static void Op3CM1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX1, BIT8); }
static void Op3CM0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX1, BIT16); }
static void Op3CM1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX0, BIT8); }
static void Op3CM0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX0, BIT16); }
static void Op3CSlow(struct S9xState *st) { rOPM(st, AbsoluteIndexedXSlow, BIT8, BIT16); }

/* CMP ********************************************************************* */

template<typename Tint, typename Treg, typename F> static inline void CxPxWrapper(struct S9xState *st, const Treg &reg, F f)
{
	Tint intVal = static_cast<Tint>(reg) - static_cast<Tint>(f(st, READ));
	st->ICPU._Carry = intVal >= 0;
	SetZN(st, static_cast<Treg>(intVal));
}

static void OpC9M1(struct S9xState *st)
{
	CxPxWrapper<int16_t>(st, st->Registers.A.B.l, Immediate8);
}

static void OpC9M0(struct S9xState *st)
{
	CxPxWrapper<int32_t>(st, st->Registers.A.W, Immediate16);
}

static void OpC9Slow(struct S9xState *st)
{
	if (CheckMemory(st))
		CxPxWrapper<int16_t>(st, st->Registers.A.B.l, Immediate8Slow);
	else
		CxPxWrapper<int32_t>(st, st->Registers.A.W, Immediate16Slow);
}

static void OpC5M1(struct S9xState *st) { rOP8(st, Direct, CMP8); }
static void OpC5M0(struct S9xState *st) { rOP16(st, Direct, CMP16, WRAP_BANK); }
static void OpC5Slow(struct S9xState *st) { rOPM(st, DirectSlow, CMP8, CMP16, WRAP_BANK); }

static void OpD5E1(struct S9xState *st) { rOP8(st, DirectIndexedXE1, CMP8); }
static void OpD5E0M1(struct S9xState *st) { rOP8(st, DirectIndexedXE0, CMP8); }
static void OpD5E0M0(struct S9xState *st) { rOP16(st, DirectIndexedXE0, CMP16, WRAP_BANK); }
static void OpD5Slow(struct S9xState *st) { rOPM(st, DirectIndexedXSlow, CMP8, CMP16, WRAP_BANK); }

static void OpD2E1(struct S9xState *st) { rOP8(st, DirectIndirectE1, CMP8); }
static void OpD2E0M1(struct S9xState *st) { rOP8(st, DirectIndirectE0, CMP8); }
static void OpD2E0M0(struct S9xState *st) { rOP16(st, DirectIndirectE0, CMP16); }
static void OpD2Slow(struct S9xState *st) { rOPM(st, DirectIndirectSlow, CMP8, CMP16); }

static void OpC1E1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE1, CMP8); }
static void OpC1E0M1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE0, CMP8); }
static void OpC1E0M0(struct S9xState *st) { rOP16(st, DirectIndexedIndirectE0, CMP16); }
static void OpC1Slow(struct S9xState *st) { rOPM(st, DirectIndexedIndirectSlow, CMP8, CMP16); }

static void OpD1E1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE1, CMP8); }
static void OpD1E0M1X1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X1, CMP8); }
static void OpD1E0M0X1(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X1, CMP16); }
static void OpD1E0M1X0(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X0, CMP8); }
static void OpD1E0M0X0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X0, CMP16); }
static void OpD1Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedSlow, CMP8, CMP16); }

static void OpC7M1(struct S9xState *st) { rOP8(st, DirectIndirectLong, CMP8); }
static void OpC7M0(struct S9xState *st) { rOP16(st, DirectIndirectLong, CMP16); }
static void OpC7Slow(struct S9xState *st) { rOPM(st, DirectIndirectLongSlow, CMP8, CMP16); }

static void OpD7M1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedLong, CMP8); }
static void OpD7M0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedLong, CMP16); }
static void OpD7Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedLongSlow, CMP8, CMP16); }

static void OpCDM1(struct S9xState *st) { rOP8(st, Absolute, CMP8); }
static void OpCDM0(struct S9xState *st) { rOP16(st, Absolute, CMP16); }
static void OpCDSlow(struct S9xState *st) { rOPM(st, AbsoluteSlow, CMP8, CMP16); }

static void OpDDM1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX1, CMP8); }
static void OpDDM0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX1, CMP16); }
static void OpDDM1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX0, CMP8); }
static void OpDDM0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX0, CMP16); }
static void OpDDSlow(struct S9xState *st) { rOPM(st, AbsoluteIndexedXSlow, CMP8, CMP16); }

static void OpD9M1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX1, CMP8); }
static void OpD9M0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX1, CMP16); }
static void OpD9M1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX0, CMP8); }
static void OpD9M0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX0, CMP16); }
static void OpD9Slow(struct S9xState *st) { rOPM(st, AbsoluteIndexedYSlow, CMP8, CMP16); }

static void OpCFM1(struct S9xState *st) { rOP8(st, AbsoluteLong, CMP8); }
static void OpCFM0(struct S9xState *st) { rOP16(st, AbsoluteLong, CMP16); }
static void OpCFSlow(struct S9xState *st) { rOPM(st, AbsoluteLongSlow, CMP8, CMP16); }

static void OpDFM1(struct S9xState *st) { rOP8(st, AbsoluteLongIndexedX, CMP8); }
static void OpDFM0(struct S9xState *st) { rOP16(st, AbsoluteLongIndexedX, CMP16); }
static void OpDFSlow(struct S9xState *st) { rOPM(st, AbsoluteLongIndexedXSlow, CMP8, CMP16); }

static void OpC3M1(struct S9xState *st) { rOP8(st, StackRelative, CMP8); }
static void OpC3M0(struct S9xState *st) { rOP16(st, StackRelative, CMP16); }
static void OpC3Slow(struct S9xState *st) { rOPM(st, StackRelativeSlow, CMP8, CMP16); }

static void OpD3M1(struct S9xState *st) { rOP8(st, StackRelativeIndirectIndexed, CMP8); }
static void OpD3M0(struct S9xState *st) { rOP16(st, StackRelativeIndirectIndexed, CMP16); }
static void OpD3Slow(struct S9xState *st) { rOPM(st, StackRelativeIndirectIndexedSlow, CMP8, CMP16); }

/* CPX ********************************************************************* */

static void OpE0X1(struct S9xState *st)
{
	CxPxWrapper<int16_t>(st, st->Registers.X.B.l, Immediate8);
}

static void OpE0X0(struct S9xState *st)
{
	CxPxWrapper<int32_t>(st, st->Registers.X.W, Immediate16);
}

static void OpE0Slow(struct S9xState *st)
{
	if (CheckIndex(st))
		CxPxWrapper<int16_t>(st, st->Registers.X.B.l, Immediate8Slow);
	else
		CxPxWrapper<int32_t>(st, st->Registers.X.W, Immediate16Slow);
}

static void OpE4X1(struct S9xState *st) { rOP8(st, Direct, CPX8); }
static void OpE4X0(struct S9xState *st) { rOP16(st, Direct, CPX16, WRAP_BANK); }
static void OpE4Slow(struct S9xState *st) { rOPX(st, DirectSlow, CPX8, CPX16, WRAP_BANK); }

static void OpECX1(struct S9xState *st) { rOP8(st, Absolute, CPX8); }
static void OpECX0(struct S9xState *st) { rOP16(st, Absolute, CPX16); }
static void OpECSlow(struct S9xState *st) { rOPX(st, AbsoluteSlow, CPX8, CPX16); }

/* CPY ********************************************************************* */

static void OpC0X1(struct S9xState *st)
{
	CxPxWrapper<int16_t>(st, st->Registers.Y.B.l, Immediate8);
}

static void OpC0X0(struct S9xState *st)
{
	CxPxWrapper<int32_t>(st, st->Registers.Y.W, Immediate16);
}

static void OpC0Slow(struct S9xState *st)
{
	if (CheckIndex(st))
		CxPxWrapper<int16_t>(st, st->Registers.Y.B.l, Immediate8Slow);
	else
		CxPxWrapper<int32_t>(st, st->Registers.Y.W, Immediate16Slow);
}

static void OpC4X1(struct S9xState *st) { rOP8(st, Direct, CPY8); }
static void OpC4X0(struct S9xState *st) { rOP16(st, Direct, CPY16, WRAP_BANK); }
static void OpC4Slow(struct S9xState *st) { rOPX(st, DirectSlow, CPY8, CPY16, WRAP_BANK); }

static void OpCCX1(struct S9xState *st) { rOP8(st, Absolute, CPY8); }
static void OpCCX0(struct S9xState *st) { rOP16(st, Absolute, CPY16); }
static void OpCCSlow(struct S9xState *st) { rOPX(st, AbsoluteSlow, CPY8, CPY16); }

/* DEC ********************************************************************* */

static inline void Op3AM1(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	--st->Registers.A.B.l;
	SetZN(st, st->Registers.A.B.l);
}

static inline void Op3AM0(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	--st->Registers.A.W;
	SetZN(st, st->Registers.A.W);
}

static void Op3ASlow(struct S9xState *st)
{
	if (CheckMemory(st))
		Op3AM1(st);
	else
		Op3AM0(st);
}

static void OpC6M1(struct S9xState *st) { mOP8(st, Direct, DEC8); }
static void OpC6M0(struct S9xState *st) { mOP16(st, Direct, DEC16, WRAP_BANK); }
static void OpC6Slow(struct S9xState *st) { mOPM(st, DirectSlow, DEC8, DEC16, WRAP_BANK); }

static void OpD6E1(struct S9xState *st) { mOP8(st, DirectIndexedXE1, DEC8); }
static void OpD6E0M1(struct S9xState *st) { mOP8(st, DirectIndexedXE0, DEC8); }
static void OpD6E0M0(struct S9xState *st) { mOP16(st, DirectIndexedXE0, DEC16, WRAP_BANK); }
static void OpD6Slow(struct S9xState *st) { mOPM(st, DirectIndexedXSlow, DEC8, DEC16, WRAP_BANK); }

static void OpCEM1(struct S9xState *st) { mOP8(st, Absolute, DEC8); }
static void OpCEM0(struct S9xState *st) { mOP16(st, Absolute, DEC16); }
static void OpCESlow(struct S9xState *st) { mOPM(st, AbsoluteSlow, DEC8, DEC16); }

static void OpDEM1X1(struct S9xState *st) { mOP8(st, AbsoluteIndexedXX1, DEC8); }
static void OpDEM0X1(struct S9xState *st) { mOP16(st, AbsoluteIndexedXX1, DEC16); }
static void OpDEM1X0(struct S9xState *st) { mOP8(st, AbsoluteIndexedXX0, DEC8); }
static void OpDEM0X0(struct S9xState *st) { mOP16(st, AbsoluteIndexedXX0, DEC16); }
static void OpDESlow(struct S9xState *st) { mOPM(st, AbsoluteIndexedXSlow, DEC8, DEC16); }

/* EOR ********************************************************************* */

template<typename T, typename F> static inline void Op49Wrapper(struct S9xState *st, T &reg, F f)
{
	reg ^= f(st, READ);
	SetZN(st, reg);
}

static void Op49M1(struct S9xState *st)
{
	Op49Wrapper(st, st->Registers.A.B.l, Immediate8);
}

static void Op49M0(struct S9xState *st)
{
	Op49Wrapper(st, st->Registers.A.W, Immediate16);
}

static void Op49Slow(struct S9xState *st)
{
	if (CheckMemory(st))
		Op49Wrapper(st, st->Registers.A.B.l, Immediate8Slow);
	else
		Op49Wrapper(st, st->Registers.A.W, Immediate16Slow);
}

static void Op45M1(struct S9xState *st) { rOP8(st, Direct, EOR8); }
static void Op45M0(struct S9xState *st) { rOP16(st, Direct, EOR16, WRAP_BANK); }
static void Op45Slow(struct S9xState *st) { rOPM(st, DirectSlow, EOR8, EOR16, WRAP_BANK); }

static void Op55E1(struct S9xState *st) { rOP8(st, DirectIndexedXE1, EOR8); }
static void Op55E0M1(struct S9xState *st) { rOP8(st, DirectIndexedXE0, EOR8); }
static void Op55E0M0(struct S9xState *st) { rOP16(st, DirectIndexedXE0, EOR16, WRAP_BANK); }
static void Op55Slow(struct S9xState *st) { rOPM(st, DirectIndexedXSlow, EOR8, EOR16, WRAP_BANK); }

static void Op52E1(struct S9xState *st) { rOP8(st, DirectIndirectE1, EOR8); }
static void Op52E0M1(struct S9xState *st) { rOP8(st, DirectIndirectE0, EOR8); }
static void Op52E0M0(struct S9xState *st) { rOP16(st, DirectIndirectE0, EOR16); }
static void Op52Slow(struct S9xState *st) { rOPM(st, DirectIndirectSlow, EOR8, EOR16); }

static void Op41E1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE1, EOR8); }
static void Op41E0M1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE0, EOR8); }
static void Op41E0M0(struct S9xState *st) { rOP16(st, DirectIndexedIndirectE0, EOR16); }
static void Op41Slow(struct S9xState *st) { rOPM(st, DirectIndexedIndirectSlow, EOR8, EOR16); }

static void Op51E1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE1, EOR8); }
static void Op51E0M1X1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X1, EOR8); }
static void Op51E0M0X1(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X1, EOR16); }
static void Op51E0M1X0(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X0, EOR8); }
static void Op51E0M0X0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X0, EOR16); }
static void Op51Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedSlow, EOR8, EOR16); }

static void Op47M1(struct S9xState *st) { rOP8(st, DirectIndirectLong, EOR8); }
static void Op47M0(struct S9xState *st) { rOP16(st, DirectIndirectLong, EOR16); }
static void Op47Slow(struct S9xState *st) { rOPM(st, DirectIndirectLongSlow, EOR8, EOR16); }

static void Op57M1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedLong, EOR8); }
static void Op57M0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedLong, EOR16); }
static void Op57Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedLongSlow, EOR8, EOR16); }

static void Op4DM1(struct S9xState *st) { rOP8(st, Absolute, EOR8); }
static void Op4DM0(struct S9xState *st) { rOP16(st, Absolute, EOR16); }
static void Op4DSlow(struct S9xState *st) { rOPM(st, AbsoluteSlow, EOR8, EOR16); }

static void Op5DM1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX1, EOR8); }
static void Op5DM0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX1, EOR16); }
static void Op5DM1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX0, EOR8); }
static void Op5DM0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX0, EOR16); }
static void Op5DSlow(struct S9xState *st) { rOPM(st, AbsoluteIndexedXSlow, EOR8, EOR16); }

static void Op59M1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX1, EOR8); }
static void Op59M0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX1, EOR16); }
static void Op59M1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX0, EOR8); }
static void Op59M0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX0, EOR16); }
static void Op59Slow(struct S9xState *st) { rOPM(st, AbsoluteIndexedYSlow, EOR8, EOR16); }

static void Op4FM1(struct S9xState *st) { rOP8(st, AbsoluteLong, EOR8); }
static void Op4FM0(struct S9xState *st) { rOP16(st, AbsoluteLong, EOR16); }
static void Op4FSlow(struct S9xState *st) { rOPM(st, AbsoluteLongSlow, EOR8, EOR16); }

static void Op5FM1(struct S9xState *st) { rOP8(st, AbsoluteLongIndexedX, EOR8); }
static void Op5FM0(struct S9xState *st) { rOP16(st, AbsoluteLongIndexedX, EOR16); }
static void Op5FSlow(struct S9xState *st) { rOPM(st, AbsoluteLongIndexedXSlow, EOR8, EOR16); }

static void Op43M1(struct S9xState *st) { rOP8(st, StackRelative, EOR8); }
static void Op43M0(struct S9xState *st) { rOP16(st, StackRelative, EOR16); }
static void Op43Slow(struct S9xState *st) { rOPM(st, StackRelativeSlow, EOR8, EOR16); }

static void Op53M1(struct S9xState *st) { rOP8(st, StackRelativeIndirectIndexed, EOR8); }
static void Op53M0(struct S9xState *st) { rOP16(st, StackRelativeIndirectIndexed, EOR16); }
static void Op53Slow(struct S9xState *st) { rOPM(st, StackRelativeIndirectIndexedSlow, EOR8, EOR16); }

/* INC ********************************************************************* */

static inline void Op1AM1(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	++st->Registers.A.B.l;
	SetZN(st, st->Registers.A.B.l);
}

static inline void Op1AM0(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	++st->Registers.A.W;
	SetZN(st, st->Registers.A.W);
}

static void Op1ASlow(struct S9xState *st)
{
	if (CheckMemory(st))
		Op1AM1(st);
	else
		Op1AM0(st);
}

static void OpE6M1(struct S9xState *st) { mOP8(st, Direct, INC8); }
static void OpE6M0(struct S9xState *st) { mOP16(st, Direct, INC16, WRAP_BANK); }
static void OpE6Slow(struct S9xState *st) { mOPM(st, DirectSlow, INC8, INC16, WRAP_BANK); }

static void OpF6E1(struct S9xState *st) { mOP8(st, DirectIndexedXE1, INC8); }
static void OpF6E0M1(struct S9xState *st) { mOP8(st, DirectIndexedXE0, INC8); }
static void OpF6E0M0(struct S9xState *st) { mOP16(st, DirectIndexedXE0, INC16, WRAP_BANK); }
static void OpF6Slow(struct S9xState *st) { mOPM(st, DirectIndexedXSlow, INC8, INC16, WRAP_BANK); }

static void OpEEM1(struct S9xState *st) { mOP8(st, Absolute, INC8); }
static void OpEEM0(struct S9xState *st) { mOP16(st, Absolute, INC16); }
static void OpEESlow(struct S9xState *st) { mOPM(st, AbsoluteSlow, INC8, INC16); }

static void OpFEM1X1(struct S9xState *st) { mOP8(st, AbsoluteIndexedXX1, INC8); }
static void OpFEM0X1(struct S9xState *st) { mOP16(st, AbsoluteIndexedXX1, INC16); }
static void OpFEM1X0(struct S9xState *st) { mOP8(st, AbsoluteIndexedXX0, INC8); }
static void OpFEM0X0(struct S9xState *st) { mOP16(st, AbsoluteIndexedXX0, INC16); }
static void OpFESlow(struct S9xState *st) { mOPM(st, AbsoluteIndexedXSlow, INC8, INC16); }

/* LDA ********************************************************************* */

template<typename T, typename F> static inline void LDWrapper(struct S9xState *st, T &reg, F f)
{
	reg = f(st, READ);
	SetZN(st, reg);
}

static void OpA9M1(struct S9xState *st)
{
	LDWrapper(st, st->Registers.A.B.l, Immediate8);
}

static void OpA9M0(struct S9xState *st)
{
	LDWrapper(st, st->Registers.A.W, Immediate16);
}

static void OpA9Slow(struct S9xState *st)
{
	if (CheckMemory(st))
		LDWrapper(st, st->Registers.A.B.l, Immediate8Slow);
	else
		LDWrapper(st, st->Registers.A.W, Immediate16Slow);
}

static void OpA5M1(struct S9xState *st) { rOP8(st, Direct, LDA8); }
static void OpA5M0(struct S9xState *st) { rOP16(st, Direct, LDA16, WRAP_BANK); }
static void OpA5Slow(struct S9xState *st) { rOPM(st, DirectSlow, LDA8, LDA16, WRAP_BANK); }

static void OpB5E1(struct S9xState *st) { rOP8(st, DirectIndexedXE1, LDA8); }
static void OpB5E0M1(struct S9xState *st) { rOP8(st, DirectIndexedXE0, LDA8); }
static void OpB5E0M0(struct S9xState *st) { rOP16(st, DirectIndexedXE0, LDA16, WRAP_BANK); }
static void OpB5Slow(struct S9xState *st) { rOPM(st, DirectIndexedXSlow, LDA8, LDA16, WRAP_BANK); }

static void OpB2E1(struct S9xState *st) { rOP8(st, DirectIndirectE1, LDA8); }
static void OpB2E0M1(struct S9xState *st) { rOP8(st, DirectIndirectE0, LDA8); }
static void OpB2E0M0(struct S9xState *st) { rOP16(st, DirectIndirectE0, LDA16); }
static void OpB2Slow(struct S9xState *st) { rOPM(st, DirectIndirectSlow, LDA8, LDA16); }

static void OpA1E1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE1, LDA8); }
static void OpA1E0M1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE0, LDA8); }
static void OpA1E0M0(struct S9xState *st) { rOP16(st, DirectIndexedIndirectE0, LDA16); }
static void OpA1Slow(struct S9xState *st) { rOPM(st, DirectIndexedIndirectSlow, LDA8, LDA16); }

static void OpB1E1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE1, LDA8); }
static void OpB1E0M1X1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X1, LDA8); }
static void OpB1E0M0X1(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X1, LDA16); }
static void OpB1E0M1X0(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X0, LDA8); }
static void OpB1E0M0X0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X0, LDA16); }
static void OpB1Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedSlow, LDA8, LDA16); }

static void OpA7M1(struct S9xState *st) { rOP8(st, DirectIndirectLong, LDA8); }
static void OpA7M0(struct S9xState *st) { rOP16(st, DirectIndirectLong, LDA16); }
static void OpA7Slow(struct S9xState *st) { rOPM(st, DirectIndirectLongSlow, LDA8, LDA16); }

static void OpB7M1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedLong, LDA8); }
static void OpB7M0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedLong, LDA16); }
static void OpB7Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedLongSlow, LDA8, LDA16); }

static void OpADM1(struct S9xState *st) { rOP8(st, Absolute, LDA8); }
static void OpADM0(struct S9xState *st) { rOP16(st, Absolute, LDA16); }
static void OpADSlow(struct S9xState *st) { rOPM(st, AbsoluteSlow, LDA8, LDA16); }

static void OpBDM1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX1, LDA8); }
static void OpBDM0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX1, LDA16); }
static void OpBDM1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX0, LDA8); }
static void OpBDM0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX0, LDA16); }
static void OpBDSlow(struct S9xState *st) { rOPM(st, AbsoluteIndexedXSlow, LDA8, LDA16); }

static void OpB9M1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX1, LDA8); }
static void OpB9M0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX1, LDA16); }
static void OpB9M1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX0, LDA8); }
static void OpB9M0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX0, LDA16); }
static void OpB9Slow(struct S9xState *st) { rOPM(st, AbsoluteIndexedYSlow, LDA8, LDA16); }

static void OpAFM1(struct S9xState *st) { rOP8(st, AbsoluteLong, LDA8); }
static void OpAFM0(struct S9xState *st) { rOP16(st, AbsoluteLong, LDA16); }
static void OpAFSlow(struct S9xState *st) { rOPM(st, AbsoluteLongSlow, LDA8, LDA16); }

static void OpBFM1(struct S9xState *st) { rOP8(st, AbsoluteLongIndexedX, LDA8); }
static void OpBFM0(struct S9xState *st) { rOP16(st, AbsoluteLongIndexedX, LDA16); }
static void OpBFSlow(struct S9xState *st) { rOPM(st, AbsoluteLongIndexedXSlow, LDA8, LDA16); }

static void OpA3M1(struct S9xState *st) { rOP8(st, StackRelative, LDA8); }
static void OpA3M0(struct S9xState *st) { rOP16(st, StackRelative, LDA16); }
static void OpA3Slow(struct S9xState *st) { rOPM(st, StackRelativeSlow, LDA8, LDA16); }

static void OpB3M1(struct S9xState *st) { rOP8(st, StackRelativeIndirectIndexed, LDA8); }
static void OpB3M0(struct S9xState *st) { rOP16(st, StackRelativeIndirectIndexed, LDA16); }
static void OpB3Slow(struct S9xState *st) { rOPM(st, StackRelativeIndirectIndexedSlow, LDA8, LDA16); }

/* LDX ********************************************************************* */

static void OpA2X1(struct S9xState *st)
{
	LDWrapper(st, st->Registers.X.B.l, Immediate8);
}

static void OpA2X0(struct S9xState *st)
{
	LDWrapper(st, st->Registers.X.W, Immediate16);
}

static void OpA2Slow(struct S9xState *st)
{
	if (CheckIndex(st))
		LDWrapper(st, st->Registers.X.B.l, Immediate8Slow);
	else
		LDWrapper(st, st->Registers.X.W, Immediate16Slow);
}

static void OpA6X1(struct S9xState *st) { rOP8(st, Direct, LDX8); }
static void OpA6X0(struct S9xState *st) { rOP16(st, Direct, LDX16, WRAP_BANK); }
static void OpA6Slow(struct S9xState *st) { rOPX(st, DirectSlow, LDX8, LDX16, WRAP_BANK); }

static void OpB6E1(struct S9xState *st) { rOP8(st, DirectIndexedYE1, LDX8); }
static void OpB6E0X1(struct S9xState *st) { rOP8(st, DirectIndexedYE0, LDX8); }
static void OpB6E0X0(struct S9xState *st) { rOP16(st, DirectIndexedYE0, LDX16, WRAP_BANK); }
static void OpB6Slow(struct S9xState *st) { rOPX(st, DirectIndexedYSlow, LDX8, LDX16, WRAP_BANK); }

static void OpAEX1(struct S9xState *st) { rOP8(st, Absolute, LDX8); }
static void OpAEX0(struct S9xState *st) { rOP16(st, Absolute, LDX16, WRAP_BANK); }
static void OpAESlow(struct S9xState *st) { rOPX(st, AbsoluteSlow, LDX8, LDX16, WRAP_BANK); }

static void OpBEX1(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX1, LDX8); }
static void OpBEX0(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX0, LDX16, WRAP_BANK); }
static void OpBESlow(struct S9xState *st) { rOPX(st, AbsoluteIndexedYSlow, LDX8, LDX16, WRAP_BANK); }

/* LDY ********************************************************************* */

static void OpA0X1(struct S9xState *st)
{
	LDWrapper(st, st->Registers.Y.B.l, Immediate8);
}

static void OpA0X0(struct S9xState *st)
{
	LDWrapper(st, st->Registers.Y.W, Immediate16);
}

static void OpA0Slow(struct S9xState *st)
{
	if (CheckIndex(st))
		LDWrapper(st, st->Registers.Y.B.l, Immediate8Slow);
	else
		LDWrapper(st, st->Registers.Y.W, Immediate16Slow);
}

static void OpA4X1(struct S9xState *st) { rOP8(st, Direct, LDY8); }
static void OpA4X0(struct S9xState *st) { rOP16(st, Direct, LDY16, WRAP_BANK); }
static void OpA4Slow(struct S9xState *st) { rOPX(st, DirectSlow, LDY8, LDY16, WRAP_BANK); }

static void OpB4E1(struct S9xState *st) { rOP8(st, DirectIndexedXE1, LDY8); }
static void OpB4E0X1(struct S9xState *st) { rOP8(st, DirectIndexedXE0, LDY8); }
static void OpB4E0X0(struct S9xState *st) { rOP16(st, DirectIndexedXE0, LDY16, WRAP_BANK); }
static void OpB4Slow(struct S9xState *st) { rOPX(st, DirectIndexedXSlow, LDY8, LDY16, WRAP_BANK); }

static void OpACX1(struct S9xState *st) { rOP8(st, Absolute, LDY8); }
static void OpACX0(struct S9xState *st) { rOP16(st, Absolute, LDY16, WRAP_BANK); }
static void OpACSlow(struct S9xState *st) { rOPX(st, AbsoluteSlow, LDY8, LDY16, WRAP_BANK); }

static void OpBCX1(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX1, LDY8); }
static void OpBCX0(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX0, LDY16, WRAP_BANK); }
static void OpBCSlow(struct S9xState *st) { rOPX(st, AbsoluteIndexedXSlow, LDY8, LDY16, WRAP_BANK); }

/* LSR ********************************************************************* */

static inline void Op4AM1(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	st->ICPU._Carry = st->Registers.A.B.l & 1;
	st->Registers.A.B.l >>= 1;
	SetZN(st, st->Registers.A.B.l);
}

static inline void Op4AM0(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	st->ICPU._Carry = st->Registers.A.W & 1;
	st->Registers.A.W >>= 1;
	SetZN(st, st->Registers.A.W);
}

static void Op4ASlow(struct S9xState *st)
{
	if (CheckMemory(st))
		Op4AM1(st);
	else
		Op4AM0(st);
}

static void Op46M1(struct S9xState *st) { mOP8(st, Direct, LSR8); }
static void Op46M0(struct S9xState *st) { mOP16(st, Direct, LSR16, WRAP_BANK); }
static void Op46Slow(struct S9xState *st) { mOPM(st, DirectSlow, LSR8, LSR16, WRAP_BANK); }

static void Op56E1(struct S9xState *st) { mOP8(st, DirectIndexedXE1, LSR8); }
static void Op56E0M1(struct S9xState *st) { mOP8(st, DirectIndexedXE0, LSR8); }
static void Op56E0M0(struct S9xState *st) { mOP16(st, DirectIndexedXE0, LSR16, WRAP_BANK); }
static void Op56Slow(struct S9xState *st) { mOPM(st, DirectIndexedXSlow, LSR8, LSR16, WRAP_BANK); }

static void Op4EM1(struct S9xState *st) { mOP8(st, Absolute, LSR8); }
static void Op4EM0(struct S9xState *st) { mOP16(st, Absolute, LSR16); }
static void Op4ESlow(struct S9xState *st) { mOPM(st, AbsoluteSlow, LSR8, LSR16); }

static void Op5EM1X1(struct S9xState *st) { mOP8(st, AbsoluteIndexedXX1, LSR8); }
static void Op5EM0X1(struct S9xState *st) { mOP16(st, AbsoluteIndexedXX1, LSR16); }
static void Op5EM1X0(struct S9xState *st) { mOP8(st, AbsoluteIndexedXX0, LSR8); }
static void Op5EM0X0(struct S9xState *st) { mOP16(st, AbsoluteIndexedXX0, LSR16); }
static void Op5ESlow(struct S9xState *st) { mOPM(st, AbsoluteIndexedXSlow, LSR8, LSR16); }

/* ORA ********************************************************************* */

template<typename T, typename F> static inline void Op09Wrapper(struct S9xState *st, T &reg, F f)
{
	reg |= f(st, READ);
	SetZN(st, reg);
}

static void Op09M1(struct S9xState *st)
{
	Op09Wrapper(st, st->Registers.A.B.l, Immediate8);
}

static void Op09M0(struct S9xState *st)
{
	Op09Wrapper(st, st->Registers.A.W, Immediate16);
}

static void Op09Slow(struct S9xState *st)
{
	if (CheckMemory(st))
		Op09Wrapper(st, st->Registers.A.B.l, Immediate8Slow);
	else
		Op09Wrapper(st, st->Registers.A.W, Immediate16Slow);
}

static void Op05M1(struct S9xState *st) { rOP8(st, Direct, ORA8); }
static void Op05M0(struct S9xState *st) { rOP16(st, Direct, ORA16, WRAP_BANK); }
static void Op05Slow(struct S9xState *st) { rOPM(st, DirectSlow, ORA8, ORA16, WRAP_BANK); }

static void Op15E1(struct S9xState *st) { rOP8(st, DirectIndexedXE1, ORA8); }
static void Op15E0M1(struct S9xState *st) { rOP8(st, DirectIndexedXE0, ORA8); }
static void Op15E0M0(struct S9xState *st) { rOP16(st, DirectIndexedXE0, ORA16, WRAP_BANK); }
static void Op15Slow(struct S9xState *st) { rOPM(st, DirectIndexedXSlow, ORA8, ORA16, WRAP_BANK); }

static void Op12E1(struct S9xState *st) { rOP8(st, DirectIndirectE1, ORA8); }
static void Op12E0M1(struct S9xState *st) { rOP8(st, DirectIndirectE0, ORA8); }
static void Op12E0M0(struct S9xState *st) { rOP16(st, DirectIndirectE0, ORA16); }
static void Op12Slow(struct S9xState *st) { rOPM(st, DirectIndirectSlow, ORA8, ORA16); }

static void Op01E1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE1, ORA8); }
static void Op01E0M1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE0, ORA8); }
static void Op01E0M0(struct S9xState *st) { rOP16(st, DirectIndexedIndirectE0, ORA16); }
static void Op01Slow(struct S9xState *st) { rOPM(st, DirectIndexedIndirectSlow, ORA8, ORA16); }

static void Op11E1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE1, ORA8); }
static void Op11E0M1X1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X1, ORA8); }
static void Op11E0M0X1(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X1, ORA16); }
static void Op11E0M1X0(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X0, ORA8); }
static void Op11E0M0X0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X0, ORA16); }
static void Op11Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedSlow, ORA8, ORA16); }

static void Op07M1(struct S9xState *st) { rOP8(st, DirectIndirectLong, ORA8); }
static void Op07M0(struct S9xState *st) { rOP16(st, DirectIndirectLong, ORA16); }
static void Op07Slow(struct S9xState *st) { rOPM(st, DirectIndirectLongSlow, ORA8, ORA16); }

static void Op17M1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedLong, ORA8); }
static void Op17M0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedLong, ORA16); }
static void Op17Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedLongSlow, ORA8, ORA16); }

static void Op0DM1(struct S9xState *st) { rOP8(st, Absolute, ORA8); }
static void Op0DM0(struct S9xState *st) { rOP16(st, Absolute, ORA16); }
static void Op0DSlow(struct S9xState *st) { rOPM(st, AbsoluteSlow, ORA8, ORA16); }

static void Op1DM1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX1, ORA8); }
static void Op1DM0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX1, ORA16); }
static void Op1DM1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX0, ORA8); }
static void Op1DM0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX0, ORA16); }
static void Op1DSlow(struct S9xState *st) { rOPM(st, AbsoluteIndexedXSlow, ORA8, ORA16); }

static void Op19M1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX1, ORA8); }
static void Op19M0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX1, ORA16); }
static void Op19M1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX0, ORA8); }
static void Op19M0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX0, ORA16); }
static void Op19Slow(struct S9xState *st) { rOPM(st, AbsoluteIndexedYSlow, ORA8, ORA16); }

static void Op0FM1(struct S9xState *st) { rOP8(st, AbsoluteLong, ORA8); }
static void Op0FM0(struct S9xState *st) { rOP16(st, AbsoluteLong, ORA16); }
static void Op0FSlow(struct S9xState *st) { rOPM(st, AbsoluteLongSlow, ORA8, ORA16); }

static void Op1FM1(struct S9xState *st) { rOP8(st, AbsoluteLongIndexedX, ORA8); }
static void Op1FM0(struct S9xState *st) { rOP16(st, AbsoluteLongIndexedX, ORA16); }
static void Op1FSlow(struct S9xState *st) { rOPM(st, AbsoluteLongIndexedXSlow, ORA8, ORA16); }

static void Op03M1(struct S9xState *st) { rOP8(st, StackRelative, ORA8); }
static void Op03M0(struct S9xState *st) { rOP16(st, StackRelative, ORA16); }
static void Op03Slow(struct S9xState *st) { rOPM(st, StackRelativeSlow, ORA8, ORA16); }

static void Op13M1(struct S9xState *st) { rOP8(st, StackRelativeIndirectIndexed, ORA8); }
static void Op13M0(struct S9xState *st) { rOP16(st, StackRelativeIndirectIndexed, ORA16); }
static void Op13Slow(struct S9xState *st) { rOPM(st, StackRelativeIndirectIndexedSlow, ORA8, ORA16); }

/* ROL ********************************************************************* */

static inline void Op2AM1(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	uint16_t w = (static_cast<uint16_t>(st->Registers.A.B.l) << 1) | static_cast<uint16_t>(CheckCarry(st));
	st->ICPU._Carry = w >= 0x100;
	st->Registers.A.B.l = static_cast<uint8_t>(w);
	SetZN(st, st->Registers.A.B.l);
}

static inline void Op2AM0(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	uint32_t w = (static_cast<uint32_t>(st->Registers.A.W) << 1) | static_cast<uint32_t>(CheckCarry(st));
	st->ICPU._Carry = w >= 0x10000;
	st->Registers.A.W = static_cast<uint16_t>(w);
	SetZN(st, st->Registers.A.W);
}

static void Op2ASlow(struct S9xState *st)
{
	if (CheckMemory(st))
		Op2AM1(st);
	else
		Op2AM0(st);
}

static void Op26M1(struct S9xState *st) { mOP8(st, Direct, ROL8); }
static void Op26M0(struct S9xState *st) { mOP16(st, Direct, ROL16, WRAP_BANK); }
static void Op26Slow(struct S9xState *st) { mOPM(st, DirectSlow, ROL8, ROL16, WRAP_BANK); }

static void Op36E1(struct S9xState *st) { mOP8(st, DirectIndexedXE1, ROL8); }
static void Op36E0M1(struct S9xState *st) { mOP8(st, DirectIndexedXE0, ROL8); }
static void Op36E0M0(struct S9xState *st) { mOP16(st, DirectIndexedXE0, ROL16, WRAP_BANK); }
static void Op36Slow(struct S9xState *st) { mOPM(st, DirectIndexedXSlow, ROL8, ROL16, WRAP_BANK); }

static void Op2EM1(struct S9xState *st) { mOP8(st, Absolute, ROL8); }
static void Op2EM0(struct S9xState *st) { mOP16(st, Absolute, ROL16); }
static void Op2ESlow(struct S9xState *st) { mOPM(st, AbsoluteSlow, ROL8, ROL16); }

static void Op3EM1X1(struct S9xState *st) { mOP8(st, AbsoluteIndexedXX1, ROL8); }
static void Op3EM0X1(struct S9xState *st) { mOP16(st, AbsoluteIndexedXX1, ROL16); }
static void Op3EM1X0(struct S9xState *st) { mOP8(st, AbsoluteIndexedXX0, ROL8); }
static void Op3EM0X0(struct S9xState *st) { mOP16(st, AbsoluteIndexedXX0, ROL16); }
static void Op3ESlow(struct S9xState *st) { mOPM(st, AbsoluteIndexedXSlow, ROL8, ROL16); }

/* ROR ********************************************************************* */

static inline void Op6AM1(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	uint16_t w = static_cast<uint16_t>(st->Registers.A.B.l) | (static_cast<uint16_t>(CheckCarry(st)) << 8);
	st->ICPU._Carry = w & 1;
	w >>= 1;
	st->Registers.A.B.l = static_cast<uint8_t>(w);
	SetZN(st, st->Registers.A.B.l);
}

static inline void Op6AM0(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	uint32_t w = static_cast<uint32_t>(st->Registers.A.W) | (static_cast<uint32_t>(CheckCarry(st)) << 16);
	st->ICPU._Carry = w & 1;
	w >>= 1;
	st->Registers.A.W = static_cast<uint16_t>(w);
	SetZN(st, st->Registers.A.W);
}

static void Op6ASlow(struct S9xState *st)
{
	if (CheckMemory(st))
		Op6AM1(st);
	else
		Op6AM0(st);
}

static void Op66M1(struct S9xState *st) { mOP8(st, Direct, ROR8); }
static void Op66M0(struct S9xState *st) { mOP16(st, Direct, ROR16, WRAP_BANK); }
static void Op66Slow(struct S9xState *st) { mOPM(st, DirectSlow, ROR8, ROR16, WRAP_BANK); }

static void Op76E1(struct S9xState *st) { mOP8(st, DirectIndexedXE1, ROR8); }
static void Op76E0M1(struct S9xState *st) { mOP8(st, DirectIndexedXE0, ROR8); }
static void Op76E0M0(struct S9xState *st) { mOP16(st, DirectIndexedXE0, ROR16, WRAP_BANK); }
static void Op76Slow(struct S9xState *st) { mOPM(st, DirectIndexedXSlow, ROR8, ROR16, WRAP_BANK); }

static void Op6EM1(struct S9xState *st) { mOP8(st, Absolute, ROR8); }
static void Op6EM0(struct S9xState *st) { mOP16(st, Absolute, ROR16); }
static void Op6ESlow(struct S9xState *st) { mOPM(st, AbsoluteSlow, ROR8, ROR16); }

static void Op7EM1X1(struct S9xState *st) { mOP8(st, AbsoluteIndexedXX1, ROR8); }
static void Op7EM0X1(struct S9xState *st) { mOP16(st, AbsoluteIndexedXX1, ROR16); }
static void Op7EM1X0(struct S9xState *st) { mOP8(st, AbsoluteIndexedXX0, ROR8); }
static void Op7EM0X0(struct S9xState *st) { mOP16(st, AbsoluteIndexedXX0, ROR16); }
static void Op7ESlow(struct S9xState *st) { mOPM(st, AbsoluteIndexedXSlow, ROR8, ROR16); }

/* SBC ********************************************************************* */

template<typename T> static inline void SBC(struct S9xState *, T)
{
}

template<> inline void SBC<uint16_t>(struct S9xState *st, uint16_t Work16)
{
	SBC16(st, Work16);
}

template<> inline void SBC<uint8_t>(struct S9xState *st, uint8_t Work8)
{
	SBC8(st, Work8);
}

template<typename F> static inline void OpE9Wrapper(struct S9xState *st, F f)
{
	SBC(st, f(st, READ));
}

static void OpE9M1(struct S9xState *st)
{
	OpE9Wrapper(st, Immediate8);
}

static void OpE9M0(struct S9xState *st)
{
	OpE9Wrapper(st, Immediate16);
}

static void OpE9Slow(struct S9xState *st)
{
	if (CheckMemory(st))
		OpE9Wrapper(st, Immediate8Slow);
	else
		OpE9Wrapper(st, Immediate16Slow);
}

static void OpE5M1(struct S9xState *st) { rOP8(st, Direct, SBC8); }
static void OpE5M0(struct S9xState *st) { rOP16(st, Direct, SBC16, WRAP_BANK); }
static void OpE5Slow(struct S9xState *st) { rOPM(st, DirectSlow, SBC8, SBC16, WRAP_BANK); }

static void OpF5E1(struct S9xState *st) { rOP8(st, DirectIndexedXE1, SBC8); }
static void OpF5E0M1(struct S9xState *st) { rOP8(st, DirectIndexedXE0, SBC8); }
static void OpF5E0M0(struct S9xState *st) { rOP16(st, DirectIndexedXE0, SBC16, WRAP_BANK); }
static void OpF5Slow(struct S9xState *st) { rOPM(st, DirectIndexedXSlow, SBC8, SBC16, WRAP_BANK); }

static void OpF2E1(struct S9xState *st) { rOP8(st, DirectIndirectE1, SBC8); }
static void OpF2E0M1(struct S9xState *st) { rOP8(st, DirectIndirectE0, SBC8); }
static void OpF2E0M0(struct S9xState *st) { rOP16(st, DirectIndirectE0, SBC16); }
static void OpF2Slow(struct S9xState *st) { rOPM(st, DirectIndirectSlow, SBC8, SBC16); }

static void OpE1E1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE1, SBC8); }
static void OpE1E0M1(struct S9xState *st) { rOP8(st, DirectIndexedIndirectE0, SBC8); }
static void OpE1E0M0(struct S9xState *st) { rOP16(st, DirectIndexedIndirectE0, SBC16); }
static void OpE1Slow(struct S9xState *st) { rOPM(st, DirectIndexedIndirectSlow, SBC8, SBC16); }

static void OpF1E1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE1, SBC8); }
static void OpF1E0M1X1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X1, SBC8); }
static void OpF1E0M0X1(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X1, SBC16); }
static void OpF1E0M1X0(struct S9xState *st) { rOP8(st, DirectIndirectIndexedE0X0, SBC8); }
static void OpF1E0M0X0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedE0X0, SBC16); }
static void OpF1Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedSlow, SBC8, SBC16); }

static void OpE7M1(struct S9xState *st) { rOP8(st, DirectIndirectLong, SBC8); }
static void OpE7M0(struct S9xState *st) { rOP16(st, DirectIndirectLong, SBC16); }
static void OpE7Slow(struct S9xState *st) { rOPM(st, DirectIndirectLongSlow, SBC8, SBC16); }

static void OpF7M1(struct S9xState *st) { rOP8(st, DirectIndirectIndexedLong, SBC8); }
static void OpF7M0(struct S9xState *st) { rOP16(st, DirectIndirectIndexedLong, SBC16); }
static void OpF7Slow(struct S9xState *st) { rOPM(st, DirectIndirectIndexedLongSlow, SBC8, SBC16); }

static void OpEDM1(struct S9xState *st) { rOP8(st, Absolute, SBC8); }
static void OpEDM0(struct S9xState *st) { rOP16(st, Absolute, SBC16); }
static void OpEDSlow(struct S9xState *st) { rOPM(st, AbsoluteSlow, SBC8, SBC16); }

static void OpFDM1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX1, SBC8); }
static void OpFDM0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX1, SBC16); }
static void OpFDM1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedXX0, SBC8); }
static void OpFDM0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedXX0, SBC16); }
static void OpFDSlow(struct S9xState *st) { rOPM(st, AbsoluteIndexedXSlow, SBC8, SBC16); }

static void OpF9M1X1(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX1, SBC8); }
static void OpF9M0X1(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX1, SBC16); }
static void OpF9M1X0(struct S9xState *st) { rOP8(st, AbsoluteIndexedYX0, SBC8); }
static void OpF9M0X0(struct S9xState *st) { rOP16(st, AbsoluteIndexedYX0, SBC16); }
static void OpF9Slow(struct S9xState *st) { rOPM(st, AbsoluteIndexedYSlow, SBC8, SBC16); }

static void OpEFM1(struct S9xState *st) { rOP8(st, AbsoluteLong, SBC8); }
static void OpEFM0(struct S9xState *st) { rOP16(st, AbsoluteLong, SBC16); }
static void OpEFSlow(struct S9xState *st) { rOPM(st, AbsoluteLongSlow, SBC8, SBC16); }

static void OpFFM1(struct S9xState *st) { rOP8(st, AbsoluteLongIndexedX, SBC8); }
static void OpFFM0(struct S9xState *st) { rOP16(st, AbsoluteLongIndexedX, SBC16); }
static void OpFFSlow(struct S9xState *st) { rOPM(st, AbsoluteLongIndexedXSlow, SBC8, SBC16); }

static void OpE3M1(struct S9xState *st) { rOP8(st, StackRelative, SBC8); }
static void OpE3M0(struct S9xState *st) { rOP16(st, StackRelative, SBC16); }
static void OpE3Slow(struct S9xState *st) { rOPM(st, StackRelativeSlow, SBC8, SBC16); }

static void OpF3M1(struct S9xState *st) { rOP8(st, StackRelativeIndirectIndexed, SBC8); }
static void OpF3M0(struct S9xState *st) { rOP16(st, StackRelativeIndirectIndexed, SBC16); }
static void OpF3Slow(struct S9xState *st) { rOPM(st, StackRelativeIndirectIndexedSlow, SBC8, SBC16); }

/* STA ********************************************************************* */

static void Op85M1(struct S9xState *st) { wOP8(st, Direct, STA8); }
static void Op85M0(struct S9xState *st) { wOP16(st, Direct, STA16, WRAP_BANK); }
static void Op85Slow(struct S9xState *st) { wOPM(st, DirectSlow, STA8, STA16, WRAP_BANK); }

static void Op95E1(struct S9xState *st) { wOP8(st, DirectIndexedXE1, STA8); }
static void Op95E0M1(struct S9xState *st) { wOP8(st, DirectIndexedXE0, STA8); }
static void Op95E0M0(struct S9xState *st) { wOP16(st, DirectIndexedXE0, STA16, WRAP_BANK); }
static void Op95Slow(struct S9xState *st) { wOPM(st, DirectIndexedXSlow, STA8, STA16, WRAP_BANK); }

static void Op92E1(struct S9xState *st) { wOP8(st, DirectIndirectE1, STA8); }
static void Op92E0M1(struct S9xState *st) { wOP8(st, DirectIndirectE0, STA8); }
static void Op92E0M0(struct S9xState *st) { wOP16(st, DirectIndirectE0, STA16); }
static void Op92Slow(struct S9xState *st) { wOPM(st, DirectIndirectSlow, STA8, STA16); }

static void Op81E1(struct S9xState *st) { wOP8(st, DirectIndexedIndirectE1, STA8); }
static void Op81E0M1(struct S9xState *st) { wOP8(st, DirectIndexedIndirectE0, STA8); }
static void Op81E0M0(struct S9xState *st) { wOP16(st, DirectIndexedIndirectE0, STA16); }
static void Op81Slow(struct S9xState *st) { wOPM(st, DirectIndexedIndirectSlow, STA8, STA16); }

static void Op91E1(struct S9xState *st) { wOP8(st, DirectIndirectIndexedE1, STA8); }
static void Op91E0M1X1(struct S9xState *st) { wOP8(st, DirectIndirectIndexedE0X1, STA8); }
static void Op91E0M0X1(struct S9xState *st) { wOP16(st, DirectIndirectIndexedE0X1, STA16); }
static void Op91E0M1X0(struct S9xState *st) { wOP8(st, DirectIndirectIndexedE0X0, STA8); }
static void Op91E0M0X0(struct S9xState *st) { wOP16(st, DirectIndirectIndexedE0X0, STA16); }
static void Op91Slow(struct S9xState *st) { wOPM(st, DirectIndirectIndexedSlow, STA8, STA16); }

static void Op87M1(struct S9xState *st) { wOP8(st, DirectIndirectLong, STA8); }
static void Op87M0(struct S9xState *st) { wOP16(st, DirectIndirectLong, STA16); }
static void Op87Slow(struct S9xState *st) { wOPM(st, DirectIndirectLongSlow, STA8, STA16); }

static void Op97M1(struct S9xState *st) { wOP8(st, DirectIndirectIndexedLong, STA8); }
static void Op97M0(struct S9xState *st) { wOP16(st, DirectIndirectIndexedLong, STA16); }
static void Op97Slow(struct S9xState *st) { wOPM(st, DirectIndirectIndexedLongSlow, STA8, STA16); }

static void Op8DM1(struct S9xState *st) { wOP8(st, Absolute, STA8); }
static void Op8DM0(struct S9xState *st) { wOP16(st, Absolute, STA16); }
static void Op8DSlow(struct S9xState *st) { wOPM(st, AbsoluteSlow, STA8, STA16); }

static void Op9DM1X1(struct S9xState *st) { wOP8(st, AbsoluteIndexedXX1, STA8); }
static void Op9DM0X1(struct S9xState *st) { wOP16(st, AbsoluteIndexedXX1, STA16); }
static void Op9DM1X0(struct S9xState *st) { wOP8(st, AbsoluteIndexedXX0, STA8); }
static void Op9DM0X0(struct S9xState *st) { wOP16(st, AbsoluteIndexedXX0, STA16); }
static void Op9DSlow(struct S9xState *st) { wOPM(st, AbsoluteIndexedXSlow, STA8, STA16); }

static void Op99M1X1(struct S9xState *st) { wOP8(st, AbsoluteIndexedYX1, STA8); }
static void Op99M0X1(struct S9xState *st) { wOP16(st, AbsoluteIndexedYX1, STA16); }
static void Op99M1X0(struct S9xState *st) { wOP8(st, AbsoluteIndexedYX0, STA8); }
static void Op99M0X0(struct S9xState *st) { wOP16(st, AbsoluteIndexedYX0, STA16); }
static void Op99Slow(struct S9xState *st) { wOPM(st, AbsoluteIndexedYSlow, STA8, STA16); }

static void Op8FM1(struct S9xState *st) { wOP8(st, AbsoluteLong, STA8); }
static void Op8FM0(struct S9xState *st) { wOP16(st, AbsoluteLong, STA16); }
static void Op8FSlow(struct S9xState *st) { wOPM(st, AbsoluteLongSlow, STA8, STA16); }

static void Op9FM1(struct S9xState *st) { wOP8(st, AbsoluteLongIndexedX, STA8); }
static void Op9FM0(struct S9xState *st) { wOP16(st, AbsoluteLongIndexedX, STA16); }
static void Op9FSlow(struct S9xState *st) { wOPM(st, AbsoluteLongIndexedXSlow, STA8, STA16); }

static void Op83M1(struct S9xState *st) { wOP8(st, StackRelative, STA8); }
static void Op83M0(struct S9xState *st) { wOP16(st, StackRelative, STA16); }
static void Op83Slow(struct S9xState *st) { wOPM(st, StackRelativeSlow, STA8, STA16); }

static void Op93M1(struct S9xState *st) { wOP8(st, StackRelativeIndirectIndexed, STA8); }
static void Op93M0(struct S9xState *st) { wOP16(st, StackRelativeIndirectIndexed, STA16); }
static void Op93Slow(struct S9xState *st) { wOPM(st, StackRelativeIndirectIndexedSlow, STA8, STA16); }

/* STX ********************************************************************* */

static void Op86X1(struct S9xState *st) { wOP8(st, Direct, STX8); }
static void Op86X0(struct S9xState *st) { wOP16(st, Direct, STX16, WRAP_BANK); }
static void Op86Slow(struct S9xState *st) { wOPX(st, DirectSlow, STX8, STX16, WRAP_BANK); }

static void Op96E1(struct S9xState *st) { wOP8(st, DirectIndexedYE1, STX8); }
static void Op96E0X1(struct S9xState *st) { wOP8(st, DirectIndexedYE0, STX8); }
static void Op96E0X0(struct S9xState *st) { wOP16(st, DirectIndexedYE0, STX16, WRAP_BANK); }
static void Op96Slow(struct S9xState *st) { wOPX(st, DirectIndexedYSlow, STX8, STX16, WRAP_BANK); }

static void Op8EX1(struct S9xState *st) { wOP8(st, Absolute, STX8); }
static void Op8EX0(struct S9xState *st) { wOP16(st, Absolute, STX16, WRAP_BANK); }
static void Op8ESlow(struct S9xState *st) { wOPX(st, AbsoluteSlow, STX8, STX16, WRAP_BANK); }

/* STY ********************************************************************* */

static void Op84X1(struct S9xState *st) { wOP8(st, Direct, STY8); }
static void Op84X0(struct S9xState *st) { wOP16(st, Direct, STY16, WRAP_BANK); }
static void Op84Slow(struct S9xState *st) { wOPX(st, DirectSlow, STY8, STY16, WRAP_BANK); }

static void Op94E1(struct S9xState *st) { wOP8(st, DirectIndexedXE1, STY8); }
static void Op94E0X1(struct S9xState *st) { wOP8(st, DirectIndexedXE0, STY8); }
static void Op94E0X0(struct S9xState *st) { wOP16(st, DirectIndexedXE0, STY16, WRAP_BANK); }
static void Op94Slow(struct S9xState *st) { wOPX(st, DirectIndexedXSlow, STY8, STY16, WRAP_BANK); }

static void Op8CX1(struct S9xState *st) { wOP8(st, Absolute, STY8); }
static void Op8CX0(struct S9xState *st) { wOP16(st, Absolute, STY16, WRAP_BANK); }
static void Op8CSlow(struct S9xState *st) { wOPX(st, AbsoluteSlow, STY8, STY16, WRAP_BANK); }

/* STZ ********************************************************************* */

static void Op64M1(struct S9xState *st) { wOP8(st, Direct, STZ8); }
static void Op64M0(struct S9xState *st) { wOP16(st, Direct, STZ16, WRAP_BANK); }
static void Op64Slow(struct S9xState *st) { wOPM(st, DirectSlow, STZ8, STZ16, WRAP_BANK); }

static void Op74E1(struct S9xState *st) { wOP8(st, DirectIndexedXE1, STZ8); }
static void Op74E0M1(struct S9xState *st) { wOP8(st, DirectIndexedXE0, STZ8); }
static void Op74E0M0(struct S9xState *st) { wOP16(st, DirectIndexedXE0, STZ16, WRAP_BANK); }
static void Op74Slow(struct S9xState *st) { wOPM(st, DirectIndexedXSlow, STZ8, STZ16, WRAP_BANK); }

static void Op9CM1(struct S9xState *st) { wOP8(st, Absolute, STZ8); }
static void Op9CM0(struct S9xState *st) { wOP16(st, Absolute, STZ16); }
static void Op9CSlow(struct S9xState *st) { wOPM(st, AbsoluteSlow, STZ8, STZ16); }

static void Op9EM1X1(struct S9xState *st) { wOP8(st, AbsoluteIndexedXX1, STZ8); }
static void Op9EM0X1(struct S9xState *st) { wOP16(st, AbsoluteIndexedXX1, STZ16); }
static void Op9EM1X0(struct S9xState *st) { wOP8(st, AbsoluteIndexedXX0, STZ8); }
static void Op9EM0X0(struct S9xState *st) { wOP16(st, AbsoluteIndexedXX0, STZ16); }
static void Op9ESlow(struct S9xState *st) { wOPM(st, AbsoluteIndexedXSlow, STZ8, STZ16); }

/* TRB ********************************************************************* */

static void Op14M1(struct S9xState *st) { mOP8(st, Direct, TRB8); }
static void Op14M0(struct S9xState *st) { mOP16(st, Direct, TRB16, WRAP_BANK); }
static void Op14Slow(struct S9xState *st) { mOPM(st, DirectSlow, TRB8, TRB16, WRAP_BANK); }

static void Op1CM1(struct S9xState *st) { mOP8(st, Absolute, TRB8); }
static void Op1CM0(struct S9xState *st) { mOP16(st, Absolute, TRB16, WRAP_BANK); }
static void Op1CSlow(struct S9xState *st) { mOPM(st, AbsoluteSlow, TRB8, TRB16, WRAP_BANK); }

/* TSB ********************************************************************* */

static void Op04M1(struct S9xState *st) { mOP8(st, Direct, TSB8); }
static void Op04M0(struct S9xState *st) { mOP16(st, Direct, TSB16, WRAP_BANK); }
static void Op04Slow(struct S9xState *st) { mOPM(st, DirectSlow, TSB8, TSB16, WRAP_BANK); }

static void Op0CM1(struct S9xState *st) { mOP8(st, Absolute, TSB8); }
static void Op0CM0(struct S9xState *st) { mOP16(st, Absolute, TSB16, WRAP_BANK); }
static void Op0CSlow(struct S9xState *st) { mOPM(st, AbsoluteSlow, TSB8, TSB16, WRAP_BANK); }

/* Branch Instructions ***************************************************** */

// BCC
static void Op90E0(struct S9xState *st) { bOP(st, Relative, !CheckCarry(st), false); }
static void Op90E1(struct S9xState *st) { bOP(st, Relative, !CheckCarry(st), true); }
static void Op90Slow(struct S9xState *st) { bOP(st, RelativeSlow, !CheckCarry(st), CheckEmulation(st)); }

// BCS
static void OpB0E0(struct S9xState *st) { bOP(st, Relative, CheckCarry(st), false); }
static void OpB0E1(struct S9xState *st) { bOP(st, Relative, CheckCarry(st), true); }
static void OpB0Slow(struct S9xState *st) { bOP(st, RelativeSlow, CheckCarry(st), CheckEmulation(st)); }

// BEQ
static void OpF0E0(struct S9xState *st) { bOP(st, Relative, CheckZero(st), false); }
static void OpF0E1(struct S9xState *st) { bOP(st, Relative, CheckZero(st), true); }
static void OpF0Slow(struct S9xState *st) { bOP(st, RelativeSlow, CheckZero(st), CheckEmulation(st)); }

// BMI
static void Op30E0(struct S9xState *st) { bOP(st, Relative, CheckNegative(st), false); }
static void Op30E1(struct S9xState *st) { bOP(st, Relative, CheckNegative(st), true); }
static void Op30Slow(struct S9xState *st) { bOP(st, RelativeSlow, CheckNegative(st), CheckEmulation(st)); }

// BNE
static void OpD0E0(struct S9xState *st) { bOP(st, Relative, !CheckZero(st), false); }
static void OpD0E1(struct S9xState *st) { bOP(st, Relative, !CheckZero(st), true); }
static void OpD0Slow(struct S9xState *st) { bOP(st, RelativeSlow, !CheckZero(st), CheckEmulation(st)); }

// BPL
static void Op10E0(struct S9xState *st) { bOP(st, Relative, !CheckNegative(st), false); }
static void Op10E1(struct S9xState *st) { bOP(st, Relative, !CheckNegative(st), true); }
static void Op10Slow(struct S9xState *st) { bOP(st, RelativeSlow, !CheckNegative(st), CheckEmulation(st)); }

// BRA
static void Op80E0(struct S9xState *st) { bOP(st, Relative, true, false); }
static void Op80E1(struct S9xState *st) { bOP(st, Relative, true, true); }
static void Op80Slow(struct S9xState *st) { bOP(st, RelativeSlow, true, CheckEmulation(st)); }

// BVC
static void Op50E0(struct S9xState *st) { bOP(st, Relative, !CheckOverflow(st), false); }
static void Op50E1(struct S9xState *st) { bOP(st, Relative, !CheckOverflow(st), true); }
static void Op50Slow(struct S9xState *st) { bOP(st, RelativeSlow, !CheckOverflow(st), CheckEmulation(st)); }

// BVS
static void Op70E0(struct S9xState *st) { bOP(st, Relative, CheckOverflow(st), false); }
static void Op70E1(struct S9xState *st) { bOP(st, Relative, CheckOverflow(st), true); }
static void Op70Slow(struct S9xState *st) { bOP(st, RelativeSlow, CheckOverflow(st), CheckEmulation(st)); }

// BRL
template<typename F> static inline void Op82Wrapper(struct S9xState *st, F f)
{
	S9xSetPCBase(st, st->ICPU.ShiftedPB + f(st, JUMP));
}

static void Op82(struct S9xState *st)
{
	Op82Wrapper(st, RelativeLong);
}

static void Op82Slow(struct S9xState *st)
{
	Op82Wrapper(st, RelativeLongSlow);
}

/* Flag Instructions ******************************************************* */

// CLC
static void Op18(struct S9xState *st)
{
	ClearCarry(st);
	AddCycles(st, ONE_CYCLE);
}

// SEC
static void Op38(struct S9xState *st)
{
	SetCarry(st);
	AddCycles(st, ONE_CYCLE);
}

// CLD
static void OpD8(struct S9xState *st)
{
	ClearDecimal(st);
	AddCycles(st, ONE_CYCLE);
}

// SED
static void OpF8(struct S9xState *st)
{
	SetDecimal(st);
	AddCycles(st, ONE_CYCLE);
}

// CLI
static void Op58(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);

	st->Timings.IRQFlagChanging |= IRQ_CLEAR_FLAG;
}

// SEI
static void Op78(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);

	st->Timings.IRQFlagChanging |= IRQ_SET_FLAG;
}

// CLV
static void OpB8(struct S9xState *st)
{
	ClearOverflow(st);
	AddCycles(st, ONE_CYCLE);
}

/* DEX/DEY ***************************************************************** */

template<typename T> static inline void DEWrapper(struct S9xState *st, T &reg)
{
	AddCycles(st, ONE_CYCLE);
	--reg;
	SetZN(st, reg);
}

static inline void OpCAX1(struct S9xState *st)
{
	DEWrapper(st, st->Registers.X.B.l);
}

static inline void OpCAX0(struct S9xState *st)
{
	DEWrapper(st, st->Registers.X.W);
}

static void OpCASlow(struct S9xState *st)
{
	if (CheckIndex(st))
		OpCAX1(st);
	else
		OpCAX0(st);
}

static inline void Op88X1(struct S9xState *st)
{
	DEWrapper(st, st->Registers.Y.B.l);
}

static inline void Op88X0(struct S9xState *st)
{
	DEWrapper(st, st->Registers.Y.W);
}

static void Op88Slow(struct S9xState *st)
{
	if (CheckIndex(st))
		Op88X1(st);
	else
		Op88X0(st);
}

/* INX/INY ***************************************************************** */

template<typename T> static inline void INWrapper(struct S9xState *st, T &reg)
{
	AddCycles(st, ONE_CYCLE);
	++reg;
	SetZN(st, reg);
}

static inline void OpE8X1(struct S9xState *st)
{
	INWrapper(st, st->Registers.X.B.l);
}

static inline void OpE8X0(struct S9xState *st)
{
	INWrapper(st, st->Registers.X.W);
}

static void OpE8Slow(struct S9xState *st)
{
	if (CheckIndex(st))
		OpE8X1(st);
	else
		OpE8X0(st);
}

static inline void OpC8X1(struct S9xState *st)
{
	INWrapper(st, st->Registers.Y.B.l);
}

static inline void OpC8X0(struct S9xState *st)
{
	INWrapper(st, st->Registers.Y.W);
}

static void OpC8Slow(struct S9xState *st)
{
	if (CheckIndex(st))
		OpC8X1(st);
	else
		OpC8X0(st);
}

/* NOP ********************************************************************* */

static void OpEA(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
}

/* PUSH Instructions ******************************************************* */

static inline void PushW(struct S9xState *st, uint16_t w)
{
	S9xSetWord(st, w, st->Registers.S.W - 1, WRAP_BANK, WRITE_10);
	st->Registers.S.W -= 2;
}

static inline void PushWE(struct S9xState *st, uint16_t w)
{
	--st->Registers.S.B.l;
	S9xSetWord(st, w, st->Registers.S.W, WRAP_PAGE, WRITE_10);
	--st->Registers.S.B.l;
}

static inline void PushB(struct S9xState *st, uint8_t b)
{
	S9xSetByte(st, b, st->Registers.S.W--);
}

static inline void PushBE(struct S9xState *st, uint8_t b)
{
	S9xSetByte(st, b, st->Registers.S.W);
	--st->Registers.S.B.l;
}

template<typename F> static inline void PEWrapper(struct S9xState *st, F f, bool cond)
{
	uint16_t val = static_cast<uint16_t>(f(st, NONE));
	PushW(st, val);
	st->OpenBus = val & 0xff;
	if (cond)
		st->Registers.S.B.h = 1;
}

// PEA
static void OpF4E0(struct S9xState *st)
{
	PEWrapper(st, Absolute, false);
}

static void OpF4E1(struct S9xState *st)
{
	// Note: PEA is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	PEWrapper(st, Absolute, true);
}

static void OpF4Slow(struct S9xState *st)
{
	PEWrapper(st, AbsoluteSlow, CheckEmulation(st));
}

// PEI
static void OpD4E0(struct S9xState *st)
{
	PEWrapper(st, DirectIndirectE0, false);
}

static void OpD4E1(struct S9xState *st)
{
	// Note: PEI is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	PEWrapper(st, DirectIndirectE1, true);
}

static void OpD4Slow(struct S9xState *st)
{
	PEWrapper(st, DirectIndirectSlow, CheckEmulation(st));
}

// PER
static void Op62E0(struct S9xState *st)
{
	PEWrapper(st, RelativeLong, false);
}

static void Op62E1(struct S9xState *st)
{
	// Note: PER is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	PEWrapper(st, RelativeLong, true);
}

static void Op62Slow(struct S9xState *st)
{
	PEWrapper(st, RelativeLongSlow, CheckEmulation(st));
}

template<typename Treg, typename Tob, typename F> static inline void PHWrapper(struct S9xState *st, const Treg &pushReg, const Tob &obReg, F f, bool cond)
{
	AddCycles(st, ONE_CYCLE);
	f(st, pushReg);
	st->OpenBus = obReg;
	if (cond)
		st->Registers.S.B.h = 1;
}

// PHA
static inline void Op48E1(struct S9xState *st)
{
	PHWrapper(st, st->Registers.A.B.l, st->Registers.A.B.l, PushBE, false);
}

static inline void Op48E0M1(struct S9xState *st)
{
	PHWrapper(st, st->Registers.A.B.l, st->Registers.A.B.l, PushB, false);
}

static inline void Op48E0M0(struct S9xState *st)
{
	PHWrapper(st, st->Registers.A.W, st->Registers.A.B.l, PushW, false);
}

static void Op48Slow(struct S9xState *st)
{
	if (CheckEmulation(st))
		Op48E1(st);
	else if (CheckMemory(st))
		Op48E0M1(st);
	else
		Op48E0M0(st);
}

// PHB
static inline void Op8BE1(struct S9xState *st)
{
	PHWrapper(st, st->Registers.DB, st->Registers.DB, PushBE, false);
}

static inline void Op8BE0(struct S9xState *st)
{
	PHWrapper(st, st->Registers.DB, st->Registers.DB, PushB, false);
}

static void Op8BSlow(struct S9xState *st)
{
	if (CheckEmulation(st))
		Op8BE1(st);
	else
		Op8BE0(st);
}

// PHD
static void Op0BE0(struct S9xState *st)
{
	PHWrapper(st, st->Registers.D.W, st->Registers.D.B.l, PushW, false);
}

static void Op0BE1(struct S9xState *st)
{
	// Note: PHD is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	PHWrapper(st, st->Registers.D.W, st->Registers.D.B.l, PushW, true);
}

static void Op0BSlow(struct S9xState *st)
{
	PHWrapper(st, st->Registers.D.W, st->Registers.D.B.l, PushW, CheckEmulation(st));
}

// PHK
static inline void Op4BE1(struct S9xState *st)
{
	PHWrapper(st, st->Registers.PC.B.xPB, st->Registers.PC.B.xPB, PushBE, false);
}

static inline void Op4BE0(struct S9xState *st)
{
	PHWrapper(st, st->Registers.PC.B.xPB, st->Registers.PC.B.xPB, PushB, false);
}

static void Op4BSlow(struct S9xState *st)
{
	if (CheckEmulation(st))
		Op4BE1(st);
	else
		Op4BE0(st);
}

// PHP
static inline void Op08E1(struct S9xState *st)
{
	S9xPackStatus(st);
	PHWrapper(st, st->Registers.P.B.l, st->Registers.P.B.l, PushBE, false);
}

static inline void Op08E0(struct S9xState *st)
{
	S9xPackStatus(st);
	PHWrapper(st, st->Registers.P.B.l, st->Registers.P.B.l, PushB, false);
}

static void Op08Slow(struct S9xState *st)
{
	if (CheckEmulation(st))
		Op08E1(st);
	else
		Op08E0(st);
}

// PHX
static inline void OpDAE1(struct S9xState *st)
{
	PHWrapper(st, st->Registers.X.B.l, st->Registers.X.B.l, PushBE, false);
}

static inline void OpDAE0X1(struct S9xState *st)
{
	PHWrapper(st, st->Registers.X.B.l, st->Registers.X.B.l, PushB, false);
}

static inline void OpDAE0X0(struct S9xState *st)
{
	PHWrapper(st, st->Registers.X.W, st->Registers.X.B.l, PushW, false);
}

static void OpDASlow(struct S9xState *st)
{
	if (CheckEmulation(st))
		OpDAE1(st);
	else if (CheckIndex(st))
		OpDAE0X1(st);
	else
		OpDAE0X0(st);
}

// PHY
static inline void Op5AE1(struct S9xState *st)
{
	PHWrapper(st, st->Registers.Y.B.l, st->Registers.Y.B.l, PushBE, false);
}

static inline void Op5AE0X1(struct S9xState *st)
{
	PHWrapper(st, st->Registers.Y.B.l, st->Registers.Y.B.l, PushB, false);
}

static inline void Op5AE0X0(struct S9xState *st)
{
	PHWrapper(st, st->Registers.Y.W, st->Registers.Y.B.l, PushW, false);
}

static void Op5ASlow(struct S9xState *st)
{
	if (CheckEmulation(st))
		Op5AE1(st);
	else if (CheckIndex(st))
		Op5AE0X1(st);
	else
		Op5AE0X0(st);
}

/* PULL Instructions ******************************************************* */

static inline void PullW(struct S9xState *st, uint16_t &w)
{
	w = S9xGetWord(st, st->Registers.S.W + 1, WRAP_BANK);
	st->Registers.S.W += 2;
}

static inline void PullWE(struct S9xState *st, uint16_t &w)
{
	++st->Registers.S.B.l;
	w = S9xGetWord(st, st->Registers.S.W, WRAP_PAGE);
	++st->Registers.S.B.l;
}

static inline void PullB(struct S9xState *st, uint8_t &b)
{
	b = S9xGetByte(st, ++st->Registers.S.W);
}

static inline void PullBE(struct S9xState *st, uint8_t &b)
{
	++st->Registers.S.B.l;
	b = S9xGetByte(st, st->Registers.S.W);
}

// PLA
template<typename Treg, typename Tob, typename F> static inline void Op68Wrapper(struct S9xState *st, Treg &pullReg, const Tob &obReg, F f)
{
	AddCycles(st, TWO_CYCLES);
	f(st, pullReg);
	SetZN(st, pullReg);
	st->OpenBus = obReg;
}

static inline void Op68E1(struct S9xState *st)
{
	Op68Wrapper(st, st->Registers.A.B.l, st->Registers.A.B.l, PullBE);
}

static inline void Op68E0M1(struct S9xState *st)
{
	Op68Wrapper(st, st->Registers.A.B.l, st->Registers.A.B.l, PullB);
}

static inline void Op68E0M0(struct S9xState *st)
{
	Op68Wrapper(st, st->Registers.A.W, st->Registers.A.B.h, PullW);
}

static void Op68Slow(struct S9xState *st)
{
	if (CheckEmulation(st))
		Op68E1(st);
	else if (CheckMemory(st))
		Op68E0M1(st);
	else
		Op68E0M0(st);
}

// PLB
template<typename F> static inline void OpABWrapper(struct S9xState *st, F f)
{
	AddCycles(st, TWO_CYCLES);
	f(st, st->Registers.DB);
	SetZN(st, st->Registers.DB);
	st->ICPU.ShiftedDB = st->Registers.DB << 16;
	st->OpenBus = st->Registers.DB;
}

static inline void OpABE1(struct S9xState *st)
{
	OpABWrapper(st, PullBE);
}

static inline void OpABE0(struct S9xState *st)
{
	OpABWrapper(st, PullB);
}

static void OpABSlow(struct S9xState *st)
{
	if (CheckEmulation(st))
		OpABE1(st);
	else
		OpABE0(st);
}

// PLD
static inline void Op2BWrapper(struct S9xState *st, bool cond)
{
	AddCycles(st, TWO_CYCLES);
	PullW(st, st->Registers.D.W);
	SetZN(st, st->Registers.D.W);
	st->OpenBus = st->Registers.D.B.h;
	if (cond)
		st->Registers.S.B.h = 1;
}

static void Op2BE0(struct S9xState *st)
{
	Op2BWrapper(st, false);
}

static void Op2BE1(struct S9xState *st)
{
	// Note: PLD is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	Op2BWrapper(st, true);
}

static void Op2BSlow(struct S9xState *st)
{
	Op2BWrapper(st, CheckEmulation(st));
}

// PLP
static void Op28E1(struct S9xState *st)
{
	AddCycles(st, TWO_CYCLES);
	PullBE(st, st->Registers.P.B.l);
	st->OpenBus = st->Registers.P.B.l;
	SetFlags(st, MemoryFlag | IndexFlag);
	S9xUnpackStatus(st);
	S9xFixCycles(st);
}

static void Op28E0(struct S9xState *st)
{
	AddCycles(st, TWO_CYCLES);
	PullB(st, st->Registers.P.B.l);
	st->OpenBus = st->Registers.P.B.l;
	S9xUnpackStatus(st);

	if (CheckIndex(st))
		st->Registers.X.B.h = st->Registers.Y.B.h = 0;

	S9xFixCycles(st);
}

static void Op28Slow(struct S9xState *st)
{
	AddCycles(st, TWO_CYCLES);

	if (CheckEmulation(st))
	{
		PullBE(st, st->Registers.P.B.l);
		st->OpenBus = st->Registers.P.B.l;
		SetFlags(st, MemoryFlag | IndexFlag);
	}
	else
	{
		PullB(st, st->Registers.P.B.l);
		st->OpenBus = st->Registers.P.B.l;
	}

	S9xUnpackStatus(st);

	if (CheckIndex(st))
		st->Registers.X.B.h = st->Registers.Y.B.h = 0;

	S9xFixCycles(st);
}

// PLX
static inline void OpFAE1(struct S9xState *st)
{
	AddCycles(st, TWO_CYCLES);
	PullBE(st, st->Registers.X.B.l);
	SetZN(st, st->Registers.X.B.l);
	st->OpenBus = st->Registers.X.B.l;
}

static inline void OpFAE0X1(struct S9xState *st)
{
	AddCycles(st, TWO_CYCLES);
	PullB(st, st->Registers.X.B.l);
	SetZN(st, st->Registers.X.B.l);
	st->OpenBus = st->Registers.X.B.l;
}

static inline void OpFAE0X0(struct S9xState *st)
{
	AddCycles(st, TWO_CYCLES);
	PullW(st, st->Registers.X.W);
	SetZN(st, st->Registers.X.W);
	st->OpenBus = st->Registers.X.B.h;
}

static void OpFASlow(struct S9xState *st)
{
	if (CheckEmulation(st))
		OpFAE1(st);
	else if (CheckIndex(st))
		OpFAE0X1(st);
	else
		OpFAE0X0(st);
}

// PLY
static inline void Op7AE1(struct S9xState *st)
{
	AddCycles(st, TWO_CYCLES);
	PullBE(st, st->Registers.Y.B.l);
	SetZN(st, st->Registers.Y.B.l);
	st->OpenBus = st->Registers.Y.B.l;
}

static inline void Op7AE0X1(struct S9xState *st)
{
	AddCycles(st, TWO_CYCLES);
	PullB(st, st->Registers.Y.B.l);
	SetZN(st, st->Registers.Y.B.l);
	st->OpenBus = st->Registers.Y.B.l;
}

static inline void Op7AE0X0(struct S9xState *st)
{
	AddCycles(st, TWO_CYCLES);
	PullW(st, st->Registers.Y.W);
	SetZN(st, st->Registers.Y.W);
	st->OpenBus = st->Registers.Y.B.h;
}

static void Op7ASlow(struct S9xState *st)
{
	if (CheckEmulation(st))
		Op7AE1(st);
	else if (CheckIndex(st))
		Op7AE0X1(st);
	else
		Op7AE0X0(st);
}

/* Transfer Instructions *************************************************** */

template<typename T> static inline void TransferWrapper(struct S9xState *st, T &reg1, const T &reg2)
{
	AddCycles(st, ONE_CYCLE);
	reg1 = reg2;
	SetZN(st, reg1);
}

// TAX
static inline void OpAAX1(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.X.B.l, st->Registers.A.B.l);
}

static inline void OpAAX0(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.X.W, st->Registers.A.W);
}

static void OpAASlow(struct S9xState *st)
{
	if (CheckIndex(st))
		OpAAX1(st);
	else
		OpAAX0(st);
}

// TAY
static inline void OpA8X1(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.Y.B.l, st->Registers.A.B.l);
}

static inline void OpA8X0(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.Y.W, st->Registers.A.W);
}

static void OpA8Slow(struct S9xState *st)
{
	if (CheckIndex(st))
		OpA8X1(st);
	else
		OpA8X0(st);
}

// TCD
static void Op5B(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.D.W, st->Registers.A.W);
}

// TCS
static void Op1B(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	st->Registers.S.W = st->Registers.A.W;
	if (CheckEmulation(st))
		st->Registers.S.B.h = 1;
}

// TDC
static void Op7B(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.A.W, st->Registers.D.W);
}

// TSC
static void Op3B(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.A.W, st->Registers.S.W);
}

// TSX
static inline void OpBAX1(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.X.B.l, st->Registers.S.B.l);
}

static inline void OpBAX0(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.X.W, st->Registers.S.W);
}

static void OpBASlow(struct S9xState *st)
{
	if (CheckIndex(st))
		OpBAX1(st);
	else
		OpBAX0(st);
}

// TXA
static inline void Op8AM1(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.A.B.l, st->Registers.X.B.l);
}

static inline void Op8AM0(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.A.W, st->Registers.X.W);
}

static void Op8ASlow(struct S9xState *st)
{
	if (CheckMemory(st))
		Op8AM1(st);
	else
		Op8AM0(st);
}

// TXS
static void Op9A(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);
	st->Registers.S.W = st->Registers.X.W;
	if (CheckEmulation(st))
		st->Registers.S.B.h = 1;
}

// TXY
static inline void Op9BX1(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.Y.B.l, st->Registers.X.B.l);
}

static inline void Op9BX0(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.Y.W, st->Registers.X.W);
}

static void Op9BSlow(struct S9xState *st)
{
	if (CheckIndex(st))
		Op9BX1(st);
	else
		Op9BX0(st);
}

// TYA
static inline void Op98M1(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.A.B.l, st->Registers.Y.B.l);
}

static inline void Op98M0(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.A.W, st->Registers.Y.W);
}

static void Op98Slow(struct S9xState *st)
{
	if (CheckMemory(st))
		Op98M1(st);
	else
		Op98M0(st);
}

// TYX
static inline void OpBBX1(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.X.B.l, st->Registers.Y.B.l);
}

static inline void OpBBX0(struct S9xState *st)
{
	TransferWrapper(st, st->Registers.X.W, st->Registers.Y.W);
}

static void OpBBSlow(struct S9xState *st)
{
	if (CheckIndex(st))
		OpBBX1(st);
	else
		OpBBX0(st);
}

/* XCE ********************************************************************* */

static void OpFB(struct S9xState *st)
{
	AddCycles(st, ONE_CYCLE);

	uint8_t A1 = st->ICPU._Carry;
	uint8_t A2 = st->Registers.P.B.h;

	st->ICPU._Carry = A2 & 1;
	st->Registers.P.B.h = A1;

	if (CheckEmulation(st))
	{
		SetFlags(st, MemoryFlag | IndexFlag);
		st->Registers.S.B.h = 1;
	}

	if (CheckIndex(st))
		st->Registers.X.B.h = st->Registers.Y.B.h = 0;

	S9xFixCycles(st);
}

/* BRK ********************************************************************* */

static void Op00(struct S9xState *st)
{
	AddCycles(st, st->CPU.MemSpeed);

	if (!CheckEmulation(st))
	{
		PushB(st, st->Registers.PC.B.xPB);
		PushW(st, st->Registers.PC.W.xPC + 1);
		S9xPackStatus(st);
		PushB(st, st->Registers.P.B.l);
	}
	else
	{
		PushWE(st, st->Registers.PC.W.xPC + 1);
		S9xPackStatus(st);
		PushBE(st, st->Registers.P.B.l);
	}
	st->OpenBus = st->Registers.P.B.l;
	ClearDecimal(st);
	SetIRQ(st);

	uint16_t addr = S9xGetWord(st, 0xFFFE);

	S9xSetPCBase(st, addr);
	st->OpenBus = addr >> 8;
}

/* IRQ ********************************************************************* */

void S9xOpcode_IRQ(struct S9xState *st)
{
	// IRQ and NMI do an opcode fetch as their first "IO" cycle.
	AddCycles(st, st->CPU.MemSpeed + ONE_CYCLE);

	uint16_t addr;
	if (!CheckEmulation(st))
	{
		PushB(st, st->Registers.PC.B.xPB);
		PushW(st, st->Registers.PC.W.xPC);
		S9xPackStatus(st);
		PushB(st, st->Registers.P.B.l);
		st->OpenBus = st->Registers.P.B.l;
		ClearDecimal(st);
		SetIRQ(st);

		addr = S9xGetWord(st, 0xFFEE);
	}
	else
	{
		PushWE(st, st->Registers.PC.W.xPC);
		S9xPackStatus(st);
		PushBE(st, st->Registers.P.B.l);
		st->OpenBus = st->Registers.P.B.l;
		ClearDecimal(st);
		SetIRQ(st);

		addr = S9xGetWord(st, 0xFFFE);
	}
	st->OpenBus = addr >> 8;
	S9xSetPCBase(st, addr);
}

/* NMI ********************************************************************* */

void S9xOpcode_NMI(struct S9xState *st)
{
	// IRQ and NMI do an opcode fetch as their first "IO" cycle.
	AddCycles(st, st->CPU.MemSpeed + ONE_CYCLE);

	uint16_t addr;
	if (!CheckEmulation(st))
	{
		PushB(st, st->Registers.PC.B.xPB);
		PushW(st, st->Registers.PC.W.xPC);
		S9xPackStatus(st);
		PushB(st, st->Registers.P.B.l);
		st->OpenBus = st->Registers.P.B.l;
		ClearDecimal(st);
		SetIRQ(st);

		addr = S9xGetWord(st, 0xFFEA);
	}
	else
	{
		PushWE(st, st->Registers.PC.W.xPC);
		S9xPackStatus(st);
		PushBE(st, st->Registers.P.B.l);
		st->OpenBus = st->Registers.P.B.l;
		ClearDecimal(st);
		SetIRQ(st);

		addr = S9xGetWord(st, 0xFFFA);
	}
	st->OpenBus = addr >> 8;
	S9xSetPCBase(st, addr);
}

/* COP ********************************************************************* */

static void Op02(struct S9xState *st)
{
	AddCycles(st, st->CPU.MemSpeed);

	if (!CheckEmulation(st))
	{
		PushB(st, st->Registers.PC.B.xPB);
		PushW(st, st->Registers.PC.W.xPC + 1);
		S9xPackStatus(st);
		PushB(st, st->Registers.P.B.l);
	}
	else
	{
		PushWE(st, st->Registers.PC.W.xPC + 1);
		S9xPackStatus(st);
		PushBE(st, st->Registers.P.B.l);
	}
	st->OpenBus = st->Registers.P.B.l;
	ClearDecimal(st);
	SetIRQ(st);

	uint16_t addr = S9xGetWord(st, 0xFFF4);

	S9xSetPCBase(st, addr);
	st->OpenBus = addr >> 8;
}

/* JML ********************************************************************* */

template<typename F> static inline void JMLWrapper(struct S9xState *st, F f)
{
	S9xSetPCBase(st, f(st, JUMP));
}

static void OpDC(struct S9xState *st)
{
	JMLWrapper(st, AbsoluteIndirectLong);
}

static void OpDCSlow(struct S9xState *st)
{
	JMLWrapper(st, AbsoluteIndirectLongSlow);
}

static void Op5C(struct S9xState *st)
{
	JMLWrapper(st, AbsoluteLong);
}

static void Op5CSlow(struct S9xState *st)
{
	JMLWrapper(st, AbsoluteLongSlow);
}

/* JMP ********************************************************************* */

template<typename F> static inline void JMPWrapper(struct S9xState *st, F f)
{
	S9xSetPCBase(st, st->ICPU.ShiftedPB + static_cast<uint16_t>(f(st, JUMP)));
}

static void Op4C(struct S9xState *st)
{
	JMPWrapper(st, Absolute);
}

static void Op4CSlow(struct S9xState *st)
{
	JMPWrapper(st, AbsoluteSlow);
}

static void Op6C(struct S9xState *st)
{
	JMPWrapper(st, AbsoluteIndirect);
}

static void Op6CSlow(struct S9xState *st)
{
	JMPWrapper(st, AbsoluteIndirectSlow);
}

static void Op7C(struct S9xState *st)
{
	JMPWrapper(st, AbsoluteIndexedIndirect);
}

static void Op7CSlow(struct S9xState *st)
{
	JMPWrapper(st, AbsoluteIndexedIndirectSlow);
}

/* JSL/RTL ***************************************************************** */

template<typename F> static inline void Op22Wrapper(struct S9xState *st, F f, bool cond)
{
	uint32_t addr = f(st, JSR);
	PushB(st, st->Registers.PC.B.xPB);
	PushW(st, st->Registers.PC.W.xPC - 1);
	if (cond)
		st->Registers.S.B.h = 1;
	S9xSetPCBase(st, addr);
}

static void Op22E0(struct S9xState *st)
{
	Op22Wrapper(st, AbsoluteLong, false);
}

static void Op22E1(struct S9xState *st)
{
	// Note: JSL is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	Op22Wrapper(st, AbsoluteLong, true);
}

static void Op22Slow(struct S9xState *st)
{
	Op22Wrapper(st, AbsoluteLongSlow, CheckEmulation(st));
}

static inline void Op6BWrapper(struct S9xState *st, bool cond)
{
	AddCycles(st, TWO_CYCLES);
	PullW(st, st->Registers.PC.W.xPC);
	PullB(st, st->Registers.PC.B.xPB);
	if (cond)
		st->Registers.S.B.h = 1;
	++st->Registers.PC.W.xPC;
	S9xSetPCBase(st, st->Registers.PC.xPBPC);
}

static void Op6BE0(struct S9xState *st)
{
	Op6BWrapper(st, false);
}

static void Op6BE1(struct S9xState *st)
{
	// Note: RTL is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	Op6BWrapper(st, true);
}

static void Op6BSlow(struct S9xState *st)
{
	Op6BWrapper(st, CheckEmulation(st));
}

/* JSR/RTS ***************************************************************** */

template<typename F> static inline void Op20Wrapper(struct S9xState *st, F f)
{
	uint16_t addr = Absolute(st, JSR);
	AddCycles(st, ONE_CYCLE);
	f(st, st->Registers.PC.W.xPC - 1);
	S9xSetPCBase(st, st->ICPU.ShiftedPB + addr);
}

static inline void Op20E1(struct S9xState *st)
{
	Op20Wrapper(st, PushWE);
}

static void Op20E0(struct S9xState *st)
{
	Op20Wrapper(st, PushW);
}

static inline void Op20Slow(struct S9xState *st)
{
	if (CheckEmulation(st))
		Op20E1(st);
	else
		Op20E0(st);
}

template<typename F> static inline void OpFCWrapper(struct S9xState *st, F f, bool cond)
{
	uint16_t addr = f(st, JSR);
	PushW(st, st->Registers.PC.W.xPC - 1);
	if (cond)
		st->Registers.S.B.h = 1;
	S9xSetPCBase(st, st->ICPU.ShiftedPB + addr);
}

static void OpFCE0(struct S9xState *st)
{
	OpFCWrapper(st, AbsoluteIndexedIndirect, false);
}

static void OpFCE1(struct S9xState *st)
{
	// Note: JSR (a,X) is a new instruction,
	// and so doesn't respect the emu-mode stack bounds.
	OpFCWrapper(st, AbsoluteIndexedIndirect, true);
}

static void OpFCSlow(struct S9xState *st)
{
	OpFCWrapper(st, AbsoluteIndexedIndirectSlow, CheckEmulation(st));
}

template<typename F> static inline void Op60Wrapper(struct S9xState *st, F f)
{
	AddCycles(st, TWO_CYCLES);
	f(st, st->Registers.PC.W.xPC);
	AddCycles(st, ONE_CYCLE);
	++st->Registers.PC.W.xPC;
	S9xSetPCBase(st, st->Registers.PC.xPBPC);
}

static inline void Op60E1(struct S9xState *st)
{
	Op60Wrapper(st, PullWE);
}

static inline void Op60E0(struct S9xState *st)
{
	Op60Wrapper(st, PullW);
}

static void Op60Slow(struct S9xState *st)
{
	if (CheckEmulation(st))
		Op60E1(st);
	else
		Op60E0(st);
}

/* MVN/MVP ***************************************************************** */

template<typename F> static inline void MVWrapper(struct S9xState *st, F f, bool cond, int inc, bool setOpenBusOnFirstF = false)
{
	uint32_t SrcBank;

	st->Registers.DB = f(st, NONE);
	if (setOpenBusOnFirstF)
		st->OpenBus = st->Registers.DB;
	st->ICPU.ShiftedDB = st->Registers.DB << 16;
	st->OpenBus = SrcBank = f(st, NONE);

	S9xSetByte(st, st->OpenBus = S9xGetByte(st, (SrcBank << 16) + st->Registers.X.W), st->ICPU.ShiftedDB + st->Registers.Y.W);

	if (cond)
	{
		st->Registers.X.B.l += inc;
		st->Registers.Y.B.l += inc;
	}
	else
	{
		st->Registers.X.W += inc;
		st->Registers.Y.W += inc;
	}

	--st->Registers.A.W;
	if (st->Registers.A.W != 0xffff)
		st->Registers.PC.W.xPC -= 3;

	AddCycles(st, TWO_CYCLES);
}

static void Op54X1(struct S9xState *st)
{
	MVWrapper(st, Immediate8, true, 1);
}

static void Op54X0(struct S9xState *st)
{
	MVWrapper(st, Immediate8, false, 1);
}

static void Op54Slow(struct S9xState *st)
{
	MVWrapper(st, Immediate8Slow, CheckIndex(st), 1, true);
}

static void Op44X1(struct S9xState *st)
{
	MVWrapper(st, Immediate8, true, -1);
}

static void Op44X0(struct S9xState *st)
{
	MVWrapper(st, Immediate8, false, -1);
}

static void Op44Slow(struct S9xState *st)
{
	MVWrapper(st, Immediate8Slow, CheckIndex(st), -1, true);
}

/* REP/SEP ***************************************************************** */

template<typename F> static inline void OpC2Wrapper(struct S9xState *st, F f)
{
	uint8_t Work8 = ~f(st, READ);
	st->Registers.P.B.l &= Work8;
	st->ICPU._Carry &= Work8;
	st->ICPU._Overflow &= Work8 >> 6;
	st->ICPU._Negative &= Work8;
	st->ICPU._Zero |= ~Work8 & Zero;

	AddCycles(st, ONE_CYCLE);

	if (CheckEmulation(st))
		SetFlags(st, MemoryFlag | IndexFlag);

	if (CheckIndex(st))
		st->Registers.X.B.h = st->Registers.Y.B.h = 0;

	S9xFixCycles(st);
}

static void OpC2(struct S9xState *st)
{
	OpC2Wrapper(st, Immediate8);
}

static void OpC2Slow(struct S9xState *st)
{
	OpC2Wrapper(st, Immediate8Slow);
}

template<typename F> static inline void OpE2Wrapper(struct S9xState *st, F f)
{
	uint8_t Work8 = f(st, READ);
	st->Registers.P.B.l |= Work8;
	st->ICPU._Carry |= Work8 & 1;
	st->ICPU._Overflow |= (Work8 >> 6) & 1;
	st->ICPU._Negative |= Work8;
	if (Work8 & Zero)
		st->ICPU._Zero = 0;

	AddCycles(st, ONE_CYCLE);

	if (CheckEmulation(st))
		SetFlags(st, MemoryFlag | IndexFlag);

	if (CheckIndex(st))
		st->Registers.X.B.h = st->Registers.Y.B.h = 0;

	S9xFixCycles(st);
}

static void OpE2(struct S9xState *st)
{
	OpE2Wrapper(st, Immediate8);
}

static void OpE2Slow(struct S9xState *st)
{
	OpE2Wrapper(st, Immediate8Slow);
}

/* XBA ********************************************************************* */

static void OpEB(struct S9xState *st)
{
	std::swap(st->Registers.A.B.l, st->Registers.A.B.h);
	SetZN(st, st->Registers.A.B.l);
	AddCycles(st, TWO_CYCLES);
}

/* RTI ********************************************************************* */

static void Op40Slow(struct S9xState *st)
{
	AddCycles(st, TWO_CYCLES);

	if (!CheckEmulation(st))
	{
		PullB(st, st->Registers.P.B.l);
		S9xUnpackStatus(st);
		PullW(st, st->Registers.PC.W.xPC);
		PullB(st, st->Registers.PC.B.xPB);
		st->OpenBus = st->Registers.PC.B.xPB;
		st->ICPU.ShiftedPB = st->Registers.PC.B.xPB << 16;
	}
	else
	{
		PullBE(st, st->Registers.P.B.l);
		S9xUnpackStatus(st);
		PullWE(st, st->Registers.PC.W.xPC);
		st->OpenBus = st->Registers.PC.B.xPCh;
		SetFlags(st, MemoryFlag | IndexFlag);
	}

	S9xSetPCBase(st, st->Registers.PC.xPBPC);

	if (CheckIndex(st))
		st->Registers.X.B.h = st->Registers.Y.B.h = 0;

	S9xFixCycles(st);
}

/* STP/WAI ***************************************************************** */

// WAI
static void OpCB(struct S9xState *st)
{
	st->CPU.WaitingForInterrupt = true;

	--st->Registers.PC.W.xPC;
	AddCycles(st, ONE_CYCLE);
}

// STP
static void OpDB(struct S9xState *st)
{
	--st->Registers.PC.W.xPC;
	st->CPU.Flags |= DEBUG_MODE_FLAG | HALTED_FLAG;
}

/* WDM (Reserved S9xOpcode) ************************************************ */

static void Op42(struct S9xState *st)
{
	S9xGetWord(st, st->Registers.PC.xPBPC);
	++st->Registers.PC.W.xPC;
}

/* CPU-S9xOpcodes Definitions ************************************************/

const struct SOpcodes S9xOpcodesM1X1[256] =
{
	{ Op00 },        { Op01E0M1 },    { Op02 },        { Op03M1 },      { Op04M1 },
	{ Op05M1 },      { Op06M1 },      { Op07M1 },      { Op08E0 },      { Op09M1 },
	{ Op0AM1 },      { Op0BE0 },      { Op0CM1 },      { Op0DM1 },      { Op0EM1 },
	{ Op0FM1 },      { Op10E0 },      { Op11E0M1X1 },  { Op12E0M1 },    { Op13M1 },
	{ Op14M1 },      { Op15E0M1 },    { Op16E0M1 },    { Op17M1 },      { Op18 },
	{ Op19M1X1 },    { Op1AM1 },      { Op1B },        { Op1CM1 },      { Op1DM1X1 },
	{ Op1EM1X1 },    { Op1FM1 },      { Op20E0 },      { Op21E0M1 },    { Op22E0 },
	{ Op23M1 },      { Op24M1 },      { Op25M1 },      { Op26M1 },      { Op27M1 },
	{ Op28E0 },      { Op29M1 },      { Op2AM1 },      { Op2BE0 },      { Op2CM1 },
	{ Op2DM1 },      { Op2EM1 },      { Op2FM1 },      { Op30E0 },      { Op31E0M1X1 },
	{ Op32E0M1 },    { Op33M1 },      { Op34E0M1 },    { Op35E0M1 },    { Op36E0M1 },
	{ Op37M1 },      { Op38 },        { Op39M1X1 },    { Op3AM1 },      { Op3B },
	{ Op3CM1X1 },    { Op3DM1X1 },    { Op3EM1X1 },    { Op3FM1 },      { Op40Slow },
	{ Op41E0M1 },    { Op42 },        { Op43M1 },      { Op44X1 },      { Op45M1 },
	{ Op46M1 },      { Op47M1 },      { Op48E0M1 },    { Op49M1 },      { Op4AM1 },
	{ Op4BE0 },      { Op4C },        { Op4DM1 },      { Op4EM1 },      { Op4FM1 },
	{ Op50E0 },      { Op51E0M1X1 },  { Op52E0M1 },    { Op53M1 },      { Op54X1 },
	{ Op55E0M1 },    { Op56E0M1 },    { Op57M1 },      { Op58 },        { Op59M1X1 },
	{ Op5AE0X1 },    { Op5B },        { Op5C },        { Op5DM1X1 },    { Op5EM1X1 },
	{ Op5FM1 },      { Op60E0 },      { Op61E0M1 },    { Op62E0 },      { Op63M1 },
	{ Op64M1 },      { Op65M1 },      { Op66M1 },      { Op67M1 },      { Op68E0M1 },
	{ Op69M1 },      { Op6AM1 },      { Op6BE0 },      { Op6C },        { Op6DM1 },
	{ Op6EM1 },      { Op6FM1 },      { Op70E0 },      { Op71E0M1X1 },  { Op72E0M1 },
	{ Op73M1 },      { Op74E0M1 },    { Op75E0M1 },    { Op76E0M1 },    { Op77M1 },
	{ Op78 },        { Op79M1X1 },    { Op7AE0X1 },    { Op7B },        { Op7C },
	{ Op7DM1X1 },    { Op7EM1X1 },    { Op7FM1 },      { Op80E0 },      { Op81E0M1 },
	{ Op82 },        { Op83M1 },      { Op84X1 },      { Op85M1 },      { Op86X1 },
	{ Op87M1 },      { Op88X1 },      { Op89M1 },      { Op8AM1 },      { Op8BE0 },
	{ Op8CX1 },      { Op8DM1 },      { Op8EX1 },      { Op8FM1 },      { Op90E0 },
	{ Op91E0M1X1 },  { Op92E0M1 },    { Op93M1 },      { Op94E0X1 },    { Op95E0M1 },
	{ Op96E0X1 },    { Op97M1 },      { Op98M1 },      { Op99M1X1 },    { Op9A },
	{ Op9BX1 },      { Op9CM1 },      { Op9DM1X1 },    { Op9EM1X1 },    { Op9FM1 },
	{ OpA0X1 },      { OpA1E0M1 },    { OpA2X1 },      { OpA3M1 },      { OpA4X1 },
	{ OpA5M1 },      { OpA6X1 },      { OpA7M1 },      { OpA8X1 },      { OpA9M1 },
	{ OpAAX1 },      { OpABE0 },      { OpACX1 },      { OpADM1 },      { OpAEX1 },
	{ OpAFM1 },      { OpB0E0 },      { OpB1E0M1X1 },  { OpB2E0M1 },    { OpB3M1 },
	{ OpB4E0X1 },    { OpB5E0M1 },    { OpB6E0X1 },    { OpB7M1 },      { OpB8 },
	{ OpB9M1X1 },    { OpBAX1 },      { OpBBX1 },      { OpBCX1 },      { OpBDM1X1 },
	{ OpBEX1 },      { OpBFM1 },      { OpC0X1 },      { OpC1E0M1 },    { OpC2 },
	{ OpC3M1 },      { OpC4X1 },      { OpC5M1 },      { OpC6M1 },      { OpC7M1 },
	{ OpC8X1 },      { OpC9M1 },      { OpCAX1 },      { OpCB },        { OpCCX1 },
	{ OpCDM1 },      { OpCEM1 },      { OpCFM1 },      { OpD0E0 },      { OpD1E0M1X1 },
	{ OpD2E0M1 },    { OpD3M1 },      { OpD4E0 },      { OpD5E0M1 },    { OpD6E0M1 },
	{ OpD7M1 },      { OpD8 },        { OpD9M1X1 },    { OpDAE0X1 },    { OpDB },
	{ OpDC },        { OpDDM1X1 },    { OpDEM1X1 },    { OpDFM1 },      { OpE0X1 },
	{ OpE1E0M1 },    { OpE2 },        { OpE3M1 },      { OpE4X1 },      { OpE5M1 },
	{ OpE6M1 },      { OpE7M1 },      { OpE8X1 },      { OpE9M1 },      { OpEA },
	{ OpEB },        { OpECX1 },      { OpEDM1 },      { OpEEM1 },      { OpEFM1 },
	{ OpF0E0 },      { OpF1E0M1X1 },  { OpF2E0M1 },    { OpF3M1 },      { OpF4E0 },
	{ OpF5E0M1 },    { OpF6E0M1 },    { OpF7M1 },      { OpF8 },        { OpF9M1X1 },
	{ OpFAE0X1 },    { OpFB },        { OpFCE0 },      { OpFDM1X1 },    { OpFEM1X1 },
	{ OpFFM1 }
};

const struct SOpcodes S9xOpcodesE1[256] =
{
	{ Op00 },        { Op01E1 },      { Op02 },        { Op03M1 },      { Op04M1 },
	{ Op05M1 },      { Op06M1 },      { Op07M1 },      { Op08E1 },      { Op09M1 },
	{ Op0AM1 },      { Op0BE1 },      { Op0CM1 },      { Op0DM1 },      { Op0EM1 },
	{ Op0FM1 },      { Op10E1 },      { Op11E1 },      { Op12E1 },      { Op13M1 },
	{ Op14M1 },      { Op15E1 },      { Op16E1 },      { Op17M1 },      { Op18 },
	{ Op19M1X1 },    { Op1AM1 },      { Op1B },        { Op1CM1 },      { Op1DM1X1 },
	{ Op1EM1X1 },    { Op1FM1 },      { Op20E1 },      { Op21E1 },      { Op22E1 },
	{ Op23M1 },      { Op24M1 },      { Op25M1 },      { Op26M1 },      { Op27M1 },
	{ Op28E1 },      { Op29M1 },      { Op2AM1 },      { Op2BE1 },      { Op2CM1 },
	{ Op2DM1 },      { Op2EM1 },      { Op2FM1 },      { Op30E1 },      { Op31E1 },
	{ Op32E1 },      { Op33M1 },      { Op34E1 },      { Op35E1 },      { Op36E1 },
	{ Op37M1 },      { Op38 },        { Op39M1X1 },    { Op3AM1 },      { Op3B },
	{ Op3CM1X1 },    { Op3DM1X1 },    { Op3EM1X1 },    { Op3FM1 },      { Op40Slow },
	{ Op41E1 },      { Op42 },        { Op43M1 },      { Op44X1 },      { Op45M1 },
	{ Op46M1 },      { Op47M1 },      { Op48E1 },      { Op49M1 },      { Op4AM1 },
	{ Op4BE1 },      { Op4C },        { Op4DM1 },      { Op4EM1 },      { Op4FM1 },
	{ Op50E1 },      { Op51E1 },      { Op52E1 },      { Op53M1 },      { Op54X1 },
	{ Op55E1 },      { Op56E1 },      { Op57M1 },      { Op58 },        { Op59M1X1 },
	{ Op5AE1 },      { Op5B },        { Op5C },        { Op5DM1X1 },    { Op5EM1X1 },
	{ Op5FM1 },      { Op60E1 },      { Op61E1 },      { Op62E1 },      { Op63M1 },
	{ Op64M1 },      { Op65M1 },      { Op66M1 },      { Op67M1 },      { Op68E1 },
	{ Op69M1 },      { Op6AM1 },      { Op6BE1 },      { Op6C },        { Op6DM1 },
	{ Op6EM1 },      { Op6FM1 },      { Op70E1 },      { Op71E1 },      { Op72E1 },
	{ Op73M1 },      { Op74E1 },      { Op75E1 },      { Op76E1 },      { Op77M1 },
	{ Op78 },        { Op79M1X1 },    { Op7AE1 },      { Op7B },        { Op7C },
	{ Op7DM1X1 },    { Op7EM1X1 },    { Op7FM1 },      { Op80E1 },      { Op81E1 },
	{ Op82 },        { Op83M1 },      { Op84X1 },      { Op85M1 },      { Op86X1 },
	{ Op87M1 },      { Op88X1 },      { Op89M1 },      { Op8AM1 },      { Op8BE1 },
	{ Op8CX1 },      { Op8DM1 },      { Op8EX1 },      { Op8FM1 },      { Op90E1 },
	{ Op91E1 },      { Op92E1 },      { Op93M1 },      { Op94E1 },      { Op95E1 },
	{ Op96E1 },      { Op97M1 },      { Op98M1 },      { Op99M1X1 },    { Op9A },
	{ Op9BX1 },      { Op9CM1 },      { Op9DM1X1 },    { Op9EM1X1 },    { Op9FM1 },
	{ OpA0X1 },      { OpA1E1 },      { OpA2X1 },      { OpA3M1 },      { OpA4X1 },
	{ OpA5M1 },      { OpA6X1 },      { OpA7M1 },      { OpA8X1 },      { OpA9M1 },
	{ OpAAX1 },      { OpABE1 },      { OpACX1 },      { OpADM1 },      { OpAEX1 },
	{ OpAFM1 },      { OpB0E1 },      { OpB1E1 },      { OpB2E1 },      { OpB3M1 },
	{ OpB4E1 },      { OpB5E1 },      { OpB6E1 },      { OpB7M1 },      { OpB8 },
	{ OpB9M1X1 },    { OpBAX1 },      { OpBBX1 },      { OpBCX1 },      { OpBDM1X1 },
	{ OpBEX1 },      { OpBFM1 },      { OpC0X1 },      { OpC1E1 },      { OpC2 },
	{ OpC3M1 },      { OpC4X1 },      { OpC5M1 },      { OpC6M1 },      { OpC7M1 },
	{ OpC8X1 },      { OpC9M1 },      { OpCAX1 },      { OpCB },        { OpCCX1 },
	{ OpCDM1 },      { OpCEM1 },      { OpCFM1 },      { OpD0E1 },      { OpD1E1 },
	{ OpD2E1 },      { OpD3M1 },      { OpD4E1 },      { OpD5E1 },      { OpD6E1 },
	{ OpD7M1 },      { OpD8 },        { OpD9M1X1 },    { OpDAE1 },      { OpDB },
	{ OpDC },        { OpDDM1X1 },    { OpDEM1X1 },    { OpDFM1 },      { OpE0X1 },
	{ OpE1E1 },      { OpE2 },        { OpE3M1 },      { OpE4X1 },      { OpE5M1 },
	{ OpE6M1 },      { OpE7M1 },      { OpE8X1 },      { OpE9M1 },      { OpEA },
	{ OpEB },        { OpECX1 },      { OpEDM1 },      { OpEEM1 },      { OpEFM1 },
	{ OpF0E1 },      { OpF1E1 },      { OpF2E1 },      { OpF3M1 },      { OpF4E1 },
	{ OpF5E1 },      { OpF6E1 },      { OpF7M1 },      { OpF8 },        { OpF9M1X1 },
	{ OpFAE1 },      { OpFB },        { OpFCE1 },      { OpFDM1X1 },    { OpFEM1X1 },
	{ OpFFM1 }
};

const struct SOpcodes S9xOpcodesM1X0[256] =
{
	{ Op00 },        { Op01E0M1 },    { Op02 },        { Op03M1 },      { Op04M1 },
	{ Op05M1 },      { Op06M1 },      { Op07M1 },      { Op08E0 },      { Op09M1 },
	{ Op0AM1 },      { Op0BE0 },      { Op0CM1 },      { Op0DM1 },      { Op0EM1 },
	{ Op0FM1 },      { Op10E0 },      { Op11E0M1X0 },  { Op12E0M1 },    { Op13M1 },
	{ Op14M1 },      { Op15E0M1 },    { Op16E0M1 },    { Op17M1 },      { Op18 },
	{ Op19M1X0 },    { Op1AM1 },      { Op1B },        { Op1CM1 },      { Op1DM1X0 },
	{ Op1EM1X0 },    { Op1FM1 },      { Op20E0 },      { Op21E0M1 },    { Op22E0 },
	{ Op23M1 },      { Op24M1 },      { Op25M1 },      { Op26M1 },      { Op27M1 },
	{ Op28E0 },      { Op29M1 },      { Op2AM1 },      { Op2BE0 },      { Op2CM1 },
	{ Op2DM1 },      { Op2EM1 },      { Op2FM1 },      { Op30E0 },      { Op31E0M1X0 },
	{ Op32E0M1 },    { Op33M1 },      { Op34E0M1 },    { Op35E0M1 },    { Op36E0M1 },
	{ Op37M1 },      { Op38 },        { Op39M1X0 },    { Op3AM1 },      { Op3B },
	{ Op3CM1X0 },    { Op3DM1X0 },    { Op3EM1X0 },    { Op3FM1 },      { Op40Slow },
	{ Op41E0M1 },    { Op42 },        { Op43M1 },      { Op44X0 },      { Op45M1 },
	{ Op46M1 },      { Op47M1 },      { Op48E0M1 },    { Op49M1 },      { Op4AM1 },
	{ Op4BE0 },      { Op4C },        { Op4DM1 },      { Op4EM1 },      { Op4FM1 },
	{ Op50E0 },      { Op51E0M1X0 },  { Op52E0M1 },    { Op53M1 },      { Op54X0 },
	{ Op55E0M1 },    { Op56E0M1 },    { Op57M1 },      { Op58 },        { Op59M1X0 },
	{ Op5AE0X0 },    { Op5B },        { Op5C },        { Op5DM1X0 },    { Op5EM1X0 },
	{ Op5FM1 },      { Op60E0 },      { Op61E0M1 },    { Op62E0 },      { Op63M1 },
	{ Op64M1 },      { Op65M1 },      { Op66M1 },      { Op67M1 },      { Op68E0M1 },
	{ Op69M1 },      { Op6AM1 },      { Op6BE0 },      { Op6C },        { Op6DM1 },
	{ Op6EM1 },      { Op6FM1 },      { Op70E0 },      { Op71E0M1X0 },  { Op72E0M1 },
	{ Op73M1 },      { Op74E0M1 },    { Op75E0M1 },    { Op76E0M1 },    { Op77M1 },
	{ Op78 },        { Op79M1X0 },    { Op7AE0X0 },    { Op7B },        { Op7C },
	{ Op7DM1X0 },    { Op7EM1X0 },    { Op7FM1 },      { Op80E0 },      { Op81E0M1 },
	{ Op82 },        { Op83M1 },      { Op84X0 },      { Op85M1 },      { Op86X0 },
	{ Op87M1 },      { Op88X0 },      { Op89M1 },      { Op8AM1 },      { Op8BE0 },
	{ Op8CX0 },      { Op8DM1 },      { Op8EX0 },      { Op8FM1 },      { Op90E0 },
	{ Op91E0M1X0 },  { Op92E0M1 },    { Op93M1 },      { Op94E0X0 },    { Op95E0M1 },
	{ Op96E0X0 },    { Op97M1 },      { Op98M1 },      { Op99M1X0 },    { Op9A },
	{ Op9BX0 },      { Op9CM1 },      { Op9DM1X0 },    { Op9EM1X0 },    { Op9FM1 },
	{ OpA0X0 },      { OpA1E0M1 },    { OpA2X0 },      { OpA3M1 },      { OpA4X0 },
	{ OpA5M1 },      { OpA6X0 },      { OpA7M1 },      { OpA8X0 },      { OpA9M1 },
	{ OpAAX0 },      { OpABE0 },      { OpACX0 },      { OpADM1 },      { OpAEX0 },
	{ OpAFM1 },      { OpB0E0 },      { OpB1E0M1X0 },  { OpB2E0M1 },    { OpB3M1 },
	{ OpB4E0X0 },    { OpB5E0M1 },    { OpB6E0X0 },    { OpB7M1 },      { OpB8 },
	{ OpB9M1X0 },    { OpBAX0 },      { OpBBX0 },      { OpBCX0 },      { OpBDM1X0 },
	{ OpBEX0 },      { OpBFM1 },      { OpC0X0 },      { OpC1E0M1 },    { OpC2 },
	{ OpC3M1 },      { OpC4X0 },      { OpC5M1 },      { OpC6M1 },      { OpC7M1 },
	{ OpC8X0 },      { OpC9M1 },      { OpCAX0 },      { OpCB },        { OpCCX0 },
	{ OpCDM1 },      { OpCEM1 },      { OpCFM1 },      { OpD0E0 },      { OpD1E0M1X0 },
	{ OpD2E0M1 },    { OpD3M1 },      { OpD4E0 },      { OpD5E0M1 },    { OpD6E0M1 },
	{ OpD7M1 },      { OpD8 },        { OpD9M1X0 },    { OpDAE0X0 },    { OpDB },
	{ OpDC },        { OpDDM1X0 },    { OpDEM1X0 },    { OpDFM1 },      { OpE0X0 },
	{ OpE1E0M1 },    { OpE2 },        { OpE3M1 },      { OpE4X0 },      { OpE5M1 },
	{ OpE6M1 },      { OpE7M1 },      { OpE8X0 },      { OpE9M1 },      { OpEA },
	{ OpEB },        { OpECX0 },      { OpEDM1 },      { OpEEM1 },      { OpEFM1 },
	{ OpF0E0 },      { OpF1E0M1X0 },  { OpF2E0M1 },    { OpF3M1 },      { OpF4E0 },
	{ OpF5E0M1 },    { OpF6E0M1 },    { OpF7M1 },      { OpF8 },        { OpF9M1X0 },
	{ OpFAE0X0 },    { OpFB },        { OpFCE0 },      { OpFDM1X0 },    { OpFEM1X0 },
	{ OpFFM1 }
};

const struct SOpcodes S9xOpcodesM0X0[256] =
{
	{ Op00 },        { Op01E0M0 },    { Op02 },        { Op03M0 },      { Op04M0 },
	{ Op05M0 },      { Op06M0 },      { Op07M0 },      { Op08E0 },      { Op09M0 },
	{ Op0AM0 },      { Op0BE0 },      { Op0CM0 },      { Op0DM0 },      { Op0EM0 },
	{ Op0FM0 },      { Op10E0 },      { Op11E0M0X0 },  { Op12E0M0 },    { Op13M0 },
	{ Op14M0 },      { Op15E0M0 },    { Op16E0M0 },    { Op17M0 },      { Op18 },
	{ Op19M0X0 },    { Op1AM0 },      { Op1B },        { Op1CM0 },      { Op1DM0X0 },
	{ Op1EM0X0 },    { Op1FM0 },      { Op20E0 },      { Op21E0M0 },    { Op22E0 },
	{ Op23M0 },      { Op24M0 },      { Op25M0 },      { Op26M0 },      { Op27M0 },
	{ Op28E0 },      { Op29M0 },      { Op2AM0 },      { Op2BE0 },      { Op2CM0 },
	{ Op2DM0 },      { Op2EM0 },      { Op2FM0 },      { Op30E0 },      { Op31E0M0X0 },
	{ Op32E0M0 },    { Op33M0 },      { Op34E0M0 },    { Op35E0M0 },    { Op36E0M0 },
	{ Op37M0 },      { Op38 },        { Op39M0X0 },    { Op3AM0 },      { Op3B },
	{ Op3CM0X0 },    { Op3DM0X0 },    { Op3EM0X0 },    { Op3FM0 },      { Op40Slow },
	{ Op41E0M0 },    { Op42 },        { Op43M0 },      { Op44X0 },      { Op45M0 },
	{ Op46M0 },      { Op47M0 },      { Op48E0M0 },    { Op49M0 },      { Op4AM0 },
	{ Op4BE0 },      { Op4C },        { Op4DM0 },      { Op4EM0 },      { Op4FM0 },
	{ Op50E0 },      { Op51E0M0X0 },  { Op52E0M0 },    { Op53M0 },      { Op54X0 },
	{ Op55E0M0 },    { Op56E0M0 },    { Op57M0 },      { Op58 },        { Op59M0X0 },
	{ Op5AE0X0 },    { Op5B },        { Op5C },        { Op5DM0X0 },    { Op5EM0X0 },
	{ Op5FM0 },      { Op60E0 },      { Op61E0M0 },    { Op62E0 },      { Op63M0 },
	{ Op64M0 },      { Op65M0 },      { Op66M0 },      { Op67M0 },      { Op68E0M0 },
	{ Op69M0 },      { Op6AM0 },      { Op6BE0 },      { Op6C },        { Op6DM0 },
	{ Op6EM0 },      { Op6FM0 },      { Op70E0 },      { Op71E0M0X0 },  { Op72E0M0 },
	{ Op73M0 },      { Op74E0M0 },    { Op75E0M0 },    { Op76E0M0 },    { Op77M0 },
	{ Op78 },        { Op79M0X0 },    { Op7AE0X0 },    { Op7B },        { Op7C },
	{ Op7DM0X0 },    { Op7EM0X0 },    { Op7FM0 },      { Op80E0 },      { Op81E0M0 },
	{ Op82 },        { Op83M0 },      { Op84X0 },      { Op85M0 },      { Op86X0 },
	{ Op87M0 },      { Op88X0 },      { Op89M0 },      { Op8AM0 },      { Op8BE0 },
	{ Op8CX0 },      { Op8DM0 },      { Op8EX0 },      { Op8FM0 },      { Op90E0 },
	{ Op91E0M0X0 },  { Op92E0M0 },    { Op93M0 },      { Op94E0X0 },    { Op95E0M0 },
	{ Op96E0X0 },    { Op97M0 },      { Op98M0 },      { Op99M0X0 },    { Op9A },
	{ Op9BX0 },      { Op9CM0 },      { Op9DM0X0 },    { Op9EM0X0 },    { Op9FM0 },
	{ OpA0X0 },      { OpA1E0M0 },    { OpA2X0 },      { OpA3M0 },      { OpA4X0 },
	{ OpA5M0 },      { OpA6X0 },      { OpA7M0 },      { OpA8X0 },      { OpA9M0 },
	{ OpAAX0 },      { OpABE0 },      { OpACX0 },      { OpADM0 },      { OpAEX0 },
	{ OpAFM0 },      { OpB0E0 },      { OpB1E0M0X0 },  { OpB2E0M0 },    { OpB3M0 },
	{ OpB4E0X0 },    { OpB5E0M0 },    { OpB6E0X0 },    { OpB7M0 },      { OpB8 },
	{ OpB9M0X0 },    { OpBAX0 },      { OpBBX0 },      { OpBCX0 },      { OpBDM0X0 },
	{ OpBEX0 },      { OpBFM0 },      { OpC0X0 },      { OpC1E0M0 },    { OpC2 },
	{ OpC3M0 },      { OpC4X0 },      { OpC5M0 },      { OpC6M0 },      { OpC7M0 },
	{ OpC8X0 },      { OpC9M0 },      { OpCAX0 },      { OpCB },        { OpCCX0 },
	{ OpCDM0 },      { OpCEM0 },      { OpCFM0 },      { OpD0E0 },      { OpD1E0M0X0 },
	{ OpD2E0M0 },    { OpD3M0 },      { OpD4E0 },      { OpD5E0M0 },    { OpD6E0M0 },
	{ OpD7M0 },      { OpD8 },        { OpD9M0X0 },    { OpDAE0X0 },    { OpDB },
	{ OpDC },        { OpDDM0X0 },    { OpDEM0X0 },    { OpDFM0 },      { OpE0X0 },
	{ OpE1E0M0 },    { OpE2 },        { OpE3M0 },      { OpE4X0 },      { OpE5M0 },
	{ OpE6M0 },      { OpE7M0 },      { OpE8X0 },      { OpE9M0 },      { OpEA },
	{ OpEB },        { OpECX0 },      { OpEDM0 },      { OpEEM0 },      { OpEFM0 },
	{ OpF0E0 },      { OpF1E0M0X0 },  { OpF2E0M0 },    { OpF3M0 },      { OpF4E0 },
	{ OpF5E0M0 },    { OpF6E0M0 },    { OpF7M0 },      { OpF8 },        { OpF9M0X0 },
	{ OpFAE0X0 },    { OpFB },        { OpFCE0 },      { OpFDM0X0 },    { OpFEM0X0 },
	{ OpFFM0 }
};

const struct SOpcodes S9xOpcodesM0X1[256] =
{
	{ Op00 },        { Op01E0M0 },    { Op02 },        { Op03M0 },      { Op04M0 },
	{ Op05M0 },      { Op06M0 },      { Op07M0 },      { Op08E0 },      { Op09M0 },
	{ Op0AM0 },      { Op0BE0 },      { Op0CM0 },      { Op0DM0 },      { Op0EM0 },
	{ Op0FM0 },      { Op10E0 },      { Op11E0M0X1 },  { Op12E0M0 },    { Op13M0 },
	{ Op14M0 },      { Op15E0M0 },    { Op16E0M0 },    { Op17M0 },      { Op18 },
	{ Op19M0X1 },    { Op1AM0 },      { Op1B },        { Op1CM0 },      { Op1DM0X1 },
	{ Op1EM0X1 },    { Op1FM0 },      { Op20E0 },      { Op21E0M0 },    { Op22E0 },
	{ Op23M0 },      { Op24M0 },      { Op25M0 },      { Op26M0 },      { Op27M0 },
	{ Op28E0 },      { Op29M0 },      { Op2AM0 },      { Op2BE0 },      { Op2CM0 },
	{ Op2DM0 },      { Op2EM0 },      { Op2FM0 },      { Op30E0 },      { Op31E0M0X1 },
	{ Op32E0M0 },    { Op33M0 },      { Op34E0M0 },    { Op35E0M0 },    { Op36E0M0 },
	{ Op37M0 },      { Op38 },        { Op39M0X1 },    { Op3AM0 },      { Op3B },
	{ Op3CM0X1 },    { Op3DM0X1 },    { Op3EM0X1 },    { Op3FM0 },      { Op40Slow },
	{ Op41E0M0 },    { Op42 },        { Op43M0 },      { Op44X1 },      { Op45M0 },
	{ Op46M0 },      { Op47M0 },      { Op48E0M0 },    { Op49M0 },      { Op4AM0 },
	{ Op4BE0 },      { Op4C },        { Op4DM0 },      { Op4EM0 },      { Op4FM0 },
	{ Op50E0 },      { Op51E0M0X1 },  { Op52E0M0 },    { Op53M0 },      { Op54X1 },
	{ Op55E0M0 },    { Op56E0M0 },    { Op57M0 },      { Op58 },        { Op59M0X1 },
	{ Op5AE0X1 },    { Op5B },        { Op5C },        { Op5DM0X1 },    { Op5EM0X1 },
	{ Op5FM0 },      { Op60E0 },      { Op61E0M0 },    { Op62E0 },      { Op63M0 },
	{ Op64M0 },      { Op65M0 },      { Op66M0 },      { Op67M0 },      { Op68E0M0 },
	{ Op69M0 },      { Op6AM0 },      { Op6BE0 },      { Op6C },        { Op6DM0 },
	{ Op6EM0 },      { Op6FM0 },      { Op70E0 },      { Op71E0M0X1 },  { Op72E0M0 },
	{ Op73M0 },      { Op74E0M0 },    { Op75E0M0 },    { Op76E0M0 },    { Op77M0 },
	{ Op78 },        { Op79M0X1 },    { Op7AE0X1 },    { Op7B },        { Op7C },
	{ Op7DM0X1 },    { Op7EM0X1 },    { Op7FM0 },      { Op80E0 },      { Op81E0M0 },
	{ Op82 },        { Op83M0 },      { Op84X1 },      { Op85M0 },      { Op86X1 },
	{ Op87M0 },      { Op88X1 },      { Op89M0 },      { Op8AM0 },      { Op8BE0 },
	{ Op8CX1 },      { Op8DM0 },      { Op8EX1 },      { Op8FM0 },      { Op90E0 },
	{ Op91E0M0X1 },  { Op92E0M0 },    { Op93M0 },      { Op94E0X1 },    { Op95E0M0 },
	{ Op96E0X1 },    { Op97M0 },      { Op98M0 },      { Op99M0X1 },    { Op9A },
	{ Op9BX1 },      { Op9CM0 },      { Op9DM0X1 },    { Op9EM0X1 },    { Op9FM0 },
	{ OpA0X1 },      { OpA1E0M0 },    { OpA2X1 },      { OpA3M0 },      { OpA4X1 },
	{ OpA5M0 },      { OpA6X1 },      { OpA7M0 },      { OpA8X1 },      { OpA9M0 },
	{ OpAAX1 },      { OpABE0 },      { OpACX1 },      { OpADM0 },      { OpAEX1 },
	{ OpAFM0 },      { OpB0E0 },      { OpB1E0M0X1 },  { OpB2E0M0 },    { OpB3M0 },
	{ OpB4E0X1 },    { OpB5E0M0 },    { OpB6E0X1 },    { OpB7M0 },      { OpB8 },
	{ OpB9M0X1 },    { OpBAX1 },      { OpBBX1 },      { OpBCX1 },      { OpBDM0X1 },
	{ OpBEX1 },      { OpBFM0 },      { OpC0X1 },      { OpC1E0M0 },    { OpC2 },
	{ OpC3M0 },      { OpC4X1 },      { OpC5M0 },      { OpC6M0 },      { OpC7M0 },
	{ OpC8X1 },      { OpC9M0 },      { OpCAX1 },      { OpCB },        { OpCCX1 },
	{ OpCDM0 },      { OpCEM0 },      { OpCFM0 },      { OpD0E0 },      { OpD1E0M0X1 },
	{ OpD2E0M0 },    { OpD3M0 },      { OpD4E0 },      { OpD5E0M0 },    { OpD6E0M0 },
	{ OpD7M0 },      { OpD8 },        { OpD9M0X1 },    { OpDAE0X1 },    { OpDB },
	{ OpDC },        { OpDDM0X1 },    { OpDEM0X1 },    { OpDFM0 },      { OpE0X1 },
	{ OpE1E0M0 },    { OpE2 },        { OpE3M0 },      { OpE4X1 },      { OpE5M0 },
	{ OpE6M0 },      { OpE7M0 },      { OpE8X1 },      { OpE9M0 },      { OpEA },
	{ OpEB },        { OpECX1 },      { OpEDM0 },      { OpEEM0 },      { OpEFM0 },
	{ OpF0E0 },      { OpF1E0M0X1 },  { OpF2E0M0 },    { OpF3M0 },      { OpF4E0 },
	{ OpF5E0M0 },    { OpF6E0M0 },    { OpF7M0 },      { OpF8 },        { OpF9M0X1 },
	{ OpFAE0X1 },    { OpFB },        { OpFCE0 },      { OpFDM0X1 },    { OpFEM0X1 },
	{ OpFFM0 }
};

const struct SOpcodes S9xOpcodesSlow[256] =
{
	{ Op00 },        { Op01Slow },    { Op02 },        { Op03Slow },    { Op04Slow },
	{ Op05Slow },    { Op06Slow },    { Op07Slow },    { Op08Slow },    { Op09Slow },
	{ Op0ASlow },    { Op0BSlow },    { Op0CSlow },    { Op0DSlow },    { Op0ESlow },
	{ Op0FSlow },    { Op10Slow },    { Op11Slow },    { Op12Slow },    { Op13Slow },
	{ Op14Slow },    { Op15Slow },    { Op16Slow },    { Op17Slow },    { Op18 },
	{ Op19Slow },    { Op1ASlow },    { Op1B },        { Op1CSlow },    { Op1DSlow },
	{ Op1ESlow },    { Op1FSlow },    { Op20Slow },    { Op21Slow },    { Op22Slow },
	{ Op23Slow },    { Op24Slow },    { Op25Slow },    { Op26Slow },    { Op27Slow },
	{ Op28Slow },    { Op29Slow },    { Op2ASlow },    { Op2BSlow },    { Op2CSlow },
	{ Op2DSlow },    { Op2ESlow },    { Op2FSlow },    { Op30Slow },    { Op31Slow },
	{ Op32Slow },    { Op33Slow },    { Op34Slow },    { Op35Slow },    { Op36Slow },
	{ Op37Slow },    { Op38 },        { Op39Slow },    { Op3ASlow },    { Op3B },
	{ Op3CSlow },    { Op3DSlow },    { Op3ESlow },    { Op3FSlow },    { Op40Slow },
	{ Op41Slow },    { Op42 },        { Op43Slow },    { Op44Slow },    { Op45Slow },
	{ Op46Slow },    { Op47Slow },    { Op48Slow },    { Op49Slow },    { Op4ASlow },
	{ Op4BSlow },    { Op4CSlow },    { Op4DSlow },    { Op4ESlow },    { Op4FSlow },
	{ Op50Slow },    { Op51Slow },    { Op52Slow },    { Op53Slow },    { Op54Slow },
	{ Op55Slow },    { Op56Slow },    { Op57Slow },    { Op58 },        { Op59Slow },
	{ Op5ASlow },    { Op5B },        { Op5CSlow },    { Op5DSlow },    { Op5ESlow },
	{ Op5FSlow },    { Op60Slow },    { Op61Slow },    { Op62Slow },    { Op63Slow },
	{ Op64Slow },    { Op65Slow },    { Op66Slow },    { Op67Slow },    { Op68Slow },
	{ Op69Slow },    { Op6ASlow },    { Op6BSlow },    { Op6CSlow },    { Op6DSlow },
	{ Op6ESlow },    { Op6FSlow },    { Op70Slow },    { Op71Slow },    { Op72Slow },
	{ Op73Slow },    { Op74Slow },    { Op75Slow },    { Op76Slow },    { Op77Slow },
	{ Op78 },        { Op79Slow },    { Op7ASlow },    { Op7B },        { Op7CSlow },
	{ Op7DSlow },    { Op7ESlow },    { Op7FSlow },    { Op80Slow },    { Op81Slow },
	{ Op82Slow },    { Op83Slow },    { Op84Slow },    { Op85Slow },    { Op86Slow },
	{ Op87Slow },    { Op88Slow },    { Op89Slow },    { Op8ASlow },    { Op8BSlow },
	{ Op8CSlow },    { Op8DSlow },    { Op8ESlow },    { Op8FSlow },    { Op90Slow },
	{ Op91Slow },    { Op92Slow },    { Op93Slow },    { Op94Slow },    { Op95Slow },
	{ Op96Slow },    { Op97Slow },    { Op98Slow },    { Op99Slow },    { Op9A },
	{ Op9BSlow },    { Op9CSlow },    { Op9DSlow },    { Op9ESlow },    { Op9FSlow },
	{ OpA0Slow },    { OpA1Slow },    { OpA2Slow },    { OpA3Slow },    { OpA4Slow },
	{ OpA5Slow },    { OpA6Slow },    { OpA7Slow },    { OpA8Slow },    { OpA9Slow },
	{ OpAASlow },    { OpABSlow },    { OpACSlow },    { OpADSlow },    { OpAESlow },
	{ OpAFSlow },    { OpB0Slow },    { OpB1Slow },    { OpB2Slow },    { OpB3Slow },
	{ OpB4Slow },    { OpB5Slow },    { OpB6Slow },    { OpB7Slow },    { OpB8 },
	{ OpB9Slow },    { OpBASlow },    { OpBBSlow },    { OpBCSlow },    { OpBDSlow },
	{ OpBESlow },    { OpBFSlow },    { OpC0Slow },    { OpC1Slow },    { OpC2Slow },
	{ OpC3Slow },    { OpC4Slow },    { OpC5Slow },    { OpC6Slow },    { OpC7Slow },
	{ OpC8Slow },    { OpC9Slow },    { OpCASlow },    { OpCB },        { OpCCSlow },
	{ OpCDSlow },    { OpCESlow },    { OpCFSlow },    { OpD0Slow },    { OpD1Slow },
	{ OpD2Slow },    { OpD3Slow },    { OpD4Slow },    { OpD5Slow },    { OpD6Slow },
	{ OpD7Slow },    { OpD8 },        { OpD9Slow },    { OpDASlow },    { OpDB },
	{ OpDCSlow },    { OpDDSlow },    { OpDESlow },    { OpDFSlow },    { OpE0Slow },
	{ OpE1Slow },    { OpE2Slow },    { OpE3Slow },    { OpE4Slow },    { OpE5Slow },
	{ OpE6Slow },    { OpE7Slow },    { OpE8Slow },    { OpE9Slow },    { OpEA },
	{ OpEB },        { OpECSlow },    { OpEDSlow },    { OpEESlow },    { OpEFSlow },
	{ OpF0Slow },    { OpF1Slow },    { OpF2Slow },    { OpF3Slow },    { OpF4Slow },
	{ OpF5Slow },    { OpF6Slow },    { OpF7Slow },    { OpF8 },        { OpF9Slow },
	{ OpFASlow },    { OpFB },        { OpFCSlow },    { OpFDSlow },    { OpFESlow },
	{ OpFFSlow }
};
