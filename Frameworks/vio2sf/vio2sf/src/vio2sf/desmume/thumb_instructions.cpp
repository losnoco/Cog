/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2008 shash
	Copyright (C) 2008-2013 DeSmuME team

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

#include <cassert>

#define ST (cpu->st)

#include "vio2sf.h"
#include "bios.h"

#define TEMPLATE template<int PROCNUM>

static inline uint32_t REG_NUM(uint32_t i, uint32_t n) { return (i >> n) & 0x7; }

//-----------------------------------------------------------------------------
//   Undefined instruction
//-----------------------------------------------------------------------------
TEMPLATE static uint32_t FASTCALL OP_UND_THUMB(armcpu_t *cpu, uint32_t)
{
	//INFO("THUMB%c: Undefined instruction: 0x%08X (%s) PC=0x%08X\n", cpu->proc_ID?'7':'9', cpu->instruction, decodeIntruction(true, cpu->instruction), cpu->instruct_adr);
	TRAPUNDEF(cpu);
	return 1;
}

//-----------------------------------------------------------------------------
//   LSL
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_LSL_0(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_NUM(i, 0)] = cpu->R[REG_NUM(i, 3)];
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_LSL(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = (i >> 6) & 0x1F;
	cpu->CPSR.bits.C = BIT_N(cpu->R[REG_NUM(i, 3)], 32 - v);
	cpu->R[REG_NUM(i, 0)] = cpu->R[REG_NUM(i, 3)] << v;
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_LSL_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_NUM(i, 3)] & 0xFF;

	if (!v)
	{
		cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
		cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
		return 2;
	}
	if (v < 32)
	{
		cpu->CPSR.bits.C = BIT_N(cpu->R[REG_NUM(i, 0)], 32 - v);
		cpu->R[REG_NUM(i, 0)] <<= v;
		cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
		cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
		return 2;
	}
	if (v == 32)
		cpu->CPSR.bits.C = BIT0(cpu->R[REG_NUM(i, 0)]);
	else
		cpu->CPSR.bits.C = 0;

	cpu->R[REG_NUM(i, 0)] = 0;
	cpu->CPSR.bits.N = 0;
	cpu->CPSR.bits.Z = 1;

	return 2;
}

//-----------------------------------------------------------------------------
//   LSR
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_LSR_0(armcpu_t *cpu, uint32_t i)
{
	cpu->CPSR.bits.C = BIT31(cpu->R[REG_NUM(i, 3)]);
	cpu->R[REG_NUM(i, 0)] = 0;
	cpu->CPSR.bits.N = 0;
	cpu->CPSR.bits.Z = 1;

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_LSR(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = (i >> 6) & 0x1F;
	cpu->CPSR.bits.C = BIT_N(cpu->R[REG_NUM(i, 3)], v - 1);
	cpu->R[REG_NUM(i, 0)] = cpu->R[REG_NUM(i, 3)] >> v;
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_LSR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_NUM(i, 3)] & 0xFF;

	if (!v)
	{
		cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
		cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
		return 2;
	}
	if (v < 32)
	{
		cpu->CPSR.bits.C = BIT_N(cpu->R[REG_NUM(i, 0)], v - 1);
		cpu->R[REG_NUM(i, 0)] >>= v;
		cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
		cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
		return 2;
	}
	if (v == 32)
		cpu->CPSR.bits.C = BIT31(cpu->R[REG_NUM(i, 0)]);
	else
		cpu->CPSR.bits.C = 0;

	cpu->R[REG_NUM(i, 0)] = 0;
	cpu->CPSR.bits.N = 0;
	cpu->CPSR.bits.Z = 1;

	return 2;
}

//-----------------------------------------------------------------------------
//   ASR
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_ASR_0(armcpu_t *cpu, uint32_t i)
{
	cpu->CPSR.bits.C = BIT31(cpu->R[REG_NUM(i, 3)]);
	cpu->R[REG_NUM(i, 0)] = BIT31(cpu->R[REG_NUM(i, 3)]) * 0xFFFFFFFF;
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_ASR(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = (i >> 6) & 0x1F;
	cpu->CPSR.bits.C = BIT_N(cpu->R[REG_NUM(i, 3)], v-1);
	cpu->R[REG_NUM(i, 0)] = static_cast<uint32_t>(static_cast<int32_t>(cpu->R[REG_NUM(i, 3)]) >> v);
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_ASR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_NUM(i, 3)] & 0xFF;

	if (!v)
	{
		cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
		cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
		return 2;
	}
	if (v < 32)
	{
		cpu->CPSR.bits.C = BIT_N(cpu->R[REG_NUM(i, 0)], v - 1);
		cpu->R[REG_NUM(i, 0)] = static_cast<uint32_t>(static_cast<int32_t>(cpu->R[REG_NUM(i, 0)]) >> v);
		cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
		cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
		return 2;
	}

	cpu->CPSR.bits.C = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->R[REG_NUM(i, 0)] = BIT31(cpu->R[REG_NUM(i, 0)]) * 0xFFFFFFFF;
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];

	return 2;
}

//-----------------------------------------------------------------------------
//   ADD
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_ADD_IMM3(armcpu_t *cpu, uint32_t i)
{
	uint32_t imm3 = (i >> 6) & 0x07;
	uint32_t Rn = cpu->R[REG_NUM(i, 3)];

	if (!imm3) // mov 2
	{
		cpu->R[REG_NUM(i, 0)] = Rn;

		cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
		cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
		cpu->CPSR.bits.C = cpu->CPSR.bits.V = 0;
		return 1;
	}

	cpu->R[REG_NUM(i, 0)] = Rn + imm3;
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
	cpu->CPSR.bits.C = CarryFrom(Rn, imm3);
	cpu->CPSR.bits.V = OverflowFromADD(cpu->R[REG_NUM(i, 0)], Rn, imm3);

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_ADD_IMM8(armcpu_t *cpu, uint32_t i)
{
	uint32_t imm8 = i & 0xFF;
	uint32_t Rd = cpu->R[REG_NUM(i, 8)];

	cpu->R[REG_NUM(i, 8)] = Rd + imm8;
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 8)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 8)];
	cpu->CPSR.bits.C = CarryFrom(Rd, imm8);
	cpu->CPSR.bits.V = OverflowFromADD(cpu->R[REG_NUM(i, 8)], Rd, imm8);

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_ADD_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t Rn = cpu->R[REG_NUM(i, 3)];
	uint32_t Rm = cpu->R[REG_NUM(i, 6)];

	cpu->R[REG_NUM(i, 0)] = Rn + Rm;
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
	cpu->CPSR.bits.C = CarryFrom(Rn, Rm);
	cpu->CPSR.bits.V = OverflowFromADD(cpu->R[REG_NUM(i, 0)], Rn, Rm);

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_ADD_SPE(armcpu_t *cpu, uint32_t i)
{
	uint32_t Rd = REG_NUM(i, 0) | ((i >> 4) & 8);

	cpu->R[Rd] += cpu->R[REG_POS(i, 3)];

	if (Rd == 15)
	{
		cpu->next_instruction = cpu->R[15];
		return 3;
	}

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_ADD_2PC(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_NUM(i, 8)] = (cpu->R[15] & 0xFFFFFFFC) + ((i & 0xFF) << 2);

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_ADD_2SP(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_NUM(i, 8)] = cpu->R[13] + ((i & 0xFF) << 2);

	return 1;
}

//-----------------------------------------------------------------------------
//   SUB
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_SUB_IMM3(armcpu_t *cpu, uint32_t i)
{
	uint32_t imm3 = (i >> 6) & 0x07;
	uint32_t Rn = cpu->R[REG_NUM(i, 3)];
	uint32_t tmp = Rn - imm3;

	cpu->R[REG_NUM(i, 0)] = tmp;
	cpu->CPSR.bits.N = BIT31(tmp);
	cpu->CPSR.bits.Z = !tmp;
	cpu->CPSR.bits.C = !BorrowFrom(Rn, imm3);
	cpu->CPSR.bits.V = OverflowFromSUB(tmp, Rn, imm3);

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_SUB_IMM8(armcpu_t *cpu, uint32_t i)
{
	uint32_t imm8 = i & 0xFF;
	uint32_t Rd = cpu->R[REG_NUM(i, 8)];
	uint32_t tmp = Rd - imm8;

	cpu->R[REG_NUM(i, 8)] = tmp;
	cpu->CPSR.bits.N = BIT31(tmp);
	cpu->CPSR.bits.Z = !tmp;
	cpu->CPSR.bits.C = !BorrowFrom(Rd, imm8);
	cpu->CPSR.bits.V = OverflowFromSUB(tmp, Rd, imm8);

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_SUB_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t Rn = cpu->R[REG_NUM(i, 3)];
	uint32_t Rm = cpu->R[REG_NUM(i, 6)];
	uint32_t tmp = Rn - Rm;

	cpu->R[REG_NUM(i, 0)] = tmp;
	cpu->CPSR.bits.N = BIT31(tmp);
	cpu->CPSR.bits.Z = !tmp;
	cpu->CPSR.bits.C = !BorrowFrom(Rn, Rm);
	cpu->CPSR.bits.V = OverflowFromSUB(tmp, Rn, Rm);

	return 1;
}

//-----------------------------------------------------------------------------
//   MOV
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_MOV_IMM8(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_NUM(i, 8)] = i & 0xFF;
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 8)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 8)];

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_MOV_SPE(armcpu_t *cpu, uint32_t i)
{
	uint32_t Rd = REG_NUM(i, 0) | ((i >> 4) & 8);

	cpu->R[Rd] = cpu->R[REG_POS(i, 3)];

	if (Rd == 15)
	{
		cpu->next_instruction = cpu->R[15];
		return 3;
	}

	return 1;
}

//-----------------------------------------------------------------------------
//   CMP
//-----------------------------------------------------------------------------
TEMPLATE static uint32_t FASTCALL OP_CMP_IMM8(armcpu_t *cpu, uint32_t i)
{
	uint32_t tmp = cpu->R[REG_NUM(i, 8)] - (i & 0xFF);

	cpu->CPSR.bits.N = BIT31(tmp);
	cpu->CPSR.bits.Z = !tmp;
	cpu->CPSR.bits.C = !BorrowFrom(cpu->R[REG_NUM(i, 8)], i & 0xFF);
	cpu->CPSR.bits.V = OverflowFromSUB(tmp, cpu->R[REG_NUM(i, 8)], i & 0xFF);

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_CMP(armcpu_t *cpu, uint32_t i)
{
	uint32_t tmp = cpu->R[REG_NUM(i, 0)] - cpu->R[REG_NUM(i, 3)];

	cpu->CPSR.bits.N = BIT31(tmp);
	cpu->CPSR.bits.Z = !tmp;
	cpu->CPSR.bits.C = !BorrowFrom(cpu->R[REG_NUM(i, 0)], cpu->R[REG_NUM(i, 3)]);
	cpu->CPSR.bits.V = OverflowFromSUB(tmp, cpu->R[REG_NUM(i, 0)], cpu->R[REG_NUM(i, 3)]);

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_CMP_SPE(armcpu_t *cpu, uint32_t i)
{
	uint32_t Rn = (i & 7) | ((i >> 4) & 8);

	uint32_t tmp = cpu->R[Rn] - cpu->R[REG_POS(i, 3)];

	cpu->CPSR.bits.N = BIT31(tmp);
	cpu->CPSR.bits.Z = !tmp;
	cpu->CPSR.bits.C = !BorrowFrom(cpu->R[Rn], cpu->R[REG_POS(i, 3)]);
	cpu->CPSR.bits.V = OverflowFromSUB(tmp, cpu->R[Rn], cpu->R[REG_POS(i, 3)]);

	return 1;
}

//-----------------------------------------------------------------------------
//   AND
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_AND(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_NUM(i, 0)] &= cpu->R[REG_NUM(i, 3)];
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
	return 1;
}

//-----------------------------------------------------------------------------
//   EOR
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_EOR(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_NUM(i, 0)] ^= cpu->R[REG_NUM(i, 3)];
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];

	return 1;
}

//-----------------------------------------------------------------------------
//   ADC
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_ADC_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t Rd = cpu->R[REG_NUM(i, 0)];
	uint32_t Rm = cpu->R[REG_NUM(i, 3)];

	if (!cpu->CPSR.bits.C)
	{
		cpu->R[REG_NUM(i, 0)] = Rd + Rm;
		cpu->CPSR.bits.C = cpu->R[REG_NUM(i, 0)] < Rm;
	}
	else
	{
		cpu->R[REG_NUM(i, 0)] = Rd + Rm + 1;
		cpu->CPSR.bits.C =  cpu->R[REG_NUM(i, 0)] <= Rm;
	}
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
	cpu->CPSR.bits.V = BIT31((Rd ^ Rm ^ -1) & (Rd ^ cpu->R[REG_NUM(i, 0)]));

	return 1;
}

//-----------------------------------------------------------------------------
//   SBC
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_SBC_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t Rd = cpu->R[REG_NUM(i, 0)];
	uint32_t Rm = cpu->R[REG_NUM(i, 3)];

	if (!cpu->CPSR.bits.C)
	{
		cpu->R[REG_NUM(i, 0)] = Rd - Rm - 1;
		cpu->CPSR.bits.C = Rd > Rm;
	}
	else
	{
		cpu->R[REG_NUM(i, 0)] = Rd - Rm;
		cpu->CPSR.bits.C = Rd >= Rm;
	}

	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
	cpu->CPSR.bits.V = BIT31((Rd ^ Rm) & (Rd ^ cpu->R[REG_NUM(i, 0)]));

	return 1;
}

//-----------------------------------------------------------------------------
//   ROR
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_ROR_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_NUM(i, 3)] & 0xFF;

	if (!v)
	{
		cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
		cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
		return 2;
	}

	v &= 0x1F;
	if (!v)
	{
		cpu->CPSR.bits.C = BIT31(cpu->R[REG_NUM(i, 0)]);
		cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
		cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
		return 2;
	}

	cpu->CPSR.bits.C = BIT_N(cpu->R[REG_NUM(i, 0)], v - 1);
	cpu->R[REG_NUM(i, 0)] = ROR(cpu->R[REG_NUM(i, 0)], v);
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];

	return 2;
}

//-----------------------------------------------------------------------------
//   TST
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_TST(armcpu_t *cpu, uint32_t i)
{
	uint32_t tmp = cpu->R[REG_NUM(i, 0)] & cpu->R[REG_NUM(i, 3)];
	cpu->CPSR.bits.N = BIT31(tmp);
	cpu->CPSR.bits.Z = !tmp;

	return 1;
}

//-----------------------------------------------------------------------------
//   NEG
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_NEG(armcpu_t *cpu, uint32_t i)
{
	uint32_t Rm = cpu->R[REG_NUM(i, 3)];

	cpu->R[REG_NUM(i, 0)] = static_cast<uint32_t>(-static_cast<int32_t>(Rm));

	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
	cpu->CPSR.bits.C = !BorrowFrom(0, Rm);
	cpu->CPSR.bits.V = OverflowFromSUB(cpu->R[REG_NUM(i, 0)], 0, Rm);

	return 1;
}

//-----------------------------------------------------------------------------
//   CMN
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_CMN(armcpu_t *cpu, uint32_t i)
{
	uint32_t tmp = cpu->R[REG_NUM(i, 0)] + cpu->R[REG_NUM(i, 3)];

	cpu->CPSR.bits.N = BIT31(tmp);
	cpu->CPSR.bits.Z = !tmp;
	cpu->CPSR.bits.C = CarryFrom(cpu->R[REG_NUM(i, 0)], cpu->R[REG_NUM(i, 3)]);
	cpu->CPSR.bits.V = OverflowFromADD(tmp, cpu->R[REG_NUM(i, 0)], cpu->R[REG_NUM(i, 3)]);

	return 1;
}

//-----------------------------------------------------------------------------
//   ORR
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_ORR(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_NUM(i, 0)] |= cpu->R[REG_NUM(i, 3)];

	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];

	return 1;
}

//-----------------------------------------------------------------------------
//   BIC
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_BIC(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_NUM(i, 0)] &= ~cpu->R[REG_NUM(i, 3)];

	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];

	return 1;
}

//-----------------------------------------------------------------------------
//   MVN
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_MVN(armcpu_t *cpu, uint32_t i)
{
	cpu->R[REG_NUM(i, 0)] = ~cpu->R[REG_NUM(i, 3)];

	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];

	return 1;
}

//-----------------------------------------------------------------------------
//   MUL
//-----------------------------------------------------------------------------

#define MUL_Mxx_END_THUMB(c) \
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

TEMPLATE static uint32_t FASTCALL OP_MUL_REG(armcpu_t *cpu, uint32_t i)
{
	uint32_t v = cpu->R[REG_NUM(i, 3)];

	// FIXME:
	//------ Rd = (Rm * Rd)[31:0]
	//------ u64 res = ((u64)cpu->R[REG_NUM(i, 0)] * (u64)v));
	//------ cpu->R[REG_NUM(i, 0)] = (uint32_t)(res  & 0xFFFFFFFF);
	//------

	cpu->R[REG_NUM(i, 0)] *= v;
	cpu->CPSR.bits.N = BIT31(cpu->R[REG_NUM(i, 0)]);
	cpu->CPSR.bits.Z = !cpu->R[REG_NUM(i, 0)];
	//The MUL instruction is defined to leave the C flag unchanged in ARMv5 and above.
	//In earlier versions of the architecture, the value of the C flag was UNPREDICTABLE
	//after a MUL instruction.

	if (PROCNUM == 1) // ARM4T 1S + mI, m = 3
		return 4;

	MUL_Mxx_END_THUMB(1);
}

//-----------------------------------------------------------------------------
//   STRB / LDRB
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_STRB_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 3)] + ((i >> 6) & 0x1F);
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_NUM(i, 0)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(ST, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRB_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 3)] + ((i>>6)&0x1F);
	cpu->R[REG_NUM(i, 0)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(ST, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRB_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 3)] + cpu->R[REG_NUM(i, 6)];
	WRITE8(cpu->mem_if->data, adr, static_cast<uint8_t>(cpu->R[REG_NUM(i, 0)]));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_WRITE>(ST, 2, adr);
}

TEMPLATE static  uint32_t FASTCALL OP_LDRB_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 3)] + cpu->R[REG_NUM(i, 6)];
	cpu->R[REG_NUM(i, 0)] = static_cast<uint32_t>(READ8(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(ST, 3, adr);
}

//-----------------------------------------------------------------------------
//   LDRSB
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_LDRSB_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 3)] + cpu->R[REG_NUM(i, 6)];
	cpu->R[REG_NUM(i, 0)] = static_cast<uint32_t>(static_cast<int8_t>(READ8(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 8, MMU_AD_READ>(ST, 3, adr);
}

//-----------------------------------------------------------------------------
//   STRH / LDRH
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_STRH_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 3)] + ((i >> 5) & 0x3E);
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_NUM(i, 0)]));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(ST, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRH_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 3)] + ((i >> 5) & 0x3E);
	cpu->R[REG_NUM(i, 0)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(ST, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STRH_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 3)] + cpu->R[REG_NUM(i, 6)];
	WRITE16(cpu->mem_if->data, adr, static_cast<uint16_t>(cpu->R[REG_NUM(i, 0)]));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_WRITE>(ST, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDRH_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 3)] + cpu->R[REG_NUM(i, 6)];
	cpu->R[REG_NUM(i, 0)] = static_cast<uint32_t>(READ16(cpu->mem_if->data, adr));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(ST, 3, adr);
}

//-----------------------------------------------------------------------------
//   LDRSH
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_LDRSH_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 3)] + cpu->R[REG_NUM(i, 6)];
	cpu->R[REG_NUM(i, 0)] = static_cast<uint32_t>(static_cast<int16_t>(READ16(cpu->mem_if->data, adr)));

	return MMU_aluMemAccessCycles<PROCNUM, 16, MMU_AD_READ>(ST, 3, adr);
}

//-----------------------------------------------------------------------------
//   STR / LDR
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_STR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 3)] + ((i >> 4) & 0x7C);
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_NUM(i, 0)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(ST, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_IMM_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 3)] + ((i >> 4) & 0x7C);
	uint32_t tempValue = READ32(cpu->mem_if->data, adr);
	adr = (adr & 3) * 8;
	tempValue = (tempValue >> adr) | (tempValue << (32 - adr));
	cpu->R[REG_NUM(i, 0)] = tempValue;

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_READ>(ST, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 6)] + cpu->R[REG_NUM(i, 3)];
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_NUM(i, 0)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(ST, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_REG_OFF(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 3)] + cpu->R[REG_NUM(i, 6)];
	uint32_t tempValue = READ32(cpu->mem_if->data, adr);
	adr = (adr & 3) * 8;
	tempValue = (tempValue >> adr) | (tempValue << (32 - adr));
	cpu->R[REG_NUM(i, 0)] = tempValue;

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_READ>(ST, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_STR_SPREL(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[13] + ((i & 0xFF) << 2);
	WRITE32(cpu->mem_if->data, adr, cpu->R[REG_NUM(i, 8)]);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(ST, 2, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_SPREL(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[13] + ((i & 0xFF) << 2);
	cpu->R[REG_NUM(i, 8)] = READ32(cpu->mem_if->data, adr);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_READ>(ST, 3, adr);
}

TEMPLATE static uint32_t FASTCALL OP_LDR_PCREL(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = (cpu->R[15] & 0xFFFFFFFC) + ((i & 0xFF) << 2);

	cpu->R[REG_NUM(i, 8)] = READ32(cpu->mem_if->data, adr);

	return MMU_aluMemAccessCycles<PROCNUM, 32, MMU_AD_READ>(ST, 3, adr);
}

//-----------------------------------------------------------------------------
//   Adjust SP
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_ADJUST_P_SP(armcpu_t *cpu, uint32_t i)
{
	cpu->R[13] += (i & 0x7F) << 2;

	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_ADJUST_M_SP(armcpu_t *cpu, uint32_t i)
{
	cpu->R[13] -= (i & 0x7F) << 2;

	return 1;
}

//-----------------------------------------------------------------------------
//   PUSH / POP
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_PUSH(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[13] - 4;
	uint32_t c = 0;

	for (uint32_t j = 0; j < 8; ++j)
		if (BIT_N(i, 7 - j))
		{
			WRITE32(cpu->mem_if->data, adr, cpu->R[7 - j]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(ST, adr);
			adr -= 4;
		}
	cpu->R[13] = adr + 4;

	return MMU_aluMemCycles<PROCNUM>(3, c);
}

TEMPLATE static uint32_t FASTCALL OP_PUSH_LR(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[13] - 4;
	uint32_t c = 0;

	WRITE32(cpu->mem_if->data, adr, cpu->R[14]);
	c += MMU_memAccessCycles<PROCNUM,32,MMU_AD_WRITE>(ST, adr);
	adr -= 4;

	for (uint32_t j = 0; j < 8; ++j)
		if (BIT_N(i, 7 - j))
		{
			WRITE32(cpu->mem_if->data, adr, cpu->R[7 - j]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(ST, adr);
			adr -= 4;
		}
	cpu->R[13] = adr + 4;

	return MMU_aluMemCycles<PROCNUM>(4, c);
}

TEMPLATE static uint32_t FASTCALL OP_POP(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[13];
	uint32_t c = 0;

	for (uint32_t j = 0; j < 8; ++j)
		if (BIT_N(i, j))
		{
			cpu->R[j] = READ32(cpu->mem_if->data, adr);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(ST, adr);
			adr += 4;
		}
	cpu->R[13] = adr;

	return MMU_aluMemCycles<PROCNUM>(2, c);
}

// In ARMv5 and above, bit[0] of the loaded value
// determines whether execution continues after this branch in ARM state or in Thumb state, as though the
// following instruction had been executed:
// BX (loaded_value)
// In T variants of ARMv4, bit[0] of the loaded value is ignored and execution continues in Thumb state, as
// though the following instruction had been executed:
// MOV PC,(loaded_value)
TEMPLATE static uint32_t FASTCALL OP_POP_PC(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[13];
	uint32_t c = 0;

	for (uint32_t j = 0; j < 8; ++j)
		if (BIT_N(i, j))
		{
			cpu->R[j] = READ32(cpu->mem_if->data, adr);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(ST, adr);
			adr += 4;
		}

	uint32_t v = READ32(cpu->mem_if->data, adr);
	c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(ST, adr);
	if (!PROCNUM)
		cpu->CPSR.bits.T = BIT0(v);

	cpu->R[15] = v & 0xFFFFFFFE;
	cpu->next_instruction = cpu->R[15];

	cpu->R[13] = adr + 4;
	return MMU_aluMemCycles<PROCNUM>(5, c);
}

//-----------------------------------------------------------------------------
//   STMIA / LDMIA
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_STMIA_THUMB(armcpu_t *cpu, uint32_t i)
{
	uint32_t adr = cpu->R[REG_NUM(i, 8)];
	uint32_t c = 0;
	bool erList = true; //Empty Register List

	// ------ ARM_REF:
	// ------ If <Rn> is specified in <registers>:
	// ------	* If <Rn> is the lowest-numbered register specified in <registers>, the original value of <Rn> is stored.
	// ------	* Otherwise, the stored value of <Rn> is UNPREDICTABLE.
	if (BIT_N(i, REG_NUM(i, 8)))
		fprintf(stderr, "STMIA with Rb in Rlist\n");

	for (uint32_t j = 0; j < 8; ++j)
		if (BIT_N(i, j))
		{
			WRITE32(cpu->mem_if->data, adr, cpu->R[j]);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_WRITE>(ST, adr);
			adr += 4;
			erList = false; //Register List isnt empty
		}

	if (erList)
		 fprintf(stderr, "STMIA with Empty Rlist\n");

	cpu->R[REG_NUM(i, 8)] = adr;
	return MMU_aluMemCycles<PROCNUM>(2, c);
}

TEMPLATE static uint32_t FASTCALL OP_LDMIA_THUMB(armcpu_t *cpu, uint32_t i)
{
	uint32_t regIndex = REG_NUM(i, 8);
	uint32_t adr = cpu->R[regIndex];
	uint32_t c = 0;
	bool erList = true; //Empty Register List

	//if (BIT_N(i, regIndex))
	//	 fprintf(stderr, "LDMIA with Rb in Rlist at %08X\n",cpu->instruct_adr);

	for (uint32_t j = 0; j < 8; ++j)
		if (BIT_N(i, j))
		{
			cpu->R[j] = READ32(cpu->mem_if->data, adr);
			c += MMU_memAccessCycles<PROCNUM, 32, MMU_AD_READ>(ST, adr);
			adr += 4;
			erList = false; //Register List isnt empty
		}

	if (erList)
		 fprintf(stderr, "LDMIA with Empty Rlist\n");

	// ARM_REF:	THUMB: Causes base register write-back, and is not optional
	// ARM_REF:	If the base register <Rn> is specified in <registers>, the final value of <Rn> is the loaded value
	//			(not the written-back value).
	if (!BIT_N(i, regIndex))
		cpu->R[regIndex] = adr;

	return MMU_aluMemCycles<PROCNUM>(3, c);
}

//-----------------------------------------------------------------------------
//   BKPT
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_BKPT_THUMB(armcpu_t *cpu, uint32_t)
{
	fprintf(stderr, "THUMB%c: OP_BKPT triggered\n", PROCNUM?'7':'9');
	Status_Reg tmp = cpu->CPSR;
	armcpu_switchMode(cpu, ABT); // enter abt mode
	cpu->R[14] = cpu->instruct_adr + 4;
	cpu->SPSR = tmp; // save old CPSR as new SPSR
	cpu->CPSR.bits.T = 0; // handle as ARM32 code
	cpu->CPSR.bits.I = 1;
	cpu->changeCPSR();
	cpu->R[15] = cpu->intVector + 0x0C;
	cpu->next_instruction = cpu->R[15];
	return 1;
}

//-----------------------------------------------------------------------------
//   SWI
//-----------------------------------------------------------------------------

TEMPLATE static uint32_t FASTCALL OP_SWI_THUMB(armcpu_t *cpu, uint32_t i)
{
	uint32_t swinum = i & 0xFF;

	//ideas-style debug prints (execute this SWI with the null terminated string address in R0)
	if (swinum == 0xFC)
	{
		//IdeasLog(cpu);
		return 0;
	}

	//if the user has changed the intVector to point away from the nds bioses,
	//then it doesn't really make any sense to use the builtin SWI's since
	//the bios ones aren't getting called anyway
	bool bypassBuiltinSWI = (cpu->intVector == 0x00000000 && !PROCNUM) || (cpu->intVector == 0xFFFF0000 && PROCNUM == 1);

	if (cpu->swi_tab && !bypassBuiltinSWI)
	{
		//zero 25-dec-2008 - in arm, we were masking to 0x1F.
		//this is probably safer since an invalid opcode could crash the emu
		//zero 30-jun-2009 - but they say that the ideas 0xFF should crash the device...
		//uint32_t swinum = cpu->instruction & 0xFF;
		swinum &= 0x1F;
		//fprintf(stderr, "%d ARM SWI %d\n",PROCNUM,swinum);
		return cpu->swi_tab[swinum](cpu) + 3;
	}
	else
	{
		/* we use an irq thats not in the irq tab, as
		it was replaced due to a changed intVector */
		Status_Reg tmp = cpu->CPSR;
		armcpu_switchMode(cpu, SVC); /* enter svc mode */
		cpu->R[14] = cpu->next_instruction; /* jump to swi Vector */
		cpu->SPSR = tmp; /* save old CPSR as new SPSR */
		cpu->CPSR.bits.T = 0; /* handle as ARM32 code */
		cpu->CPSR.bits.I = 1;
		cpu->changeCPSR();
		cpu->R[15] = cpu->intVector + 0x08;
		cpu->next_instruction = cpu->R[15];
		return 3;
	}
}

//-----------------------------------------------------------------------------
//   Branch
//-----------------------------------------------------------------------------

static inline uint32_t SIGNEEXT_IMM11(uint32_t i) { return (i & 0x7FF) | (BIT10(i) * 0xFFFFF800); }
static inline uint32_t SIGNEXTEND_11(uint32_t i) { return static_cast<uint32_t>((static_cast<int32_t>(i) << 21) >> 21); }

TEMPLATE static uint32_t FASTCALL OP_B_COND(armcpu_t *cpu, uint32_t i)
{
	if (!TEST_COND((i >> 8) & 0xF, 0, cpu->CPSR))
		return 1;

	cpu->R[15] += static_cast<uint32_t>(static_cast<int8_t>(i & 0xFF)) << 1;
	cpu->next_instruction = cpu->R[15];
	return 3;
}

TEMPLATE static uint32_t FASTCALL OP_B_UNCOND(armcpu_t *cpu, uint32_t i)
{
	cpu->R[15] += SIGNEEXT_IMM11(i) << 1;
	cpu->next_instruction = cpu->R[15];
	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_BLX(armcpu_t *cpu, uint32_t i)
{
	cpu->R[15] = (cpu->R[14] + ((i & 0x7FF) << 1)) & 0xFFFFFFFC;
	cpu->R[14] = cpu->next_instruction | 1;
	cpu->next_instruction = cpu->R[15];
	cpu->CPSR.bits.T = 0;
	return 3;
}

TEMPLATE static uint32_t FASTCALL OP_BL_10(armcpu_t *cpu, uint32_t i)
{
	cpu->R[14] = cpu->R[15] + (SIGNEXTEND_11(i) << 12);
	return 1;
}

TEMPLATE static uint32_t FASTCALL OP_BL_11(armcpu_t *cpu, uint32_t i)
{
	cpu->R[15] = (cpu->R[14] + ((i & 0x7FF) << 1));
	cpu->R[14] = cpu->next_instruction | 1;
	cpu->next_instruction = cpu->R[15];
	return 4;
}

TEMPLATE static uint32_t FASTCALL OP_BX_THUMB(armcpu_t *cpu, uint32_t i)
{
	// When using PC as operand with BX opcode, switch to ARM state and jump to (instruct_adr+4)
	// Reference: http://nocash.emubase.de/gbatek.htm#thumb5hiregisteroperationsbranchexchange

#if 0
	if (REG_POS(i, 3) == 15)
	{
		cpu->CPSR.bits.T = 0;
		cpu->R[15] &= 0xFFFFFFFC;
		cpu->next_instruction = cpu->R[15];
	}
	else
	{
		uint32_t Rm = cpu->R[REG_POS(i, 3)];

		cpu->CPSR.bits.T = BIT0(Rm);
		cpu->R[15] = (Rm & 0xFFFFFFFE);
		cpu->next_instruction = cpu->R[15];
	}
#else
	uint32_t Rm = cpu->R[REG_POS(i, 3)];
	//----- ARM_REF:
	//----- Register 15 can be specified for <Rm>. If this is done, R15 is read as normal for Thumb code,
	//----- that is, it is the address of the BX instruction itself plus 4. If the BX instruction is at a
	//----- word-aligned address, this results in a branch to the next word, executing in ARM state.
	//----- However, if the BX instruction is not at a word-aligned address, this means that the results of
	//----- the instruction are UNPREDICTABLE (because the value read for R15 has bits[1:0]==0b10).
	if (Rm == 15)
	{
		fprintf(stderr, "THUMB%c: BX using PC as operand\n", PROCNUM?'7':'9');
		//emu_halt();
	}
	cpu->CPSR.bits.T = BIT0(Rm);
	cpu->R[15] = Rm & (0xFFFFFFFC | (1 << cpu->CPSR.bits.T));
	cpu->next_instruction = cpu->R[15];
#endif
	return 3;
}

TEMPLATE static uint32_t FASTCALL OP_BLX_THUMB(armcpu_t *cpu, uint32_t i)
{
	uint32_t Rm = cpu->R[REG_POS(i, 3)];
	cpu->CPSR.bits.T = BIT0(Rm);
	cpu->R[15] = Rm & 0xFFFFFFFE;
	cpu->R[14] = cpu->next_instruction | 1;
	cpu->next_instruction = cpu->R[15];

	return 4;
}

//-----------------------------------------------------------------------------
//   The End
//-----------------------------------------------------------------------------

const OpFunc thumb_instructions_set[2][1024] =
{
	{
#define TABDECL(x) x<0>
#include "thumb_tabdef.inc"
#undef TABDECL
	}, {
#define TABDECL(x) x<1>
#include "thumb_tabdef.inc"
#undef TABDECL
	}
};
