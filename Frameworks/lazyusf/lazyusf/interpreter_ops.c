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

#include <stdio.h>
#include <math.h>
#include <float.h>
#include "main.h"
#include "cpu.h"
#include "usf.h"

#ifdef _MSC_VER
#define INLINE      __forceinline
#else
#define INLINE      inline __attribute__((always_inline))
#endif

#include "usf_internal.h"

#define ADDRESS_ERROR_EXCEPTION(Address,FromRead) \
	DoAddressError(state,state->NextInstruction == JUMP,Address,FromRead);\
	state->NextInstruction = JUMP;\
	state->JumpToLocation = state->PROGRAM_COUNTER;\
	return;

//#define TEST_COP1_USABLE_EXCEPTION
#define TEST_COP1_USABLE_EXCEPTION \
	if ((STATUS_REGISTER & STATUS_CU1) == 0) {\
		DoCopUnusableException(state,state->NextInstruction == JUMP,1);\
		state->NextInstruction = JUMP;\
		state->JumpToLocation = state->PROGRAM_COUNTER;\
		return;\
	}

#define TLB_READ_EXCEPTION(Address) \
	DoTLBMiss(state,state->NextInstruction == JUMP,Address);\
	state->NextInstruction = JUMP;\
	state->JumpToLocation = state->PROGRAM_COUNTER;\
	return;

/************************* OpCode functions *************************/
void r4300i_J (usf_state_t * state) {
	state->NextInstruction = DELAY_SLOT;
	state->JumpToLocation = (state->PROGRAM_COUNTER & 0xF0000000) + (state->Opcode.u.d.target << 2);
	TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,0,0);
}

void r4300i_JAL (usf_state_t * state) {
	state->NextInstruction = DELAY_SLOT;
	state->JumpToLocation = (state->PROGRAM_COUNTER & 0xF0000000) + (state->Opcode.u.d.target << 2);
	TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,0,0);
	state->GPR[31].DW= (int32_t)(state->PROGRAM_COUNTER + 8);
}

void r4300i_BEQ (usf_state_t * state) {
	state->NextInstruction = DELAY_SLOT;
	if (state->GPR[state->Opcode.u.b.rs].DW == state->GPR[state->Opcode.u.b.rt].DW) {
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,state->Opcode.u.b.rt);
	} else {
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_BNE (usf_state_t * state) {
	state->NextInstruction = DELAY_SLOT;
	if (state->GPR[state->Opcode.u.b.rs].DW != state->GPR[state->Opcode.u.b.rt].DW) {
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,state->Opcode.u.b.rt);
	} else {
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_BLEZ (usf_state_t * state) {
	state->NextInstruction = DELAY_SLOT;
	if (state->GPR[state->Opcode.u.b.rs].DW <= 0) {
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,0);
	} else {
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_BGTZ (usf_state_t * state) {
	state->NextInstruction = DELAY_SLOT;
	if (state->GPR[state->Opcode.u.b.rs].DW > 0) {
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,0);
	} else {
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_ADDI (usf_state_t * state) {
	if (state->Opcode.u.b.rt == 0) { return; }
	state->GPR[state->Opcode.u.b.rt].DW = (state->GPR[state->Opcode.u.b.rs].W[0] + ((int16_t)state->Opcode.u.c.immediate));
}

void r4300i_ADDIU (usf_state_t * state) {
	state->GPR[state->Opcode.u.b.rt].DW = (state->GPR[state->Opcode.u.b.rs].W[0] + ((int16_t)state->Opcode.u.c.immediate));
}

void r4300i_SLTI (usf_state_t * state) {
	if (state->GPR[state->Opcode.u.b.rs].DW < (int64_t)((int16_t)state->Opcode.u.c.immediate)) {
		state->GPR[state->Opcode.u.b.rt].DW = 1;
	} else {
		state->GPR[state->Opcode.u.b.rt].DW = 0;
	}
}

void r4300i_SLTIU (usf_state_t * state) {
	int32_t imm32 = (int16_t)state->Opcode.u.c.immediate;
	int64_t imm64;

	imm64 = imm32;
	state->GPR[state->Opcode.u.b.rt].DW = state->GPR[state->Opcode.u.b.rs].UDW < (uint64_t)imm64?1:0;
}

void r4300i_ANDI (usf_state_t * state) {
	state->GPR[state->Opcode.u.b.rt].DW = state->GPR[state->Opcode.u.b.rs].DW & state->Opcode.u.c.immediate;
}

void r4300i_ORI (usf_state_t * state) {
	state->GPR[state->Opcode.u.b.rt].DW = state->GPR[state->Opcode.u.b.rs].DW | state->Opcode.u.c.immediate;
}

void r4300i_XORI (usf_state_t * state) {
	state->GPR[state->Opcode.u.b.rt].DW = state->GPR[state->Opcode.u.b.rs].DW ^ state->Opcode.u.c.immediate;
}

void r4300i_LUI (usf_state_t * state) {
	if (state->Opcode.u.b.rt == 0) { return; }
	state->GPR[state->Opcode.u.b.rt].DW = (int32_t)((int16_t)state->Opcode.u.b.offset << 16);
}

void r4300i_BEQL (usf_state_t * state) {
	if (state->GPR[state->Opcode.u.b.rs].DW == state->GPR[state->Opcode.u.b.rt].DW) {
		state->NextInstruction = DELAY_SLOT;
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,state->Opcode.u.b.rt);
	} else {
		state->NextInstruction = JUMP;
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_BNEL (usf_state_t * state) {
	if (state->GPR[state->Opcode.u.b.rs].DW != state->GPR[state->Opcode.u.b.rt].DW) {
		state->NextInstruction = DELAY_SLOT;
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,state->Opcode.u.b.rt);
	} else {
		state->NextInstruction = JUMP;
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_BLEZL (usf_state_t * state) {
	if (state->GPR[state->Opcode.u.b.rs].DW <= 0) {
		state->NextInstruction = DELAY_SLOT;
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,0);
	} else {
		state->NextInstruction = JUMP;
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_BGTZL (usf_state_t * state) {
	if (state->GPR[state->Opcode.u.b.rs].DW > 0) {
		state->NextInstruction = DELAY_SLOT;
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,0);
	} else {
		state->NextInstruction = JUMP;
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_DADDIU (usf_state_t * state) {
	state->GPR[state->Opcode.u.b.rt].DW = state->GPR[state->Opcode.u.b.rs].DW + (int64_t)((int16_t)state->Opcode.u.c.immediate);
}

uint64_t LDL_MASK[8] = { 0ULL,0xFFULL,0xFFFFULL,0xFFFFFFULL,0xFFFFFFFFULL,0xFFFFFFFFFFULL, 0xFFFFFFFFFFFFULL, 0xFFFFFFFFFFFFFFULL };
int32_t LDL_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };

void r4300i_LDL (usf_state_t * state) {
	uint32_t Offset, Address;
	uint64_t Value;

	Address = state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	Offset  = Address & 7;

	if (!r4300i_LD_VAddr(state,(Address & ~7),&Value)) {
		return;
	}
	state->GPR[state->Opcode.u.b.rt].DW = state->GPR[state->Opcode.u.b.rt].DW & LDL_MASK[Offset];
	state->GPR[state->Opcode.u.b.rt].DW += Value << LDL_SHIFT[Offset];
}

uint64_t LDR_MASK[8] = { 0xFFFFFFFFFFFFFF00ULL, 0xFFFFFFFFFFFF0000ULL,
                      0xFFFFFFFFFF000000ULL, 0xFFFFFFFF00000000ULL,
                      0xFFFFFF0000000000ULL, 0xFFFF000000000000ULL,
                      0xFF00000000000000ULL, 0 };
int32_t LDR_SHIFT[8] = { 56, 48, 40, 32, 24, 16, 8, 0 };

void r4300i_LDR (usf_state_t * state) {
	uint32_t Offset, Address;
	uint64_t Value;

	Address = state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	Offset  = Address & 7;

	if (!r4300i_LD_VAddr(state,(Address & ~7),&Value)) {
		return;
	}

	state->GPR[state->Opcode.u.b.rt].DW = state->GPR[state->Opcode.u.b.rt].DW & LDR_MASK[Offset];
	state->GPR[state->Opcode.u.b.rt].DW += Value >> LDR_SHIFT[Offset];

}

void r4300i_LB (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	if (state->Opcode.u.b.rt == 0) { return; }
	if (!r4300i_LB_VAddr(state,Address,&state->GPR[state->Opcode.u.b.rt].UB[0])) {
		TLB_READ_EXCEPTION(Address);
	} else {
		state->GPR[state->Opcode.u.b.rt].DW = state->GPR[state->Opcode.u.b.rt].B[0];
	}
}

void r4300i_LH (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	if ((Address & 1) != 0) { ADDRESS_ERROR_EXCEPTION(Address,1); }
	if (!r4300i_LH_VAddr(state,Address,&state->GPR[state->Opcode.u.b.rt].UHW[0])) {
		//if (ShowTLBMisses) {
            // Too spammy
			//DisplayError(state, "LH TLB: %X",Address);
		//}
		TLB_READ_EXCEPTION(Address);
	} else {
		state->GPR[state->Opcode.u.b.rt].DW = state->GPR[state->Opcode.u.b.rt].HW[0];
	}
}

uint32_t LWL_MASK[4] = { 0,0xFF,0xFFFF,0xFFFFFF };
int32_t LWL_SHIFT[4] = { 0, 8, 16, 24};

void r4300i_LWL (usf_state_t * state) {
	uint32_t Offset, Address, Value;

	Address = state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	Offset  = Address & 3;

	if (!r4300i_LW_VAddr(state,(Address & ~3),&Value)) {
		return;
	}

	state->GPR[state->Opcode.u.b.rt].DW = (int32_t)(state->GPR[state->Opcode.u.b.rt].W[0] & LWL_MASK[Offset]);
	state->GPR[state->Opcode.u.b.rt].DW += (int32_t)(Value << LWL_SHIFT[Offset]);
}

void r4300i_LW (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;


//	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,1); }

	if (state->Opcode.u.b.rt == 0) { return; }


	if (!r4300i_LW_VAddr(state,Address,&state->GPR[state->Opcode.u.b.rt].UW[0])) {
		//if (ShowTLBMisses) {
			//printf("LW TLB: %X",Address);
		//}
		TLB_READ_EXCEPTION(Address);
	} else {
		state->GPR[state->Opcode.u.b.rt].DW = state->GPR[state->Opcode.u.b.rt].W[0];
	}
}

void r4300i_LBU (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	if (!r4300i_LB_VAddr(state,Address,&state->GPR[state->Opcode.u.b.rt].UB[0])) {
		//if (ShowTLBMisses) {
            // Too spammy
			//DisplayError(state, "LBU TLB: %X",Address);
		//}
		TLB_READ_EXCEPTION(Address);
	} else {
		state->GPR[state->Opcode.u.b.rt].UDW = state->GPR[state->Opcode.u.b.rt].UB[0];
	}
}

void r4300i_LHU (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	if ((Address & 1) != 0) { ADDRESS_ERROR_EXCEPTION(Address,1); }
	if (!r4300i_LH_VAddr(state,Address,&state->GPR[state->Opcode.u.b.rt].UHW[0])) {
		//if (ShowTLBMisses) {
            // Too spammy
			//DisplayError(state, "LHU TLB: %X",Address);
		//}
		TLB_READ_EXCEPTION(Address);
	} else {
		state->GPR[state->Opcode.u.b.rt].UDW = state->GPR[state->Opcode.u.b.rt].UHW[0];
	}
}

uint32_t LWR_MASK[4] = { 0xFFFFFF00, 0xFFFF0000, 0xFF000000, 0 };
int32_t LWR_SHIFT[4] = { 24, 16 ,8, 0 };

void r4300i_LWR (usf_state_t * state) {
	uint32_t Offset, Address, Value;

	Address = state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	Offset  = Address & 3;

	if (!r4300i_LW_VAddr(state,(Address & ~3),&Value)) {
		return;
	}

	state->GPR[state->Opcode.u.b.rt].DW = (int32_t)(state->GPR[state->Opcode.u.b.rt].W[0] & LWR_MASK[Offset]);
	state->GPR[state->Opcode.u.b.rt].DW += (int32_t)(Value >> LWR_SHIFT[Offset]);
}

void r4300i_LWU (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,1); }
	if (state->Opcode.u.b.rt == 0) { return; }

	if (!r4300i_LW_VAddr(state,Address,&state->GPR[state->Opcode.u.b.rt].UW[0])) {
		//if (ShowTLBMisses) {
            // Too spammy
			//DisplayError(state, "LWU TLB: %X",Address);
		//}
		TLB_READ_EXCEPTION(Address);
	} else {
		state->GPR[state->Opcode.u.b.rt].UDW = state->GPR[state->Opcode.u.b.rt].UW[0];
	}
}

void r4300i_SB (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	if (!r4300i_SB_VAddr(state,Address,state->GPR[state->Opcode.u.b.rt].UB[0])) {
	}
}

void r4300i_SH (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	if ((Address & 1) != 0) { ADDRESS_ERROR_EXCEPTION(Address,0); }
	if (!r4300i_SH_VAddr(state,Address,state->GPR[state->Opcode.u.b.rt].UHW[0])) {
	}
}

uint32_t SWL_MASK[4] = { 0,0xFF000000,0xFFFF0000,0xFFFFFF00 };
int32_t SWL_SHIFT[4] = { 0, 8, 16, 24 };

void r4300i_SWL (usf_state_t * state) {
	uint32_t Offset, Address, Value;

	Address = state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	Offset  = Address & 3;

	if (!r4300i_LW_VAddr(state,(Address & ~3),&Value)) {
		return;
	}

	Value &= SWL_MASK[Offset];
	Value += state->GPR[state->Opcode.u.b.rt].UW[0] >> SWL_SHIFT[Offset];

	if (!r4300i_SW_VAddr(state,(Address & ~0x03),Value)) {
	}
}


void r4300i_SW (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,0); }
	if (!r4300i_SW_VAddr(state,Address,state->GPR[state->Opcode.u.b.rt].UW[0])) {
	}
	//TranslateVaddr(&Address);
	//if (Address == 0x00090AA0) {
	//	LogMessage("%X: Write %X to %X",state->PROGRAM_COUNTER,state->GPR[state->Opcode.u.b.rt].UW[0],state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset);
	//}
}

uint64_t SDL_MASK[8] = { 0,0xFF00000000000000ULL,
						0xFFFF000000000000ULL,
						0xFFFFFF0000000000ULL,
						0xFFFFFFFF00000000ULL,
					    0xFFFFFFFFFF000000ULL,
						0xFFFFFFFFFFFF0000ULL,
						0xFFFFFFFFFFFFFF00ULL
					};
int32_t SDL_SHIFT[8] = { 0, 8, 16, 24, 32, 40, 48, 56 };

void r4300i_SDL (usf_state_t * state) {
	uint32_t Offset, Address;
	uint64_t Value;

	Address = state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	Offset  = Address & 7;

	if (!r4300i_LD_VAddr(state,(Address & ~7),&Value)) {
		return;
	}

	Value &= SDL_MASK[Offset];
	Value += state->GPR[state->Opcode.u.b.rt].UDW >> SDL_SHIFT[Offset];

	if (!r4300i_SD_VAddr(state,(Address & ~7),Value)) {
	}
}

uint64_t SDR_MASK[8] = { 0x00FFFFFFFFFFFFFFULL,
					  0x0000FFFFFFFFFFFFULL,
					  0x000000FFFFFFFFFFULL,
					  0x00000000FFFFFFFFULL,
					  0x0000000000FFFFFFULL,
					  0x000000000000FFFFULL,
					  0x00000000000000FFULL,
					  0x0000000000000000ULL
					};
int32_t SDR_SHIFT[8] = { 56,48,40,32,24,16,8,0 };

void r4300i_SDR (usf_state_t * state) {
	uint32_t Offset, Address;
	uint64_t Value;

	Address = state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	Offset  = Address & 7;

	if (!r4300i_LD_VAddr(state,(Address & ~7),&Value)) {
		return;
	}

	Value &= SDR_MASK[Offset];
	Value += state->GPR[state->Opcode.u.b.rt].UDW << SDR_SHIFT[Offset];

	if (!r4300i_SD_VAddr(state,(Address & ~7),Value)) {
	}
}

uint32_t SWR_MASK[4] = { 0x00FFFFFF,0x0000FFFF,0x000000FF,0x00000000 };
int32_t SWR_SHIFT[4] = { 24, 16 , 8, 0  };

void r4300i_SWR (usf_state_t * state) {
	uint32_t Offset, Address, Value;

	Address = state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	Offset  = Address & 3;

	if (!r4300i_LW_VAddr(state,(Address & ~3),&Value)) {
		return;
	}

	Value &= SWR_MASK[Offset];
	Value += state->GPR[state->Opcode.u.b.rt].UW[0] << SWR_SHIFT[Offset];

	if (!r4300i_SW_VAddr(state,(Address & ~0x03),Value)) {
	}
}

void r4300i_CACHE (usf_state_t * state) {
    (void)state;
}

void r4300i_LL (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	uintptr_t ll = 0;
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,1); }

	if (state->Opcode.u.b.rt == 0) { return; }

	if (!r4300i_LW_VAddr(state,Address,&state->GPR[state->Opcode.u.b.rt].UW[0])) {
		//if (ShowTLBMisses) {
            // Too spammy
			//DisplayError(state, "LW TLB: %X",Address);
		//}
		TLB_READ_EXCEPTION(Address);
	} else {
		state->GPR[state->Opcode.u.b.rt].DW = state->GPR[state->Opcode.u.b.rt].W[0];
	}
	state->LLBit = 1;
	state->LLAddr = Address;
	ll = state->LLAddr;
	TranslateVaddr(state, &ll);
	state->LLAddr = (uint32_t) ll;
}

void r4300i_LWC1 (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (uint32_t)((int16_t)state->Opcode.u.b.offset);
	TEST_COP1_USABLE_EXCEPTION
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,1); }
	if (!r4300i_LW_VAddr(state,Address,&*(uint32_t *)state->FPRFloatLocation[state->Opcode.u.f.ft])) {
		//if (ShowTLBMisses) {
            // Too spammy
			//DisplayError(state, "LWC1 TLB: %X",Address);
		//}
		TLB_READ_EXCEPTION(Address);
	}
}

void r4300i_SC (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,0); }
	if (state->LLBit == 1) {
		if (!r4300i_SW_VAddr(state,Address,state->GPR[state->Opcode.u.b.rt].UW[0])) {
			DisplayError(state, "SW TLB: %X",Address);
		}
	}
	state->GPR[state->Opcode.u.b.rt].UW[0] = state->LLBit;
}

void r4300i_LD (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	if ((Address & 7) != 0) { ADDRESS_ERROR_EXCEPTION(Address,1); }
	if (!r4300i_LD_VAddr(state,Address,&state->GPR[state->Opcode.u.b.rt].UDW)) {
	}
}


void r4300i_LDC1 (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;

	TEST_COP1_USABLE_EXCEPTION
	if ((Address & 7) != 0) { ADDRESS_ERROR_EXCEPTION(Address,1); }
	if (!r4300i_LD_VAddr(state,Address,&*(uint64_t *)state->FPRDoubleLocation[state->Opcode.u.f.ft])) {
	}
}

void r4300i_SWC1 (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	TEST_COP1_USABLE_EXCEPTION
	if ((Address & 3) != 0) { ADDRESS_ERROR_EXCEPTION(Address,0); }

	if (!r4300i_SW_VAddr(state,Address,*(uint32_t *)state->FPRFloatLocation[state->Opcode.u.f.ft])) {
	}
}

void r4300i_SDC1 (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;

	TEST_COP1_USABLE_EXCEPTION
	if ((Address & 7) != 0) { ADDRESS_ERROR_EXCEPTION(Address,0); }
	if (!r4300i_SD_VAddr(state,Address,*(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.ft])) {
	}
}

void r4300i_SD (usf_state_t * state) {
	uint32_t Address =  state->GPR[state->Opcode.u.c.base].UW[0] + (int16_t)state->Opcode.u.b.offset;
	if ((Address & 7) != 0) { ADDRESS_ERROR_EXCEPTION(Address,0); }
	if (!r4300i_SD_VAddr(state,Address,state->GPR[state->Opcode.u.b.rt].UDW)) {
	}
}
/********************** R4300i state->Opcodes: Special **********************/
void r4300i_SPECIAL_SLL (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = (state->GPR[state->Opcode.u.b.rt].W[0] << state->Opcode.u.e.sa);
}

void r4300i_SPECIAL_SRL (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = (int32_t)(state->GPR[state->Opcode.u.b.rt].UW[0] >> state->Opcode.u.e.sa);
}

void r4300i_SPECIAL_SRA (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = (state->GPR[state->Opcode.u.b.rt].W[0] >> state->Opcode.u.e.sa);
}

void r4300i_SPECIAL_SLLV (usf_state_t * state) {
	if (state->Opcode.u.e.rd == 0) { return; }
	state->GPR[state->Opcode.u.e.rd].DW = (state->GPR[state->Opcode.u.b.rt].W[0] << (state->GPR[state->Opcode.u.b.rs].UW[0] & 0x1F));
}

void r4300i_SPECIAL_SRLV (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = (int32_t)(state->GPR[state->Opcode.u.b.rt].UW[0] >> (state->GPR[state->Opcode.u.b.rs].UW[0] & 0x1F));
}

void r4300i_SPECIAL_SRAV (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = (state->GPR[state->Opcode.u.b.rt].W[0] >> (state->GPR[state->Opcode.u.b.rs].UW[0] & 0x1F));
}

void r4300i_SPECIAL_JR (usf_state_t * state) {
	state->NextInstruction = DELAY_SLOT;
	state->JumpToLocation = state->GPR[state->Opcode.u.b.rs].UW[0];
}

void r4300i_SPECIAL_JALR (usf_state_t * state) {
	state->NextInstruction = DELAY_SLOT;
	state->JumpToLocation = state->GPR[state->Opcode.u.b.rs].UW[0];
	state->GPR[state->Opcode.u.e.rd].DW = (int32_t)(state->PROGRAM_COUNTER + 8);
}

void r4300i_SPECIAL_SYSCALL (usf_state_t * state) {
	DoSysCallException(state, state->NextInstruction == JUMP);
	state->NextInstruction = JUMP;
	state->JumpToLocation = state->PROGRAM_COUNTER;
}

void r4300i_SPECIAL_BREAK (usf_state_t * state) {
	*state->WaitMode=1;
}

void r4300i_SPECIAL_SYNC (usf_state_t * state) {
    (void)state;
}

void r4300i_SPECIAL_MFHI (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->HI.DW;
}

void r4300i_SPECIAL_MTHI (usf_state_t * state) {
	state->HI.DW = state->GPR[state->Opcode.u.b.rs].DW;
}

void r4300i_SPECIAL_MFLO (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->LO.DW;
}

void r4300i_SPECIAL_MTLO (usf_state_t * state) {
	state->LO.DW = state->GPR[state->Opcode.u.b.rs].DW;
}

void r4300i_SPECIAL_DSLLV (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->GPR[state->Opcode.u.b.rt].DW << (state->GPR[state->Opcode.u.b.rs].UW[0] & 0x3F);
}

void r4300i_SPECIAL_DSRLV (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].UDW = state->GPR[state->Opcode.u.b.rt].UDW >> (state->GPR[state->Opcode.u.b.rs].UW[0] & 0x3F);
}

void r4300i_SPECIAL_DSRAV (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->GPR[state->Opcode.u.b.rt].DW >> (state->GPR[state->Opcode.u.b.rs].UW[0] & 0x3F);
}

void r4300i_SPECIAL_MULT (usf_state_t * state) {
	state->HI.DW = (int64_t)(state->GPR[state->Opcode.u.b.rs].W[0]) * (int64_t)(state->GPR[state->Opcode.u.b.rt].W[0]);
	state->LO.DW = state->HI.W[0];
	state->HI.DW = state->HI.W[1];
}

void r4300i_SPECIAL_MULTU (usf_state_t * state) {
	state->HI.DW = (uint64_t)(state->GPR[state->Opcode.u.b.rs].UW[0]) * (uint64_t)(state->GPR[state->Opcode.u.b.rt].UW[0]);
	state->LO.DW = state->HI.W[0];
	state->HI.DW = state->HI.W[1];
}

void r4300i_SPECIAL_DIV (usf_state_t * state) {
	if ( state->GPR[state->Opcode.u.b.rt].UDW != 0 ) {
		state->LO.DW = state->GPR[state->Opcode.u.b.rs].W[0] / state->GPR[state->Opcode.u.b.rt].W[0];
		state->HI.DW = state->GPR[state->Opcode.u.b.rs].W[0] % state->GPR[state->Opcode.u.b.rt].W[0];
	} else {
	}
}

void r4300i_SPECIAL_DIVU (usf_state_t * state) {
	if ( state->GPR[state->Opcode.u.b.rt].UDW != 0 ) {
		state->LO.DW = (int32_t)(state->GPR[state->Opcode.u.b.rs].UW[0] / state->GPR[state->Opcode.u.b.rt].UW[0]);
		state->HI.DW = (int32_t)(state->GPR[state->Opcode.u.b.rs].UW[0] % state->GPR[state->Opcode.u.b.rt].UW[0]);
	} else {
	}
}

void r4300i_SPECIAL_DMULT (usf_state_t * state) {
	MIPS_DWORD Tmp[3];

	state->LO.UDW = (uint64_t)state->GPR[state->Opcode.u.b.rs].UW[0] * (uint64_t)state->GPR[state->Opcode.u.b.rt].UW[0];
	Tmp[0].UDW = (int64_t)state->GPR[state->Opcode.u.b.rs].W[1] * (int64_t)(uint64_t)state->GPR[state->Opcode.u.b.rt].UW[0];
	Tmp[1].UDW = (int64_t)(uint64_t)state->GPR[state->Opcode.u.b.rs].UW[0] * (int64_t)state->GPR[state->Opcode.u.b.rt].W[1];
	state->HI.UDW = (int64_t)state->GPR[state->Opcode.u.b.rs].W[1] * (int64_t)state->GPR[state->Opcode.u.b.rt].W[1];

	Tmp[2].UDW = (uint64_t)state->LO.UW[1] + (uint64_t)Tmp[0].UW[0] + (uint64_t)Tmp[1].UW[0];
	state->LO.UDW += ((uint64_t)Tmp[0].UW[0] + (uint64_t)Tmp[1].UW[0]) << 32;
	state->HI.UDW += (uint64_t)Tmp[0].W[1] + (uint64_t)Tmp[1].W[1] + Tmp[2].UW[1];
}

void r4300i_SPECIAL_DMULTU (usf_state_t * state) {
	MIPS_DWORD Tmp[3];

	state->LO.UDW = (uint64_t)state->GPR[state->Opcode.u.b.rs].UW[0] * (uint64_t)state->GPR[state->Opcode.u.b.rt].UW[0];
	Tmp[0].UDW = (uint64_t)state->GPR[state->Opcode.u.b.rs].UW[1] * (uint64_t)state->GPR[state->Opcode.u.b.rt].UW[0];
	Tmp[1].UDW = (uint64_t)state->GPR[state->Opcode.u.b.rs].UW[0] * (uint64_t)state->GPR[state->Opcode.u.b.rt].UW[1];
	state->HI.UDW = (uint64_t)state->GPR[state->Opcode.u.b.rs].UW[1] * (uint64_t)state->GPR[state->Opcode.u.b.rt].UW[1];

	Tmp[2].UDW = (uint64_t)state->LO.UW[1] + (uint64_t)Tmp[0].UW[0] + (uint64_t)Tmp[1].UW[0];
	state->LO.UDW += ((uint64_t)Tmp[0].UW[0] + (uint64_t)Tmp[1].UW[0]) << 32;
	state->HI.UDW += (uint64_t)Tmp[0].UW[1] + (uint64_t)Tmp[1].UW[1] + Tmp[2].UW[1];
}

void r4300i_SPECIAL_DDIV (usf_state_t * state) {
	if ( state->GPR[state->Opcode.u.b.rt].UDW != 0 ) {
		state->LO.DW = state->GPR[state->Opcode.u.b.rs].DW / state->GPR[state->Opcode.u.b.rt].DW;
		state->HI.DW = state->GPR[state->Opcode.u.b.rs].DW % state->GPR[state->Opcode.u.b.rt].DW;
	} else {
	}
}

void r4300i_SPECIAL_DDIVU (usf_state_t * state) {
	if ( state->GPR[state->Opcode.u.b.rt].UDW != 0 ) {
		state->LO.UDW = state->GPR[state->Opcode.u.b.rs].UDW / state->GPR[state->Opcode.u.b.rt].UDW;
		state->HI.UDW = state->GPR[state->Opcode.u.b.rs].UDW % state->GPR[state->Opcode.u.b.rt].UDW;
	} else {
	}
}

void r4300i_SPECIAL_ADD (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->GPR[state->Opcode.u.b.rs].W[0] + state->GPR[state->Opcode.u.b.rt].W[0];
}

void r4300i_SPECIAL_ADDU (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->GPR[state->Opcode.u.b.rs].W[0] + state->GPR[state->Opcode.u.b.rt].W[0];
}

void r4300i_SPECIAL_SUB (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->GPR[state->Opcode.u.b.rs].W[0] - state->GPR[state->Opcode.u.b.rt].W[0];
}

void r4300i_SPECIAL_SUBU (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->GPR[state->Opcode.u.b.rs].W[0] - state->GPR[state->Opcode.u.b.rt].W[0];
}

void r4300i_SPECIAL_AND (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->GPR[state->Opcode.u.b.rs].DW & state->GPR[state->Opcode.u.b.rt].DW;
}

void r4300i_SPECIAL_OR (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->GPR[state->Opcode.u.b.rs].DW | state->GPR[state->Opcode.u.b.rt].DW;
}

void r4300i_SPECIAL_XOR (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->GPR[state->Opcode.u.b.rs].DW ^ state->GPR[state->Opcode.u.b.rt].DW;
}

void r4300i_SPECIAL_NOR (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = ~(state->GPR[state->Opcode.u.b.rs].DW | state->GPR[state->Opcode.u.b.rt].DW);
}

void r4300i_SPECIAL_SLT (usf_state_t * state) {
	if (state->GPR[state->Opcode.u.b.rs].DW < state->GPR[state->Opcode.u.b.rt].DW) {
		state->GPR[state->Opcode.u.e.rd].DW = 1;
	} else {
		state->GPR[state->Opcode.u.e.rd].DW = 0;
	}
}

void r4300i_SPECIAL_SLTU (usf_state_t * state) {
	if (state->GPR[state->Opcode.u.b.rs].UDW < state->GPR[state->Opcode.u.b.rt].UDW) {
		state->GPR[state->Opcode.u.e.rd].DW = 1;
	} else {
		state->GPR[state->Opcode.u.e.rd].DW = 0;
	}
}

void r4300i_SPECIAL_DADD (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->GPR[state->Opcode.u.b.rs].DW + state->GPR[state->Opcode.u.b.rt].DW;
}

void r4300i_SPECIAL_DADDU (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->GPR[state->Opcode.u.b.rs].DW + state->GPR[state->Opcode.u.b.rt].DW;
}

void r4300i_SPECIAL_DSUB (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->GPR[state->Opcode.u.b.rs].DW - state->GPR[state->Opcode.u.b.rt].DW;
}

void r4300i_SPECIAL_DSUBU (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = state->GPR[state->Opcode.u.b.rs].DW - state->GPR[state->Opcode.u.b.rt].DW;
}

void r4300i_SPECIAL_TEQ (usf_state_t * state) {
	if (state->GPR[state->Opcode.u.b.rs].DW == state->GPR[state->Opcode.u.b.rt].DW) {
	}
}

void r4300i_SPECIAL_DSLL (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = (state->GPR[state->Opcode.u.b.rt].DW << state->Opcode.u.e.sa);
}

void r4300i_SPECIAL_DSRL (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].UDW = (state->GPR[state->Opcode.u.b.rt].UDW >> state->Opcode.u.e.sa);
}

void r4300i_SPECIAL_DSRA (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = (state->GPR[state->Opcode.u.b.rt].DW >> state->Opcode.u.e.sa);
}

void r4300i_SPECIAL_DSLL32 (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = (state->GPR[state->Opcode.u.b.rt].DW << (state->Opcode.u.e.sa + 32));
}

void r4300i_SPECIAL_DSRL32 (usf_state_t * state) {
   state->GPR[state->Opcode.u.e.rd].UDW = (state->GPR[state->Opcode.u.b.rt].UDW >> (state->Opcode.u.e.sa + 32));
}

void r4300i_SPECIAL_DSRA32 (usf_state_t * state) {
	state->GPR[state->Opcode.u.e.rd].DW = (state->GPR[state->Opcode.u.b.rt].DW >> (state->Opcode.u.e.sa + 32));
}

/********************** R4300i state->Opcodes: RegImm **********************/
void r4300i_REGIMM_BLTZ (usf_state_t * state) {
	state->NextInstruction = DELAY_SLOT;
	if (state->GPR[state->Opcode.u.b.rs].DW < 0) {
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,0);
	} else {
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_REGIMM_BGEZ (usf_state_t * state) {
	state->NextInstruction = DELAY_SLOT;
	if (state->GPR[state->Opcode.u.b.rs].DW >= 0) {
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,0);
	} else {
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_REGIMM_BLTZL (usf_state_t * state) {
	if (state->GPR[state->Opcode.u.b.rs].DW < 0) {
		state->NextInstruction = DELAY_SLOT;
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,0);
	} else {
		state->NextInstruction = JUMP;
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_REGIMM_BGEZL (usf_state_t * state) {
	if (state->GPR[state->Opcode.u.b.rs].DW >= 0) {
		state->NextInstruction = DELAY_SLOT;
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,0);
	} else {
		state->NextInstruction = JUMP;
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_REGIMM_BLTZAL (usf_state_t * state) {
	state->NextInstruction = DELAY_SLOT;
	if (state->GPR[state->Opcode.u.b.rs].DW < 0) {
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,0);
	} else {
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
	state->GPR[31].DW= (int32_t)(state->PROGRAM_COUNTER + 8);
}

void r4300i_REGIMM_BGEZAL (usf_state_t * state) {
	state->NextInstruction = DELAY_SLOT;
	if (state->GPR[state->Opcode.u.b.rs].DW >= 0) {
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
		TestInterpreterJump(state,state->PROGRAM_COUNTER,state->JumpToLocation,state->Opcode.u.b.rs,0);
	} else {
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
	state->GPR[31].DW = (int32_t)(state->PROGRAM_COUNTER + 8);
}
/************************** COP0 functions **************************/
void r4300i_COP0_MF (usf_state_t * state) {
	state->GPR[state->Opcode.u.b.rt].DW = (int32_t)state->CP0[state->Opcode.u.e.rd];
}

void r4300i_COP0_MT (usf_state_t * state) {
	switch (state->Opcode.u.e.rd) {
	case 0: //Index
	case 2: //EntryLo0
	case 3: //EntryLo1
	case 5: //PageMask
	case 6: //Wired
	case 10: //Entry Hi
	case 14: //EPC
	case 16: //Config
	case 18: //WatchLo
	case 19: //WatchHi
	case 28: //Tag lo
	case 29: //Tag Hi
	case 30: //ErrEPC
		state->CP0[state->Opcode.u.e.rd] = state->GPR[state->Opcode.u.b.rt].UW[0];
		break;
	case 4: //Context
		state->CP0[state->Opcode.u.e.rd] = state->GPR[state->Opcode.u.b.rt].UW[0] & 0xFF800000;
		break;
	case 9: //Count
		state->CP0[state->Opcode.u.e.rd]= state->GPR[state->Opcode.u.b.rt].UW[0];
		ChangeCompareTimer(state);
		break;
	case 11: //Compare
		state->CP0[state->Opcode.u.e.rd] = state->GPR[state->Opcode.u.b.rt].UW[0];
		FAKE_CAUSE_REGISTER &= ~CAUSE_IP7;
		ChangeCompareTimer(state);
		break;
	case 12: //Status
		if ((state->CP0[state->Opcode.u.e.rd] ^ state->GPR[state->Opcode.u.b.rt].UW[0]) != 0) {
			state->CP0[state->Opcode.u.e.rd] = state->GPR[state->Opcode.u.b.rt].UW[0];
			SetFpuLocations(state);
		} else {
			state->CP0[state->Opcode.u.e.rd] = state->GPR[state->Opcode.u.b.rt].UW[0];
		}
		if ((state->CP0[state->Opcode.u.e.rd] & 0x18) != 0) {
		}
		CheckInterrupts(state);
		break;
	case 13: //cause
		state->CP0[state->Opcode.u.e.rd] &= 0xFFFFCFF;
		break;
	default:
		R4300i_UnknownOpcode(state);
	}

}

/************************** COP0 CO functions ***********************/
void r4300i_COP0_CO_TLBR (usf_state_t * state) {
	TLB_Read(state);
}

void r4300i_COP0_CO_TLBWI (usf_state_t * state) {
	WriteTLBEntry(state, INDEX_REGISTER & 0x1F);
}

void r4300i_COP0_CO_TLBWR (usf_state_t * state) {
	WriteTLBEntry(state, RANDOM_REGISTER & 0x1F);
}

void r4300i_COP0_CO_TLBP (usf_state_t * state) {
	TLB_Probe(state);
}

void r4300i_COP0_CO_ERET (usf_state_t * state) {
	state->NextInstruction = JUMP;
	if ((STATUS_REGISTER & STATUS_ERL) != 0) {
		state->JumpToLocation = ERROREPC_REGISTER;
		STATUS_REGISTER &= ~STATUS_ERL;
	} else {
		state->JumpToLocation = EPC_REGISTER;
		STATUS_REGISTER &= ~STATUS_EXL;
	}
	state->LLBit = 0;
	CheckInterrupts(state);
}

/************************** COP1 functions **************************/
void r4300i_COP1_MF (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	state->GPR[state->Opcode.u.b.rt].DW = *(int32_t *)state->FPRFloatLocation[state->Opcode.u.f.fs];
}

void r4300i_COP1_DMF (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	state->GPR[state->Opcode.u.b.rt].DW = *(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.fs];
}

void r4300i_COP1_CF (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	if (state->Opcode.u.f.fs != 31 && state->Opcode.u.f.fs != 0) {
		return;
	}
	state->GPR[state->Opcode.u.b.rt].DW = (int32_t)state->FPCR[state->Opcode.u.f.fs];
}

void r4300i_COP1_MT (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(int32_t *)state->FPRFloatLocation[state->Opcode.u.f.fs] = state->GPR[state->Opcode.u.b.rt].W[0];
}

void r4300i_COP1_DMT (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.fs] = state->GPR[state->Opcode.u.b.rt].DW;
}

void r4300i_COP1_CT (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	if (state->Opcode.u.f.fs == 31) {
		state->FPCR[state->Opcode.u.f.fs] = state->GPR[state->Opcode.u.b.rt].W[0];
		return;
	}
}

/************************* COP1: BC1 functions ***********************/
void r4300i_COP1_BCF (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	state->NextInstruction = DELAY_SLOT;
	if ((state->FPCR[31] & FPCSR_C) == 0) {
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
	} else {
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_COP1_BCT (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	state->NextInstruction = DELAY_SLOT;
	if ((state->FPCR[31] & FPCSR_C) != 0) {
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
	} else {
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_COP1_BCFL (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	if ((state->FPCR[31] & FPCSR_C) == 0) {
		state->NextInstruction = DELAY_SLOT;
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
	} else {
		state->NextInstruction = JUMP;
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}

void r4300i_COP1_BCTL (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	if ((state->FPCR[31] & FPCSR_C) != 0) {
		state->NextInstruction = DELAY_SLOT;
		state->JumpToLocation = state->PROGRAM_COUNTER + ((int16_t)state->Opcode.u.b.offset << 2) + 4;
	} else {
		state->NextInstruction = JUMP;
		state->JumpToLocation = state->PROGRAM_COUNTER + 8;
	}
}
/************************** COP1: S functions ************************/
static INLINE void Float_RoundToInteger32( int32_t * Dest, float * Source ) {
	*Dest = (int32_t)*Source;
}

static INLINE void Float_RoundToInteger64( int64_t * Dest, float * Source ) {
	*Dest = (int64_t)*Source;
}

void r4300i_COP1_S_ADD (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(float *)state->FPRFloatLocation[state->Opcode.u.f.fd] = (*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs] + *(float *)state->FPRFloatLocation[state->Opcode.u.f.ft]);
}

void r4300i_COP1_S_SUB (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(float *)state->FPRFloatLocation[state->Opcode.u.f.fd] = (*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs] - *(float *)state->FPRFloatLocation[state->Opcode.u.f.ft]);
}

void r4300i_COP1_S_MUL (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(float *)state->FPRFloatLocation[state->Opcode.u.f.fd] = (*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs] * *(float *)state->FPRFloatLocation[state->Opcode.u.f.ft]);
}

void r4300i_COP1_S_DIV (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(float *)state->FPRFloatLocation[state->Opcode.u.f.fd] = (*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs] / *(float *)state->FPRFloatLocation[state->Opcode.u.f.ft]);
}

void r4300i_COP1_S_SQRT (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(float *)state->FPRFloatLocation[state->Opcode.u.f.fd] = (float)sqrt(*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_S_ABS (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(float *)state->FPRFloatLocation[state->Opcode.u.f.fd] = (float)fabs(*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_S_MOV (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(float *)state->FPRFloatLocation[state->Opcode.u.f.fd] = *(float *)state->FPRFloatLocation[state->Opcode.u.f.fs];
}

void r4300i_COP1_S_NEG (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(float *)state->FPRFloatLocation[state->Opcode.u.f.fd] = (*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs] * -1.0f);
}

void r4300i_COP1_S_TRUNC_L (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	//_controlfp(_RC_CHOP,_MCW_RC);
	Float_RoundToInteger64(&*(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.fd],&*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_S_CEIL_L (usf_state_t * state) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	//_controlfp(_RC_UP,_MCW_RC);
	Float_RoundToInteger64(&*(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.fd],&*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_S_FLOOR_L (usf_state_t * state) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	Float_RoundToInteger64(&*(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.fd],&*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_S_ROUND_W (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	Float_RoundToInteger32(&*(int32_t *)state->FPRFloatLocation[state->Opcode.u.f.fd],&*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_S_TRUNC_W (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	//_controlfp(_RC_CHOP,_MCW_RC);
	Float_RoundToInteger32(&*(int32_t *)state->FPRFloatLocation[state->Opcode.u.f.fd],&*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_S_CEIL_W (usf_state_t * state) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	//_controlfp(_RC_UP,_MCW_RC);
	Float_RoundToInteger32(&*(int32_t *)state->FPRFloatLocation[state->Opcode.u.f.fd],&*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_S_FLOOR_W (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	Float_RoundToInteger32(&*(int32_t *)state->FPRFloatLocation[state->Opcode.u.f.fd],&*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_S_CVT_D (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fd] = (double)(*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_S_CVT_W (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	Float_RoundToInteger32(&*(int32_t *)state->FPRFloatLocation[state->Opcode.u.f.fd],&*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_S_CVT_L (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	Float_RoundToInteger64(&*(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.fd],&*(float *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_S_CMP (usf_state_t * state) {
	int32_t less, equal, unorded, condition;
	float Temp0, Temp1;

	TEST_COP1_USABLE_EXCEPTION

	Temp0 = *(float *)state->FPRFloatLocation[state->Opcode.u.f.fs];
	Temp1 = *(float *)state->FPRFloatLocation[state->Opcode.u.f.ft];

	if(0) {
	//if (_isnan(Temp0) || _isnan(Temp1)) {
		less = 0;
		equal = 0;
		unorded = 1;
		if ((state->Opcode.u.e.funct & 8) != 0) {
		}
	} else {
		less = Temp0 < Temp1;
		equal = Temp0 == Temp1;
		unorded = 0;
	}

	condition = ((state->Opcode.u.e.funct & 4) && less) | ((state->Opcode.u.e.funct & 2) && equal) |
		((state->Opcode.u.e.funct & 1) && unorded);

	if (condition) {
		state->FPCR[31] |= FPCSR_C;
	} else {
		state->FPCR[31] &= ~FPCSR_C;
	}

}

/************************** COP1: D functions ************************/
static INLINE void Double_RoundToInteger32( int32_t * Dest, double * Source ) {
	*Dest = (int32_t)*Source;
}

static INLINE void Double_RoundToInteger64( int64_t * Dest, double * Source ) {
	*Dest = (int64_t)*Source;
}

void r4300i_COP1_D_ADD (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fd] = *(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs] + *(double *)state->FPRDoubleLocation[state->Opcode.u.f.ft];
}

void r4300i_COP1_D_SUB (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fd] = *(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs] - *(double *)state->FPRDoubleLocation[state->Opcode.u.f.ft];
}

void r4300i_COP1_D_MUL (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fd] = *(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs] * *(double *)state->FPRDoubleLocation[state->Opcode.u.f.ft];
}

void r4300i_COP1_D_DIV (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fd] = *(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs] / *(double *)state->FPRDoubleLocation[state->Opcode.u.f.ft];
}

void r4300i_COP1_D_SQRT (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fd] = (double)sqrt(*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_D_ABS (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fd] = fabs(*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_D_MOV (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.fd] = *(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.fs];
}

void r4300i_COP1_D_NEG (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fd] = (*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs] * -1.0);
}

void r4300i_COP1_D_TRUNC_L (usf_state_t * state) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	Double_RoundToInteger64(&*(int64_t *)state->FPRFloatLocation[state->Opcode.u.f.fd],&*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs] );
}

void r4300i_COP1_D_CEIL_L (usf_state_t * state) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	Double_RoundToInteger64(&*(int64_t *)state->FPRFloatLocation[state->Opcode.u.f.fd],&*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs] );
}

void r4300i_COP1_D_FLOOR_L (usf_state_t * state) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	Double_RoundToInteger64(&*(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.fd],&*(double *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_D_ROUND_W (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	Double_RoundToInteger32(&*(int32_t *)state->FPRFloatLocation[state->Opcode.u.f.fd],&*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs] );
}

void r4300i_COP1_D_TRUNC_W (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	Double_RoundToInteger32(&*(int32_t *)state->FPRFloatLocation[state->Opcode.u.f.fd],&*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs] );
}

void r4300i_COP1_D_CEIL_W (usf_state_t * state) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	Double_RoundToInteger32(&*(int32_t *)state->FPRFloatLocation[state->Opcode.u.f.fd],&*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs] );
}

void r4300i_COP1_D_FLOOR_W (usf_state_t * state) {	//added by Witten
	TEST_COP1_USABLE_EXCEPTION
	Double_RoundToInteger32(&*(int32_t *)state->FPRDoubleLocation[state->Opcode.u.f.fd],&*(double *)state->FPRFloatLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_D_CVT_S (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(float *)state->FPRFloatLocation[state->Opcode.u.f.fd] = (float)*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs];
}

void r4300i_COP1_D_CVT_W (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	Double_RoundToInteger32(&*(int32_t *)state->FPRFloatLocation[state->Opcode.u.f.fd],&*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs] );
}

void r4300i_COP1_D_CVT_L (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	Double_RoundToInteger64(&*(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.fd],&*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fs]);
}

void r4300i_COP1_D_CMP (usf_state_t * state) {
	int32_t less, equal, unorded, condition;
	MIPS_DWORD Temp0, Temp1;

	TEST_COP1_USABLE_EXCEPTION

	Temp0.DW = *(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.fs];
	Temp1.DW = *(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.ft];

	if(0) {
	//if (_isnan(Temp0.D) || _isnan(Temp1.D)) {
		less = 0;
		equal = 0;
		unorded = 1;
		if ((state->Opcode.u.e.funct & 8) != 0) {
		}
	} else {
		less = Temp0.D < Temp1.D;
		equal = Temp0.D == Temp1.D;
		unorded = 0;
	}

	condition = ((state->Opcode.u.e.funct & 4) && less) | ((state->Opcode.u.e.funct & 2) && equal) |
		((state->Opcode.u.e.funct & 1) && unorded);

	if (condition) {
		state->FPCR[31] |= FPCSR_C;
	} else {
		state->FPCR[31] &= ~FPCSR_C;
	}
}

/************************** COP1: W functions ************************/
void r4300i_COP1_W_CVT_S (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(float *)state->FPRFloatLocation[state->Opcode.u.f.fd] = (float)*(int32_t *)state->FPRFloatLocation[state->Opcode.u.f.fs];
}

void r4300i_COP1_W_CVT_D (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fd] = (double)*(int32_t *)state->FPRFloatLocation[state->Opcode.u.f.fs];
}

/************************** COP1: L functions ************************/
void r4300i_COP1_L_CVT_S (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(float *)state->FPRFloatLocation[state->Opcode.u.f.fd] = (float)*(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.fs];
}

void r4300i_COP1_L_CVT_D (usf_state_t * state) {
	TEST_COP1_USABLE_EXCEPTION
	*(double *)state->FPRDoubleLocation[state->Opcode.u.f.fd] = (double)*(int64_t *)state->FPRDoubleLocation[state->Opcode.u.f.fs];
}

/************************** Other functions **************************/
void R4300i_UnknownOpcode (usf_state_t * state) {
	DisplayError(state, "Unknown R4300i Opcode.\tPC:%08x\tOp:%08x\n", state->PROGRAM_COUNTER,state->Opcode.u.Hex);
	StopEmulation(state);
}
