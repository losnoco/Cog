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

// #ifdef EXT_REGS

#include "usf.h"

#include "main.h"
#include "cpu.h"
#include "types.h"

#include "usf_internal.h"

void SetupRegisters(usf_state_t * state, N64_REGISTERS * n64_Registers) {
	state->PROGRAM_COUNTER = n64_Registers->PROGRAM_COUNTER;
	state->HI.DW    = n64_Registers->HI.DW;
	state->LO.DW    = n64_Registers->LO.DW;
	state->CP0      = n64_Registers->CP0;
	state->GPR      = n64_Registers->GPR;
	state->FPR      = n64_Registers->FPR;
	state->FPCR     = n64_Registers->FPCR;
	state->RegRDRAM = n64_Registers->RDRAM;
	state->RegSP    = n64_Registers->SP;
	state->RegDPC   = n64_Registers->DPC;
	state->RegMI    = n64_Registers->MI;
	state->RegVI    = n64_Registers->VI;
	state->RegAI    = n64_Registers->AI;
	state->RegPI    = n64_Registers->PI;
	state->RegRI    = n64_Registers->RI;
	state->RegSI    = n64_Registers->SI;
	state->PIF_Ram  = (uint8_t *) n64_Registers->PIF_Ram;
}

void ChangeMiIntrMask (usf_state_t * state) {
	if ( ( state->RegModValue & MI_INTR_MASK_CLR_SP ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_SP; }
	if ( ( state->RegModValue & MI_INTR_MASK_SET_SP ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_SP; }
	if ( ( state->RegModValue & MI_INTR_MASK_CLR_SI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_SI; }
	if ( ( state->RegModValue & MI_INTR_MASK_SET_SI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_SI; }
	if ( ( state->RegModValue & MI_INTR_MASK_CLR_AI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_AI; }
	if ( ( state->RegModValue & MI_INTR_MASK_SET_AI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_AI; }
	if ( ( state->RegModValue & MI_INTR_MASK_CLR_VI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_VI; }
	if ( ( state->RegModValue & MI_INTR_MASK_SET_VI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_VI; }
	if ( ( state->RegModValue & MI_INTR_MASK_CLR_PI ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_PI; }
	if ( ( state->RegModValue & MI_INTR_MASK_SET_PI ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_PI; }
	if ( ( state->RegModValue & MI_INTR_MASK_CLR_DP ) != 0 ) { MI_INTR_MASK_REG &= ~MI_INTR_MASK_DP; }
	if ( ( state->RegModValue & MI_INTR_MASK_SET_DP ) != 0 ) { MI_INTR_MASK_REG |= MI_INTR_MASK_DP; }
}

void ChangeMiModeReg (usf_state_t * state) {
	MI_MODE_REG &= ~0x7F;
	MI_MODE_REG |= (state->RegModValue & 0x7F);
	if ( ( state->RegModValue & MI_CLR_INIT ) != 0 ) { MI_MODE_REG &= ~MI_MODE_INIT; }
	if ( ( state->RegModValue & MI_SET_INIT ) != 0 ) { MI_MODE_REG |= MI_MODE_INIT; }
	if ( ( state->RegModValue & MI_CLR_EBUS ) != 0 ) { MI_MODE_REG &= ~MI_MODE_EBUS; }
	if ( ( state->RegModValue & MI_SET_EBUS ) != 0 ) { MI_MODE_REG |= MI_MODE_EBUS; }
	if ( ( state->RegModValue & MI_CLR_DP_INTR ) != 0 ) { MI_INTR_REG &= ~MI_INTR_DP; }
	if ( ( state->RegModValue & MI_CLR_RDRAM ) != 0 ) { MI_MODE_REG &= ~MI_MODE_RDRAM; }
	if ( ( state->RegModValue & MI_SET_RDRAM ) != 0 ) { MI_MODE_REG |= MI_MODE_RDRAM; }
}

void ChangeSpStatus (usf_state_t * state) {
	if ( ( state->RegModValue & SP_CLR_HALT ) != 0) { SP_STATUS_REG &= ~SP_STATUS_HALT; }
	if ( ( state->RegModValue & SP_SET_HALT ) != 0) { SP_STATUS_REG |= SP_STATUS_HALT;  }
	if ( ( state->RegModValue & SP_CLR_BROKE ) != 0) { SP_STATUS_REG &= ~SP_STATUS_BROKE; }
	if ( ( state->RegModValue & SP_CLR_INTR ) != 0) {
		MI_INTR_REG &= ~MI_INTR_SP;
		CheckInterrupts(state);
	}

	if ( ( state->RegModValue & SP_CLR_SSTEP ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SSTEP; }
	if ( ( state->RegModValue & SP_SET_SSTEP ) != 0) { SP_STATUS_REG |= SP_STATUS_SSTEP;  }
	if ( ( state->RegModValue & SP_CLR_INTR_BREAK ) != 0) { SP_STATUS_REG &= ~SP_STATUS_INTR_BREAK; }
	if ( ( state->RegModValue & SP_SET_INTR_BREAK ) != 0) { SP_STATUS_REG |= SP_STATUS_INTR_BREAK;  }
	if ( ( state->RegModValue & SP_CLR_SIG0 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG0; }
	if ( ( state->RegModValue & SP_SET_SIG0 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG0;  }
	if ( ( state->RegModValue & SP_CLR_SIG1 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG1; }
	if ( ( state->RegModValue & SP_SET_SIG1 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG1;  }
	if ( ( state->RegModValue & SP_CLR_SIG2 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG2; }
	if ( ( state->RegModValue & SP_SET_SIG2 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG2;  }
	if ( ( state->RegModValue & SP_CLR_SIG3 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG3; }
	if ( ( state->RegModValue & SP_SET_SIG3 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG3;  }
	if ( ( state->RegModValue & SP_CLR_SIG4 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG4; }
	if ( ( state->RegModValue & SP_SET_SIG4 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG4;  }
	if ( ( state->RegModValue & SP_CLR_SIG5 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG5; }
	if ( ( state->RegModValue & SP_SET_SIG5 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG5;  }
	if ( ( state->RegModValue & SP_CLR_SIG6 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG6; }
	if ( ( state->RegModValue & SP_SET_SIG6 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG6;  }
	if ( ( state->RegModValue & SP_CLR_SIG7 ) != 0) { SP_STATUS_REG &= ~SP_STATUS_SIG7; }
	if ( ( state->RegModValue & SP_SET_SIG7 ) != 0) { SP_STATUS_REG |= SP_STATUS_SIG7;  }

	RunRsp(state);

}

void UpdateCurrentHalfLine (usf_state_t * state) {
	if (state->Timers->Timer < 0) {
		state->HalfLine = 0;
		return;
	}
	state->HalfLine = (state->Timers->Timer / 1500);
	state->HalfLine &= ~1;
	state->HalfLine += state->ViFieldNumber;
}

void SetFpuLocations (usf_state_t * state) {
    int count;
    
    if ((STATUS_REGISTER & STATUS_FR) == 0) {
        for (count = 0; count < 32; count ++) {
            state->FPRFloatLocation[count] = (void *)(&state->FPR[count >> 1].W[count & 1]);
            //state->FPRDoubleLocation[count] = state->FPRFloatLocation[count];
            state->FPRDoubleLocation[count] = (void *)(&state->FPR[count >> 1].DW);
        }
    } else {
        for (count = 0; count < 32; count ++) {
            state->FPRFloatLocation[count] = (void *)(&state->FPR[count].W[1]);
            //state->FPRFloatLocation[count] = (void *)(&state->FPR[count].W[1]);
            //state->FPRDoubleLocation[count] = state->FPRFloatLocation[count];
            state->FPRDoubleLocation[count] = (void *)(&state->FPR[count].DW);
        }
    }
}
