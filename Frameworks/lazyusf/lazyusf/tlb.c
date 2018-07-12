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

#include "usf_internal.h"

void SetupTLB_Entry (usf_state_t * state, int32_t Entry);

uint32_t AddressDefined ( usf_state_t * state, uintptr_t VAddr) {
	uint32_t i;

	if (VAddr >= 0x80000000 && VAddr <= 0xBFFFFFFF) {
		return 1;
	}

	for (i = 0; i < 64; i++) {
		if (state->FastTlb[i].ValidEntry == 0) { continue; }
		if (VAddr >= state->FastTlb[i].VSTART && VAddr <= state->FastTlb[i].VEND) {
			return 1;
		}
	}
	return 0;
}

void InitilizeTLB (usf_state_t * state) {
	uint32_t count;

	for (count = 0; count < 32; count++) { state->tlb[count].EntryDefined = 0; }
	for (count = 0; count < 64; count++) { state->FastTlb[count].ValidEntry = 0; }
	SetupTLB(state);
}

void SetupTLB (usf_state_t * state) {
	uint32_t count;

	memset(state->TLB_Map,0,(0xFFFFF * sizeof(uintptr_t)));
	for (count = 0x80000000; count < 0xC0000000; count += 0x1000) {
		state->TLB_Map[count >> 12] = ((uintptr_t)state->N64MEM + (count & 0x1FFFFFFF)) - count;
	}
	for (count = 0; count < 32; count ++) { SetupTLB_Entry(state, count); }
}
/*
test=(BYTE *) VirtualAlloc( 0x10, 0x70000, MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if(test == 0) {
		//printf("FAIL!\n");
		exit(0);
	}
*/

void SetupTLB_Entry (usf_state_t * state, int Entry) {
	int32_t FastIndx;


	if (!state->tlb[Entry].EntryDefined) { return; }
	FastIndx = Entry << 1;
	state->FastTlb[FastIndx].VSTART=state->tlb[Entry].EntryHi.b.VPN2 << 13;
	state->FastTlb[FastIndx].VEND = state->FastTlb[FastIndx].VSTART + (state->tlb[Entry].PageMask.b.Mask << 12) + 0xFFF;
	state->FastTlb[FastIndx].PHYSSTART = state->tlb[Entry].EntryLo0.b.PFN << 12;
	state->FastTlb[FastIndx].VALID = state->tlb[Entry].EntryLo0.b.V;
	state->FastTlb[FastIndx].DIRTY = state->tlb[Entry].EntryLo0.b.D;
	state->FastTlb[FastIndx].GLOBAL = state->tlb[Entry].EntryLo0.b.GLOBAL & state->tlb[Entry].EntryLo1.b.GLOBAL;
	state->FastTlb[FastIndx].ValidEntry = 0;

	FastIndx = (Entry << 1) + 1;
	state->FastTlb[FastIndx].VSTART=(state->tlb[Entry].EntryHi.b.VPN2 << 13) + ((state->tlb[Entry].PageMask.b.Mask << 12) + 0xFFF + 1);
	state->FastTlb[FastIndx].VEND = state->FastTlb[FastIndx].VSTART + (state->tlb[Entry].PageMask.b.Mask << 12) + 0xFFF;
	state->FastTlb[FastIndx].PHYSSTART = state->tlb[Entry].EntryLo1.b.PFN << 12;
	state->FastTlb[FastIndx].VALID = state->tlb[Entry].EntryLo1.b.V;
	state->FastTlb[FastIndx].DIRTY = state->tlb[Entry].EntryLo1.b.D;
	state->FastTlb[FastIndx].GLOBAL = state->tlb[Entry].EntryLo0.b.GLOBAL & state->tlb[Entry].EntryLo1.b.GLOBAL;
	state->FastTlb[FastIndx].ValidEntry = 0;

	for ( FastIndx = Entry << 1; FastIndx <= (Entry << 1) + 1; FastIndx++) {
		uint32_t count;

		if (!state->FastTlb[FastIndx].VALID) {
			state->FastTlb[FastIndx].ValidEntry = 1;
			continue;
		}
		if (state->FastTlb[FastIndx].VEND <= state->FastTlb[FastIndx].VSTART) {
			continue;
		}
		if (state->FastTlb[FastIndx].VSTART >= 0x80000000 && state->FastTlb[FastIndx].VEND <= 0xBFFFFFFF) {
			continue;
		}
		if (state->FastTlb[FastIndx].PHYSSTART > 0x1FFFFFFF) {
			continue;
		}

		//test if overlap
		state->FastTlb[FastIndx].ValidEntry = 1;
		for (count = state->FastTlb[FastIndx].VSTART; count < state->FastTlb[FastIndx].VEND; count += 0x1000) {
			state->TLB_Map[count >> 12] = ((uintptr_t)state->N64MEM + (count - state->FastTlb[FastIndx].VSTART + state->FastTlb[FastIndx].PHYSSTART)) - count;
		}
	}
}

void TLB_Probe (usf_state_t * state) {
	uint32_t Counter;


	INDEX_REGISTER |= 0x80000000;
	for (Counter = 0; Counter < 32; Counter ++) {
		uint32_t TlbValue = state->tlb[Counter].EntryHi.Value & (~state->tlb[Counter].PageMask.b.Mask << 13);
		uint32_t EntryHi = ENTRYHI_REGISTER & (~state->tlb[Counter].PageMask.b.Mask << 13);

		if (TlbValue == EntryHi) {
			uint32_t Global = (state->tlb[Counter].EntryHi.Value & 0x100) != 0;
			uint32_t SameAsid = ((state->tlb[Counter].EntryHi.Value & 0xFF) == (ENTRYHI_REGISTER & 0xFF));

			if (Global || SameAsid) {
				INDEX_REGISTER = Counter;
				return;
			}
		}
	}
}

void TLB_Read (usf_state_t * state) {
	uint32_t index = INDEX_REGISTER & 0x1F;

	PAGE_MASK_REGISTER = state->tlb[index].PageMask.Value ;
	ENTRYHI_REGISTER = (state->tlb[index].EntryHi.Value & ~state->tlb[index].PageMask.Value) ;
	ENTRYLO0_REGISTER = state->tlb[index].EntryLo0.Value;
	ENTRYLO1_REGISTER = state->tlb[index].EntryLo1.Value;
}

uint32_t TranslateVaddr ( usf_state_t * state, uintptr_t * Addr) {
	if (state->TLB_Map[((*Addr) & 0xffffffff) >> 12] == 0) { return 0; }
	*Addr = (uintptr_t)((uint8_t *)(state->TLB_Map[((*Addr) & 0xffffffff) >> 12] + ((*Addr) & 0xffffffff)) - (uintptr_t)state->N64MEM);
	return 1;
}

void WriteTLBEntry (usf_state_t * state, int32_t index) {
	int32_t FastIndx;

	FastIndx = index << 1;
	if ((state->PROGRAM_COUNTER >= state->FastTlb[FastIndx].VSTART &&
		state->PROGRAM_COUNTER < state->FastTlb[FastIndx].VEND &&
		state->FastTlb[FastIndx].ValidEntry && state->FastTlb[FastIndx].VALID)
		||
		(state->PROGRAM_COUNTER >= state->FastTlb[FastIndx + 1].VSTART &&
		state->PROGRAM_COUNTER < state->FastTlb[FastIndx + 1].VEND &&
		state->FastTlb[FastIndx + 1].ValidEntry && state->FastTlb[FastIndx + 1].VALID))
	{
		return;
	}

	if (state->tlb[index].EntryDefined) {
		uint32_t count;

		for ( FastIndx = index << 1; FastIndx <= (index << 1) + 1; FastIndx++) {
			if (!state->FastTlb[FastIndx].ValidEntry) { continue; }
			if (!state->FastTlb[FastIndx].VALID) { continue; }
			for (count = state->FastTlb[FastIndx].VSTART; count < state->FastTlb[FastIndx].VEND; count += 0x1000) {
				state->TLB_Map[count >> 12] = 0;
			}
		}
	}
	state->tlb[index].PageMask.Value = PAGE_MASK_REGISTER;
	state->tlb[index].EntryHi.Value = ENTRYHI_REGISTER;
	state->tlb[index].EntryLo0.Value = ENTRYLO0_REGISTER;
	state->tlb[index].EntryLo1.Value = ENTRYLO1_REGISTER;
	state->tlb[index].EntryDefined = 1;


	SetupTLB_Entry(state, index);
}
