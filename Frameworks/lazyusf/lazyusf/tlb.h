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
#ifndef _TLB_H_
#define _TLB_H_

typedef struct {
	uint32_t EntryDefined;
	union {
		uint32_t Value;
		uint8_t A[4];

		struct {
			unsigned zero : 13;
			unsigned Mask : 12;
			unsigned zero2 : 7;
		} b;

	} PageMask;

	union {
		uint32_t Value;
		uint8_t A[4];

		struct {
			unsigned ASID : 8;
			unsigned Zero : 4;
			unsigned G : 1;
			unsigned VPN2 : 19;
		} b;

	} EntryHi;

	union {
		uint32_t Value;
		uint8_t A[4];

		struct {
			unsigned GLOBAL: 1;
			unsigned V : 1;
			unsigned D : 1;
			unsigned C : 3;
			unsigned PFN : 20;
			unsigned ZERO: 6;
		} b;

	} EntryLo0;

	union {
		uint32_t Value;
		uint8_t A[4];

		struct {
			unsigned GLOBAL: 1;
			unsigned V : 1;
			unsigned D : 1;
			unsigned C : 3;
			unsigned PFN : 20;
			unsigned ZERO: 6;
		} b;

	} EntryLo1;
} TLB;

typedef struct {
   uint32_t VSTART;
   uint32_t VEND;
   uint32_t PHYSSTART;
   uint32_t VALID;
   uint32_t DIRTY;
   uint32_t GLOBAL;
   uint32_t ValidEntry;
} FASTTLB;

uint32_t AddressDefined ( usf_state_t *, uintptr_t VAddr);
void InitilizeTLB   ( usf_state_t * );
void SetupTLB       ( usf_state_t * );
void TLB_Probe      ( usf_state_t * );
void TLB_Read       ( usf_state_t * );
uint32_t TranslateVaddr ( usf_state_t *, uintptr_t * Addr);
void WriteTLBEntry  ( usf_state_t *, int32_t index );

#endif
