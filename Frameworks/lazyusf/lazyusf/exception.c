/*
 * Project 64 - A Nintendo 64 emulator.
 *
 * (c) Copyright 2001 zilmar (zilmar@emulation64.com) and
 * Jabo (jabo@emulation64.com).
 *
 * pj64 homepage: www.pj64.net
 *
 * Permission to use, copy, modify and distribute Project64 in both binary and
 * source form, for non-commercial purposes, is hereby granted without fee,
 * providing that this license information and copyright notice appear with
 * all copies and any derived work.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event shall the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Project64 is freeware for PERSONAL USE only. Commercial users should
 * seek permission of the copyright holders first. Commercial use includes
 * charging money for Project64 or software derived from Project64.
 *
 * The copyright holders request that bug fixes and improvements to the code
 * should be forwarded to them so if they want them.
 *
 */

#include "main.h"
#include "cpu.h"

#include "usf_internal.h"

void CheckInterrupts ( usf_state_t * state ) {

	MI_INTR_REG &= ~MI_INTR_AI;
	MI_INTR_REG |= (state->AudioIntrReg & MI_INTR_AI);
	if ((MI_INTR_MASK_REG & MI_INTR_REG) != 0) {
		FAKE_CAUSE_REGISTER |= CAUSE_IP2;
	} else  {
		FAKE_CAUSE_REGISTER &= ~CAUSE_IP2;
	}

	if (( STATUS_REGISTER & STATUS_IE   ) == 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { return; }

	if (( STATUS_REGISTER & FAKE_CAUSE_REGISTER & 0xFF00) != 0) {
		if (!state->CPU_Action->DoInterrupt) {
			state->CPU_Action->DoSomething = 1;
			state->CPU_Action->DoInterrupt = 1;
		}
	}
}

void DoAddressError ( usf_state_t * state, uint32_t DelaySlot, uint32_t BadVaddr, uint32_t FromRead) {
	if (FromRead) {
		CAUSE_REGISTER = EXC_RADE;
	} else {
		CAUSE_REGISTER = EXC_WADE;
	}
	BAD_VADDR_REGISTER = BadVaddr;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = state->PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = state->PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	state->PROGRAM_COUNTER = 0x80000180;
}

void DoBreakException ( usf_state_t * state, uint32_t DelaySlot) {
	CAUSE_REGISTER = EXC_BREAK;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = state->PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = state->PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	state->PROGRAM_COUNTER = 0x80000180;
}

void DoCopUnusableException ( usf_state_t * state, uint32_t DelaySlot, uint32_t Coprocessor ) {
	CAUSE_REGISTER = EXC_CPU;
	if (Coprocessor == 1) { CAUSE_REGISTER |= 0x10000000; }
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = state->PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = state->PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	state->PROGRAM_COUNTER = 0x80000180;
}

void DoIntrException ( usf_state_t * state, uint32_t DelaySlot ) {

	if (( STATUS_REGISTER & STATUS_IE   ) == 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_EXL  ) != 0 ) { return; }
	if (( STATUS_REGISTER & STATUS_ERL  ) != 0 ) { return; }
	CAUSE_REGISTER = FAKE_CAUSE_REGISTER;
	CAUSE_REGISTER |= EXC_INT;
	EPC_REGISTER = state->PROGRAM_COUNTER;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER -= 4;
	}
	STATUS_REGISTER |= STATUS_EXL;
	state->PROGRAM_COUNTER = 0x80000180;
}

void DoTLBMiss ( usf_state_t * state, uint32_t DelaySlot, uint32_t BadVaddr ) {

	CAUSE_REGISTER = EXC_RMISS;
	BAD_VADDR_REGISTER = BadVaddr;
	CONTEXT_REGISTER &= 0xFF80000F;
	CONTEXT_REGISTER |= (BadVaddr >> 9) & 0x007FFFF0;
	ENTRYHI_REGISTER = (BadVaddr & 0xFFFFE000);
	if ((STATUS_REGISTER & STATUS_EXL) == 0) {
		if (DelaySlot) {
			CAUSE_REGISTER |= CAUSE_BD;
			EPC_REGISTER = state->PROGRAM_COUNTER - 4;
		} else {
			EPC_REGISTER = state->PROGRAM_COUNTER;
		}
		if (AddressDefined(state, BadVaddr)) {
			state->PROGRAM_COUNTER = 0x80000180;
		} else {
			state->PROGRAM_COUNTER = 0x80000000;
		}
		STATUS_REGISTER |= STATUS_EXL;
	} else {
		state->PROGRAM_COUNTER = 0x80000180;
	}
}

void DoSysCallException ( usf_state_t * state, uint32_t DelaySlot) {
	CAUSE_REGISTER = EXC_SYSCALL;
	if (DelaySlot) {
		CAUSE_REGISTER |= CAUSE_BD;
		EPC_REGISTER = state->PROGRAM_COUNTER - 4;
	} else {
		EPC_REGISTER = state->PROGRAM_COUNTER;
	}
	STATUS_REGISTER |= STATUS_EXL;
	state->PROGRAM_COUNTER = 0x80000180;
}
