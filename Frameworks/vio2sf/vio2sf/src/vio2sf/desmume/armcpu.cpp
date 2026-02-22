/*
	Copyright (C) 2006 yopyop
	Copyright (C) 2009-2012 DeSmuME team

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

#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include "vio2sf.h"
#include "instructions.h"
#include "cp15.h"
#include "bios.h"

template<uint32_t> static uint32_t armcpu_prefetch(armcpu_t *const armcpu);

static inline uint32_t armcpu_prefetch(armcpu_t *const armcpu)
{
	if (!armcpu->proc_ID)
		return armcpu_prefetch<0>(armcpu);
	else
		return armcpu_prefetch<1>(armcpu);
}

int armcpu_new(vio2sf_state *st, armcpu_t *armcpu, uint32_t id)
{
	armcpu->st = st;

	armcpu->proc_ID = id;

	armcpu->stalled = 0;

	armcpu_init(armcpu, 0);

	return 0;
}

// call this whenever CPSR is changed (other than CNVZQ or T flags); interrupts may need to be unleashed
void armcpu_t::changeCPSR()
{
	// but all it does is give them a chance to unleash by forcing an immediate reschedule
	// TODO - we could actually set CPSR through here and look for a change in the I bit
	// that would be a little optimization as well as a safety measure if we prevented setting CPSR directly
	NDS_Reschedule(st);
}

void armcpu_init(armcpu_t *armcpu, uint32_t adr)
{
	armcpu->LDTBit = !armcpu->proc_ID; // Si ARM9 utiliser le syte v5 pour le load
	armcpu->intVector = 0xFFFF0000 * !armcpu->proc_ID;
	armcpu->waitIRQ = false;
	armcpu->halt_IE_and_IF = false;
	armcpu->intrWaitARM_state = 0;

	for (int i = 0; i < 16; ++i)
		armcpu->R[i] = 0;

	armcpu->CPSR.val = armcpu->SPSR.val = SYS;

	armcpu->R13_usr = armcpu->R14_usr = 0;
	armcpu->R13_svc = armcpu->R14_svc = 0;
	armcpu->R13_abt = armcpu->R14_abt = 0;
	armcpu->R13_und = armcpu->R14_und = 0;
	armcpu->R13_irq = armcpu->R14_irq = 0;
	armcpu->R8_fiq = armcpu->R9_fiq = armcpu->R10_fiq = armcpu->R11_fiq = armcpu->R12_fiq = armcpu->R13_fiq = armcpu->R14_fiq = 0;

	armcpu->SPSR_svc.val = armcpu->SPSR_abt.val = armcpu->SPSR_und.val = armcpu->SPSR_irq.val = armcpu->SPSR_fiq.val = 0;

	armcpu->next_instruction = adr;

	armcpu_prefetch(armcpu);
}

uint32_t armcpu_switchMode(armcpu_t *armcpu, uint8_t mode)
{
	uint32_t oldmode = armcpu->CPSR.bits.mode;

	switch (oldmode)
	{
		case USR:
		case SYS:
			armcpu->R13_usr = armcpu->R[13];
			armcpu->R14_usr = armcpu->R[14];
			break;
		case FIQ:
			std::swap(armcpu->R[8], armcpu->R8_fiq);
			std::swap(armcpu->R[9], armcpu->R9_fiq);
			std::swap(armcpu->R[10], armcpu->R10_fiq);
			std::swap(armcpu->R[11], armcpu->R11_fiq);
			std::swap(armcpu->R[12], armcpu->R12_fiq);
			armcpu->R13_fiq = armcpu->R[13];
			armcpu->R14_fiq = armcpu->R[14];
			armcpu->SPSR_fiq = armcpu->SPSR;
			break;
		case IRQ:
			armcpu->R13_irq = armcpu->R[13];
			armcpu->R14_irq = armcpu->R[14];
			armcpu->SPSR_irq = armcpu->SPSR;
			break;
		case SVC:
			armcpu->R13_svc = armcpu->R[13];
			armcpu->R14_svc = armcpu->R[14];
			armcpu->SPSR_svc = armcpu->SPSR;
			break;
		case ABT:
			armcpu->R13_abt = armcpu->R[13];
			armcpu->R14_abt = armcpu->R[14];
			armcpu->SPSR_abt = armcpu->SPSR;
			break;
		case UND:
			armcpu->R13_und = armcpu->R[13];
			armcpu->R14_und = armcpu->R[14];
			armcpu->SPSR_und = armcpu->SPSR;
			break;
		default:
			fprintf(stderr, "switchMode: WRONG mode %02X\n",mode);
	}

	switch (mode)
	{
		case USR:
		case SYS:
			armcpu->R[13] = armcpu->R13_usr;
			armcpu->R[14] = armcpu->R14_usr;
			//SPSR = CPSR;
			break;
		case FIQ:
			std::swap(armcpu->R[8], armcpu->R8_fiq);
			std::swap(armcpu->R[9], armcpu->R9_fiq);
			std::swap(armcpu->R[10], armcpu->R10_fiq);
			std::swap(armcpu->R[11], armcpu->R11_fiq);
			std::swap(armcpu->R[12], armcpu->R12_fiq);
			armcpu->R[13] = armcpu->R13_fiq;
			armcpu->R[14] = armcpu->R14_fiq;
			armcpu->SPSR = armcpu->SPSR_fiq;
			break;
		case IRQ:
			armcpu->R[13] = armcpu->R13_irq;
			armcpu->R[14] = armcpu->R14_irq;
			armcpu->SPSR = armcpu->SPSR_irq;
			break;
		case SVC:
			armcpu->R[13] = armcpu->R13_svc;
			armcpu->R[14] = armcpu->R14_svc;
			armcpu->SPSR = armcpu->SPSR_svc;
			break;
		case ABT:
			armcpu->R[13] = armcpu->R13_abt;
			armcpu->R[14] = armcpu->R14_abt;
			armcpu->SPSR = armcpu->SPSR_abt;
			break;
		case UND:
			armcpu->R[13] = armcpu->R13_und;
			armcpu->R[14] = armcpu->R14_und;
			armcpu->SPSR = armcpu->SPSR_und;
			break;
		default:
			break;
	}

	armcpu->CPSR.bits.mode = mode & 0x1F;
	armcpu->changeCPSR();
	return oldmode;
}

uint32_t armcpu_Wait4IRQ(armcpu_t *cpu)
{
	cpu->waitIRQ = true;
	cpu->halt_IE_and_IF = true;
	return 1;
}

template<uint32_t PROCNUM> static inline uint32_t armcpu_prefetch(armcpu_t *const armcpu)
{
	uint32_t curInstruction = armcpu->next_instruction;

	if (!armcpu->CPSR.bits.T)
	{
		curInstruction &= 0xFFFFFFFC; // please don't change this to 0x0FFFFFFC -- the NDS will happily run on 0xF******* addresses all day long
		// please note that we must setup R[15] before reading the instruction since there is a protection
		// which prevents PC > 0x3FFF from reading the bios region
		armcpu->instruct_adr = curInstruction;
		armcpu->next_instruction = curInstruction + 4;
		armcpu->R[15] = curInstruction + 8;
		armcpu->instruction = _MMU_read32<PROCNUM, MMU_AT_CODE>(armcpu->st, curInstruction);

		return MMU_codeFetchCycles<PROCNUM, 32>(armcpu->st, curInstruction);
	}
	curInstruction &= 0xFFFFFFFE; // please don't change this to 0x0FFFFFFE -- the NDS will happily run on 0xF******* addresses all day long
	// please note that we must setup R[15] before reading the instruction since there is a protection
	// which prevents PC > 0x3FFF from reading the bios region
	armcpu->instruct_adr = curInstruction;
	armcpu->next_instruction = curInstruction + 2;
	armcpu->R[15] = curInstruction + 4;
	armcpu->instruction = _MMU_read16<PROCNUM, MMU_AT_CODE>(armcpu->st, curInstruction);

	if (!PROCNUM)
	{
		// arm9 fetches 2 instructions at a time in thumb mode
		if (!(curInstruction == armcpu->instruct_adr + 2 && (curInstruction & 2)))
			return MMU_codeFetchCycles<PROCNUM, 32>(armcpu->st, curInstruction);
		else
			return 0;
	}

	return MMU_codeFetchCycles<PROCNUM, 16>(armcpu->st, curInstruction);
}

// TODO - merge with armcpu_irqException?
// http://www.ethernut.de/en/documents/arm-exceptions.html
// http://docs.google.com/viewer?a=v&q=cache:V4ht1YkxprMJ:www.cs.nctu.edu.tw/~wjtsai/EmbeddedSystemDesign/Ch3-1.pdf+arm+exception+handling&hl=en&gl=us&pid=bl&srcid=ADGEEShx9VTHbUhWdDOrTVRzLkcCsVfJiijncNDkkgkrlJkLa7D0LCpO8fQ_hhU3DTcgZh9rcZWWQq4TYhhCovJ625h41M0ZUX3WGasyzWQFxYzDCB-VS6bsUmpoJnRxAc-bdkD0qmsu&sig=AHIEtbR9VHvDOCRmZFQDUVwy53iJDjoSPQ
void armcpu_exception(armcpu_t *cpu, uint32_t number)
{
	Mode cpumode = USR;
	switch (number)
	{
		case EXCEPTION_RESET:
			cpumode = SVC;
			break;
		case EXCEPTION_UNDEFINED_INSTRUCTION:
			cpumode = UND;
			break;
		case EXCEPTION_SWI:
			cpumode = SVC;
			break;
		case EXCEPTION_PREFETCH_ABORT:
			cpumode = ABT;
			break;
		case EXCEPTION_DATA_ABORT:
			cpumode = ABT;
			break;
		case EXCEPTION_RESERVED_0x14:
			cpu->st->execute = false;
			break;
		case EXCEPTION_IRQ:
			cpumode = IRQ;
			break;
		case EXCEPTION_FAST_IRQ:
			cpumode = FIQ;
	}

	Status_Reg tmp = cpu->CPSR;
	armcpu_switchMode(cpu, cpumode); // enter new mode
	cpu->R[14] = cpu->next_instruction;
	cpu->SPSR = tmp; // save old CPSR as new SPSR
	cpu->CPSR.bits.T = 0; // handle as ARM32 code
	cpu->CPSR.bits.I = 1;
	cpu->changeCPSR();
	cpu->R[15] = cpu->intVector + number;
	cpu->next_instruction = cpu->R[15];
	fprintf(stderr, "armcpu_exception!\n");

	// HOW DOES THIS WORTK WITHOUT A PREFETCH, LIKE IRQ BELOW?
	// I REALLY WISH WE DIDNT PREFETCH BEFORE EXECUTING
}

bool armcpu_irqException(armcpu_t *armcpu)
{
	Status_Reg tmp;

	tmp = armcpu->CPSR;
	armcpu_switchMode(armcpu, IRQ);

	armcpu->R[14] = armcpu->instruct_adr + 4;
	armcpu->SPSR = tmp;
	armcpu->CPSR.bits.T = 0;
	armcpu->CPSR.bits.I = 1;
	armcpu->next_instruction = armcpu->intVector + 0x18;
	armcpu->waitIRQ = 0;

	// must retain invariant of having next instruction to be executed prefetched
	// (yucky)
	armcpu_prefetch(armcpu);

	return true;
}

uint32_t TRAPUNDEF(armcpu_t *cpu)
{
	if (!!cpu->intVector ^ (cpu->proc_ID == ARMCPU_ARM9))
	{
		armcpu_exception(&cpu->st->NDS_ARM9, EXCEPTION_UNDEFINED_INSTRUCTION);
		return 4;
	}
	else
	{
		cpu->st->execute = false;
		return 4;
	}
}

template<int PROCNUM> uint32_t armcpu_exec(armcpu_t *cpu)
{
	// Usually, fetching and executing are processed parallelly.
	// So this function stores the cycles of each process to
	// the variables below, and returns appropriate cycle count.
	uint32_t cFetch = 0;
	uint32_t cExecute = 0;

	// this assert is annoying. but sometimes it is handy.
	//assert(ARMPROC.instruct_adr!=0x00000000);

	//cFetch = armcpu_prefetch(&ARMPROC);

	//fprintf(stderr, "%d: %08X\n",PROCNUM,ARMPROC.instruct_adr);

	if (!cpu->CPSR.bits.T)
	{
		if (
			CONDITION(cpu->instruction) == 0x0E  // fast path for unconditional instructions
			|| (TEST_COND(CONDITION(cpu->instruction), CODE(cpu->instruction), cpu->CPSR)) // handles any condition
		)
		{
			cExecute = arm_instructions_set[PROCNUM][INSTRUCTION_INDEX(cpu->instruction)](cpu, cpu->instruction);
		}
		else
			cExecute = 1; // If condition=false: 1S cycle
		cFetch = armcpu_prefetch<PROCNUM>(cpu);
		return MMU_fetchExecuteCycles<PROCNUM>(cpu->st, cExecute, cFetch);
	}

	cExecute = thumb_instructions_set[PROCNUM][cpu->instruction>>6](cpu, cpu->instruction);

	cFetch = armcpu_prefetch<PROCNUM>(cpu);
	return MMU_fetchExecuteCycles<PROCNUM>(cpu->st, cExecute, cFetch);
}

// these templates needed to be instantiated manually
template uint32_t armcpu_exec<0>(armcpu_t *);
template uint32_t armcpu_exec<1>(armcpu_t *);

void setIF(vio2sf_state *st, int PROCNUM, uint32_t flag)
{
	// don't set generated bits!!!
	assert(!(flag&0x00200000));

	st->MMU.reg_IF_bits[PROCNUM] |= flag;

	extern void NDS_Reschedule(vio2sf_state *st);
	NDS_Reschedule(st);
}
