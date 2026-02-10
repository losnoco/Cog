/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#include <algorithm>
#include <snes9x/snes9x.h>

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
	st->CPU.NMIPending = st->CPU.IRQLine = st->CPU.IRQTransition = false;
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
	st->Timings.NextIRQTimer = 0x0fffffff;
	st->Timings.IRQFlagChanging = IRQ_NONE;

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
	std::fill_n(&st->Memory.RAM[0], 0x20000, 0x55);
	std::fill_n(&st->Memory.VRAM[0], 0x10000, 0);
	std::fill_n(&st->Memory.FillRAM[0], 0x8000, 0);

	S9xResetCPU(st);
	S9xResetPPU(st);
	S9xResetDMA(st);
	S9xResetAPU(st);
}
