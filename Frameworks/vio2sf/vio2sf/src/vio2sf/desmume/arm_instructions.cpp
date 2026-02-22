/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2006-2007 shash
	Copyright (C) 2008-2012 DeSmuME team

	This file is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This file is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with the this software.  If not, see <http://www.gnu.org/licenses/>.
*/

// ARM core TODO:
// - Check all the LDM/STM opcodes: quirks when Rb included in Rlist; opcodes
//     operating on user registers (LDMXX2/STMXX2)
// - Force User mode memory access for LDRx/STRx opcodes with bit24=0 and bit21=1
//     (has to be done at memory side; once the PU is emulated well enough)
// - Check LDMxx2/STMxx2 (those opcodes that act on User mode registers instead
//     of current ones)

#define ST (cpu->st)

#include "vio2sf.h"

#define TEMPLATE template<int PROCNUM>

// -----------------------------------------------------------------------------
//   Shifting macros
// -----------------------------------------------------------------------------

#define LSL_IMM \
	uint32_t shift_op = cpu->R[REG_POS(i, 0)] << ((i >> 7) & 0x1F);

#define S_LSL_IMM \
	uint32_t shift_op = (i >> 7) & 0x1F; \
	uint32_t c = cpu->CPSR.bits.C; \
	if (!shift_op) \
		shift_op = cpu->R[REG_POS(i, 0)]; \
	else \
	{ \
		c = BIT_N(cpu->R[REG_POS(i, 0)], 32 - shift_op); \
		shift_op = cpu->R[REG_POS(i, 0)] << shift_op; \
	}

#define LSL_REG \
	uint32_t shift_op = cpu->R[REG_POS(i, 8)] & 0xFF; \
	if (shift_op >= 32) \
		shift_op = 0; \
	else \
		shift_op = cpu->R[REG_POS(i, 0)] << shift_op;

#define S_LSL_REG \
	uint32_t shift_op = cpu->R[REG_POS(i,8)] & 0xFF; \
	uint32_t c = cpu->CPSR.bits.C; \
	if (!shift_op) \
		shift_op = cpu->R[REG_POS(i, 0)]; \
	else if (shift_op < 32) \
	{ \
		c = BIT_N(cpu->R[REG_POS(i, 0)], 32 - shift_op); \
		shift_op = cpu->R[REG_POS(i, 0)] << shift_op; \
	} \
	else if (shift_op == 32) \
	{ \
		shift_op = 0; \
		c = BIT0(cpu->R[REG_POS(i, 0)]); \
	} \
	else \
	{ \
		shift_op = 0; \
		c = 0; \
	}

#define LSR_IMM \
	uint32_t shift_op = (i >> 7) & 0x1F; \
	if (shift_op) \
		shift_op = cpu->R[REG_POS(i, 0)] >> shift_op;

#define S_LSR_IMM \
	uint32_t shift_op = (i >> 7) & 0x1F; \
	uint32_t c = cpu->CPSR.bits.C; \
	if (!shift_op) \
		c = BIT31(cpu->R[REG_POS(i, 0)]); \
	else \
	{ \
		c = BIT_N(cpu->R[REG_POS(i, 0)], shift_op - 1); \
		shift_op = cpu->R[REG_POS(i, 0)]>>shift_op; \
	}

#define LSR_REG \
	uint32_t shift_op = cpu->R[REG_POS(i,8)] & 0xFF; \
	if (shift_op >= 32) \
		shift_op = 0; \
	else \
		shift_op = cpu->R[REG_POS(i, 0)] >> shift_op;

#define S_LSR_REG \
	uint32_t shift_op = cpu->R[REG_POS(i,8)] & 0xFF; \
	uint32_t c = cpu->CPSR.bits.C; \
	if (!shift_op) \
		shift_op = cpu->R[REG_POS(i ,0)]; \
	else if (shift_op < 32) \
	{ \
		c = BIT_N(cpu->R[REG_POS(i, 0)], shift_op - 1); \
		shift_op = cpu->R[REG_POS(i, 0)] >> shift_op; \
	} \
	else if (shift_op == 32) \
	{ \
		c = BIT31(cpu->R[REG_POS(i, 0)]); \
		shift_op = 0; \
	} \
	else \
	{ \
		c = 0; \
		shift_op = 0; \
	}

#define ASR_IMM \
	uint32_t shift_op = (i >> 7) & 0x1F; \
	if (!shift_op) \
		shift_op = BIT31(cpu->R[REG_POS(i, 0)]) * 0xFFFFFFFF; \
	else \
		shift_op = static_cast<uint32_t>(static_cast<int32_t>(cpu->R[REG_POS(i, 0)]) >> shift_op);

#define S_ASR_IMM \
	uint32_t shift_op = (i >> 7) & 0x1F; \
	uint32_t c = cpu->CPSR.bits.C; \
	if (!shift_op) \
	{ \
		shift_op = BIT31(cpu->R[REG_POS(i, 0)]) * 0xFFFFFFFF; \
		c = BIT31(cpu->R[REG_POS(i, 0)]); \
	} \
	else \
	{ \
		c = BIT_N(cpu->R[REG_POS(i, 0)], shift_op - 1); \
		shift_op = static_cast<uint32_t>(static_cast<int32_t>(cpu->R[REG_POS(i, 0)]) >> shift_op); \
	}

#define ASR_REG \
	uint32_t shift_op = cpu->R[REG_POS(i,8)] & 0xFF; \
	if (!shift_op) \
		shift_op = cpu->R[REG_POS(i, 0)]; \
	else if (shift_op < 32) \
		shift_op = static_cast<uint32_t>(static_cast<int32_t>(cpu->R[REG_POS(i, 0)]) >> shift_op); \
	else \
		shift_op = BIT31(cpu->R[REG_POS(i, 0)]) * 0xFFFFFFFF;

#define S_ASR_REG \
	uint32_t shift_op = cpu->R[REG_POS(i, 8)] & 0xFF; \
	uint32_t c = cpu->CPSR.bits.C; \
	if (!shift_op) \
		shift_op = cpu->R[REG_POS(i, 0)]; \
	else if (shift_op < 32) \
	{ \
		c = BIT_N(cpu->R[REG_POS(i, 0)], shift_op - 1); \
		shift_op = static_cast<uint32_t>(static_cast<int32_t>(cpu->R[REG_POS(i, 0)]) >> shift_op); \
	} \
	else \
	{ \
		c = BIT31(cpu->R[REG_POS(i, 0)]); \
		shift_op = BIT31(cpu->R[REG_POS(i, 0)]) * 0xFFFFFFFF; \
	}

#define ROR_IMM \
	uint32_t shift_op = (i >> 7) & 0x1F; \
	if (!shift_op) \
		shift_op = (static_cast<uint32_t>(cpu->CPSR.bits.C) << 31) | (cpu->R[REG_POS(i, 0)] >> 1); \
	else \
		shift_op = ROR(cpu->R[REG_POS(i, 0)], shift_op);

#define S_ROR_IMM \
	uint32_t shift_op = (i >> 7) & 0x1F; \
	uint32_t c = cpu->CPSR.bits.C; \
	if (!shift_op) \
	{ \
		shift_op = (static_cast<uint32_t>(cpu->CPSR.bits.C) << 31) | (cpu->R[REG_POS(i, 0)] >> 1); \
		c = BIT0(cpu->R[REG_POS(i, 0)]); \
	} \
	else \
	{ \
		c = BIT_N(cpu->R[REG_POS(i, 0)], shift_op - 1); \
		shift_op = ROR(cpu->R[REG_POS(i, 0)], shift_op); \
	}

#define ROR_REG \
	uint32_t shift_op = cpu->R[REG_POS(i, 8)] & 0xFF; \
	if (!shift_op || !(shift_op & 0x1F)) \
		shift_op = cpu->R[REG_POS(i, 0)]; \
	else \
		shift_op = ROR(cpu->R[REG_POS(i, 0)], shift_op & 0x1F);

#define S_ROR_REG \
	uint32_t shift_op = cpu->R[REG_POS(i, 8)] & 0xFF; \
	uint32_t c = cpu->CPSR.bits.C; \
	if (!shift_op) \
		shift_op = cpu->R[REG_POS(i, 0)]; \
	else \
	{ \
		shift_op &= 0x1F; \
		if (!shift_op) \
		{ \
			shift_op = cpu->R[REG_POS(i, 0)]; \
			c = BIT31(cpu->R[REG_POS(i, 0)]); \
		} \
		else \
		{ \
			c = BIT_N(cpu->R[REG_POS(i, 0)], shift_op - 1); \
			shift_op = ROR(cpu->R[REG_POS(i, 0)], shift_op); \
		} \
	}

#define IMM_VALUE \
	uint32_t shift_op = ROR(i & 0xFF, (i >> 7) & 0x1E);

#define S_IMM_VALUE \
	uint32_t shift_op = ROR(i & 0xFF, (i >> 7) & 0x1E); \
	uint32_t c = cpu->CPSR.bits.C; \
	if ((i >> 8) & 0xF) \
		c = BIT31(shift_op);

#define IMM_OFF (((i >> 4) & 0xF0) + (i & 0xF))

#define IMM_OFF_12 ((i) & 0xFFF)

// -----------------------------------------------------------------------------
//   Undefined instruction
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_UND(armcpu_t *cpu, uint32_t)
{
	TRAPUNDEF(cpu);
	return 1;
}

// -----------------------------------------------------------------------------
//   AND / ANDS
//   Timing: OK
// -----------------------------------------------------------------------------

#define OP_AND(a, b) \
	cpu->R[REG_POS(i, 12)] = cpu->R[REG_POS(i, 16)] & shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	return a;

#define OP_ANDS(a, b) \
	cpu->R[REG_POS(i, 12)] = cpu->R[REG_POS(i, 16)] & shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		Status_Reg SPSR = cpu->SPSR; \
		armcpu_switchMode(cpu, SPSR.bits.mode); \
		cpu->CPSR = SPSR; \
		cpu->changeCPSR(); \
		cpu->R[15] &= 0xFFFFFFFC | (static_cast<uint32_t>(cpu->CPSR.bits.T) << 1); \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	cpu->CPSR.bits.C = c; \
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 12)]); \
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 12)]; \
	return a;

TEMPLATE static uint32_t FASTCALL OP_AND_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_AND(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_AND_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	OP_AND(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_AND_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_AND(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_AND_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	OP_AND(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_AND_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_AND(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_AND_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_AND(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_AND_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_AND(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_AND_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_AND(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_AND_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_AND(1, 3);
}


TEMPLATE static uint32_t FASTCALL OP_AND_S_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSL_IMM;
	OP_ANDS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_AND_S_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSL_REG;
	OP_ANDS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_AND_S_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSR_IMM;
	OP_ANDS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_AND_S_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSR_REG;
	OP_ANDS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_AND_S_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ASR_IMM;
	OP_ANDS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_AND_S_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ASR_REG;
	OP_ANDS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_AND_S_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ROR_IMM;
	OP_ANDS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_AND_S_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ROR_REG;
	OP_ANDS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_AND_S_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	S_IMM_VALUE;
	OP_ANDS(1, 3);
}

// -----------------------------------------------------------------------------
//   EOR / EORS
// -----------------------------------------------------------------------------

#define OP_EOR(a, b) \
	cpu->R[REG_POS(i, 12)] = cpu->R[REG_POS(i, 16)] ^ shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	return a;

#define OP_EORS(a, b) \
	cpu->R[REG_POS(i, 12)] = cpu->R[REG_POS(i, 16)] ^ shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		Status_Reg SPSR = cpu->SPSR; \
		armcpu_switchMode(cpu, SPSR.bits.mode); \
		cpu->CPSR = SPSR; \
		cpu->changeCPSR(); \
		cpu->R[15] &= 0xFFFFFFFC | (static_cast<uint32_t>(cpu->CPSR.bits.T) << 1); \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	cpu->CPSR.bits.C = c; \
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 12)]); \
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 12)]; \
	return a;

TEMPLATE static uint32_t FASTCALL OP_EOR_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_EOR(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	OP_EOR(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_EOR(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	OP_EOR(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_EOR(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_EOR(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_EOR(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_EOR(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_EOR(1, 3);
}


TEMPLATE static uint32_t FASTCALL OP_EOR_S_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSL_IMM;
	OP_EORS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_S_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSL_REG;
	OP_EORS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_S_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSR_IMM;
	OP_EORS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_S_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSR_REG;
	OP_EORS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_S_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ASR_IMM;
	OP_EORS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_S_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ASR_REG;
	OP_EORS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_S_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ROR_IMM;
	OP_EORS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_S_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ROR_REG;
	OP_EORS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_EOR_S_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	S_IMM_VALUE;
	OP_EORS(1, 3);
}

// -----------------------------------------------------------------------------
//   SUB / SUBS
// -----------------------------------------------------------------------------

#define OP_SUB(a, b) \
	cpu->R[REG_POS(i, 12)] = cpu->R[REG_POS(i, 16)] - shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	return a;

#define OP_SUBS(a, b) \
	cpu->R[REG_POS(i, 12)] = v - shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		Status_Reg SPSR = cpu->SPSR; \
		armcpu_switchMode(cpu, SPSR.bits.mode); \
		cpu->CPSR = SPSR; \
		cpu->changeCPSR(); \
		cpu->R[15] &= 0xFFFFFFFC | (static_cast<uint32_t>(cpu->CPSR.bits.T) << 1); \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 12)]); \
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 12)]; \
	cpu->CPSR.bits.C = !BorrowFrom(v, shift_op); \
	cpu->CPSR.bits.V = OverflowFromSUB(cpu->R[REG_POS(i,12)], v, shift_op); \
	return a;

TEMPLATE static uint32_t FASTCALL OP_SUB_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_SUB(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	OP_SUB(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_SUB(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	OP_SUB(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_SUB(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_SUB(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_SUB(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_SUB(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_SUB(1, 3);
}


TEMPLATE static uint32_t FASTCALL OP_SUB_S_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSL_IMM;
	OP_SUBS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_S_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSL_REG;
	OP_SUBS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_S_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSR_IMM;
	OP_SUBS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_S_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSR_REG;
	OP_SUBS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_S_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ASR_IMM;
	OP_SUBS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_S_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ASR_REG;
	OP_SUBS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_S_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ROR_IMM;
	OP_SUBS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_S_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ROR_REG;
	OP_SUBS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SUB_S_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	IMM_VALUE;
	OP_SUBS(1, 3);
}

// -----------------------------------------------------------------------------
//   RSB / RSBS
// -----------------------------------------------------------------------------

#define OP_RSB(a, b) \
	cpu->R[REG_POS(i, 12)] = shift_op - cpu->R[REG_POS(i, 16)]; \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	return a;

#define OP_RSBS(a, b) \
	cpu->R[REG_POS(i, 12)] = shift_op - v; \
	if (REG_POS(i, 12) == 15) \
	{ \
		Status_Reg SPSR = cpu->SPSR; \
		armcpu_switchMode(cpu, SPSR.bits.mode); \
		cpu->CPSR = SPSR; \
		cpu->changeCPSR(); \
		cpu->R[15] &= 0xFFFFFFFC | (static_cast<uint32_t>(cpu->CPSR.bits.T) << 1); \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 12)]); \
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 12)]; \
	cpu->CPSR.bits.C = !BorrowFrom(shift_op, v); \
	cpu->CPSR.bits.V = OverflowFromSUB(cpu->R[REG_POS(i, 12)], shift_op, v); \
	return a;

TEMPLATE static uint32_t FASTCALL OP_RSB_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_RSB(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	OP_RSB(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_RSB(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	OP_RSB(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_RSB(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_RSB(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_RSB(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_RSB(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_RSB(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_S_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSL_IMM;
	OP_RSBS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_S_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSL_REG;
	OP_RSBS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_S_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSR_IMM;
	OP_RSBS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_S_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSR_REG;
	OP_RSBS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_S_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ASR_IMM;
	OP_RSBS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_S_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ASR_REG;
	OP_RSBS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_S_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ROR_IMM;
	OP_RSBS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_S_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ROR_REG;
	OP_RSBS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_RSB_S_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	IMM_VALUE;
	OP_RSBS(1, 3);
}

// -----------------------------------------------------------------------------
//   ADD / ADDS
// -----------------------------------------------------------------------------

#define OP_ADD(a, b) \
	cpu->R[REG_POS(i, 12)] = cpu->R[REG_POS(i, 16)] + shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	return a;

#define OP_ADDS(a, b) \
	cpu->R[REG_POS(i,12)] = v + shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		Status_Reg SPSR = cpu->SPSR; \
		armcpu_switchMode(cpu, SPSR.bits.mode); \
		cpu->CPSR = SPSR; \
		cpu->changeCPSR(); \
		cpu->R[15] &= 0xFFFFFFFC | (static_cast<uint32_t>(cpu->CPSR.bits.T) << 1); \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 12)]); \
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 12)]; \
	cpu->CPSR.bits.C = CarryFrom(v, shift_op); \
	cpu->CPSR.bits.V = OverflowFromADD(cpu->R[REG_POS(i, 12)], v, shift_op); \
	return a;

TEMPLATE static uint32_t FASTCALL OP_ADD_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_ADD(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	OP_ADD(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_ADD(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	OP_ADD(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_ADD(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_ADD(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_ADD(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_ADD(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_ADD(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_S_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSL_IMM;
	OP_ADDS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_S_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSL_REG;
	OP_ADDS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_S_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSR_IMM;
	OP_ADDS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_S_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSR_REG;
	OP_ADDS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_S_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ASR_IMM;
	OP_ADDS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_S_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ASR_REG;
	OP_ADDS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_S_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ROR_IMM;
	OP_ADDS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_S_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ROR_REG;
	OP_ADDS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADD_S_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	IMM_VALUE;
	OP_ADDS(1, 3);
}

// -----------------------------------------------------------------------------
//   ADC / ADCS
// -----------------------------------------------------------------------------
#define OP_ADC(a, b) \
	cpu->R[REG_POS(i, 12)] = cpu->R[REG_POS(i, 16)] + shift_op + cpu->CPSR.bits.C; \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	return a;

#define OP_ADCS(a, b) \
{ \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->R[REG_POS(i, 12)] = v + shift_op + cpu->CPSR.bits.C; \
		Status_Reg SPSR = cpu->SPSR; \
		armcpu_switchMode(cpu, SPSR.bits.mode); \
		cpu->CPSR = SPSR; \
		cpu->changeCPSR(); \
		cpu->R[15] &= 0xFFFFFFFC | (static_cast<uint32_t>(cpu->CPSR.bits.T) << 1); \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	if (!cpu->CPSR.bits.C) \
	{ \
		cpu->R[REG_POS(i, 12)] = v + shift_op; \
		cpu->CPSR.bits.C = cpu->R[REG_POS(i, 12)] < v; \
	} \
	else \
	{ \
		cpu->R[REG_POS(i, 12)] = v + shift_op + 1; \
		cpu->CPSR.bits.C = cpu->R[REG_POS(i, 12)] <= v; \
	} \
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 12)]); \
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 12)]; \
	cpu->CPSR.bits.V = BIT31((v ^ shift_op ^ -1) & (v ^ cpu->R[REG_POS(i, 12)])); \
	return a; \
}

TEMPLATE static uint32_t FASTCALL OP_ADC_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_ADC(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	OP_ADC(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_ADC(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	OP_ADC(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_ADC(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_ADC(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_ADC(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_ADC(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_ADC(1, 3);
}


TEMPLATE static uint32_t FASTCALL OP_ADC_S_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSL_IMM;
	OP_ADCS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_S_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSL_REG;
	OP_ADCS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_S_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSR_IMM;
	OP_ADCS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_S_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSR_REG;
	OP_ADCS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_S_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ASR_IMM;
	OP_ADCS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_S_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i,16)];
	ASR_REG;
	OP_ADCS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_S_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ROR_IMM;
	OP_ADCS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_S_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ROR_REG;
	OP_ADCS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ADC_S_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i,16)];
	IMM_VALUE;
	OP_ADCS(1, 3);
}

// -----------------------------------------------------------------------------
//   SBC / SBCS
// -----------------------------------------------------------------------------

#define OP_SBC(a, b) \
	cpu->R[REG_POS(i, 12)] = cpu->R[REG_POS(i, 16)] - shift_op - !cpu->CPSR.bits.C; \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	return a;

#define OP_SBCS(a, b) \
{ \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->R[REG_POS(i,12)] = v - shift_op - !cpu->CPSR.bits.C; \
		Status_Reg SPSR = cpu->SPSR; \
		armcpu_switchMode(cpu, SPSR.bits.mode); \
		cpu->CPSR = SPSR; \
		cpu->changeCPSR(); \
		cpu->R[15] &= 0xFFFFFFFC | (static_cast<uint32_t>(cpu->CPSR.bits.T) << 1); \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	if (!cpu->CPSR.bits.C) \
	{ \
		cpu->R[REG_POS(i, 12)] = v - shift_op - 1; \
		cpu->CPSR.bits.C = v > shift_op; \
	} \
	else \
	{ \
		cpu->R[REG_POS(i, 12)] = v - shift_op; \
		cpu->CPSR.bits.C = v >= shift_op; \
	} \
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 12)]); \
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 12)]; \
	cpu->CPSR.bits.V = BIT31((v ^ shift_op) & (v ^ cpu->R[REG_POS(i, 12)])); \
	return a; \
}

TEMPLATE static uint32_t FASTCALL OP_SBC_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_SBC(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	OP_SBC(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_SBC(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	OP_SBC(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_SBC(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_SBC(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_SBC(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_SBC(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_SBC(1, 3);
}


TEMPLATE static uint32_t FASTCALL OP_SBC_S_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSL_IMM;
	OP_SBCS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_S_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSL_REG;
	OP_SBCS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_S_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSR_IMM;
	OP_SBCS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_S_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSR_REG;
	OP_SBCS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_S_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ASR_IMM;
	OP_SBCS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_S_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ASR_REG;
	OP_SBCS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_S_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i,16)];
	ROR_IMM;
	OP_SBCS(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_S_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ROR_REG;
	OP_SBCS(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_SBC_S_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	IMM_VALUE;
	OP_SBCS(1, 3);
}

// -----------------------------------------------------------------------------
//   RSC / RSCS
// -----------------------------------------------------------------------------

#define OP_RSC(a, b) \
	cpu->R[REG_POS(i, 12)] =  shift_op - cpu->R[REG_POS(i, 16)] + cpu->CPSR.bits.C - 1; \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	return a;

#define OP_RSCS(a, b) \
{ \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->R[REG_POS(i, 12)] = shift_op - v - !cpu->CPSR.bits.C; \
		Status_Reg SPSR = cpu->SPSR; \
		armcpu_switchMode(cpu, SPSR.bits.mode); \
		cpu->CPSR = SPSR; \
		cpu->changeCPSR(); \
		cpu->R[15] &= 0xFFFFFFFC | (static_cast<uint32_t>(cpu->CPSR.bits.T) << 1); \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	if (!cpu->CPSR.bits.C) \
	{ \
		cpu->R[REG_POS(i, 12)] = shift_op - v - 1; \
		cpu->CPSR.bits.C = shift_op > v; \
	} \
	else \
	{ \
		cpu->R[REG_POS(i, 12)] = shift_op - v; \
		cpu->CPSR.bits.C = shift_op >= v; \
	} \
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 12)]); \
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 12)]; \
	cpu->CPSR.bits.V = BIT31((shift_op ^ v) & (shift_op ^ cpu->R[REG_POS(i, 12)])); \
	return a; \
}

TEMPLATE static uint32_t FASTCALL OP_RSC_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_RSC(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	OP_RSC(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_RSC(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	OP_RSC(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_RSC(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_RSC(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_RSC(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_RSC(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_RSC(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_S_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSL_IMM;
	OP_RSCS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_S_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSL_REG;
	OP_RSCS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_S_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i,16)];
	LSR_IMM;
	OP_RSCS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_S_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	LSR_REG;
	OP_RSCS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_S_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ASR_IMM;
	OP_RSCS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_S_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ASR_REG;
	OP_RSCS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_S_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ROR_IMM;
	OP_RSCS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_S_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	ROR_REG;
	OP_RSCS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_RSC_S_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 16)];
	IMM_VALUE;
	OP_RSCS(1,3);
}

// -----------------------------------------------------------------------------
//   TST
// -----------------------------------------------------------------------------

#define OP_TST(a) \
{ \
	uint32_t tmp = cpu->R[REG_POS(i, 16)] & shift_op; \
	cpu->CPSR.bits.C = c; \
	cpu->CPSR.bits.N = BIT31(tmp); \
	cpu->CPSR.bits.Z = !tmp; \
	return a; \
}

TEMPLATE static uint32_t FASTCALL OP_TST_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSL_IMM;
	OP_TST(1);
}

TEMPLATE static uint32_t FASTCALL OP_TST_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSL_REG;
	OP_TST(2);
}

TEMPLATE static uint32_t FASTCALL OP_TST_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSR_IMM;
	OP_TST(1);
}

TEMPLATE static uint32_t FASTCALL OP_TST_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSR_REG;
	OP_TST(2);
}

TEMPLATE static uint32_t FASTCALL OP_TST_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ASR_IMM;
	OP_TST(1);
}

TEMPLATE static uint32_t FASTCALL OP_TST_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ASR_REG;
	OP_TST(2);
}

TEMPLATE static uint32_t FASTCALL OP_TST_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ROR_IMM;
	OP_TST(1);
}

TEMPLATE static uint32_t FASTCALL OP_TST_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ROR_REG;
	OP_TST(2);
}

TEMPLATE static uint32_t FASTCALL OP_TST_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	S_IMM_VALUE;
	OP_TST(1);
}

// -----------------------------------------------------------------------------
//   TEQ
// -----------------------------------------------------------------------------

#define OP_TEQ(a) \
{ \
	unsigned tmp = cpu->R[REG_POS(i, 16)] ^ shift_op; \
	cpu->CPSR.bits.C = c; \
	cpu->CPSR.bits.N = BIT31(tmp); \
	cpu->CPSR.bits.Z = !tmp; \
	return a; \
}

TEMPLATE static uint32_t FASTCALL OP_TEQ_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSL_IMM;
	OP_TEQ(1);
}

TEMPLATE static uint32_t FASTCALL OP_TEQ_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSL_REG;
	OP_TEQ(2);
}

TEMPLATE static uint32_t FASTCALL OP_TEQ_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSR_IMM;
	OP_TEQ(1);
}

TEMPLATE static uint32_t FASTCALL OP_TEQ_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSR_REG;
	OP_TEQ(2);
}

TEMPLATE static uint32_t FASTCALL OP_TEQ_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ASR_IMM;
	OP_TEQ(1);
}

TEMPLATE static uint32_t FASTCALL OP_TEQ_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ASR_REG;
	OP_TEQ(2);
}

TEMPLATE static uint32_t FASTCALL OP_TEQ_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ROR_IMM;
	OP_TEQ(1);
}

TEMPLATE static uint32_t FASTCALL OP_TEQ_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ROR_REG;
	OP_TEQ(2);
}

TEMPLATE static uint32_t FASTCALL OP_TEQ_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	S_IMM_VALUE;
	OP_TEQ(1);
}

// -----------------------------------------------------------------------------
//   CMP
// -----------------------------------------------------------------------------

#define OP_CMP(a) \
{ \
	uint32_t tmp = cpu->R[REG_POS(i, 16)] - shift_op; \
	cpu->CPSR.bits.N = BIT31(tmp); \
	cpu->CPSR.bits.Z = !tmp; \
	cpu->CPSR.bits.C = !BorrowFrom(cpu->R[REG_POS(i, 16)], shift_op); \
	cpu->CPSR.bits.V = OverflowFromSUB(tmp, cpu->R[REG_POS(i, 16)], shift_op); \
	return a; \
}

TEMPLATE static uint32_t FASTCALL OP_CMP_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_CMP(1);
}

TEMPLATE static uint32_t FASTCALL OP_CMP_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	OP_CMP(2);
}

TEMPLATE static uint32_t FASTCALL OP_CMP_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_CMP(1);
}

TEMPLATE static uint32_t FASTCALL OP_CMP_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	OP_CMP(2);
}

TEMPLATE static uint32_t FASTCALL OP_CMP_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_CMP(1);
}

TEMPLATE static uint32_t FASTCALL OP_CMP_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_CMP(2);
}

TEMPLATE static uint32_t FASTCALL OP_CMP_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_CMP(1);
}

TEMPLATE static uint32_t FASTCALL OP_CMP_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_CMP(2);
}

TEMPLATE static uint32_t FASTCALL OP_CMP_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_CMP(1);
}

// -----------------------------------------------------------------------------
//   CMN
// -----------------------------------------------------------------------------

#define OP_CMN(a) \
{ \
	uint32_t tmp = cpu->R[REG_POS(i, 16)] + shift_op; \
	cpu->CPSR.bits.N = BIT31(tmp); \
	cpu->CPSR.bits.Z = !tmp; \
	cpu->CPSR.bits.C = CarryFrom(cpu->R[REG_POS(i, 16)], shift_op); \
	cpu->CPSR.bits.V = OverflowFromADD(tmp, cpu->R[REG_POS(i, 16)], shift_op); \
	return a; \
}

TEMPLATE static uint32_t FASTCALL OP_CMN_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_CMN(1);
}

TEMPLATE static uint32_t FASTCALL OP_CMN_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	OP_CMN(2);
}

TEMPLATE static uint32_t FASTCALL OP_CMN_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_CMN(1);
}

TEMPLATE static uint32_t FASTCALL OP_CMN_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	OP_CMN(2);
}

TEMPLATE static uint32_t FASTCALL OP_CMN_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_CMN(1);
}

TEMPLATE static uint32_t FASTCALL OP_CMN_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_CMN(2);
}

TEMPLATE static uint32_t FASTCALL OP_CMN_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_CMN(1);
}

TEMPLATE static uint32_t FASTCALL OP_CMN_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_CMN(2);
}

TEMPLATE static uint32_t FASTCALL OP_CMN_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_CMN(1);
}

// -----------------------------------------------------------------------------
//   ORR / ORRS
// -----------------------------------------------------------------------------

#define OP_ORR(a, b) \
	cpu->R[REG_POS(i, 12)] = cpu->R[REG_POS(i, 16)] | shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	return a;

#define OP_ORRS(a,b) \
{ \
	cpu->R[REG_POS(i, 12)] = cpu->R[REG_POS(i, 16)] | shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		Status_Reg SPSR = cpu->SPSR; \
		armcpu_switchMode(cpu, SPSR.bits.mode); \
		cpu->CPSR = SPSR; \
		cpu->changeCPSR(); \
		cpu->R[15] &= 0xFFFFFFFC | (static_cast<uint32_t>(cpu->CPSR.bits.T) << 1); \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	cpu->CPSR.bits.C = c; \
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 12)]); \
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 12)]; \
	return a; \
}

TEMPLATE static uint32_t FASTCALL OP_ORR_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_ORR(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	OP_ORR(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_ORR(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	OP_ORR(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_ORR(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_ORR(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_ORR(1, 3);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_ORR(2, 4);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_ORR(1, 3);
}


TEMPLATE static uint32_t FASTCALL OP_ORR_S_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSL_IMM;
	OP_ORRS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_S_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSL_REG;
	OP_ORRS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_S_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSR_IMM;
	OP_ORRS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_S_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSR_REG;
	OP_ORRS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_S_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ASR_IMM;
	OP_ORRS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_S_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ASR_REG;
	OP_ORRS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_S_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ROR_IMM;
	OP_ORRS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_S_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ROR_REG;
	OP_ORRS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_ORR_S_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	S_IMM_VALUE;
	OP_ORRS(1,3);
}

// -----------------------------------------------------------------------------
//   MOV / MOVS
// -----------------------------------------------------------------------------

#define OP_MOV(a, b) \
	cpu->R[REG_POS(i, 12)] = shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->next_instruction = shift_op; \
		return b; \
	} \
	return a;

#define OP_MOVS(a, b) \
	cpu->R[REG_POS(i, 12)] = shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		Status_Reg SPSR = cpu->SPSR; \
		armcpu_switchMode(cpu, SPSR.bits.mode); \
		cpu->CPSR = SPSR; \
		cpu->changeCPSR(); \
		cpu->R[15] &= 0xFFFFFFFC | (static_cast<uint32_t>(cpu->CPSR.bits.T) << 1); \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	cpu->CPSR.bits.C = c; \
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 12)]); \
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 12)]; \
	return a;

TEMPLATE static uint32_t FASTCALL OP_MOV_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	if (i == 0xE1A00000) // nop: MOV R0, R0
		return 1;

	LSL_IMM;
	OP_MOV(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	if (REG_POS(i, 0) == 15)
		shift_op += 4;
	OP_MOV(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_MOV(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	if (REG_POS(i, 0) == 15)
		shift_op += 4;
	OP_MOV(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_MOV(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_MOV(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_MOV(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_MOV(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_MOV(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_S_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSL_IMM;
	OP_MOVS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_S_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSL_REG;
	if (REG_POS(i, 0) == 15)
		shift_op += 4;
	OP_MOVS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_S_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSR_IMM;
	OP_MOVS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_S_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSR_REG;
	if (REG_POS(i, 0) == 15)
		shift_op += 4;
	OP_MOVS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_S_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ASR_IMM;
	OP_MOVS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_S_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ASR_REG;
	OP_MOVS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_S_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ROR_IMM;
	OP_MOVS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_S_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ROR_REG;
	OP_MOVS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MOV_S_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	S_IMM_VALUE;
	OP_MOVS(1,3);
}

// -----------------------------------------------------------------------------
//   BIC / BICS
// -----------------------------------------------------------------------------

#define OP_BIC(a, b) \
	cpu->R[REG_POS(i, 12)] = cpu->R[REG_POS(i, 16)] & ~shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	return a;

#define OP_BICS(a, b) \
	cpu->R[REG_POS(i, 12)] = cpu->R[REG_POS(i, 16)] & ~shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		Status_Reg SPSR = cpu->SPSR; \
		armcpu_switchMode(cpu, SPSR.bits.mode); \
		cpu->CPSR = SPSR; \
		cpu->changeCPSR(); \
		cpu->R[15] &= 0xFFFFFFFC | (static_cast<uint32_t>(cpu->CPSR.bits.T) << 1); \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	cpu->CPSR.bits.C = c; \
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 12)]); \
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 12)]; \
	return a;

TEMPLATE static uint32_t FASTCALL OP_BIC_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_BIC(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	OP_BIC(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_BIC(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	OP_BIC(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_BIC(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_BIC(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_BIC(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_BIC(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_BIC(1,3);
}


TEMPLATE static uint32_t FASTCALL OP_BIC_S_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSL_IMM;
	OP_BICS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_S_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSL_REG;
	OP_BICS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_S_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSR_IMM;
	OP_BICS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_S_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSR_REG;
	OP_BICS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_S_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ASR_IMM;
	OP_BICS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_S_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ASR_REG;
	OP_BICS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_S_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ROR_IMM;
	OP_BICS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_S_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ROR_REG;
	OP_BICS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_BIC_S_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	S_IMM_VALUE;
	OP_BICS(1,3);
}

// -----------------------------------------------------------------------------
//   MVN / MVNS
// -----------------------------------------------------------------------------

#define OP_MVN(a, b) \
	cpu->R[REG_POS(i, 12)] = ~shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	return a;

#define OP_MVNS(a, b) \
	cpu->R[REG_POS(i, 12)] = ~shift_op; \
	if (REG_POS(i, 12) == 15) \
	{ \
		Status_Reg SPSR = cpu->SPSR; \
		armcpu_switchMode(cpu, SPSR.bits.mode); \
		cpu->CPSR = SPSR; \
		cpu->changeCPSR(); \
		cpu->R[15] &= 0xFFFFFFFC | (static_cast<uint32_t>(cpu->CPSR.bits.T) << 1); \
		cpu->next_instruction = cpu->R[15]; \
		return b; \
	} \
	cpu->CPSR.bits.C = c; \
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 12)]); \
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 12)]; \
	return a;

TEMPLATE static uint32_t FASTCALL OP_MVN_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_MVN(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	LSL_REG;
	OP_MVN(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_MVN(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	LSR_REG;
	OP_MVN(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_MVN(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	ASR_REG;
	OP_MVN(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_MVN(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	ROR_REG;
	OP_MVN(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	IMM_VALUE;
	OP_MVN(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_S_LSL_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSL_IMM;
	OP_MVNS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_S_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSL_REG;
	OP_MVNS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_S_LSR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_LSR_IMM;
	OP_MVNS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_S_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	S_LSR_REG;
	OP_MVNS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_S_ASR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ASR_IMM;
	OP_MVNS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_S_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ASR_REG;
	OP_MVNS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_S_ROR_IMM(armcpu_t *cpu, uint32_t i)
{
	S_ROR_IMM;
	OP_MVNS(1,3);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_S_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	S_ROR_REG;
	OP_MVNS(2,4);
}

TEMPLATE static uint32_t FASTCALL OP_MVN_S_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	S_IMM_VALUE;
	OP_MVNS(1,3);
}

// -----------------------------------------------------------------------------
//   MUL / MULS / MLA / MLAS
// -----------------------------------------------------------------------------

#define MUL_Mxx_END(c) \
	v >>= 8; \
	if (!v || v == 0xFFFFFF) \
		return c + 1; \
	v >>= 8; \
	if (!v || v == 0xFFFF) \
		return c + 2; \
	v >>= 8; \
	if (!v || v == 0xFF) \
		return c + 3; \
	return c + 4;

TEMPLATE static uint32_t FASTCALL OP_MUL(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 8)];
	cpu->R[REG_POS(i, 16)] = cpu->R[REG_POS(i, 0)] * v;

	MUL_Mxx_END(1);
}

TEMPLATE static uint32_t FASTCALL OP_MLA(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 8)];
	cpu->R[REG_POS(i, 16)] = cpu->R[REG_POS(i, 0)] * v + cpu->R[REG_POS(i, 12)];

	MUL_Mxx_END(2);
}

TEMPLATE static uint32_t FASTCALL OP_MUL_S(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 8)];
	cpu->R[REG_POS(i, 16)] = cpu->R[REG_POS(i, 0)] * v;

	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 16)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 16)];

	MUL_Mxx_END(1);
}

TEMPLATE static uint32_t FASTCALL OP_MLA_S(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 8)];
	cpu->R[REG_POS(i, 16)] = cpu->R[REG_POS(i, 0)] * v + cpu->R[REG_POS(i, 12)];

	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 16)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 16)];

	MUL_Mxx_END(2);
}

// -----------------------------------------------------------------------------
//   UMULL / UMULLS / UMLAL / UMLALS
// -----------------------------------------------------------------------------

#define MUL_UMxxL_END(c) \
	v >>= 8; \
	if (!v) \
		return c + 1; \
	v >>= 8; \
	if (!v) \
		return c + 2; \
	v >>= 8; \
	if (!v) \
		return c + 3; \
	return c + 4;

TEMPLATE static uint32_t FASTCALL OP_UMULL(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 8)];
	uint64_t res = static_cast<uint64_t>(cpu->R[REG_POS(i, 0)]) * static_cast<uint64_t>(v);

	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(res);
	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(res >> 32);

	MUL_UMxxL_END(2);
}

TEMPLATE static uint32_t FASTCALL OP_UMLAL(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 8)];
	uint64_t res = static_cast<uint64_t>(cpu->R[REG_POS(i, 0)]) * static_cast<uint64_t>(v);

	// RdLo = (Rm * Rs)[31:0] + RdLo /* Unsigned multiplication */
	// RdHi = (Rm * Rs)[63:32] + RdHi + CarryFrom((Rm * Rs)[31:0] + RdLo)
	uint32_t tmp = static_cast<uint32_t>(res); // low
	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(res >> 32) + cpu->R[REG_POS(i, 16)] + CarryFrom(tmp, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 12)] += tmp;

	MUL_UMxxL_END(3);
}

TEMPLATE static uint32_t FASTCALL OP_UMULL_S(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 8)];
	uint64_t res = static_cast<uint64_t>(cpu->R[REG_POS(i, 0)]) * static_cast<uint64_t>(v);

	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(res);
	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(res >> 32);

	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 16)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 16)] && !cpu->R[REG_POS(i, 12)];

	MUL_UMxxL_END(2);
}

TEMPLATE static uint32_t FASTCALL OP_UMLAL_S(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_POS(i, 8)];
	uint64_t res = static_cast<uint64_t>(cpu->R[REG_POS(i, 0)]) * static_cast<uint64_t>(v);

	// RdLo = (Rm * Rs)[31:0] + RdLo /* Unsigned multiplication */
	// RdHi = (Rm * Rs)[63:32] + RdHi + CarryFrom((Rm * Rs)[31:0] + RdLo)
	uint32_t tmp = static_cast<uint32_t>(res); // low
	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(res >> 32) + cpu->R[REG_POS(i, 16)] + CarryFrom(tmp, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 12)] += tmp;

	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 16)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 16)] & !cpu->R[REG_POS(i, 12)];

	MUL_UMxxL_END(3);
}

// -----------------------------------------------------------------------------
//   SMULL / SMULLS / SMLAL / SMLALS
// -----------------------------------------------------------------------------

#define MUL_SMxxL_END(c) \
	v &= 0xFFFFFFFF; \
	v >>= 8; \
	if (!v || v == 0xFFFFFF) \
		return c + 1; \
	v >>= 8; \
	if (!v || v == 0xFFFF) \
		return c + 2; \
	v >>= 8; \
	if (!v || v == 0xFF) \
		return c + 3; \
	return c + 4;

TEMPLATE static uint32_t FASTCALL OP_SMULL(armcpu_t *cpu, uint32_t i)
{
	int64_t v = static_cast<int32_t>(cpu->R[REG_POS(i, 8)]);
	int64_t res = v * static_cast<int64_t>(static_cast<int32_t>(cpu->R[REG_POS(i, 0)]));

	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(res);
	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(res >> 32);

	MUL_SMxxL_END(2);
}

TEMPLATE static uint32_t FASTCALL OP_SMLAL(armcpu_t *cpu, uint32_t i)
{
	int64_t v = static_cast<int32_t>(cpu->R[REG_POS(i, 8)]);
	int64_t res = v * static_cast<int64_t>(static_cast<int32_t>(cpu->R[REG_POS(i, 0)]));

	uint32_t tmp = static_cast<uint32_t>(res);
	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(res >> 32) + cpu->R[REG_POS(i, 16)] + CarryFrom(tmp, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 12)] += tmp;

	MUL_SMxxL_END(3);
}

TEMPLATE static uint32_t FASTCALL OP_SMULL_S(armcpu_t *cpu, uint32_t i)
{
	int64_t v = static_cast<int32_t>(cpu->R[REG_POS(i, 8)]);
	int64_t res = v * static_cast<int64_t>(static_cast<int32_t>(cpu->R[REG_POS(i, 0)]));

	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(res);
	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(res >> 32);

	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 16)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 16)] & !cpu->R[REG_POS(i, 12)];

	MUL_SMxxL_END(2);
}

TEMPLATE static uint32_t FASTCALL OP_SMLAL_S(armcpu_t *cpu, uint32_t i)
{
	int64_t v = static_cast<int32_t>(cpu->R[REG_POS(i, 8)]);
	int64_t res = v * static_cast<int64_t>(static_cast<int32_t>(cpu->R[REG_POS(i, 0)]));

	uint32_t tmp = static_cast<uint32_t>(res);
	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(res >> 32) + cpu->R[REG_POS(i, 16)] + CarryFrom(tmp, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 12)] += tmp;

	cpu->CPSR.bits.N = BIT31(cpu->R[REG_POS(i, 16)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_POS(i, 16)] & !cpu->R[REG_POS(i, 12)];

	MUL_SMxxL_END(3);
}

// -----------------------------------------------------------------------------
//   SWP / SWPB
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_SWP(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	uint32_t tmp = ROR(READ32(cpu->mem_if->data, adr), (adr & 3) << 3);

	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 0)]);
	cpu->R[REG_POS(i, 12)] = tmp;

	uint32_t c = MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, adr);
	c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, adr);
	return MMU_aluMemCycles<PROCNUM>(4, c);
}

TEMPLATE static uint32_t FASTCALL OP_SWPB(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	uint8_t tmp = READ8(cpu->mem_if->data, adr);

	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 0)] & 0xFF));
	cpu->R[REG_POS(i, 12)] = tmp;

	uint32_t c = MMU_memAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, adr);
	c += MMU_memAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, adr);
	return MMU_aluMemCycles<PROCNUM>(4, c);
}

// -----------------------------------------------------------------------------
//   LDRH
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_LDRH_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRH_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRH_P_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRH_M_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRH_PRE_INDE_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRH_PRE_INDE_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRH_PRE_INDE_P_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRH_PRE_INDE_M_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRH_POS_INDE_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] += IMM_OFF;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRH_POS_INDE_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] -= IMM_OFF;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRH_POS_INDE_P_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] += cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRH_POS_INDE_M_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] -= cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

// -----------------------------------------------------------------------------
//   STRH
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_STRH_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF;
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRH_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF;
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRH_P_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + cpu->R[REG_POS(i, 0)];
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRH_M_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - cpu->R[REG_POS(i, 0)];
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRH_PRE_INDE_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRH_PRE_INDE_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRH_PRE_INDE_P_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRH_PRE_INDE_M_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRH_POS_INDE_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] += IMM_OFF;

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRH_POS_INDE_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] -= IMM_OFF;

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRH_POS_INDE_P_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] += cpu->R[REG_POS(i, 0)];

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRH_POS_INDE_M_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] -= cpu->R[REG_POS(i, 0)];

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(cpu->st, 2, adr);
}

// -----------------------------------------------------------------------------
//   LDRSH
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_LDRSH_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int16_t>(READ16(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSH_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int16_t>(READ16(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSH_P_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int16_t>(READ16(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSH_M_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int16_t>(READ16(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSH_PRE_INDE_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int16_t>(READ16(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSH_PRE_INDE_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int16_t>(READ16(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSH_PRE_INDE_P_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int16_t>(READ16(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSH_PRE_INDE_M_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int16_t>(READ16(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSH_POS_INDE_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] += IMM_OFF;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int16_t>(READ16(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSH_POS_INDE_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] -= IMM_OFF;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int16_t>(READ16(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSH_POS_INDE_P_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] += cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int16_t>(READ16(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSH_POS_INDE_M_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] -= cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int16_t>(READ16(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(cpu->st, 3, adr);
}

// -----------------------------------------------------------------------------
//   LDRSB
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_LDRSB_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int8_t>(READ8(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSB_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int8_t>(READ8(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSB_P_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int8_t>(READ8(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSB_M_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int8_t>(READ8(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSB_PRE_INDE_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int8_t>(READ8(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSB_PRE_INDE_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int8_t>(READ8(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSB_PRE_INDE_P_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int8_t>(READ8(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSB_PRE_INDE_M_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int8_t>(READ8(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSB_POS_INDE_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] += IMM_OFF;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int8_t>(READ8(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSB_POS_INDE_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] -= IMM_OFF;
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int8_t>(READ8(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSB_POS_INDE_P_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] += cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int8_t>(READ8(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRSB_POS_INDE_M_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] -= cpu->R[REG_POS(i, 0)];
	cpu->R[REG_POS(i, 12)] = static_cast<int32_t>(static_cast<int8_t>(READ8(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

// -----------------------------------------------------------------------------
//   MRS / MSR
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_MRS_CPSR(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_POS(i, 12)] = cpu->CPSR.val;

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_MRS_SPSR(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_POS(i, 12)] = cpu->SPSR.val;

	return 1;
}

#define OP_MSR_CPSR_(operand) \
	uint32_t byte_mask = cpu->CPSR.bits.mode == USR ? (BIT19(i) ? 0xFF000000 : 0x00000000) : \
		(BIT16(i) ? 0x000000FF : 0x00000000) | (BIT17(i) ? 0x0000FF00:0x00000000) | (BIT18(i) ? 0x00FF0000 : 0x00000000) | (BIT19(i) ? 0xFF000000 : 0x00000000); \
	if (cpu->CPSR.bits.mode != USR && BIT16(i)) \
		armcpu_switchMode(cpu, operand & 0x1F); \
	cpu->CPSR.val = (cpu->CPSR.val & ~byte_mask) | (operand & byte_mask); \
	cpu->changeCPSR();

#define OP_MSR_SPSR_(operand) \
	if (cpu->CPSR.bits.mode == USR || cpu->CPSR.bits.mode == SYS) \
		return 1; \
	uint32_t byte_mask = (BIT16(i) ? 0x000000FF : 0x00000000) | (BIT17(i) ? 0x0000FF00 : 0x00000000) | (BIT18(i) ? 0x00FF0000 : 0x00000000) | (BIT19(i) ? 0xFF000000 : 0x00000000); \
	cpu->SPSR.val = (cpu->SPSR.val & ~byte_mask) | (operand & byte_mask); \
	cpu->changeCPSR();

//#define __NEW_MSR
#ifdef __NEW_MSR
#define v4T_UNALLOC_MASK	0x0FFFFF00
#define v4T_USER_MASK		0xF0000000
#define v4T_PRIV_MASK		0x0000000F
#define v4T_STATE_MASK		0x00000020

#define v5TE_UNALLOC_MASK	0x07FFFF00
#define v5TE_USER_MASK		0xF8000000
#define v5TE_PRIV_MASK		0x0000000F
#define v5TE_STATE_MASK		0x00000020
#endif

TEMPLATE static uint32_t FASTCALL OP_MSR_CPSR(armcpu_t *cpu, uint32_t i)
{
	uint32_t operand = cpu->R[REG_POS(i, 0)];

#ifdef __NEW_MSR
	uint32_t mask = 0;
	uint32_t byte_mask = (BIT16(i) ? 0x000000FF : 0x00000000) | (BIT17(i) ? 0x0000FF00 : 0x00000000) | (BIT18(i) ? 0x00FF0000 : 0x00000000) | (BIT19(i) ? 0xFF000000 : 0x00000000);

	uint32_t unallocMask = PROCNUM?v4T_UNALLOC_MASK : v5TE_UNALLOC_MASK;
	uint32_t userMask = PROCNUM?v4T_USER_MASK : v5TE_USER_MASK;
	uint32_t privMask = PROCNUM?v4T_PRIV_MASK : v5TE_PRIV_MASK;
	uint32_t stateMask = PROCNUM?v4T_STATE_MASK : v5TE_STATE_MASK;

	if (operand & unallocMask)
		fprintf(stderr, "ARM%c: MSR_CPSR_REG UNPREDICTABLE UNALLOC (operand %08X)\n", PROCNUM ? '7' : '9', operand);
	if (cpu->CPSR.bits.mode != USR) // Privileged mode
	{
		if (BIT16(i))
			armcpu_switchMode(cpu, operand & 0x1F);
		if (operand & stateMask)
			fprintf(stderr, "ARM%c: MSR_CPSR_REG UNPREDICTABLE STATE (operand %08X)\n", PROCNUM ? '7' : '9', operand);
		else
			mask = byte_mask & (userMask | privMask);
	}
	else
		mask = byte_mask & userMask;

	u32 new_val = (cpu->CPSR.val & ~mask) | (operand & mask);
	cpu->CPSR.val = (cpu->CPSR.val & ~mask) | (operand & mask);
	cpu->changeCPSR();
#else
	OP_MSR_CPSR_(operand);
#endif
	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_MSR_SPSR(armcpu_t *cpu, uint32_t i)
{
	//fprintf(stderr, "OP_MSR_SPSR\n");
	uint32_t operand = cpu->R[REG_POS(i, 0)];
	OP_MSR_SPSR_(operand);
	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_MSR_CPSR_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	//fprintf(stderr, "OP_MSR_CPSR_IMM_VAL\n");
	IMM_VALUE;
	OP_MSR_CPSR_(shift_op);
	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_MSR_SPSR_IMM_VAL(armcpu_t *cpu, uint32_t i)
{
	//fprintf(stderr, "OP_MSR_SPSR_IMM_VAL\n");
	IMM_VALUE;
	OP_MSR_SPSR_(shift_op);
	return 1;
}

// -----------------------------------------------------------------------------
//   Branch
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_BX(armcpu_t *cpu, uint32_t i)
{
	uint32_t tmp = cpu->R[REG_POS(i, 0)];

	cpu->CPSR.bits.T = BIT0(tmp);
	cpu->R[15] = tmp & (0xFFFFFFFC | (cpu->CPSR.bits.T << 1));
	cpu->next_instruction = cpu->R[15];
	return 3;
}

TEMPLATE static uint32_t FASTCALL OP_BLX_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t tmp = cpu->R[REG_POS(i, 0)];

	cpu->R[14] = cpu->next_instruction;
	cpu->CPSR.bits.T = BIT0(tmp);
	cpu->R[15] = tmp & (0xFFFFFFFC | (cpu->CPSR.bits.T << 1));
	cpu->next_instruction = cpu->R[15];
	return 3;
}

static inline uint32_t SIGNEXTEND_24(uint32_t i) { return static_cast<uint32_t>((static_cast<int32_t>(i) << 8) >> 8); }

TEMPLATE static uint32_t FASTCALL OP_B(armcpu_t *cpu, uint32_t i)
{
	/*static const uint32_t mov_r12_r12 = 0xE1A0C00C;
	const uint32_t last = _MMU_read32<PROCNUM,MMU_AT_DEBUG>(cpu->st, cpu->instruct_adr-4);
	if(last == mov_r12_r12)
	{
		const uint32_t next = _MMU_read16<PROCNUM,MMU_AT_DEBUG>(cpu->st, cpu->instruct_adr+4);
		if(next == 0x6464)
			NocashMessage(cpu, 8);
	}*/

	uint32_t off = SIGNEXTEND_24(i);
	if (CONDITION(i) == 0xF)
	{
		cpu->R[14] = cpu->next_instruction;
		cpu->CPSR.bits.T = 1;
	}
	cpu->R[15] += off << 2;
	cpu->R[15] &= 0xFFFFFFFC | (cpu->CPSR.bits.T << 1);
	cpu->next_instruction = cpu->R[15];

	return 3;
}

TEMPLATE static uint32_t FASTCALL OP_BL(armcpu_t *cpu, uint32_t i)
{
	uint32_t off = SIGNEXTEND_24(i);
	if (CONDITION(i) == 0xF)
	{
		cpu->CPSR.bits.T = 1;
		cpu->R[15] += 2;
	}
	cpu->R[14] = cpu->next_instruction;
	cpu->R[15] += off << 2;
	cpu->R[15] &= 0xFFFFFFFC | (cpu->CPSR.bits.T << 1);
	cpu->next_instruction = cpu->R[15];

	return 3;
}

// -----------------------------------------------------------------------------
//   CLZ
// -----------------------------------------------------------------------------

const uint8_t CLZ_TAB[]=
{
	0,							// 0000
	1,							// 0001
	2, 2,						// 001X
	3, 3, 3, 3,					// 01XX
	4, 4, 4, 4, 4, 4, 4, 4		// 1XXX
};

TEMPLATE static uint32_t FASTCALL OP_CLZ(armcpu_t *cpu, uint32_t i)
{
	uint32_t Rm = cpu->R[REG_POS(i, 0)];

	if (!Rm)
	{
		cpu->R[REG_POS(i, 12)] = 32;
		return 2;
	}

	Rm |= Rm >> 1;
	Rm |= Rm >> 2;
	Rm |= Rm >> 4;
	Rm |= Rm >> 8;
	Rm |= Rm >> 16;

	uint32_t pos = CLZ_TAB[Rm & 0xF] + CLZ_TAB[(Rm >> 4) & 0xF] + CLZ_TAB[(Rm >> 8) & 0xF] + CLZ_TAB[(Rm >> 12) & 0xF] +
		CLZ_TAB[(Rm >> 16) & 0xF] + CLZ_TAB[(Rm >> 20) & 0xF] + CLZ_TAB[(Rm >> 24) & 0xF] + CLZ_TAB[(Rm >> 28) & 0xF];

	cpu->R[REG_POS(i, 12)] = 32 - pos;

	return 2;
}

// -----------------------------------------------------------------------------
//   QADD / QDADD / QSUB / QDSUB
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_QADD(armcpu_t *cpu, uint32_t i)
{
	uint32_t res = cpu->R[REG_POS(i, 16)] + cpu->R[REG_POS(i, 0)];

	if (SIGNED_OVERFLOW(cpu->R[REG_POS(i, 16)], cpu->R[REG_POS(i, 0)], res))
	{
		cpu->CPSR.bits.Q = 1;
		cpu->R[REG_POS(i, 12)] = 0x80000000 - BIT31(res);
		return 2;
	}
	cpu->R[REG_POS(i, 12)] = res;
	if (REG_POS(i, 12) == 15)
	{
		cpu->R[15] &= 0xFFFFFFFC;
		cpu->next_instruction = cpu->R[15];
		return 3;
	}
	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_QSUB(armcpu_t *cpu, uint32_t i)
{
	uint32_t res = cpu->R[REG_POS(i, 0)] - cpu->R[REG_POS(i, 16)];

	if (SIGNED_UNDERFLOW(cpu->R[REG_POS(i, 0)], cpu->R[REG_POS(i, 16)], res))
	{
		cpu->CPSR.bits.Q = 1;
		cpu->R[REG_POS(i, 12)] = 0x80000000 - BIT31(res);
		return 2;
	}
	cpu->R[REG_POS(i, 12)] = res;
	if (REG_POS(i, 12) == 15)
	{
		cpu->R[15] &= 0xFFFFFFFC;
		cpu->next_instruction = cpu->R[15];
		return 3;
	}
	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_QDADD(armcpu_t *cpu, uint32_t i)
{
	uint32_t mul = cpu->R[REG_POS(i, 16)] << 1;

	if (BIT31(cpu->R[REG_POS(i, 16)]) != BIT31(mul))
	{
		cpu->CPSR.bits.Q = 1;
		mul = 0x80000000 - BIT31(mul);
	}

	uint32_t res = mul + cpu->R[REG_POS(i, 0)];
	if (SIGNED_OVERFLOW(cpu->R[REG_POS(i, 0)], mul, res))
	{
		cpu->CPSR.bits.Q = 1;
		cpu->R[REG_POS(i, 12)] = 0x80000000 - BIT31(res);
		return 2;
	}
	cpu->R[REG_POS(i, 12)] = res;
	if (REG_POS(i, 12) == 15)
	{
		cpu->R[15] &= 0xFFFFFFFC;
		cpu->next_instruction = cpu->R[15];
		return 3;
	}
	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_QDSUB(armcpu_t *cpu, uint32_t i)
{
	uint32_t mul = cpu->R[REG_POS(i, 16)] << 1;

	if (BIT31(cpu->R[REG_POS(i, 16)]) != BIT31(mul))
	{
		cpu->CPSR.bits.Q = 1;
		mul = 0x80000000 - BIT31(mul);
	}

	uint32_t res = cpu->R[REG_POS(i, 0)] - mul;
	if (SIGNED_UNDERFLOW(cpu->R[REG_POS(i, 0)], mul, res))
	{
		cpu->CPSR.bits.Q = 1;
		cpu->R[REG_POS(i, 12)] = 0x80000000 - BIT31(res);
		return 2;
	}
	cpu->R[REG_POS(i, 12)] = res;
	if (REG_POS(i, 12) == 15)
	{
		cpu->R[15] &= 0xFFFFFFFC;
		cpu->next_instruction = cpu->R[15];
		return 3;
	}
	return 2;
}

// -----------------------------------------------------------------------------
//   SMUL
// -----------------------------------------------------------------------------

static inline int32_t HWORD(uint32_t i) { return static_cast<int32_t>(static_cast<int32_t>(i) >> 16); }
static inline int32_t LWORD(uint32_t i) { return static_cast<int32_t>(static_cast<int32_t>(i << 16) >> 16); }

TEMPLATE static uint32_t FASTCALL OP_SMUL_B_B(armcpu_t *cpu, uint32_t i)
{
	// checked
	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(LWORD(cpu->R[REG_POS(i, 0)]) * LWORD(cpu->R[REG_POS(i, 8)]));

	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_SMUL_B_T(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(LWORD(cpu->R[REG_POS(i, 0)]) * HWORD(cpu->R[REG_POS(i, 8)]));

	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_SMUL_T_B(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(HWORD(cpu->R[REG_POS(i, 0)]) * LWORD(cpu->R[REG_POS(i, 8)]));

	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_SMUL_T_T(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(HWORD(cpu->R[REG_POS(i, 0)]) * HWORD(cpu->R[REG_POS(i, 8)]));

	return 2;
}

// -----------------------------------------------------------------------------
//   SMLA
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_SMLA_B_B(armcpu_t *cpu, uint32_t i)
{
	uint32_t tmp = static_cast<uint32_t>(static_cast<int16_t>(cpu->R[REG_POS(i, 0)]) * static_cast<int16_t>(cpu->R[REG_POS(i, 8)]));

	cpu->R[REG_POS(i, 16)] = tmp + cpu->R[REG_POS(i, 12)];

	if (OverflowFromADD(cpu->R[REG_POS(i, 16)], tmp, cpu->R[REG_POS(i, 12)]))
		cpu->CPSR.bits.Q = 1;

	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_SMLA_B_T(armcpu_t *cpu, uint32_t i)
{
	uint32_t tmp = static_cast<uint32_t>(LWORD(cpu->R[REG_POS(i, 0)]) * HWORD(cpu->R[REG_POS(i, 8)]));
	uint32_t a = cpu->R[REG_POS(i, 12)];

	cpu->R[REG_POS(i, 16)] = tmp + a;

	if (SIGNED_OVERFLOW(tmp, a, cpu->R[REG_POS(i, 16)]))
		cpu->CPSR.bits.Q = 1;

	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_SMLA_T_B(armcpu_t *cpu, uint32_t i)
{
	uint32_t tmp = static_cast<uint32_t>(HWORD(cpu->R[REG_POS(i, 0)]) * LWORD(cpu->R[REG_POS(i, 8)]));
	uint32_t a = cpu->R[REG_POS(i, 12)];

	cpu->R[REG_POS(i, 16)] = tmp + a;

	if (SIGNED_OVERFLOW(tmp, a, cpu->R[REG_POS(i, 16)]))
		cpu->CPSR.bits.Q = 1;

	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_SMLA_T_T(armcpu_t *cpu, uint32_t i)
{
	uint32_t tmp = static_cast<uint32_t>(HWORD(cpu->R[REG_POS(i, 0)]) * HWORD(cpu->R[REG_POS(i, 8)]));
	uint32_t a = cpu->R[REG_POS(i, 12)];

	cpu->R[REG_POS(i, 16)] = tmp + a;

	if (SIGNED_OVERFLOW(tmp, a, cpu->R[REG_POS(i, 16)]))
		cpu->CPSR.bits.Q = 1;

	return 2;
}

// -----------------------------------------------------------------------------
//   SMLAL
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_SMLAL_B_B(armcpu_t *cpu, uint32_t i)
{
	int64_t tmp = static_cast<int64_t>(LWORD(cpu->R[REG_POS(i, 0)]) * LWORD(cpu->R[REG_POS(i, 8)]));
	uint64_t res = static_cast<uint64_t>(tmp) + cpu->R[REG_POS(i, 12)];

	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(res);
	cpu->R[REG_POS(i, 16)] += static_cast<uint32_t>(res + ((tmp < 0) * 0xFFFFFFFF));

	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_SMLAL_B_T(armcpu_t *cpu, uint32_t i)
{
	int64_t tmp = static_cast<int64_t>(LWORD(cpu->R[REG_POS(i, 0)]) * HWORD(cpu->R[REG_POS(i, 8)]));
	uint64_t res = static_cast<uint64_t>(tmp) + cpu->R[REG_POS(i, 12)];

	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(res);
	cpu->R[REG_POS(i, 16)] += static_cast<uint32_t>(res + ((tmp < 0) * 0xFFFFFFFF));

	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_SMLAL_T_B(armcpu_t *cpu, uint32_t i)
{
	int64_t tmp = static_cast<int64_t>(HWORD(cpu->R[REG_POS(i, 0)]) * static_cast<int64_t>(LWORD(cpu->R[REG_POS(i, 8)])));
	uint64_t res = static_cast<uint64_t>(tmp) + cpu->R[REG_POS(i, 12)];

	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(res);
	cpu->R[REG_POS(i, 16)] += static_cast<uint32_t>(res + ((tmp < 0) * 0xFFFFFFFF));

	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_SMLAL_T_T(armcpu_t *cpu, uint32_t i)
{
	int64_t tmp = static_cast<int64_t>(HWORD(cpu->R[REG_POS(i, 0)]) * HWORD(cpu->R[REG_POS(i, 8)]));
	uint64_t res = static_cast<uint64_t>(tmp) + cpu->R[REG_POS(i, 12)];

	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(res);
	cpu->R[REG_POS(i, 16)] += static_cast<uint32_t>(res + ((tmp < 0) * 0xFFFFFFFF));

	return 2;
}

// -----------------------------------------------------------------------------
//   SMULW
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_SMULW_B(armcpu_t *cpu, uint32_t i)
{
	int64_t tmp = static_cast<int64_t>(LWORD(cpu->R[REG_POS(i, 8)])) * static_cast<int64_t>(static_cast<int32_t>(cpu->R[REG_POS(i, 0)]));

	cpu->R[REG_POS(i, 16)] = (tmp >> 16) & 0xFFFFFFFF;

	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_SMULW_T(armcpu_t *cpu, uint32_t i)
{
	int64_t tmp = static_cast<int64_t>(HWORD(cpu->R[REG_POS(i, 8)])) * static_cast<int64_t>(static_cast<int32_t>(cpu->R[REG_POS(i, 0)]));

	cpu->R[REG_POS(i, 16)] = (tmp >> 16) & 0xFFFFFFFF;

	return 2;
}

// -----------------------------------------------------------------------------
//   SMLAW
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_SMLAW_B(armcpu_t *cpu, uint32_t i)
{
	int64_t tmp = static_cast<int64_t>(LWORD(cpu->R[REG_POS(i, 8)])) * static_cast<int64_t>(static_cast<int32_t>(cpu->R[REG_POS(i, 0)]));
	uint32_t a = cpu->R[REG_POS(i, 12)];

	tmp >>= 16;

	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(tmp + a);

	if (SIGNED_OVERFLOW(static_cast<uint32_t>(tmp), a, cpu->R[REG_POS(i, 16)]))
		cpu->CPSR.bits.Q = 1;

	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_SMLAW_T(armcpu_t *cpu, uint32_t i)
{
	int64_t tmp = static_cast<int64_t>(HWORD(cpu->R[REG_POS(i, 8)])) * static_cast<int64_t>(static_cast<int32_t>(cpu->R[REG_POS(i, 0)]));
	uint32_t a = cpu->R[REG_POS(i, 12)];

	tmp = (tmp >> 16) & 0xFFFFFFFF;
	cpu->R[REG_POS(i, 16)] = static_cast<uint32_t>(tmp + a);

	if (SIGNED_OVERFLOW(static_cast<uint32_t>(tmp), a, cpu->R[REG_POS(i, 16)]))
		cpu->CPSR.bits.Q = 1;

	return 2;
}

// -----------------------------------------------------------------------------
//   LDR
// -----------------------------------------------------------------------------
#define OP_LDR(a, b) \
	cpu->R[REG_POS(i, 12)] = ROR(READ32(cpu->mem_if->data, adr), 8 * (adr & 3)); \
\
	if (REG_POS(i, 12) == 15) \
	{ \
		if (!PROCNUM) \
		{ \
			cpu->CPSR.bits.T = BIT0(cpu->R[15]); \
			cpu->R[15] &= 0xFFFFFFFE; \
		} \
		else \
			cpu->R[15] &= 0xFFFFFFFC; \
		cpu->next_instruction = cpu->R[15]; \
		return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, b, adr); \
	} \
\
	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, a, adr);

// PRE
#define OP_LDR_W(a, b) \
	cpu->R[REG_POS(i, 16)] = adr;\
	cpu->R[REG_POS(i, 12)] = ROR(READ32(cpu->mem_if->data, adr), 8 * (adr & 3)); \
\
	if (REG_POS(i, 12) == 15) \
	{ \
		if (!PROCNUM) \
		{ \
			cpu->CPSR.bits.T = BIT0(cpu->R[15]); \
			cpu->R[15] &= 0xFFFFFFFE; \
		} \
		else \
			cpu->R[15] &= 0xFFFFFFFC; \
		cpu->next_instruction = cpu->R[15]; \
		return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, b, adr); \
	} \
\
	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, a, adr);

// POST
#define OP_LDR_W2(a, b, c) \
	uint32_t adr = cpu->R[REG_POS(i, 16)]; \
	cpu->R[REG_POS(i, 16)] = adr + c;\
	cpu->R[REG_POS(i, 12)] = ROR(READ32(cpu->mem_if->data, adr), 8 * (adr & 3)); \
\
	if (REG_POS(i, 12) == 15) \
	{ \
		if (!PROCNUM) \
		{ \
			cpu->CPSR.bits.T = BIT0(cpu->R[15]); \
			cpu->R[15] &= 0xFFFFFFFE; \
		} \
		else \
			cpu->R[15] &= 0xFFFFFFFC; \
		cpu->next_instruction = cpu->R[15]; \
		return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, b, adr); \
	} \
\
	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, a, adr);


TEMPLATE static uint32_t FASTCALL OP_LDR_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF_12;
	OP_LDR(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF_12;
	OP_LDR(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_LSL_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	OP_LDR(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_LSL_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	OP_LDR(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_LSR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	OP_LDR(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_LSR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	OP_LDR(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_ASR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	OP_LDR(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_ASR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	OP_LDR(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_ROR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	OP_LDR(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_ROR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	OP_LDR(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF_12;
	OP_LDR_W(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF_12;
	OP_LDR_W(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_LSL_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	OP_LDR_W(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_LSL_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	OP_LDR_W(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_LSR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	OP_LDR_W(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_LSR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	OP_LDR_W(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_ASR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	OP_LDR_W(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_ASR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	OP_LDR_W(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_ROR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	OP_LDR_W(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_ROR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	OP_LDR_W(3, 5);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	OP_LDR_W2(3, 5, IMM_OFF_12);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	OP_LDR_W2(3, 5, -IMM_OFF_12);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_LSL_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_LDR_W2(3, 5, shift_op);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_LSL_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	OP_LDR_W2(3, 5, -shift_op);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_LSR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_LDR_W2(3, 5, shift_op);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_LSR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	OP_LDR_W2(3, 5, -shift_op);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_ASR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_LDR_W2(3, 5, shift_op);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_ASR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	OP_LDR_W2(3, 5, -shift_op);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_P_ROR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_LDR_W2(3, 5, shift_op);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_M_ROR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	OP_LDR_W2(3, 5, -shift_op);
}

// -----------------------------------------------------------------------------
//   LDREX
// -----------------------------------------------------------------------------
TEMPLATE static uint32_t FASTCALL OP_LDREX(armcpu_t *cpu, uint32_t i)
{
	fprintf(stderr, "LDREX\n");
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 12)] = ROR(READ32(cpu->mem_if->data, adr), 8 * (adr & 3));
	return MMU_aluMemAccessCycles<PROCNUM,32, MMU_AD_READ>(cpu->st, 3, adr);
}

// -----------------------------------------------------------------------------
//   LDRB
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF_12;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF_12;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_LSL_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_LSL_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_LSR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 12)] = READ8(cpu->mem_if->data, adr);

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_LSR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_ASR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_ASR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_ROR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_ROR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF_12;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF_12;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_LSL_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_LSL_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_LSR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_LSR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_ASR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_ASR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_ROR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_ROR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] = adr + IMM_OFF_12;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] = adr - IMM_OFF_12;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_LSL_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] = adr + shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_LSL_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] = adr - shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_LSR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] = adr + shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_LSR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] = adr - shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_ASR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] = adr + shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_ASR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] = adr - shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_P_ROR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] = adr + shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_M_ROR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	cpu->R[REG_POS(i, 16)] = adr - shift_op;
	cpu->R[REG_POS(i, 12)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(cpu->st, 3, adr);
}

// -----------------------------------------------------------------------------
//   STR
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_STR_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF_12;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF_12;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_LSL_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_LSL_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_LSR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_LSR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_ASR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_ASR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_ROR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_ROR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF_12;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF_12;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_LSL_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_LSL_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_LSR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_LSR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_ASR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_ASR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_ROR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_ROR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 16)] = adr + IMM_OFF_12;

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 16)] = adr - IMM_OFF_12;

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_LSL_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 16)] = adr + shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_LSL_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 16)] = adr - shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_LSR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 16)] = adr + shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_LSR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 16)] = adr - shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_ASR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 16)] = adr + shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_ASR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 16)] = adr - shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_P_ROR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 16)] = adr + shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_M_ROR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 12)]);
	cpu->R[REG_POS(i, 16)] = adr - shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

// -----------------------------------------------------------------------------
//   STREX
// -----------------------------------------------------------------------------
TEMPLATE static uint32_t FASTCALL OP_STREX(armcpu_t *cpu, uint32_t i)
{
	fprintf(stderr, "STREX\n");
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_POS(i, 0)]);
	cpu->R[REG_POS(i, 12)] = 0;

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, 2, adr);
}

// -----------------------------------------------------------------------------
//   STRB
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_STRB_P_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF_12;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF_12;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_LSL_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_LSL_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_LSR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_LSR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_ASR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_ASR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_ROR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_ROR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] + IMM_OFF_12;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)] - IMM_OFF_12;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_LSL_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_LSL_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_LSR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_LSR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_ASR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_ASR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_ROR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] + shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_ROR_IMM_OFF_PREIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)] - shift_op;
	cpu->R[REG_POS(i, 16)] = adr;
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] = adr + IMM_OFF_12;

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] = adr - IMM_OFF_12;

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_LSL_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] = adr + shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_LSL_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSL_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] = adr - shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_LSR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] = adr + shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_LSR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	LSR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] = adr - shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_ASR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] = adr + shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_ASR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ASR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] = adr - shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_P_ROR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] = adr + shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_M_ROR_IMM_OFF_POSTIND(armcpu_t *cpu, uint32_t i)
{
	ROR_IMM;
	uint32_t adr = cpu->R[REG_POS(i, 16)];
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_POS(i, 12)]));
	cpu->R[REG_POS(i, 16)] = adr - shift_op;

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(cpu->st, 2, adr);
}

// -----------------------------------------------------------------------------
//   LDMIA / LDMIB / LDMDA / LDMDB
// -----------------------------------------------------------------------------

#define OP_L_IA(reg, adr) \
	if (BIT##reg(i)) \
	{ \
		registres[reg] = READ32(cpu->mem_if->data, start); \
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start); \
		adr += 4; \
	}

#define OP_L_IB(reg, adr) \
	if (BIT##reg(i)) \
	{ \
		adr += 4; \
		registres[reg] = READ32(cpu->mem_if->data, start); \
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start); \
	}

#define OP_L_DA(reg, adr) \
	if (BIT##reg(i)) \
	{ \
		registres[reg] = READ32(cpu->mem_if->data, start); \
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start); \
		adr -= 4; \
	}

#define OP_L_DB(reg, adr) \
	if (BIT##reg(i)) \
	{ \
		adr -= 4; \
		registres[reg] = READ32(cpu->mem_if->data, start); \
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start); \
	}

TEMPLATE static uint32_t FASTCALL OP_LDMIA(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	uint32_t *registres = cpu->R;

	OP_L_IA(0, start);
	OP_L_IA(1, start);
	OP_L_IA(2, start);
	OP_L_IA(3, start);
	OP_L_IA(4, start);
	OP_L_IA(5, start);
	OP_L_IA(6, start);
	OP_L_IA(7, start);
	OP_L_IA(8, start);
	OP_L_IA(9, start);
	OP_L_IA(10, start);
	OP_L_IA(11, start);
	OP_L_IA(12, start);
	OP_L_IA(13, start);
	OP_L_IA(14, start);

	if (BIT15(i))
	{
		uint32_t tmp = READ32(cpu->mem_if->data, start);
		// TODO
		// The general-purpose registers loaded can include the PC. If they do, the word loaded for the PC is treated
		// as an address and a branch occurs to that address. In ARMv5 and above, bit[0] of the loaded value
		// determines whether execution continues after this branch in ARM state or in Thumb state, as though a BX
		// (loaded_value) instruction had been executed (but see also The T and J bits on page A2-15 for operation on
		// non-T variants of ARMv5). In earlier versions of the architecture, bits[1:0] of the loaded value are ignored
		// and execution continues in ARM state, as though the instruction MOV PC,(loaded_value) had been executed.
		//
		//value = Memory[address,4]
		//if (architecture version 5 or above) then
		//	pc = value AND 0xFFFFFFFE
		//	T Bit = value[0]
		//else
		//	pc = value AND 0xFFFFFFFC
		if (!PROCNUM)
		{
			cpu->CPSR.bits.T = BIT0(tmp);
			registres[15] = tmp & 0xFFFFFFFE;
		}
		else
			registres[15] = tmp & 0xFFFFFFFC;

		//start += 4;
		cpu->next_instruction = registres[15];
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);
	}

	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMIB(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	uint32_t *registres = cpu->R;

	OP_L_IB(0, start);
	OP_L_IB(1, start);
	OP_L_IB(2, start);
	OP_L_IB(3, start);
	OP_L_IB(4, start);
	OP_L_IB(5, start);
	OP_L_IB(6, start);
	OP_L_IB(7, start);
	OP_L_IB(8, start);
	OP_L_IB(9, start);
	OP_L_IB(10, start);
	OP_L_IB(11, start);
	OP_L_IB(12, start);
	OP_L_IB(13, start);
	OP_L_IB(14, start);

	if (BIT15(i))
	{
		start += 4;
		c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_READ>(cpu->st, start);
		uint32_t tmp = READ32(cpu->mem_if->data, start);
		if (!PROCNUM)
		{
			cpu->CPSR.bits.T = BIT0(tmp);
			registres[15] = tmp & 0xFFFFFFFE;
		}
		else
			registres[15] = tmp & 0xFFFFFFFC;
		cpu->next_instruction = registres[15];
		return MMU_aluMemCycles<PROCNUM>(4, c);
	}

	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMDA(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	uint32_t *registres = cpu->R;

	if (BIT15(i))
	{
		uint32_t tmp = READ32(cpu->mem_if->data, start);
		if (!PROCNUM)
		{
			cpu->CPSR.bits.T = BIT0(tmp);
			registres[15] = tmp & 0xFFFFFFFE;
		}
		else
			registres[15] = tmp & 0xFFFFFFFC;
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);
		start -= 4;
		cpu->next_instruction = registres[15];
	}

	OP_L_DA(14, start);
	OP_L_DA(13, start);
	OP_L_DA(12, start);
	OP_L_DA(11, start);
	OP_L_DA(10, start);
	OP_L_DA(9, start);
	OP_L_DA(8, start);
	OP_L_DA(7, start);
	OP_L_DA(6, start);
	OP_L_DA(5, start);
	OP_L_DA(4, start);
	OP_L_DA(3, start);
	OP_L_DA(2, start);
	OP_L_DA(1, start);
	OP_L_DA(0, start);

	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMDB(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	uint32_t *registres = cpu->R;

	if (BIT15(i))
	{
		start -= 4;
		uint32_t tmp = READ32(cpu->mem_if->data, start);
		if (!PROCNUM)
		{
			cpu->CPSR.bits.T = BIT0(tmp);
			registres[15] = tmp & 0xFFFFFFFE;
		}
		else
			registres[15] = tmp & 0xFFFFFFFC;
		cpu->next_instruction = registres[15];
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);
	}

	OP_L_DB(14, start);
	OP_L_DB(13, start);
	OP_L_DB(12, start);
	OP_L_DB(11, start);
	OP_L_DB(10, start);
	OP_L_DB(9, start);
	OP_L_DB(8, start);
	OP_L_DB(7, start);
	OP_L_DB(6, start);
	OP_L_DB(5, start);
	OP_L_DB(4, start);
	OP_L_DB(3, start);
	OP_L_DB(2, start);
	OP_L_DB(1, start);
	OP_L_DB(0, start);

	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMIA_W(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];
	uint32_t bitList = (~((2 << REG_POS(i, 16)) - 1)) & 0xFFFF;

	uint32_t *registres = cpu->R;

	OP_L_IA(0, start);
	OP_L_IA(1, start);
	OP_L_IA(2, start);
	OP_L_IA(3, start);
	OP_L_IA(4, start);
	OP_L_IA(5, start);
	OP_L_IA(6, start);
	OP_L_IA(7, start);
	OP_L_IA(8, start);
	OP_L_IA(9, start);
	OP_L_IA(10, start);
	OP_L_IA(11, start);
	OP_L_IA(12, start);
	OP_L_IA(13, start);
	OP_L_IA(14, start);

	if (BIT15(i))
	{
		uint32_t tmp = READ32(cpu->mem_if->data, start);
		if (!PROCNUM)
		{
			cpu->CPSR.bits.T = BIT0(tmp);
			registres[15] = tmp & 0xFFFFFFFE;
		}
		else
			registres[15] = tmp & 0xFFFFFFFC;
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);
		start += 4;
		cpu->next_instruction = registres[15];
	}

	if (i & (1 << REG_POS(i, 16)))
	{
		if (i & bitList)
			cpu->R[REG_POS(i, 16)] = start;
	}
	else
		cpu->R[REG_POS(i, 16)] = start;

	return MMU_aluMemCycles<PROCNUM>(BIT15(i) ? 4 : 2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMIB_W(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];
	uint32_t bitList = (~((2 << REG_POS(i, 16)) - 1)) & 0xFFFF;

	uint32_t *registres = cpu->R;

	OP_L_IB(0, start);
	OP_L_IB(1, start);
	OP_L_IB(2, start);
	OP_L_IB(3, start);
	OP_L_IB(4, start);
	OP_L_IB(5, start);
	OP_L_IB(6, start);
	OP_L_IB(7, start);
	OP_L_IB(8, start);
	OP_L_IB(9, start);
	OP_L_IB(10, start);
	OP_L_IB(11, start);
	OP_L_IB(12, start);
	OP_L_IB(13, start);
	OP_L_IB(14, start);

	if (BIT15(i))
	{
		uint32_t tmp;
		start += 4;
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);
		tmp = READ32(cpu->mem_if->data, start);
		if (!PROCNUM)
		{
			cpu->CPSR.bits.T = BIT0(tmp);
			registres[15] = tmp & 0xFFFFFFFE;
		}
		else
			registres[15] = tmp & 0xFFFFFFFC;
		cpu->next_instruction = registres[15];
	}

	if (i & (1 << REG_POS(i, 16)))
	{
		if (i & bitList)
			cpu->R[REG_POS(i, 16)] = start;
	}
	else
		cpu->R[REG_POS(i, 16)] = start;

	return MMU_aluMemCycles<PROCNUM>(BIT15(i) ? 4 : 2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMDA_W(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];
	uint32_t bitList = (~((2 << REG_POS(i, 16)) - 1)) & 0xFFFF;

	uint32_t *registres = cpu->R;

	if (BIT15(i))
	{
		uint32_t tmp = READ32(cpu->mem_if->data, start);
		if (!PROCNUM)
		{
			cpu->CPSR.bits.T = BIT0(tmp);
			registres[15] = tmp & 0xFFFFFFFE;
		}
		else
			registres[15] = tmp & 0xFFFFFFFC;
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);
		start -= 4;
		cpu->next_instruction = registres[15];
	}

	OP_L_DA(14, start);
	OP_L_DA(13, start);
	OP_L_DA(12, start);
	OP_L_DA(11, start);
	OP_L_DA(10, start);
	OP_L_DA(9, start);
	OP_L_DA(8, start);
	OP_L_DA(7, start);
	OP_L_DA(6, start);
	OP_L_DA(5, start);
	OP_L_DA(4, start);
	OP_L_DA(3, start);
	OP_L_DA(2, start);
	OP_L_DA(1, start);
	OP_L_DA(0, start);

	if (i & (1 << REG_POS(i, 16)))
	{
		if (i & bitList)
			cpu->R[REG_POS(i, 16)] = start;
	}
	else
		cpu->R[REG_POS(i, 16)] = start;

	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMDB_W(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];
	uint32_t bitList = (~((2 << REG_POS(i, 16)) - 1)) & 0xFFFF;

	uint32_t *registres = cpu->R;

	if (BIT15(i))
	{
		uint32_t tmp;
		start -= 4;
		tmp = READ32(cpu->mem_if->data, start);
		if (!PROCNUM)
		{
			cpu->CPSR.bits.T = BIT0(tmp);
			registres[15] = tmp & 0xFFFFFFFE;
		}
		else
			registres[15] = tmp & 0xFFFFFFFC;
		cpu->next_instruction = registres[15];
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);
	}

	OP_L_DB(14, start);
	OP_L_DB(13, start);
	OP_L_DB(12, start);
	OP_L_DB(11, start);
	OP_L_DB(10, start);
	OP_L_DB(9, start);
	OP_L_DB(8, start);
	OP_L_DB(7, start);
	OP_L_DB(6, start);
	OP_L_DB(5, start);
	OP_L_DB(4, start);
	OP_L_DB(3, start);
	OP_L_DB(2, start);
	OP_L_DB(1, start);
	OP_L_DB(0, start);

	if (i & (1 << REG_POS(i, 16)))
	{
		if (i & bitList)
			cpu->R[REG_POS(i, 16)] = start;
	}
	else
		cpu->R[REG_POS(i, 16)] = start;

	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMIA2(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	uint32_t oldmode = 0;
	if (!BIT15(i))
	{
		if (cpu->CPSR.bits.mode == USR || cpu->CPSR.bits.mode == SYS)
		{
			fprintf(stderr, "ERROR1\n");
			return 1;
		}
		oldmode = armcpu_switchMode(cpu, SYS);
	}

	uint32_t *registres = cpu->R;

	OP_L_IA(0, start);
	OP_L_IA(1, start);
	OP_L_IA(2, start);
	OP_L_IA(3, start);
	OP_L_IA(4, start);
	OP_L_IA(5, start);
	OP_L_IA(6, start);
	OP_L_IA(7, start);
	OP_L_IA(8, start);
	OP_L_IA(9, start);
	OP_L_IA(10, start);
	OP_L_IA(11, start);
	OP_L_IA(12, start);
	OP_L_IA(13, start);
	OP_L_IA(14, start);

	if (!BIT15(i))
		armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
	else
	{
		uint32_t tmp = READ32(cpu->mem_if->data, start);
		Status_Reg SPSR;
		cpu->R[15] = tmp & (0XFFFFFFFC | (BIT0(tmp) << 1));
		SPSR = cpu->SPSR;
		armcpu_switchMode(cpu, SPSR.bits.mode);
		cpu->CPSR = SPSR;
		cpu->changeCPSR();
		//start += 4;
		cpu->next_instruction = cpu->R[15];
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);
	}
	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMIB2(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	uint32_t oldmode = 0;
	if (!BIT15(i))
	{
		if (cpu->CPSR.bits.mode == USR || cpu->CPSR.bits.mode == SYS)
		{
			fprintf(stderr, "ERROR1\n");
			return 1;
		}
		oldmode = armcpu_switchMode(cpu, SYS);
	}

	uint32_t *registres = cpu->R;

	OP_L_IB(0, start);
	OP_L_IB(1, start);
	OP_L_IB(2, start);
	OP_L_IB(3, start);
	OP_L_IB(4, start);
	OP_L_IB(5, start);
	OP_L_IB(6, start);
	OP_L_IB(7, start);
	OP_L_IB(8, start);
	OP_L_IB(9, start);
	OP_L_IB(10, start);
	OP_L_IB(11, start);
	OP_L_IB(12, start);
	OP_L_IB(13, start);
	OP_L_IB(14, start);

	if (!BIT15(i))
		armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
	else
	{
		Status_Reg SPSR;
		start += 4;
		uint32_t tmp = READ32(cpu->mem_if->data, start);
		registres[15] = tmp & (0XFFFFFFFC | (BIT0(tmp) << 1));
		SPSR = cpu->SPSR;
		armcpu_switchMode(cpu, SPSR.bits.mode);
		cpu->CPSR = SPSR;
		cpu->changeCPSR();
		cpu->next_instruction = registres[15];
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);
	}
	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMDA2(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i,16)];

	uint32_t oldmode = 0;
	if (!BIT15(i))
	{
		if (cpu->CPSR.bits.mode == USR || cpu->CPSR.bits.mode == SYS)
		{
			fprintf(stderr, "ERROR1\n");
			return 1;
		}
		oldmode = armcpu_switchMode(cpu, SYS);
	}

	uint32_t *registres = cpu->R;

	if (BIT15(i))
	{
		uint32_t tmp = READ32(cpu->mem_if->data, start);
		registres[15] = tmp & (0XFFFFFFFC | (BIT0(tmp) << 1));
		cpu->CPSR = cpu->SPSR;
		cpu->changeCPSR();
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);
		start -= 4;
		cpu->next_instruction = registres[15];
	}

	OP_L_DA(14, start);
	OP_L_DA(13, start);
	OP_L_DA(12, start);
	OP_L_DA(11, start);
	OP_L_DA(10, start);
	OP_L_DA(9, start);
	OP_L_DA(8, start);
	OP_L_DA(7, start);
	OP_L_DA(6, start);
	OP_L_DA(5, start);
	OP_L_DA(4, start);
	OP_L_DA(3, start);
	OP_L_DA(2, start);
	OP_L_DA(1, start);
	OP_L_DA(0, start);

	if (!BIT15(i))
		armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
	else
	{
		Status_Reg SPSR = cpu->SPSR;
		armcpu_switchMode(cpu, SPSR.bits.mode);
		cpu->CPSR = SPSR;
		cpu->changeCPSR();
	}

	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMDB2(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i,16)];

	uint32_t oldmode = 0;
	if (!BIT15(i))
	{
		if (cpu->CPSR.bits.mode == USR || cpu->CPSR.bits.mode == SYS)
		{
			fprintf(stderr, "ERROR1\n");
			return 1;
		}
		oldmode = armcpu_switchMode(cpu, SYS);
	}

	uint32_t *registres = cpu->R;

	if (BIT15(i))
	{
		uint32_t tmp;
		start -= 4;
		tmp = READ32(cpu->mem_if->data, start);
		registres[15] = tmp & (0XFFFFFFFC | (BIT0(tmp) << 1));
		cpu->CPSR = cpu->SPSR;
		cpu->changeCPSR();
		cpu->next_instruction = registres[15];
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);
	}

	OP_L_DB(14, start);
	OP_L_DB(13, start);
	OP_L_DB(12, start);
	OP_L_DB(11, start);
	OP_L_DB(10, start);
	OP_L_DB(9, start);
	OP_L_DB(8, start);
	OP_L_DB(7, start);
	OP_L_DB(6, start);
	OP_L_DB(5, start);
	OP_L_DB(4, start);
	OP_L_DB(3, start);
	OP_L_DB(2, start);
	OP_L_DB(1, start);
	OP_L_DB(0, start);

	if (!BIT15(i))
		armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
	else
	{
		Status_Reg SPSR = cpu->SPSR;
		armcpu_switchMode(cpu, SPSR.bits.mode);
		cpu->CPSR = SPSR;
		cpu->changeCPSR();
	}

	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMIA2_W(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i,16)];

	uint32_t oldmode = 0;
	if (!BIT15(i))
	{
		if (cpu->CPSR.bits.mode == USR || cpu->CPSR.bits.mode == SYS)
		{
			fprintf(stderr, "ERROR1\n");
			return 1;
		}
		oldmode = armcpu_switchMode(cpu, SYS);
	}

	uint32_t *registres = cpu->R;

	OP_L_IA(0, start);
	OP_L_IA(1, start);
	OP_L_IA(2, start);
	OP_L_IA(3, start);
	OP_L_IA(4, start);
	OP_L_IA(5, start);
	OP_L_IA(6, start);
	OP_L_IA(7, start);
	OP_L_IA(8, start);
	OP_L_IA(9, start);
	OP_L_IA(10, start);
	OP_L_IA(11, start);
	OP_L_IA(12, start);
	OP_L_IA(13, start);
	OP_L_IA(14, start);

	if (!BIT15(i))
	{
		if (!BIT_N(i, REG_POS(i, 16)))
			registres[REG_POS(i, 16)] = start;
		armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
		return MMU_aluMemCycles<PROCNUM>(2, c);
	}

	if (!BIT_N(i, REG_POS(i, 16)))
		registres[REG_POS(i, 16)] = start + 4;
	uint32_t tmp = READ32(cpu->mem_if->data, start);
	registres[15] = tmp & (0XFFFFFFFC | (BIT0(tmp) << 1));
	Status_Reg SPSR = cpu->SPSR;
	armcpu_switchMode(cpu, SPSR.bits.mode);
	cpu->CPSR = SPSR;
	cpu->changeCPSR();
	cpu->next_instruction = registres[15];
	c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);

	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMIB2_W(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	uint32_t oldmode = 0;
	if (!BIT15(i))
	{
		if (cpu->CPSR.bits.mode == USR || cpu->CPSR.bits.mode == SYS)
		{
			fprintf(stderr, "ERROR1\n");
			return 1;
		}
		oldmode = armcpu_switchMode(cpu, SYS);
	}

	uint32_t *registres = cpu->R;

	OP_L_IB(0, start);
	OP_L_IB(1, start);
	OP_L_IB(2, start);
	OP_L_IB(3, start);
	OP_L_IB(4, start);
	OP_L_IB(5, start);
	OP_L_IB(6, start);
	OP_L_IB(7, start);
	OP_L_IB(8, start);
	OP_L_IB(9, start);
	OP_L_IB(10, start);
	OP_L_IB(11, start);
	OP_L_IB(12, start);
	OP_L_IB(13, start);
	OP_L_IB(14, start);

	if (!BIT15(i))
	{
		if (!BIT_N(i, REG_POS(i, 16)))
			registres[REG_POS(i, 16)] = start;
		armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));

		return MMU_aluMemCycles<PROCNUM>(2, c);
	}

	if (!BIT_N(i, REG_POS(i,16)))
		registres[REG_POS(i,16)] = start + 4;
	uint32_t tmp = READ32(cpu->mem_if->data, start + 4);
	registres[15] = tmp & (0XFFFFFFFC | (BIT0(tmp) << 1));
	cpu->CPSR = cpu->SPSR;
	cpu->changeCPSR();
	cpu->next_instruction = registres[15];
	Status_Reg SPSR = cpu->SPSR;
	armcpu_switchMode(cpu, SPSR.bits.mode);
	cpu->CPSR = SPSR;
	cpu->changeCPSR();
	c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);

	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMDA2_W(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i,16)];

	uint32_t oldmode = 0;
	if (!BIT15(i))
	{
		if (cpu->CPSR.bits.mode == USR || cpu->CPSR.bits.mode == SYS)
		{
			fprintf(stderr, "ERROR1\n");
			return 1;
		}
		oldmode = armcpu_switchMode(cpu, SYS);
	}

	uint32_t *registres = cpu->R;

	if (BIT15(i))
	{
		if (BIT_N(i, REG_POS(i, 16)))
			fprintf(stderr, "error1_1\n");
		uint32_t tmp = READ32(cpu->mem_if->data, start);
		registres[15] = tmp & (0XFFFFFFFC | (BIT0(tmp) << 1));
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);
		start -= 4;
		cpu->next_instruction = registres[15];
	}

	OP_L_DA(14, start);
	OP_L_DA(13, start);
	OP_L_DA(12, start);
	OP_L_DA(11, start);
	OP_L_DA(10, start);
	OP_L_DA(9, start);
	OP_L_DA(8, start);
	OP_L_DA(7, start);
	OP_L_DA(6, start);
	OP_L_DA(5, start);
	OP_L_DA(4, start);
	OP_L_DA(3, start);
	OP_L_DA(2, start);
	OP_L_DA(1, start);
	OP_L_DA(0, start);

	if (!BIT_N(i, REG_POS(i, 16)))
		registres[REG_POS(i, 16)] = start;

	if (!BIT15(i))
	{
		armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
		return MMU_aluMemCycles<PROCNUM>(2, c);
	}

	Status_Reg SPSR = cpu->SPSR;
	armcpu_switchMode(cpu, SPSR.bits.mode);
	cpu->CPSR = SPSR;
	cpu->changeCPSR();
	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMDB2_W(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	uint32_t oldmode = 0;
	if (!BIT15(i))
	{
		if (cpu->CPSR.bits.mode == USR || cpu->CPSR.bits.mode == SYS)
		{
			fprintf(stderr, "ERROR1\n");
			return 1;
		}
		oldmode = armcpu_switchMode(cpu, SYS);
	}

	uint32_t *registres = cpu->R;

	if (BIT15(i))
	{
		if (BIT_N(i, REG_POS(i, 16)))
			fprintf(stderr, "error1_2\n");
		start -= 4;
		uint32_t tmp = READ32(cpu->mem_if->data, start);
		c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, start);
		registres[15] = tmp & (0XFFFFFFFC | (BIT0(tmp) << 1));
		cpu->CPSR = cpu->SPSR;
		cpu->changeCPSR();
		cpu->next_instruction = registres[15];
	}

	OP_L_DB(14, start);
	OP_L_DB(13, start);
	OP_L_DB(12, start);
	OP_L_DB(11, start);
	OP_L_DB(10, start);
	OP_L_DB(9, start);
	OP_L_DB(8, start);
	OP_L_DB(7, start);
	OP_L_DB(6, start);
	OP_L_DB(5, start);
	OP_L_DB(4, start);
	OP_L_DB(3, start);
	OP_L_DB(2, start);
	OP_L_DB(1, start);
	OP_L_DB(0, start);

	if (!BIT_N(i, REG_POS(i, 16)))
		registres[REG_POS(i, 16)] = start;

	if (!BIT15(i))
	{
		armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
		return MMU_aluMemCycles<PROCNUM>(2, c);
	}

	Status_Reg SPSR = cpu->SPSR;
	armcpu_switchMode(cpu, SPSR.bits.mode);
	cpu->CPSR = SPSR;
	cpu->changeCPSR();
	return MMU_aluMemCycles<PROCNUM>(2, c);
}

// -----------------------------------------------------------------------------
//   STMIA / STMIB / STMDA / STMDB
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_STMIA(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, b))
		{
			WRITE32(cpu->mem_if->data, start, cpu->R[b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
			start += 4;
		}
	}
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMIB(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, b))
		{
			start += 4;
			WRITE32(cpu->mem_if->data, start, cpu->R[b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
		}
	}
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMDA(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, 15 - b))
		{
			WRITE32(cpu->mem_if->data, start, cpu->R[15 - b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
			start -= 4;
		}
	}
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMDB(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, 15 - b))
		{
			start -= 4;
			WRITE32(cpu->mem_if->data, start, cpu->R[15 - b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
		}
	}
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMIA_W(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, b))
		{
			WRITE32(cpu->mem_if->data, start, cpu->R[b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
			start += 4;
		}
	}

	cpu->R[REG_POS(i, 16)] = start;
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMIB_W(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, b))
		{
			start += 4;
			WRITE32(cpu->mem_if->data, start, cpu->R[b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
		}
	}
	cpu->R[REG_POS(i, 16)] = start;
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMDA_W(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, 15 - b))
		{
			WRITE32(cpu->mem_if->data, start, cpu->R[15 - b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
			start -= 4;
		}
	}

	cpu->R[REG_POS(i, 16)] = start;
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMDB_W(armcpu_t *cpu, uint32_t i)
{
	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, 15 - b))
		{
			start -= 4;
			WRITE32(cpu->mem_if->data, start, cpu->R[15 - b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
		}
	}

	cpu->R[REG_POS(i, 16)] = start;
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMIA2(armcpu_t *cpu, uint32_t i)
{
	if (cpu->CPSR.bits.mode == USR)
		return 2;

	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];
	uint32_t oldmode = armcpu_switchMode(cpu, SYS);

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, b))
		{
			WRITE32(cpu->mem_if->data, start, cpu->R[b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
			start += 4;
		}
	}

	armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMIB2(armcpu_t *cpu, uint32_t i)
{
	if (cpu->CPSR.bits.mode == USR)
		return 2;

	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];
	uint32_t oldmode = armcpu_switchMode(cpu, SYS);

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, b))
		{
			start += 4;
			WRITE32(cpu->mem_if->data, start, cpu->R[b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
		}
	}

	armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMDA2(armcpu_t *cpu, uint32_t i)
{
	if (cpu->CPSR.bits.mode == USR)
		return 2;

	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];
	uint32_t oldmode = armcpu_switchMode(cpu, SYS);

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, 15 - b))
		{
			WRITE32(cpu->mem_if->data, start, cpu->R[15 - b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
			start -= 4;
		}
	}

	armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMDB2(armcpu_t *cpu, uint32_t i)
{
	if (cpu->CPSR.bits.mode == USR)
		return 2;

	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];
	uint32_t oldmode = armcpu_switchMode(cpu, SYS);

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, 15 - b))
		{
			start -= 4;
			WRITE32(cpu->mem_if->data, start, cpu->R[15 - b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
		}
	}

	armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMIA2_W(armcpu_t *cpu, uint32_t i)
{
	if (cpu->CPSR.bits.mode == USR)
		return 2;

	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];
	uint32_t oldmode = armcpu_switchMode(cpu, SYS);

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, b))
		{
			WRITE32(cpu->mem_if->data, start, cpu->R[b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
			start += 4;
		}
	}

	cpu->R[REG_POS(i, 16)] = start;

	armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMIB2_W(armcpu_t *cpu, uint32_t i)
{
	if (cpu->CPSR.bits.mode == USR)
		return 2;

	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];
	uint32_t oldmode = armcpu_switchMode(cpu, SYS);

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, b))
		{
			start += 4;
			WRITE32(cpu->mem_if->data, start, cpu->R[b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
		}
	}

	armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
	cpu->R[REG_POS(i, 16)] = start;

	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMDA2_W(armcpu_t *cpu, uint32_t i)
{
	if (cpu->CPSR.bits.mode == USR)
		return 2;

	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];
	uint32_t oldmode = armcpu_switchMode(cpu, SYS);

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, 15 - b))
		{
			WRITE32(cpu->mem_if->data, start, cpu->R[15 - b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
			start -= 4;
		}
	}

	cpu->R[REG_POS(i, 16)] = start;

	armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

TEMPLATE static uint32_t FASTCALL OP_STMDB2_W(armcpu_t *cpu, uint32_t i)
{
	if (cpu->CPSR.bits.mode == USR)
		return 2;

	uint32_t c = 0;
	uint32_t start = cpu->R[REG_POS(i, 16)];
	uint32_t oldmode = armcpu_switchMode(cpu, SYS);

	for (uint32_t b = 0; b < 16; ++b)
	{
		if (BIT_N(i, 15 - b))
		{
			start -= 4;
			WRITE32(cpu->mem_if->data, start, cpu->R[15 - b]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, start);
		}
	}

	cpu->R[REG_POS(i, 16)] = start;

	armcpu_switchMode(cpu, static_cast<uint8_t>(oldmode));
	return MMU_aluMemCycles<PROCNUM>(1, c);
}

// -----------------------------------------------------------------------------
//   LDRD / STRD
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_LDRD_STRD_POST_INDEX(armcpu_t *cpu, uint32_t i)
{
	uint32_t Rd_num = REG_POS(i, 12);
	uint32_t addr = cpu->R[REG_POS(i, 16)];
	uint32_t index;

	//fprintf(stderr, "%s POST\n", BIT5(i)?"STRD":"LDRD");
	/* I bit - immediate or register */
	if (BIT22(i))
		index = IMM_OFF;
	else
		index = cpu->R[REG_POS(i, 0)];

	// U bit - add or subtract
	if (BIT23(i))
		cpu->R[REG_POS(i, 16)] += index;
	else
		cpu->R[REG_POS(i, 16)] -= index;

	uint32_t c = 0;
	if (!(Rd_num & 0x1))
	{
		// Store/Load
		if (BIT5(i))
		{
			WRITE32(cpu->mem_if->data, addr, cpu->R[Rd_num]);
			WRITE32(cpu->mem_if->data, addr + 4, cpu->R[Rd_num + 1]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, addr);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, addr + 4);
		}
		else
		{
			cpu->R[Rd_num] = READ32(cpu->mem_if->data, addr);
			cpu->R[Rd_num + 1] = READ32(cpu->mem_if->data, addr + 4);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, addr);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, addr + 4);
		}
	}

	return MMU_aluMemCycles<PROCNUM>(3, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDRD_STRD_OFFSET_PRE_INDEX(armcpu_t *cpu, uint32_t i)
{
	uint32_t Rd_num = REG_POS(i, 12);
	uint32_t addr = cpu->R[REG_POS(i, 16)];
	uint32_t index;

	//fprintf(stderr, "%s PRE\n", BIT5(i)?"STRD":"LDRD");
	// I bit - immediate or register
	if (BIT22(i))
		index = IMM_OFF;
	else
		index = cpu->R[REG_POS(i, 0)];

	// U bit - add or subtract
	if (BIT23(i))
		addr += index;
	else
		addr -= index;

	uint32_t c = 0;
	if (!(Rd_num & 0x1))
	{
		// Store/Load
		if (BIT5(i))
		{
			WRITE32(cpu->mem_if->data, addr, cpu->R[Rd_num]);
			WRITE32(cpu->mem_if->data, addr + 4, cpu->R[Rd_num + 1]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, addr);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(cpu->st, addr + 4);
			// W bit - writeback
			if (BIT21(i))
				cpu->R[REG_POS(i, 16)] = addr;
		}
		else
		{
			// W bit - writeback
			if (BIT21(i))
				cpu->R[REG_POS(i, 16)] = addr;
			cpu->R[Rd_num] = READ32(cpu->mem_if->data, addr);
			cpu->R[Rd_num + 1] = READ32(cpu->mem_if->data, addr + 4);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, addr);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(cpu->st, addr + 4);
		}
	}

	return MMU_aluMemCycles<PROCNUM>(3, c);
}

// -----------------------------------------------------------------------------
//   STC
//   the NDS has no coproc that responses to a STC, no feedback is given to the arm
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_STC_P_IMM_OFF(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_STC_P_IMM_OFF\n");
	return TRAPUNDEF(cpu);
}

TEMPLATE static uint32_t FASTCALL OP_STC_M_IMM_OFF(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_STC_M_IMM_OFF\n");
	return TRAPUNDEF(cpu);
}

TEMPLATE static uint32_t FASTCALL OP_STC_P_PREIND(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_STC_P_PREIND\n");
	return TRAPUNDEF(cpu);
}

TEMPLATE static uint32_t FASTCALL OP_STC_M_PREIND(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_STC_M_PREIND\n");
	return TRAPUNDEF(cpu);
}

TEMPLATE static uint32_t FASTCALL OP_STC_P_POSTIND(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_STC_P_POSTIND: cp_num %i\n", (i>>8)&0x0F);
	return TRAPUNDEF(cpu);
}

TEMPLATE static uint32_t FASTCALL OP_STC_M_POSTIND(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_STC_M_POSTIND\n");
	return TRAPUNDEF(cpu);
}

TEMPLATE static uint32_t FASTCALL OP_STC_OPTION(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_STC_OPTION\n");
	return TRAPUNDEF(cpu);
}

// -----------------------------------------------------------------------------
//   LDC
//   the NDS has no coproc that responses to a LDC, no feedback is given to the arm
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_LDC_P_IMM_OFF(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_LDC_P_IMM_OFF\n");
	return TRAPUNDEF(cpu);
}

TEMPLATE static uint32_t FASTCALL OP_LDC_M_IMM_OFF(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_LDC_M_IMM_OFF\n");
	return TRAPUNDEF(cpu);
}

TEMPLATE static uint32_t FASTCALL OP_LDC_P_PREIND(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_LDC_P_PREIND\n");
	return TRAPUNDEF(cpu);
}

TEMPLATE static uint32_t FASTCALL OP_LDC_M_PREIND(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_LDC_M_PREIND\n");
	return TRAPUNDEF(cpu);
}

TEMPLATE static uint32_t FASTCALL OP_LDC_P_POSTIND(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_LDC_P_POSTIND\n");
	return TRAPUNDEF(cpu);
}

TEMPLATE static uint32_t FASTCALL OP_LDC_M_POSTIND(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_LDC_M_POSTIND\n");
	return TRAPUNDEF(cpu);
}

TEMPLATE static uint32_t FASTCALL OP_LDC_OPTION(armcpu_t *cpu, uint32_t)
{
	//INFO("OP_LDC_OPTION\n");
	return TRAPUNDEF(cpu);
}

// -----------------------------------------------------------------------------
//   MCR / MRC
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_MCR(armcpu_t *cpu, uint32_t i)
{
	uint32_t cpnum = REG_POS(i, 8);

	if (cpnum != 15)
		return 2;

	cpu->st->cp15.moveARM2CP(cpu->R[REG_POS(i, 12)], REG_POS(i, 16), REG_POS(i, 0), (i >> 21) & 0x7, (i >> 5) & 0x7);

	return 2;
}

TEMPLATE static uint32_t FASTCALL OP_MRC(armcpu_t *cpu, uint32_t i)
{
	//if (PROCNUM != 0) return 1;

	uint32_t cpnum = REG_POS(i, 8);

	if (cpnum != 15)
		return 2;

	// ARM REF:
	//data = value from Coprocessor[cp_num]
	//if Rd is R15 then
	//	N flag = data[31]
	//	Z flag = data[30]
	//	C flag = data[29]
	//	V flag = data[28]
	//else /* Rd is not R15 */
	//	Rd = data

	uint32_t data = 0;
	cpu->st->cp15.moveCP2ARM(&data, REG_POS(i, 16), REG_POS(i, 0), (i >> 21) & 0x7, (i >> 5) & 0x7);
	if (REG_POS(i, 12) == 15)
	{
		cpu->CPSR.bits.N = BIT31(data);
		cpu->CPSR.bits.Z = BIT30(data);
		cpu->CPSR.bits.C = BIT29(data);
		cpu->CPSR.bits.V = BIT28(data);
	}
	else
		cpu->R[REG_POS(i, 12)] = data;
	//cpu->coproc[cpnum]->moveCP2ARM(&cpu->R[REG_POS(i, 12)], REG_POS(i, 16), REG_POS(i, 0), (i>>21)&7, (i>>5)&7);
	return 4;
}

// -----------------------------------------------------------------------------
//   SWI
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_SWI(armcpu_t *cpu, uint32_t i)
{
	uint32_t swinum = (i >> 16) & 0xFF;

	// ideas-style debug prints (execute this SWI with the null terminated string address in R0)
	if (swinum == 0xFC)
		return 0;

	// if the user has changed the intVector to point away from the nds bioses,
	// then it doesn't really make any sense to use the builtin SWI's since
	// the bios ones aren't getting called anyway
	bool bypassBuiltinSWI = (cpu->intVector == 0x00000000 && !PROCNUM) || (cpu->intVector == 0xFFFF0000 && PROCNUM == 1);

	if (cpu->swi_tab && !bypassBuiltinSWI)
	{
		swinum &= 0x1F;
		//fprintf(stderr, "%d ARM SWI %d \n",PROCNUM,swinum);
		return cpu->swi_tab[swinum](cpu) + 3;
	}
	else
	{
		/* TODO (#1#): translocated SWI vectors */
		/* we use an irq thats not in the irq tab, as
		 it was replaced duie to a changed intVector */
		Status_Reg tmp = cpu->CPSR;
		armcpu_switchMode(cpu, SVC); /* enter svc mode */
		cpu->R[14] = cpu->next_instruction;
		cpu->SPSR = tmp; /* save old CPSR as new SPSR */
		cpu->CPSR.bits.T = 0; /* handle as ARM32 code */
		cpu->CPSR.bits.I = 1;
		cpu->changeCPSR();
		cpu->R[15] = cpu->intVector + 0x08;
		cpu->next_instruction = cpu->R[15];
		return 3;
	}
}

// -----------------------------------------------------------------------------
//   BKPT
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_BKPT(armcpu_t *cpu, uint32_t /*i*/)
{
	/* ARM-ref
	if (not overridden by debug hardware)
		R14_abt = address of BKPT instruction + 4
		SPSR_abt = CPSR
		CPSR[4:0] = 0b10111 // Enter Abort mode
		CPSR[5] = 0 // Execute in ARM state
		// CPSR[6] is unchanged
		CPSR[7] = 1 // Disable normal interrupts
		CPSR[8] = 1 // Disable imprecise aborts - v6 only
		CPSR[9] = CP15_reg1_EEbit
		if high vectors configured then
			PC = 0xFFFF000C
		else
			PC = 0x0000000C
	*/

	/*
	static uint32_t last_bkpt = 0xFFFFFFFF;
	if(i != last_bkpt)
		fprintf(stderr, "ARM OP_BKPT triggered\n");
	last_bkpt = i;

	//this is not 100% correctly emulated, but it does the job
	cpu->next_instruction = cpu->instruct_adr;
	return 4;
	*/

	fprintf(stderr, "ARM OP_BKPT triggered\n");
	Status_Reg tmp = cpu->CPSR;
	armcpu_switchMode(cpu, ABT); // enter abt mode
	cpu->R[14] = cpu->instruct_adr + 4;
	cpu->SPSR = tmp; // save old CPSR as new SPSR
	cpu->CPSR.bits.T = 0; // handle as ARM32 code
	cpu->CPSR.bits.I = 1;
	cpu->changeCPSR();
	cpu->R[15] = cpu->intVector + 0x0C;
	cpu->next_instruction = cpu->R[15];
	return 4;
}

// -----------------------------------------------------------------------------
//   CDP
// -----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_CDP(armcpu_t *cpu, uint32_t)
{
	//INFO("Stopped (OP_CDP) \n");
	return TRAPUNDEF(cpu);
}

//-----------------------------------------------------------------------------
//   The End
//-----------------------------------------------------------------------------

const OpFunc arm_instructions_set[2][4096] =
{
	{
#define TABDECL(x) x<0>
#include "instruction_tabdef.inc"
#undef TABDECL
	},
	{
#define TABDECL(x) x<1>
#include "instruction_tabdef.inc"
#undef TABDECL
	}
};
