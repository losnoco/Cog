/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2006-2012 DeSmuME team

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

#pragma once

#include <vio2sf/types.h>
#include <vio2sf/bits.h>
#include <vio2sf/MMU.h>
#include <vio2sf/instructions.h>

struct vio2sf_state;

inline uint32_t CODE(uint32_t i) { return (i >> 25) & 0x7; }

const uint32_t EXCEPTION_RESET = 0x00;
const uint32_t EXCEPTION_UNDEFINED_INSTRUCTION = 0x04;
const uint32_t EXCEPTION_SWI = 0x08;
const uint32_t EXCEPTION_PREFETCH_ABORT = 0x0C;
const uint32_t EXCEPTION_DATA_ABORT = 0x10;
const uint32_t EXCEPTION_RESERVED_0x14 = 0x14;
const uint32_t EXCEPTION_IRQ = 0x18;
const uint32_t EXCEPTION_FAST_IRQ = 0x1C;

inline uint32_t INSTRUCTION_INDEX(uint32_t i) { return ((i >> 16) & 0xFF0) | ((i >> 4) & 0xF); }

inline uint32_t ROR(uint32_t i, uint32_t j) { return (i >> j) | (i << (32 - j)); }

template<typename T> inline T UNSIGNED_OVERFLOW(T a, T b, T c) { return BIT31((a & b) | ((a | b) & ~c)); }

template<typename T> inline T UNSIGNED_UNDERFLOW(T a, T b, T c) { return BIT31((~a & b) | ((~a | b) & c)); }

template<typename T> inline T SIGNED_OVERFLOW(T a, T b, T c) { return BIT31((a & b & ~c) | (~a & ~b & c)); }

template<typename T> inline T SIGNED_UNDERFLOW(T a, T b, T c) { return BIT31((a & ~b & ~c) | (~a & b & c)); }

// ============================= CPRS flags funcs
inline bool CarryFrom(int32_t left, int32_t right)
{
	uint32_t res = 0xFFFFFFFFU - static_cast<uint32_t>(left);

	return static_cast<uint32_t>(right) > res;
}

inline bool BorrowFrom(int32_t left, int32_t right)
{
	return static_cast<uint32_t>(right) > static_cast<uint32_t>(left);
}

inline bool OverflowFromADD(int32_t alu_out, int32_t left, int32_t right)
{
	return ((left >= 0 && right >= 0) || (left < 0 && right < 0)) && ((left < 0 && alu_out >= 0) || (left >= 0 && alu_out < 0));
}

inline bool OverflowFromSUB(int32_t alu_out, int32_t left, int32_t right)
{
	return ((left < 0 && right >= 0) || (left >= 0 && right < 0)) && ((left < 0 && alu_out >= 0) || (left >= 0 && alu_out < 0));
}

const uint8_t arm_cond_table[] =
{
	// N=0, Z=0, C=0, V=0
	0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,	// 0x00
	0x00,0xFF,0xFF,0x00,0xFF,0x00,0xFF,0x20,	// 0x00
	// N=0, Z=0, C=0, V=1
	0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x00,	// 0x10
	0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x20,
	// N=0, Z=0, C=1, V=0
	0x00,0xFF,0xFF,0x00,0x00,0xFF,0x00,0xFF,	// 0x20
	0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x20,
	// N=0, Z=0, C=1, V=1
	0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,	// 0x30
	0xFF,0x00,0x00,0xFF,0x00,0xFF,0xFF,0x20,
	// N=0, Z=1, C=0, V=0
	0xFF,0x00,0x00,0xFF,0x00,0xFF,0x00,0xFF,	// 0x40
	0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x20,
	// N=0, Z=1, C=0, V=1
	0xFF,0x00,0x00,0xFF,0x00,0xFF,0xFF,0x00,	// 0x50
	0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x20,
	// N=0, Z=1, C=1, V=0
	0xFF,0x00,0xFF,0x00,0x00,0xFF,0x00,0xFF,	// 0x60
	0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x20,
	// N=0, Z=1, C=1, V=1
	0xFF,0x00,0xFF,0x00,0x00,0xFF,0xFF,0x00,	// 0x70
	0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x20,
	// N=1, Z=0, C=0, V=0
	0x00,0xFF,0x00,0xFF,0xFF,0x00,0x00,0xFF,	// 0x80
	0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x20,
	// N=1, Z=0, C=0, V=1
	0x00,0xFF,0x00,0xFF,0xFF,0x00,0xFF,0x00,	// 0x90
	0x00,0xFF,0xFF,0x00,0xFF,0x00,0xFF,0x20,
	// N=1, Z=0, C=1, V=0
	0x00,0xFF,0xFF,0x00,0xFF,0x00,0x00,0xFF,	// 0xA0
	0xFF,0x00,0x00,0xFF,0x00,0xFF,0xFF,0x20,
	// N=1, Z=0, C=1, V=1
	0x00,0xFF,0xFF,0x00,0xFF,0x00,0xFF,0x00,	// 0xB0
	0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x20,
	// N=1, Z=1, C=0, V=0
	0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0xFF,	// 0xC0
	0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x20,
	// N=1, Z=1, C=0, V=1
	0xFF,0x00,0x00,0xFF,0xFF,0x00,0xFF,0x00,	// 0xD0
	0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x20,
	// N=1, Z=1, C=1, V=0
	0xFF,0x00,0xFF,0x00,0xFF,0x00,0x00,0xFF,	// 0xE0
	0x00,0xFF,0x00,0xFF,0x00,0xFF,0xFF,0x20,
	// N=1, Z=1, C=1, V=1
	0xFF,0x00,0xFF,0x00,0xFF,0x00,0xFF,0x00,	// 0xF0
	0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x20
};

enum Mode
{
	USR = 0x10,
	FIQ = 0x11,
	IRQ = 0x12,
	SVC = 0x13,
	ABT = 0x17,
	UND = 0x1B,
	SYS = 0x1F
};

#ifdef WORDS_BIGENDIAN
typedef union
{
	struct
	{
		uint32_t N : 1,
		Z : 1,
		C : 1,
		V : 1,
		Q : 1,
		RAZ : 19,
		I : 1,
		F : 1,
		T : 1,
		mode : 5;
	} bits;
	uint32_t val;
} Status_Reg;
#else
typedef union
{
	struct
	{
		uint32_t mode : 5,
		T : 1,
		F : 1,
		I : 1,
		RAZ : 19,
		Q : 1,
		V : 1,
		C : 1,
		Z : 1,
		N : 1;
	} bits;
	uint32_t val;
} Status_Reg;
#endif

inline uint8_t TEST_COND(uint32_t cond, uint32_t inst, Status_Reg CPSR) { return arm_cond_table[((CPSR.val >> 24) & 0xf0) | cond] & (1 << inst); }

typedef void *armcp_t;

struct vio2sf_state;

struct armcpu_t
{
	vio2sf_state *st;

	uint32_t proc_ID;
	uint32_t instruction; //4
	uint32_t instruct_adr; //8
	uint32_t next_instruction; //12

	uint32_t R[16]; //16
	Status_Reg CPSR;  //80
	Status_Reg SPSR;

	void changeCPSR();

	uint32_t R13_usr, R14_usr;
	uint32_t R13_svc, R14_svc;
	uint32_t R13_abt, R14_abt;
	uint32_t R13_und, R14_und;
	uint32_t R13_irq, R14_irq;
	uint32_t R8_fiq, R9_fiq, R10_fiq, R11_fiq, R12_fiq, R13_fiq, R14_fiq;
	Status_Reg SPSR_svc, SPSR_abt, SPSR_und, SPSR_irq, SPSR_fiq;

	uint32_t intVector;
	uint8_t LDTBit; // 1 : ARMv5 style 0 : non ARMv5
	bool waitIRQ;
	bool halt_IE_and_IF; //the cpu is halted, waiting for IE&IF to signal something
	uint8_t intrWaitARM_state;

	bool BIOS_loaded;

	uint32_t (**swi_tab)(armcpu_t *);

	// flag indicating if the processor is stalled (for debugging)
	int stalled;
};

int armcpu_new(vio2sf_state *st, armcpu_t *armcpu, uint32_t id);
void armcpu_init(armcpu_t *armcpu, uint32_t adr);
uint32_t armcpu_switchMode(armcpu_t *armcpu, uint8_t mode);

bool armcpu_irqException(armcpu_t *armcpu);
void armcpu_exception(armcpu_t *cpu, uint32_t number);
uint32_t TRAPUNDEF(armcpu_t* cpu);
uint32_t armcpu_Wait4IRQ(armcpu_t *cpu);

template<int PROCNUM> uint32_t armcpu_exec(armcpu_t *cpu);

void setIF(vio2sf_state *st, int PROCNUM, uint32_t flag);

inline void NDS_makeIrq(vio2sf_state *st, int PROCNUM, uint32_t num)
{
	setIF(st, PROCNUM, 1 << num);
}
