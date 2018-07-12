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

#include <string.h>

#include "main.h"
#include "cpu.h"

#include "usf.h"
#include "usf_internal.h"

void PI_DMA_READ (usf_state_t * state) {

	PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
	MI_INTR_REG |= MI_INTR_PI;
	CheckInterrupts(state);
	return;
}

void PI_DMA_WRITE (usf_state_t * state) {
	uint32_t i;
	PI_STATUS_REG |= PI_STATUS_DMA_BUSY;
	if ( PI_DRAM_ADDR_REG + PI_WR_LEN_REG + 1 > state->RdramSize) {
		PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		MI_INTR_REG |= MI_INTR_PI;
		CheckInterrupts(state);
		return;
	}

	if ( PI_CART_ADDR_REG >= 0x08000000 && PI_CART_ADDR_REG <= 0x08010000) {
		return;
	}

	if ( PI_CART_ADDR_REG >= 0x10000000 && PI_CART_ADDR_REG <= 0x1FBFFFFF) {
		PI_CART_ADDR_REG -= 0x10000000;
		for (i = 0; i < PI_WR_LEN_REG + 1; i ++) {
			*(state->N64MEM+((PI_DRAM_ADDR_REG + i) ^ 3)) =  *PageROM(state, (PI_CART_ADDR_REG + i) ^ 3);
		}
		PI_CART_ADDR_REG += 0x10000000;

		PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
		MI_INTR_REG |= MI_INTR_PI;
		CheckInterrupts(state);
		CheckTimer(state);
		return;
	}

	PI_STATUS_REG &= ~PI_STATUS_DMA_BUSY;
	MI_INTR_REG |= MI_INTR_PI;
	CheckInterrupts(state);
}


void SI_DMA_READ (usf_state_t * state) {

	if ((int32_t)SI_DRAM_ADDR_REG > (int32_t)state->RdramSize) {
		return;
	}

	PifRamRead(state);
	SI_DRAM_ADDR_REG &= 0xFFFFFFF8;
	if ((int32_t)SI_DRAM_ADDR_REG < 0) {
		int32_t count, RdramPos;

		RdramPos = (int32_t)SI_DRAM_ADDR_REG;
		for (count = 0; count < 0x40; count++, RdramPos++) {
			if (RdramPos < 0) { continue; }
			state->N64MEM[RdramPos ^3] = state->PIF_Ram[count];
		}
	} else {
		int32_t count, RdramPos;

		RdramPos = (uint32_t)SI_DRAM_ADDR_REG;
		for (count = 0; count < 0x40; count++, RdramPos++) {
			if (RdramPos < 0) { continue; }
			state->N64MEM[RdramPos ^3] = state->PIF_Ram[count];
		}
	}

	MI_INTR_REG |= MI_INTR_SI;
	SI_STATUS_REG |= SI_STATUS_INTERRUPT;
	CheckInterrupts(state);
}


void SI_DMA_WRITE (usf_state_t * state) {

	if ((int32_t)SI_DRAM_ADDR_REG > (int32_t)state->RdramSize) {
		return;
	}

	SI_DRAM_ADDR_REG &= 0xFFFFFFF8;
	if ((int32_t)SI_DRAM_ADDR_REG < 0) {
		int32_t count, RdramPos;

		RdramPos = (int32_t)SI_DRAM_ADDR_REG;
		for (count = 0; count < 0x40; count++, RdramPos++) {
			if (RdramPos < 0) { state->PIF_Ram[count] = 0; continue; }
			state->PIF_Ram[count] = state->N64MEM[RdramPos ^3];
		}
	} else {
		int32_t count, RdramPos;

		RdramPos = (int32_t)SI_DRAM_ADDR_REG;
		for (count = 0; count < 0x40; count++, RdramPos++) {
			if (RdramPos < 0) { state->PIF_Ram[count] = 0; continue; }
			state->PIF_Ram[count] = state->N64MEM[RdramPos ^3];
		}
	}

	PifRamWrite(state);

	MI_INTR_REG |= MI_INTR_SI;
	SI_STATUS_REG |= SI_STATUS_INTERRUPT;
	CheckInterrupts(state);

}

void SP_DMA_READ (usf_state_t * state) {
	SP_DRAM_ADDR_REG &= 0x1FFFFFFF;

	if (SP_DRAM_ADDR_REG > state->RdramSize) {
		SP_DMA_BUSY_REG = 0;
		SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
		return;
	}

	if (SP_RD_LEN_REG + 1  + (SP_MEM_ADDR_REG & 0xFFF) > 0x1000) {
		return;
	}

	memcpy( state->DMEM + (SP_MEM_ADDR_REG & 0x1FFF), state->N64MEM + SP_DRAM_ADDR_REG,
		SP_RD_LEN_REG + 1 );

	SP_DMA_BUSY_REG = 0;
	SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
	MI_INTR_REG &= ~MI_INTR_SP;
	CheckInterrupts(state);
	CheckTimer(state);
}

void SP_DMA_WRITE (usf_state_t * state) {
	if (SP_DRAM_ADDR_REG > state->RdramSize) {
		return;
	}

	if (SP_WR_LEN_REG + 1 + (SP_MEM_ADDR_REG & 0xFFF) > 0x1000) {
		return;
	}

	memcpy( state->N64MEM + SP_DRAM_ADDR_REG, state->DMEM + (SP_MEM_ADDR_REG & 0x1FFF),
		SP_WR_LEN_REG + 1);

	SP_DMA_BUSY_REG = 0;
	SP_STATUS_REG  &= ~SP_STATUS_DMA_BUSY;
}

