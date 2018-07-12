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
#define LargeCompileBufferSize	0x03200000
#define NormalCompileBufferSize	0x01500000

#define RSP_RECOMPMEM_SIZE		0x400000
#define RSP_SECRECOMPMEM_SIZE	0x200000

#define ROM_IN_MAPSPACE

#define PageRAM2(x)     (state->N64MEM+(uint32_t)(x))
#define PageVRAM(x)     (state->TLB_Map[((uint32_t)(x))>>12]+(uint32_t)(x))
#define PageVRAM2(x)    (uint32_t)(PageVRAM(x)-(uintptr_t)state->N64MEM)

/* Memory Control */
int  Allocate_Memory             ( void * );
void Release_Memory              ( usf_state_t * );
int PreAllocate_Memory( usf_state_t * );

/* CPU memory functions */
//int  r4300i_Command_MemoryFilter ( uint32_t dwExptCode, LPEXCEPTION_POINTERS lpEP );
//int  r4300i_CPU_MemoryFilter     ( uint32_t dwExptCode, LPEXCEPTION_POINTERS lpEP );
int32_t  r4300i_LB_NonMemory         ( usf_state_t *, uint32_t PAddr, uint32_t * Value, uint32_t SignExtend );
uint32_t r4300i_LB_VAddr             ( usf_state_t *, uint32_t VAddr, uint8_t * Value );
uint32_t r4300i_LD_VAddr             ( usf_state_t *, uint32_t VAddr, uint64_t * Value );
int32_t  r4300i_LH_NonMemory         ( usf_state_t *, uint32_t PAddr, uint32_t * Value, int32_t SignExtend );
uint32_t r4300i_LH_VAddr             ( usf_state_t *, uint32_t VAddr, uint16_t * Value );
int32_t  r4300i_LW_NonMemory         ( usf_state_t *, uint32_t PAddr, uint32_t * Value );
void r4300i_LW_PAddr                 ( usf_state_t *, uint32_t PAddr, uint32_t * Value );
uint32_t r4300i_LW_VAddr             ( usf_state_t *, uint32_t VAddr, uint32_t * Value );
int32_t  r4300i_SB_NonMemory         ( usf_state_t *, uint32_t PAddr, uint8_t Value );
uint32_t r4300i_SB_VAddr             ( usf_state_t *, uint32_t VAddr, uint8_t Value );
uint32_t r4300i_SD_VAddr            ( usf_state_t *, uint32_t VAddr, uint64_t Value );
int32_t  r4300i_SH_NonMemory         ( usf_state_t *, uint32_t PAddr, uint16_t Value );
uint32_t r4300i_SH_VAddr             ( usf_state_t *, uint32_t VAddr, uint16_t Value );
int32_t  r4300i_SW_NonMemory         ( usf_state_t *, uint32_t PAddr, uint32_t Value );
uint32_t r4300i_SW_VAddr             ( usf_state_t *, uint32_t VAddr, uint32_t Value );

uint8_t * PageROM(usf_state_t *, uint32_t addr);

void memcpyn642n64(usf_state_t *, uint32_t dest, uint32_t src, uint32_t len);
