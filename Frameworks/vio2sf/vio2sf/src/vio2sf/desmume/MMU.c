/*  Copyright (C) 2006 yopyop
    yopyop156@ifrance.com
    yopyop156.ifrance.com

	Copyright (C) 2007 shash

    This file is part of DeSmuME

    DeSmuME is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    DeSmuME is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with DeSmuME; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

//#define RENDER3D

#include <stdlib.h>
#include <math.h>
#include <string.h>

//#include "gl_vertex.h"
#include "spu_exports.h"

#include "state.h"

#include "MMU.h"

#include "debug.h"
#include "NDSSystem.h"
//#include "cflash.h"
#define cflash_read(s,a) 0
#define cflash_write(s,a,d)
#include "cp15.h"
//#include "wifi.h"
#include "registers.h"
#include "isqrt.h"

#if VIO2SF_GPU_ENABLE
#include "render3D.h"
#else
#define GPU_setVideoProp(p1, p2)
#define GPU_setBGProp(p1, p2, p3)

#define GPU_setBLDCNT(p1, p2) 
#define GPU_setBLDALPHA(p1, p2) 
#define GPU_setBLDY(p1, p2) 
#define GPU_setMOSAIC(p1, p2) 


#define GPU_remove(p1,p2)
#define GPU_addBack(p1,p2)

#define GPU_ChangeGraphicsCore(p1) 0

#define GPU_set_DISPCAPCNT(p1, p2) 
#define GPU_ligne(p1, p2) 
#define GPU_setMasterBrightness(p1, p2)

#define GPU_setWIN0_H(p1, p2)
#define GPU_setWIN0_H0(p1, p2)
#define GPU_setWIN0_H1(p1, p2)

#define GPU_setWIN0_V(p1, p2)
#define GPU_setWIN0_V0(p1, p2)
#define GPU_setWIN0_V1(p1, p2)

#define GPU_setWIN1_H(p1, p2)
#define GPU_setWIN1_H0(p1, p2)
#define GPU_setWIN1_H1(p1, p2)

#define GPU_setWIN1_V(p1, p2)
#define GPU_setWIN1_V0(p1, p2)
#define GPU_setWIN1_V1(p1, p2)

#define GPU_setWININ(p1, p2)
#define GPU_setWININ0(p1, p2)
#define GPU_setWININ1(p1, p2)

#define GPU_setWINOUT16(p1, p2)
#define GPU_setWINOUT(p1, p2)
#define GPU_setWINOBJ(p1, p2)

#define GPU_setBLDCNT_LOW(p1, p2)
#define GPU_setBLDCNT_HIGH(p1, p2)
#define GPU_setBLDCNT(p1, p2)

#define GPU_setBLDALPHA(p1, p2)
#define GPU_setBLDALPHA_EVA(p1, p2)
#define GPU_setBLDALPHA_EVB(p1, p2)

#define GPU_setBLDY_EVY(p1, p2)
#endif

#define ROM_MASK 3

/*
 *
 */
//#define PROFILE_MEMORY_ACCESS 1
#define EARLY_MEMORY_ACCESS 1

#define INTERNAL_DTCM_READ 1
#define INTERNAL_DTCM_WRITE 1

//#define LOG_CARD
//#define LOG_GPU
//#define LOG_DMA
//#define LOG_DMA2
//#define LOG_DIV

#define DUP2(x)  x, x
#define DUP4(x)  x, x, x, x
#define DUP8(x)  x, x, x, x,  x, x, x, x
#define DUP16(x) x, x, x, x,  x, x, x, x,  x, x, x, x,  x, x, x, x

#if 0
#endif
	   

const u32 MMU_ARM9_WAIT16[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 1, 1, 1, 1, 1,
};

const u32 MMU_ARM9_WAIT32[16]={
	1, 1, 1, 1, 1, 2, 2, 1, 8, 8, 5, 1, 1, 1, 1, 1,
};

const u32 MMU_ARM7_WAIT16[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 1, 1, 1, 1, 1,
};

const u32 MMU_ARM7_WAIT32[16]={
	1, 1, 1, 1, 1, 1, 1, 1, 8, 8, 5, 1, 1, 1, 1, 1,
};

void MMU_Init(NDS_state *state) {
	int i;

	LOG("MMU init\n");

	memset(state->MMU, 0, sizeof(MMU_struct));

	state->MMU->CART_ROM = state->MMU->UNUSED_RAM;

        for(i = 0x80; i<0xA0; ++i)
        {
           state->MMU_ARM9_MEM_MAP[i] = state->MMU->CART_ROM;
           state->MMU_ARM7_MEM_MAP[i] = state->MMU->CART_ROM;
        }

	state->MMU->MMU_MEM[0] = state->MMU_ARM9_MEM_MAP;
	state->MMU->MMU_MEM[1] = state->MMU_ARM7_MEM_MAP;
	state->MMU->MMU_MASK[0]= state->MMU_ARM9_MEM_MASK;
	state->MMU->MMU_MASK[1] = state->MMU_ARM7_MEM_MASK;

	state->MMU->ITCMRegion = 0x00800000;

	state->MMU->MMU_WAIT16[0] = MMU_ARM9_WAIT16;
	state->MMU->MMU_WAIT16[1] = MMU_ARM7_WAIT16;
	state->MMU->MMU_WAIT32[0] = MMU_ARM9_WAIT32;
	state->MMU->MMU_WAIT32[1] = MMU_ARM7_WAIT32;

	for(i = 0;i < 16;i++)
		FIFOInit(state->MMU->fifos + i);
	
        mc_init(&state->MMU->fw, MC_TYPE_FLASH);  /* init fw device */
        mc_alloc(&state->MMU->fw, NDS_FW_SIZE_V1);
        state->MMU->fw.fp = NULL;

        // Init Backup Memory device, this should really be done when the rom is loaded
        mc_init(&state->MMU->bupmem, MC_TYPE_AUTODETECT);
        mc_alloc(&state->MMU->bupmem, 1);
        state->MMU->bupmem.fp = NULL;

} 

void MMU_DeInit(NDS_state *state) {
	LOG("MMU deinit\n");
//    if (MMU->fw.fp)
//       fclose(MMU->fw.fp);
    mc_free(&state->MMU->fw);
//    if (MMU->bupmem.fp)
//       fclose(MMU->bupmem.fp);
    mc_free(&state->MMU->bupmem);
}

//Card rom & ram


void MMU_clearMem(NDS_state *state)
{
	int i;

	memset(state->ARM9Mem->ARM9_ABG,  0, 0x080000);
	memset(state->ARM9Mem->ARM9_AOBJ, 0, 0x040000);
	memset(state->ARM9Mem->ARM9_BBG,  0, 0x020000);
	memset(state->ARM9Mem->ARM9_BOBJ, 0, 0x020000);
	memset(state->ARM9Mem->ARM9_DTCM, 0, 0x4000);
	memset(state->ARM9Mem->ARM9_ITCM, 0, 0x8000);
	memset(state->ARM9Mem->ARM9_LCD,  0, 0x0A4000);
	memset(state->ARM9Mem->ARM9_OAM,  0, 0x0800);
	memset(state->ARM9Mem->ARM9_REG,  0, 0x01000000);
	memset(state->ARM9Mem->ARM9_VMEM, 0, 0x0800);
	memset(state->ARM9Mem->ARM9_WRAM, 0, 0x01000000);
	memset(state->ARM9Mem->MAIN_MEM,  0, 0x400000);

	memset(state->ARM9Mem->blank_memory,  0, 0x020000);
	
	memset(state->MMU->ARM7_ERAM,     0, 0x010000);
	memset(state->MMU->ARM7_REG,      0, 0x010000);
	
	for(i = 0;i < 16;i++)
	FIFOInit(state->MMU->fifos + i);
	
	state->MMU->DTCMRegion = 0;
	state->MMU->ITCMRegion = 0x00800000;
	
	memset(state->MMU->timer,         0, sizeof(u16) * 2 * 4);
	memset(state->MMU->timerMODE,     0, sizeof(s32) * 2 * 4);
	memset(state->MMU->timerON,       0, sizeof(u32) * 2 * 4);
	memset(state->MMU->timerRUN,      0, sizeof(u32) * 2 * 4);
	memset(state->MMU->timerReload,   0, sizeof(u16) * 2 * 4);
	
	memset(state->MMU->reg_IME,       0, sizeof(u32) * 2);
	memset(state->MMU->reg_IE,        0, sizeof(u32) * 2);
	memset(state->MMU->reg_IF,        0, sizeof(u32) * 2);
	
	memset(state->MMU->DMAStartTime,  0, sizeof(u32) * 2 * 4);
	memset(state->MMU->DMACycle,      0, sizeof(s32) * 2 * 4);
	memset(state->MMU->DMACrt,        0, sizeof(u32) * 2 * 4);
	memset(state->MMU->DMAing,        0, sizeof(BOOL) * 2 * 4);
	
	memset(state->MMU->dscard,        0, sizeof(nds_dscard) * 2);
	
	state->MainScreen->offset = 192;
	state->SubScreen->offset  = 0;

        /* setup the texture slot pointers */
#if 0
        state->ARM9Mem->textureSlotAddr[0] = state->ARM9Mem->blank_memory;
        state->ARM9Mem->textureSlotAddr[1] = state->ARM9Mem->blank_memory;
        state->ARM9Mem->textureSlotAddr[2] = state->ARM9Mem->blank_memory;
        state->ARM9Mem->textureSlotAddr[3] = state->ARM9Mem->blank_memory;
#else
        state->ARM9Mem->textureSlotAddr[0] = &state->ARM9Mem->ARM9_LCD[0x20000 * 0];
        state->ARM9Mem->textureSlotAddr[1] = &state->ARM9Mem->ARM9_LCD[0x20000 * 1];
        state->ARM9Mem->textureSlotAddr[2] = &state->ARM9Mem->ARM9_LCD[0x20000 * 2];
        state->ARM9Mem->textureSlotAddr[3] = &state->ARM9Mem->ARM9_LCD[0x20000 * 3];
#endif
}

/* the VRAM blocks keep their content even when not blended in */
/* to ensure that we write the content back to the LCD ram */
/* FIXME: VRAM Bank E,F,G,H,I missing */
void MMU_VRAMWriteBackToLCD(NDS_state *state, u8 block)
{
	u8 *destination;
	u8 *source;
	u32 size ;
	u8 VRAMBankCnt;
	#if 1
		return ;
	#endif
	destination = 0 ;
	source = 0;
	VRAMBankCnt = MMU_read8(state, ARMCPU_ARM9,REG_VRAMCNTA+block) ;
	switch (block)
	{
		case 0: // Bank A
			destination = state->ARM9Mem->ARM9_LCD ;
			size = 0x20000 ;
			break ;
		case 1: // Bank B
			destination = state->ARM9Mem->ARM9_LCD + 0x20000 ;
			size = 0x20000 ;
			break ;
		case 2: // Bank C
			destination = state->ARM9Mem->ARM9_LCD + 0x40000 ;
			size = 0x20000 ;
			break ;
		case 3: // Bank D
			destination = state->ARM9Mem->ARM9_LCD + 0x60000 ;
			size = 0x20000 ;
			break ;
		case 4: // Bank E
			destination = state->ARM9Mem->ARM9_LCD + 0x80000 ;
			size = 0x10000 ;
			break ;
		case 5: // Bank F
			destination = state->ARM9Mem->ARM9_LCD + 0x90000 ;
			size = 0x4000 ;
			break ;
		case 6: // Bank G
			destination = state->ARM9Mem->ARM9_LCD + 0x94000 ;
			size = 0x4000 ;
			break ;
		case 8: // Bank H
			destination = state->ARM9Mem->ARM9_LCD + 0x98000 ;
			size = 0x8000 ;
			break ;
		case 9: // Bank I
			destination = state->ARM9Mem->ARM9_LCD + 0xA0000 ;
			size = 0x4000 ;
			break ;
		default:
			return ;
	}
	switch (VRAMBankCnt & 7) {
		case 0:
			/* vram is allready stored at LCD, we dont need to write it back */
			state->MMU->vScreen = 1;
			break ;
		case 1:
	switch(block){
	case 0:
	case 1:
	case 2:
	case 3:
		/* banks are in use for BG at ABG + ofs * 0x20000 */
				source = state->ARM9Mem->ARM9_ABG + ((VRAMBankCnt >> 3) & 3) * 0x20000 ;
		break ;
	case 4:
		/* bank E is in use at ABG */ 
		source = state->ARM9Mem->ARM9_ABG ;
		break;
	case 5:
	case 6:
		/* banks are in use for BG at ABG + (0x4000*OFS.0)+(0x10000*OFS.1)*/
		source = state->ARM9Mem->ARM9_ABG + (((VRAMBankCnt >> 3) & 1) * 0x4000) + (((VRAMBankCnt >> 2) & 1) * 0x10000) ;
		break;
	case 8:
		/* bank H is in use at BBG */ 
		source = state->ARM9Mem->ARM9_BBG ;
		break ;
	case 9:
		/* bank I is in use at BBG */ 
		source = state->ARM9Mem->ARM9_BBG + 0x8000 ;
		break;
	default: return ;
	}
			break ;
		case 2:
			if (block < 2)
			{
				/* banks A,B are in use for OBJ at AOBJ + ofs * 0x20000 */
				source = state->ARM9Mem->ARM9_AOBJ + ((VRAMBankCnt >> 3) & 1) * 0x20000 ;
			} else return ;
			break ;
		case 4:
	switch(block){
	case 2:
		/* bank C is in use at BBG */ 
		source = state->ARM9Mem->ARM9_BBG ;
		break ;
	case 3:
		/* bank D is in use at BOBJ */ 
		source = state->ARM9Mem->ARM9_BOBJ ;
		break ;
	default: return ;
	}
			break ;
		default:
			return ;
	}
	if (!destination) return ;
	if (!source) return ;
	memcpy(destination,source,size) ;
}

void MMU_VRAMReloadFromLCD(NDS_state *state, u8 block,u8 VRAMBankCnt)
{
	u8 *destination;
	u8 *source;
	u32 size;
	#if 1
		return ;
	#endif
	destination = 0;
	source = 0;
	size = 0;
	switch (block)
	{
		case 0: // Bank A
			source = state->ARM9Mem->ARM9_LCD ;
			size = 0x20000 ;
			break ;
		case 1: // Bank B
			source = state->ARM9Mem->ARM9_LCD + 0x20000 ;
			size = 0x20000 ;
			break ;
		case 2: // Bank C
			source = state->ARM9Mem->ARM9_LCD + 0x40000 ;
			size = 0x20000 ;
			break ;
		case 3: // Bank D
			source = state->ARM9Mem->ARM9_LCD + 0x60000 ;
			size = 0x20000 ;
			break ;
		case 4: // Bank E
			source = state->ARM9Mem->ARM9_LCD + 0x80000 ;
			size = 0x10000 ;
			break ;
		case 5: // Bank F
			source = state->ARM9Mem->ARM9_LCD + 0x90000 ;
			size = 0x4000 ;
			break ;
		case 6: // Bank G
			source = state->ARM9Mem->ARM9_LCD + 0x94000 ;
			size = 0x4000 ;
			break ;
		case 8: // Bank H
			source = state->ARM9Mem->ARM9_LCD + 0x98000 ;
			size = 0x8000 ;
			break ;
		case 9: // Bank I
			source = state->ARM9Mem->ARM9_LCD + 0xA0000 ;
			size = 0x4000 ;
			break ;
		default:
			return ;
	}
	switch (VRAMBankCnt & 7) {
		case 0:
			/* vram is allready stored at LCD, we dont need to write it back */
			state->MMU->vScreen = 1;
			break ;
		case 1:
			if (block < 4)
			{
				/* banks are in use for BG at ABG + ofs * 0x20000 */
				destination = state->ARM9Mem->ARM9_ABG + ((VRAMBankCnt >> 3) & 3) * 0x20000 ;
			} else return ;
			break ;
		case 2:
				switch(block){
	case 0:
	case 1:
	case 2:
	case 3:
		/* banks are in use for BG at ABG + ofs * 0x20000 */
				destination = state->ARM9Mem->ARM9_ABG + ((VRAMBankCnt >> 3) & 3) * 0x20000 ;
		break ;
	case 4:
		/* bank E is in use at ABG */ 
		destination = state->ARM9Mem->ARM9_ABG ;
		break;
	case 5:
	case 6:
		/* banks are in use for BG at ABG + (0x4000*OFS.0)+(0x10000*OFS.1)*/
		destination = state->ARM9Mem->ARM9_ABG + (((VRAMBankCnt >> 3) & 1) * 0x4000) + (((VRAMBankCnt >> 2) & 1) * 0x10000) ;
		break;
	case 8:
		/* bank H is in use at BBG */ 
		destination = state->ARM9Mem->ARM9_BBG ;
		break ;
	case 9:
		/* bank I is in use at BBG */ 
		destination = state->ARM9Mem->ARM9_BBG + 0x8000 ;
		break;
	default: return ;
	}
			break ;
		case 4:
	switch(block){
	case 2:
		/* bank C is in use at BBG */ 
		destination = state->ARM9Mem->ARM9_BBG ;
		break ;
	case 3:
		/* bank D is in use at BOBJ */ 
		destination = state->ARM9Mem->ARM9_BOBJ ;
		break ;
	default: return ;
	}
			break ;
		default:
			return ;
	}
	if (!destination) return ;
	if (!source) return ;
	memcpy(destination,source,size) ;
}

void MMU_setRom(NDS_state *state, u8 * rom, u32 mask)
{
	unsigned int i;
	state->MMU->CART_ROM = rom;
	
	for(i = 0x80; i<0xA0; ++i)
	{
		state->MMU_ARM9_MEM_MAP[i] = rom;
		state->MMU_ARM7_MEM_MAP[i] = rom;
		state->MMU_ARM9_MEM_MASK[i] = mask;
		state->MMU_ARM7_MEM_MASK[i] = mask;
	}
	state->rom_mask = mask;
}

void MMU_unsetRom(NDS_state *state)
{
	unsigned int i;
	state->MMU->CART_ROM=state->MMU->UNUSED_RAM;
	
	for(i = 0x80; i<0xA0; ++i)
	{
		state->MMU_ARM9_MEM_MAP[i] = state->MMU->UNUSED_RAM;
		state->MMU_ARM7_MEM_MAP[i] = state->MMU->UNUSED_RAM;
		state->MMU_ARM9_MEM_MASK[i] = ROM_MASK;
		state->MMU_ARM7_MEM_MASK[i] = ROM_MASK;
	}
	state->rom_mask = ROM_MASK;
}

u8 FASTCALL MMU_read8(NDS_state *state, u32 proc, u32 adr)
{
#ifdef INTERNAL_DTCM_READ
	if((proc==ARMCPU_ARM9)&((adr&(~0x3FFF))==state->MMU->DTCMRegion))
	{
		return state->ARM9Mem->ARM9_DTCM[adr&0x3FFF];
	}
#endif
	
	// CFlash reading, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000))
		return (unsigned char)cflash_read(state, adr);

#ifdef EXPERIMENTAL_WIFI
	/* wifi mac access */
	if ((proc==ARMCPU_ARM7) && (adr>=0x04800000)&&(adr<0x05000000))
	{
		if (adr & 1)
			return (WIFI_read16(&state->wifiMac,adr) >> 8) & 0xFF;
		else
			return WIFI_read16(&state->wifiMac,adr) & 0xFF;
	}
#endif

        return state->MMU->MMU_MEM[proc][(adr>>20)&0xFF][adr&state->MMU->MMU_MASK[proc][(adr>>20)&0xFF]];
}



u16 FASTCALL MMU_read16(NDS_state *state, u32 proc, u32 adr)
{    
#ifdef INTERNAL_DTCM_READ
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == state->MMU->DTCMRegion))
	{
		/* Returns data from DTCM (ARM9 only) */
		return T1ReadWord(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF);
	}
#endif
	
	// CFlash reading, Mic
	if ((adr>=0x08800000)&&(adr<0x09900000))
	   return (unsigned short)cflash_read(state, adr);

#ifdef EXPERIMENTAL_WIFI
	/* wifi mac access */
	if ((proc==ARMCPU_ARM7) && (adr>=0x04800000)&&(adr<0x05000000))
		return WIFI_read16(&state->wifiMac,adr) ;
#endif

	adr &= 0x0FFFFFFF;

	if(adr&0x04000000)
	{
		/* Adress is an IO register */
		switch(adr)
		{

#if VIO2SF_GPU_ENABLE
			case 0x04000604:
				return (state->gpu3D->NDS_3D_GetNumPolys(state->gpu3D)&2047);
			case 0x04000606:
				return (state->gpu3D->NDS_3D_GetNumVertex(state->gpu3D)&8191);
#endif

			case REG_IPCFIFORECV :               /* TODO (clear): ??? */
				state->execute = FALSE;
				return 1;
				
			case REG_IME :
				return (u16)state->MMU->reg_IME[proc];
				
			case REG_IE :
				return (u16)state->MMU->reg_IE[proc];
			case REG_IE + 2 :
				return (u16)(state->MMU->reg_IE[proc]>>16);
				
			case REG_IF :
				return (u16)state->MMU->reg_IF[proc];
			case REG_IF + 2 :
				return (u16)(state->MMU->reg_IF[proc]>>16);
				
			case REG_TM0CNTL :
			case REG_TM1CNTL :
			case REG_TM2CNTL :
			case REG_TM3CNTL :
				return state->MMU->timer[proc][(adr&0xF)>>2];
			
			case 0x04000630 :
				LOG("vect res\r\n");	/* TODO (clear): ??? */
				//execute = FALSE;
				return 0;
                        case REG_POSTFLG :
				return 1;
			default :
				break;
		}
	}
	
	/* Returns data from memory */
	return T1ReadWord(state->MMU->MMU_MEM[proc][(adr >> 20) & 0xFF], adr & state->MMU->MMU_MASK[proc][(adr >> 20) & 0xFF]);
}
	 
u32 FASTCALL MMU_read32(NDS_state *state, u32 proc, u32 adr)
{
#ifdef INTERNAL_DTCM_READ
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == state->MMU->DTCMRegion))
	{
		/* Returns data from DTCM (ARM9 only) */
		return T1ReadLong(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF);
	}
#endif
	
	// CFlash reading, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000))
	   return (unsigned long)cflash_read(state, adr);
		
	adr &= 0x0FFFFFFF;

	if((adr >> 24) == 4)
	{
		/* Adress is an IO register */
		switch(adr)
		{
			// This is hacked due to the only current 3D core
			case 0x04000600:
            {
                u32 fifonum = IPCFIFO+proc;

				u32 gxstat =	(state->MMU->fifos[fifonum].empty<<26) |
								(1<<25) | 
								(state->MMU->fifos[fifonum].full<<24) |
								/*((NDS_nbpush[0]&1)<<13) | ((NDS_nbpush[2]&0x1F)<<8) |*/ 
								2;

				LOG ("GXSTAT: 0x%X", gxstat);

				return	gxstat;
            }

			case 0x04000640:
			case 0x04000644:
			case 0x04000648:
			case 0x0400064C:
			case 0x04000650:
			case 0x04000654:
			case 0x04000658:
			case 0x0400065C:
			case 0x04000660:
			case 0x04000664:
			case 0x04000668:
			case 0x0400066C:
			case 0x04000670:
			case 0x04000674:
			case 0x04000678:
			case 0x0400067C:
			{
				//LOG("4000640h..67Fh - CLIPMTX_RESULT - Read Current Clip Coordinates Matrix (R)");
#if VIO2SF_GPU_ENABLE
				return state->gpu3D->NDS_3D_GetClipMatrix (state->gpu3D, (adr-0x04000640)/4);
#else
				return 0;
#endif
			}
			case 0x04000680:
			case 0x04000684:
			case 0x04000688:
			case 0x0400068C:
			case 0x04000690:
			case 0x04000694:
			case 0x04000698:
			case 0x0400069C:
			case 0x040006A0:
			{
#if VIO2SF_GPU_ENABLE
				//LOG("4000680h..6A3h - VECMTX_RESULT - Read Current Directional Vector Matrix (R)");
				return state->gpu3D->NDS_3D_GetDirectionalMatrix (state->gpu3D, (adr-0x04000680)/4);
#else
				return 0;
#endif
			}

			case 0x4000604:
			{
#if VIO2SF_GPU_ENABLE
				return (state->gpu3D->NDS_3D_GetNumPolys(state->gpu3D)&2047) & ((state->gpu3D->NDS_3D_GetNumVertex(state->gpu3D)&8191) << 16);
				//LOG ("read32 - RAM_COUNT -> 0x%X", ((u32 *)(MMU->MMU_MEM[proc][(adr>>20)&0xFF]))[(adr&MMU->MMU_MASK[proc][(adr>>20)&0xFF])>>2]);
#else
				return 0;
#endif
			}
			
			case REG_IME :
				return state->MMU->reg_IME[proc];
			case REG_IE :
				return state->MMU->reg_IE[proc];
			case REG_IF :
				return state->MMU->reg_IF[proc];
			case REG_IPCFIFORECV :
			{
				u16 IPCFIFO_CNT = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x184);
				if(IPCFIFO_CNT&0x8000)
				{
				//execute = FALSE;
				u32 fifonum = IPCFIFO+proc;
				u32 val = FIFOValue(state->MMU->fifos + fifonum);
				u32 remote = (proc+1) & 1;
				u16 IPCFIFO_CNT_remote = T1ReadWord(state->MMU->MMU_MEM[remote][0x40], 0x184);
				IPCFIFO_CNT |= (state->MMU->fifos[fifonum].empty<<8) | (state->MMU->fifos[fifonum].full<<9) | (state->MMU->fifos[fifonum].error<<14);
				IPCFIFO_CNT_remote |= (state->MMU->fifos[fifonum].empty) | (state->MMU->fifos[fifonum].full<<1);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184, IPCFIFO_CNT);
				T1WriteWord(state->MMU->MMU_MEM[remote][0x40], 0x184, IPCFIFO_CNT_remote);
				if ((state->MMU->fifos[fifonum].empty) && (IPCFIFO_CNT & BIT(2)))
					NDS_makeInt(state, remote,17) ; /* remote: SEND FIFO EMPTY */
				return val;
				}
			}
			return 0;
                        case REG_TM0CNTL :
                        case REG_TM1CNTL :
                        case REG_TM2CNTL :
                        case REG_TM3CNTL :
			{
				u32 val = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], (adr + 2) & 0xFFF);
				return state->MMU->timer[proc][(adr&0xF)>>2] | (val<<16);
			}	
			/*
			case 0x04000640 :	// TODO (clear): again, ??? 
				LOG("read proj\r\n");
			return 0;
			case 0x04000680 :
				LOG("read roat\r\n");
			return 0;
			case 0x04000620 :
				LOG("point res\r\n");
			return 0;
			*/
                        case REG_GCDATAIN:
			{
                                u32 val;

                                if(!state->MMU->dscard[proc].adress) return 0;
				
                                val = T1ReadLong(state->MMU->CART_ROM, state->MMU->dscard[proc].adress);

				state->MMU->dscard[proc].adress += 4;	/* increment adress */
				
				state->MMU->dscard[proc].transfer_count--;	/* update transfer counter */
				if(state->MMU->dscard[proc].transfer_count) /* if transfer is not ended */
				{
					return val;	/* return data */
				}
				else	/* transfer is done */
                                {                                                       
                                        T1WriteLong(state->MMU->MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, T1ReadLong(state->MMU->MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff) & ~(0x00800000 | 0x80000000));
					/* = 0x7f7fffff */
					
					/* if needed, throw irq for the end of transfer */
                                        if(T1ReadWord(state->MMU->MMU_MEM[proc][(REG_AUXSPICNT >> 20) & 0xff], REG_AUXSPICNT & 0xfff) & 0x4000)
					{
                                                if(proc == ARMCPU_ARM7) NDS_makeARM7Int(state, 19);
                                                else NDS_makeARM9Int(state, 19);
					}
					
					return val;
				}
			}

			default :
				break;
		}
	}
	
	/* Returns data from memory */
	return T1ReadLong(state->MMU->MMU_MEM[proc][(adr >> 20) & 0xFF], adr & state->MMU->MMU_MASK[proc][(adr >> 20) & 0xFF]);
}
	
void FASTCALL MMU_write8(NDS_state *state, u32 proc, u32 adr, u8 val)
{
#ifdef INTERNAL_DTCM_WRITE
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == state->MMU->DTCMRegion))
	{
		/* Writes data in DTCM (ARM9 only) */
		state->ARM9Mem->ARM9_DTCM[adr&0x3FFF] = val;
		return ;
	}
#endif
	
	// CFlash writing, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000)) {
		cflash_write(state,adr,val);
		return;
	}

	adr &= 0x0FFFFFFF;

        // This is bad, remove it
        if(proc == ARMCPU_ARM7)
        {
           if ((adr>=0x04000400)&&(adr<0x0400051D))
           {
              SPU_WriteByte(state, adr, val);
              return;
           }
        }

	if ((adr & 0xFF800000) == 0x04800000)
	{
		/* is wifi hardware, dont intermix with regular hardware registers */
		/* FIXME handle 8 bit writes */
		return ;
	}

	switch(adr)
	{
		case REG_DISPA_WIN0H: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_H1 (state->MainScreen->gpu, val);
			break ; 	 
		case REG_DISPA_WIN0H+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_H0 (state->MainScreen->gpu, val);
			break ; 	 
		case REG_DISPA_WIN1H: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_H1 (state->MainScreen->gpu,val);
			break ; 	 
		case REG_DISPA_WIN1H+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_H0 (state->MainScreen->gpu,val);
			break ; 	 

		case REG_DISPB_WIN0H: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_H1(state->SubScreen->gpu,val);
			break ; 	 
		case REG_DISPB_WIN0H+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_H0(state->SubScreen->gpu,val);
			break ; 	 
		case REG_DISPB_WIN1H: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_H1(state->SubScreen->gpu,val);
			break ; 	 
		case REG_DISPB_WIN1H+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_H0(state->SubScreen->gpu,val);
			break ;

		case REG_DISPA_WIN0V: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_V1(state->MainScreen->gpu,val) ;
			break ; 	 
		case REG_DISPA_WIN0V+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_V0(state->MainScreen->gpu,val) ;
			break ; 	 
		case REG_DISPA_WIN1V: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_V1(state->MainScreen->gpu,val) ;
			break ; 	 
		case REG_DISPA_WIN1V+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_V0(state->MainScreen->gpu,val) ;
			break ; 	 

		case REG_DISPB_WIN0V: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_V1(state->SubScreen->gpu,val) ;
			break ; 	 
		case REG_DISPB_WIN0V+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN0_V0(state->SubScreen->gpu,val) ;
			break ; 	 
		case REG_DISPB_WIN1V: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_V1(state->SubScreen->gpu,val) ;
			break ; 	 
		case REG_DISPB_WIN1V+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWIN1_V0(state->SubScreen->gpu,val) ;
			break ;

		case REG_DISPA_WININ: 	 
			if(proc == ARMCPU_ARM9) GPU_setWININ0(state->MainScreen->gpu,val) ;
			break ; 	 
		case REG_DISPA_WININ+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWININ1(state->MainScreen->gpu,val) ;
			break ; 	 
		case REG_DISPA_WINOUT: 	 
			if(proc == ARMCPU_ARM9) GPU_setWINOUT(state->MainScreen->gpu,val) ;
			break ; 	 
		case REG_DISPA_WINOUT+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWINOBJ(state->MainScreen->gpu,val);
			break ; 	 

		case REG_DISPB_WININ: 	 
			if(proc == ARMCPU_ARM9) GPU_setWININ0(state->SubScreen->gpu,val) ;
			break ; 	 
		case REG_DISPB_WININ+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWININ1(state->SubScreen->gpu,val) ;
			break ; 	 
		case REG_DISPB_WINOUT: 	 
			if(proc == ARMCPU_ARM9) GPU_setWINOUT(state->SubScreen->gpu,val) ;
			break ; 	 
		case REG_DISPB_WINOUT+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setWINOBJ(state->SubScreen->gpu,val) ;
			break ;


		case REG_DISPA_BLDCNT:
			if(proc == ARMCPU_ARM9) GPU_setBLDCNT_HIGH(state->MainScreen->gpu,val);
			break;
		case REG_DISPA_BLDCNT+1:
			if(proc == ARMCPU_ARM9) GPU_setBLDCNT_LOW (state->MainScreen->gpu,val);
			break;

		case REG_DISPB_BLDCNT: 	 
			if(proc == ARMCPU_ARM9) GPU_setBLDCNT_HIGH (state->SubScreen->gpu,val);
			break;
		case REG_DISPB_BLDCNT+1: 	 
			if(proc == ARMCPU_ARM9) GPU_setBLDCNT_LOW (state->SubScreen->gpu,val);
			break;

		case REG_DISPA_BLDALPHA: 	 
			if(proc == ARMCPU_ARM9) GPU_setBLDALPHA_EVB(state->MainScreen->gpu,val) ;
			break;
		case REG_DISPA_BLDALPHA+1:
			if(proc == ARMCPU_ARM9) GPU_setBLDALPHA_EVA(state->MainScreen->gpu,val) ;
			break;

		case REG_DISPB_BLDALPHA:
			if(proc == ARMCPU_ARM9) GPU_setBLDALPHA_EVB(state->SubScreen->gpu,val) ;
			break;
		case REG_DISPB_BLDALPHA+1:
			if(proc == ARMCPU_ARM9) GPU_setBLDALPHA_EVA(state->SubScreen->gpu,val);
			break;

		case REG_DISPA_BLDY: 	 
			if(proc == ARMCPU_ARM9) GPU_setBLDY_EVY(state->MainScreen->gpu,val) ;
			break ; 	 
		case REG_DISPB_BLDY: 	 
			if(proc == ARMCPU_ARM9) GPU_setBLDY_EVY(state->SubScreen->gpu,val) ;
			break;

		/* TODO: EEEK ! Controls for VRAMs A, B, C, D are missing ! */
		/* TODO: Not all mappings of VRAMs are handled... (especially BG and OBJ modes) */
		case REG_VRAMCNTA:
		case REG_VRAMCNTB:
		case REG_VRAMCNTC:
		case REG_VRAMCNTD:
			if(proc == ARMCPU_ARM9)
			{
                MMU_VRAMWriteBackToLCD(state, 0) ;
                MMU_VRAMWriteBackToLCD(state, 1) ;
                MMU_VRAMWriteBackToLCD(state, 2) ;
                MMU_VRAMWriteBackToLCD(state, 3) ;
				switch(val & 0x1F)
				{
				case 1 :
					state->MMU->vram_mode[adr-REG_VRAMCNTA] = 0; // BG-VRAM
					//MMU->vram_offset[0] = ARM9Mem->ARM9_ABG+(0x20000*0); // BG-VRAM
					break;
				case 1 | (1 << 3) :
					state->MMU->vram_mode[adr-REG_VRAMCNTA] = 1; // BG-VRAM
					//MMU->vram_offset[0] = ARM9Mem->ARM9_ABG+(0x20000*1); // BG-VRAM
					break;
				case 1 | (2 << 3) :
					state->MMU->vram_mode[adr-REG_VRAMCNTA] = 2; // BG-VRAM
					//MMU->vram_offset[0] = ARM9Mem->ARM9_ABG+(0x20000*2); // BG-VRAM
					break;
				case 1 | (3 << 3) :
					state->MMU->vram_mode[adr-REG_VRAMCNTA] = 3; // BG-VRAM
					//MMU->vram_offset[0] = ARM9Mem->ARM9_ABG+(0x20000*3); // BG-VRAM
					break;
				case 0: /* mapped to lcd */
                    state->MMU->vram_mode[adr-REG_VRAMCNTA] = 4 | (adr-REG_VRAMCNTA) ;
					break ;
				}
                                /*
                                 * FIXME: simply texture slot handling
                                 * This is a first stab and is not correct. It does
                                 * not handle a VRAM texture slot becoming
                                 * unconfigured.
                                 * Revisit all of VRAM control handling for future
                                 * release?
                                 */
                                if ( val & 0x80) {
                                  if ( (val & 0x7) == 3) {
                                    int slot_index = (val >> 3) & 0x3;

                                    state->ARM9Mem->textureSlotAddr[slot_index] =
                                      &state->ARM9Mem->ARM9_LCD[0x20000 * (adr - REG_VRAMCNTA)];
                                  }
                                }
                MMU_VRAMReloadFromLCD(state, adr-REG_VRAMCNTA,val) ;
			}
			break;
                case REG_VRAMCNTE :
			if(proc == ARMCPU_ARM9)
			{
                MMU_VRAMWriteBackToLCD(state, (u8)REG_VRAMCNTE) ;
                                if((val & 7) == 5)
				{
					state->ARM9Mem->ExtPal[0][0] = state->ARM9Mem->ARM9_LCD + 0x80000;
					state->ARM9Mem->ExtPal[0][1] = state->ARM9Mem->ARM9_LCD + 0x82000;
					state->ARM9Mem->ExtPal[0][2] = state->ARM9Mem->ARM9_LCD + 0x84000;
					state->ARM9Mem->ExtPal[0][3] = state->ARM9Mem->ARM9_LCD + 0x86000;
				}
                                else if((val & 7) == 3)
				{
					state->ARM9Mem->texPalSlot[0] = state->ARM9Mem->ARM9_LCD + 0x80000;
					state->ARM9Mem->texPalSlot[1] = state->ARM9Mem->ARM9_LCD + 0x82000;
					state->ARM9Mem->texPalSlot[2] = state->ARM9Mem->ARM9_LCD + 0x84000;
					state->ARM9Mem->texPalSlot[3] = state->ARM9Mem->ARM9_LCD + 0x86000;
				}
                                else if((val & 7) == 4)
				{
					state->ARM9Mem->ExtPal[0][0] = state->ARM9Mem->ARM9_LCD + 0x80000;
					state->ARM9Mem->ExtPal[0][1] = state->ARM9Mem->ARM9_LCD + 0x82000;
					state->ARM9Mem->ExtPal[0][2] = state->ARM9Mem->ARM9_LCD + 0x84000;
					state->ARM9Mem->ExtPal[0][3] = state->ARM9Mem->ARM9_LCD + 0x86000;
				}
				
				MMU_VRAMReloadFromLCD(state,adr-REG_VRAMCNTE,val) ;
			}
			break;
		
                case REG_VRAMCNTF :
			if(proc == ARMCPU_ARM9)
			{
				switch(val & 0x1F)
				{
                                        case 4 :
						state->ARM9Mem->ExtPal[0][0] = state->ARM9Mem->ARM9_LCD + 0x90000;
						state->ARM9Mem->ExtPal[0][1] = state->ARM9Mem->ARM9_LCD + 0x92000;
						break;
						
                                        case 4 | (1 << 3) :
						state->ARM9Mem->ExtPal[0][2] = state->ARM9Mem->ARM9_LCD + 0x90000;
						state->ARM9Mem->ExtPal[0][3] = state->ARM9Mem->ARM9_LCD + 0x92000;
						break;
						
                                        case 3 :
						state->ARM9Mem->texPalSlot[0] = state->ARM9Mem->ARM9_LCD + 0x90000;
						break;
						
                                        case 3 | (1 << 3) :
						state->ARM9Mem->texPalSlot[1] = state->ARM9Mem->ARM9_LCD + 0x90000;
						break;
						
                                        case 3 | (2 << 3) :
						state->ARM9Mem->texPalSlot[2] = state->ARM9Mem->ARM9_LCD + 0x90000;
						break;
						
                                        case 3 | (3 << 3) :
						state->ARM9Mem->texPalSlot[3] = state->ARM9Mem->ARM9_LCD + 0x90000;
						break;
						
                                        case 5 :
                                        case 5 | (1 << 3) :
                                        case 5 | (2 << 3) :
                                        case 5 | (3 << 3) :
						state->ARM9Mem->ObjExtPal[0][0] = state->ARM9Mem->ARM9_LCD + 0x90000;
						state->ARM9Mem->ObjExtPal[0][1] = state->ARM9Mem->ARM9_LCD + 0x92000;
						break;
				}
		 	}
			break;
                case REG_VRAMCNTG :
			if(proc == ARMCPU_ARM9)
			{
		 		switch(val & 0x1F)
				{
                                        case 4 :
						state->ARM9Mem->ExtPal[0][0] = state->ARM9Mem->ARM9_LCD + 0x94000;
						state->ARM9Mem->ExtPal[0][1] = state->ARM9Mem->ARM9_LCD + 0x96000;
						break;
						
                                        case 4 | (1 << 3) :
						state->ARM9Mem->ExtPal[0][2] = state->ARM9Mem->ARM9_LCD + 0x94000;
						state->ARM9Mem->ExtPal[0][3] = state->ARM9Mem->ARM9_LCD + 0x96000;
						break;
						
                                        case 3 :
						state->ARM9Mem->texPalSlot[0] = state->ARM9Mem->ARM9_LCD + 0x94000;
						break;
						
                                        case 3 | (1 << 3) :
						state->ARM9Mem->texPalSlot[1] = state->ARM9Mem->ARM9_LCD + 0x94000;
						break;
						
                                        case 3 | (2 << 3) :
						state->ARM9Mem->texPalSlot[2] = state->ARM9Mem->ARM9_LCD + 0x94000;
						break;
						
                                        case 3 | (3 << 3) :
						state->ARM9Mem->texPalSlot[3] = state->ARM9Mem->ARM9_LCD + 0x94000;
						break;
						
                                        case 5 :
                                        case 5 | (1 << 3) :
                                        case 5 | (2 << 3) :
                                        case 5 | (3 << 3) :
						state->ARM9Mem->ObjExtPal[0][0] = state->ARM9Mem->ARM9_LCD + 0x94000;
						state->ARM9Mem->ObjExtPal[0][1] = state->ARM9Mem->ARM9_LCD + 0x96000;
						break;
				}
			}
			break;
			
                case REG_VRAMCNTH  :
			if(proc == ARMCPU_ARM9)
			{
                MMU_VRAMWriteBackToLCD(state, (u8)REG_VRAMCNTH) ;
                
                                if((val & 7) == 2)
				{
					state->ARM9Mem->ExtPal[1][0] = state->ARM9Mem->ARM9_LCD + 0x98000;
					state->ARM9Mem->ExtPal[1][1] = state->ARM9Mem->ARM9_LCD + 0x9A000;
					state->ARM9Mem->ExtPal[1][2] = state->ARM9Mem->ARM9_LCD + 0x9C000;
					state->ARM9Mem->ExtPal[1][3] = state->ARM9Mem->ARM9_LCD + 0x9E000;
				}
				
				MMU_VRAMReloadFromLCD(state,adr-REG_VRAMCNTH,val) ;
			}
			break;
			
                case REG_VRAMCNTI  :
			if(proc == ARMCPU_ARM9)
			{
                MMU_VRAMWriteBackToLCD(state,(u8)REG_VRAMCNTI) ;
                
                                if((val & 7) == 3)
				{
					state->ARM9Mem->ObjExtPal[1][0] = state->ARM9Mem->ARM9_LCD + 0xA0000;
					state->ARM9Mem->ObjExtPal[1][1] = state->ARM9Mem->ARM9_LCD + 0xA2000;
				}
				
				MMU_VRAMReloadFromLCD(state,adr-REG_VRAMCNTI,val) ;
			}
			break;

#ifdef LOG_CARD
		case 0x040001A0 : /* TODO (clear): ??? */
		case 0x040001A1 :
		case 0x040001A2 :
		case 0x040001A8 :
		case 0x040001A9 :
		case 0x040001AA :
		case 0x040001AB :
		case 0x040001AC :
		case 0x040001AD :
		case 0x040001AE :
		case 0x040001AF :
                    LOG("%08X : %02X\r\n", adr, val);
#endif
		
		default :
			break;
	}
	
	state->MMU->MMU_MEM[proc][(adr>>20)&0xFF][adr&state->MMU->MMU_MASK[proc][(adr>>20)&0xFF]]=val;
}

void FASTCALL MMU_write16(NDS_state *state, u32 proc, u32 adr, u16 val)
{
#ifdef INTERNAL_DTCM_WRITE
	if((proc == ARMCPU_ARM9) && ((adr & ~0x3FFF) == state->MMU->DTCMRegion))
	{
		/* Writes in DTCM (ARM9 only) */
		T1WriteWord(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF, val);
		return;
	}
#endif
	
	// CFlash writing, Mic
	if ((adr>=0x08800000)&&(adr<0x09900000))
	{
		cflash_write(state,adr,val);
		return;
	}

#ifdef EXPERIMENTAL_WIFI

	/* wifi mac access */
	if ((proc==ARMCPU_ARM7) && (adr>=0x04800000)&&(adr<0x05000000))
	{
		WIFI_write16(&state->wifiMac,adr,val) ;
		return ;
	}
#else
	if ((proc==ARMCPU_ARM7) && (adr>=0x04800000)&&(adr<0x05000000))
		return ;
#endif

	adr &= 0x0FFFFFFF;

        // This is bad, remove it
        if(proc == ARMCPU_ARM7)
        {
           if ((adr>=0x04000400)&&(adr<0x0400051D))
           {
              SPU_WriteWord(state, adr, val);
              return;
           }
        }

	if((adr >> 24) == 4)
	{
		/* Adress is an IO register */
		switch(adr)
		{
#if VIO2SF_GPU_ENABLE
			case 0x0400035C:
			{
				((u16 *)(state->MMU->MMU_MEM[proc][0x40]))[0x35C>>1] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_FogOffset (state->gpu3D, val);
				}
				return;
			}
			case 0x04000340:
			{
				((u16 *)(state->MMU->MMU_MEM[proc][0x40]))[0x340>>1] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_AlphaFunc(state->gpu3D, val);
				}
				return;
			}
			case 0x04000060:
			{
				((u16 *)(state->MMU->MMU_MEM[proc][0x40]))[0x060>>1] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_Control(state->gpu3D, val);
				}
				return;
			}
			case 0x04000354:
			{
				((u16 *)(state->MMU->MMU_MEM[proc][0x40]))[0x354>>1] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_ClearDepth(state->gpu3D, val);
				}
				return;
			}
#endif

			case REG_DISPA_BLDCNT: 	 
				if(proc == ARMCPU_ARM9) GPU_setBLDCNT(state->MainScreen->gpu,val) ;
				break ; 	 
			case REG_DISPB_BLDCNT: 	 
				if(proc == ARMCPU_ARM9) GPU_setBLDCNT(state->SubScreen->gpu,val) ;
				break ; 	 
			case REG_DISPA_BLDALPHA: 	 
				if(proc == ARMCPU_ARM9) GPU_setBLDALPHA(state->MainScreen->gpu,val) ;
				break ; 	 
			case REG_DISPB_BLDALPHA: 	 
				if(proc == ARMCPU_ARM9) GPU_setBLDALPHA(state->SubScreen->gpu,val) ;
				break ; 	 
			case REG_DISPA_BLDY: 	 
				if(proc == ARMCPU_ARM9) GPU_setBLDY_EVY(state->MainScreen->gpu,val) ;
				break ; 	 
			case REG_DISPB_BLDY: 	 
				if(proc == ARMCPU_ARM9) GPU_setBLDY_EVY(state->SubScreen->gpu,val) ;
				break;
			case REG_DISPA_MASTERBRIGHT:
				GPU_setMasterBrightness (state->MainScreen->gpu, val);
				break;
				/*
			case REG_DISPA_MOSAIC: 	 
				if(proc == ARMCPU_ARM9) GPU_setMOSAIC(state->MainScreen->gpu,val) ;
				break ; 	 
			case REG_DISPB_MOSAIC: 	 
				if(proc == ARMCPU_ARM9) GPU_setMOSAIC(state->SubScreen->gpu,val) ;
				break ;
				*/

			case REG_DISPA_WIN0H: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN0_H (state->MainScreen->gpu,val) ;
				break ; 	 
			case REG_DISPA_WIN1H: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN1_H(state->MainScreen->gpu,val) ;
				break ; 	 
			case REG_DISPB_WIN0H: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN0_H(state->SubScreen->gpu,val) ;
				break ; 	 
			case REG_DISPB_WIN1H: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN1_H(state->SubScreen->gpu,val) ;
				break ; 	 
			case REG_DISPA_WIN0V: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN0_V(state->MainScreen->gpu,val) ;
				break ; 	 
			case REG_DISPA_WIN1V: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN1_V(state->MainScreen->gpu,val) ;
				break ; 	 
			case REG_DISPB_WIN0V: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN0_V(state->SubScreen->gpu,val) ;
				break ; 	 
			case REG_DISPB_WIN1V: 	 
				if(proc == ARMCPU_ARM9) GPU_setWIN1_V(state->SubScreen->gpu,val) ;
				break ; 	 
			case REG_DISPA_WININ: 	 
				if(proc == ARMCPU_ARM9) GPU_setWININ(state->MainScreen->gpu, val) ;
				break ; 	 
			case REG_DISPA_WINOUT: 	 
				if(proc == ARMCPU_ARM9) GPU_setWINOUT16(state->MainScreen->gpu, val) ;
				break ; 	 
			case REG_DISPB_WININ: 	 
				if(proc == ARMCPU_ARM9) GPU_setWININ(state->SubScreen->gpu, val) ;
				break ; 	 
			case REG_DISPB_WINOUT: 	 
				if(proc == ARMCPU_ARM9) GPU_setWINOUT16(state->SubScreen->gpu, val) ;
				break ;

			case REG_DISPB_MASTERBRIGHT:
				GPU_setMasterBrightness (state->SubScreen->gpu, val);
				break;
			
            case REG_POWCNT1 :
				if(proc == ARMCPU_ARM9)
				{
					if(val & (1<<15))
					{
						LOG("Main core on top\n");
						state->MainScreen->offset = 0;
						state->SubScreen->offset = 192;
						//nds->swapScreen();
					}
					else
					{
						LOG("Main core on bottom (%04X)\n", val);
						state->MainScreen->offset = 192;
						state->SubScreen->offset = 0;
					}
				}
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x304, val);
				return;

                        case REG_AUXSPICNT:
                                T1WriteWord(state->MMU->MMU_MEM[proc][(REG_AUXSPICNT >> 20) & 0xff], REG_AUXSPICNT & 0xfff, val);
                                state->AUX_SPI_CNT = val;

                                if (val == 0)
                                   mc_reset_com(&state->MMU->bupmem);     /* reset backup memory device communication */
				return;
				
                        case REG_AUXSPIDATA:
                                if(val!=0)
                                {
                                   state->AUX_SPI_CMD = val & 0xFF;
                                }

                                T1WriteWord(state->MMU->MMU_MEM[proc][(REG_AUXSPIDATA >> 20) & 0xff], REG_AUXSPIDATA & 0xfff, bm_transfer(&state->MMU->bupmem, (u8)val));
				return;

			case REG_SPICNT :
				if(proc == ARMCPU_ARM7)
				{
                                  int reset_firmware = 1;

                                  if ( ((state->SPI_CNT >> 8) & 0x3) == 1) {
                                    if ( ((val >> 8) & 0x3) == 1) {
                                      if ( BIT11(state->SPI_CNT)) {
                                        /* select held */
                                        reset_firmware = 0;
                                      }
                                    }
                                  }
					
                                        //MMU->fw.com == 0; /* reset fw device communication */
                                    if ( reset_firmware) {
                                      /* reset fw device communication */
                                      mc_reset_com(&state->MMU->fw);
                                    }
                                    state->SPI_CNT = val;
                                }
				
				T1WriteWord(state->MMU->MMU_MEM[proc][(REG_SPICNT >> 20) & 0xff], REG_SPICNT & 0xfff, val);
				return;
				
			case REG_SPIDATA :
				if(proc==ARMCPU_ARM7)
				{
                                        u16 spicnt;

					if(val!=0)
					{
						state->SPI_CMD = val;
					}
			
                                        spicnt = T1ReadWord(state->MMU->MMU_MEM[proc][(REG_SPICNT >> 20) & 0xff], REG_SPICNT & 0xfff);
					
                                        switch((spicnt >> 8) & 0x3)
					{
                                                case 0 :
							break;
							
                                                case 1 : /* firmware memory device */
                                                        if((spicnt & 0x3) != 0)      /* check SPI baudrate (must be 4mhz) */
							{
								T1WriteWord(state->MMU->MMU_MEM[proc][(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, 0);
								break;
							}
							T1WriteWord(state->MMU->MMU_MEM[proc][(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, fw_transfer(&state->MMU->fw, (u8)val));

							return;
							
                                                case 2 :
							switch(state->SPI_CMD & 0x70)
							{
								case 0x00 :
									val = 0;
									break;
								case 0x10 :
									//execute = FALSE;
									if(state->SPI_CNT&(1<<11))
									{
										if(state->partie)
										{
											val = ((state->nds->touchY<<3)&0x7FF);
											state->partie = 0;
											//execute = FALSE;
											break;
										}
										val = (state->nds->touchY>>5);
                                                                                state->partie = 1;
										break;
									}
									val = ((state->nds->touchY<<3)&0x7FF);
									state->partie = 1;
									break;
								case 0x20 :
									val = 0;
									break;
								case 0x30 :
									val = 0;
									break;
								case 0x40 :
									val = 0;
									break;
								case 0x50 :
                                                                        if(spicnt & 0x800)
									{
										if(state->partie)
										{
											val = ((state->nds->touchX<<3)&0x7FF);
											state->partie = 0;
											break;
										}
										val = (state->nds->touchX>>5);
										state->partie = 1;
										break;
									}
									val = ((state->nds->touchX<<3)&0x7FF);
									state->partie = 1;
									break;
								case 0x60 :
									val = 0;
									break;
								case 0x70 :
									val = 0;
									break;
							}
							break;
							
                                                case 3 :
							/* NOTICE: Device 3 of SPI is reserved (unused and unusable) */
							break;
					}
				}
				
				T1WriteWord(state->MMU->MMU_MEM[proc][(REG_SPIDATA >> 20) & 0xff], REG_SPIDATA & 0xfff, val);
				return;
				
				/* NOTICE: Perhaps we have to use gbatek-like reg names instead of libnds-like ones ...*/
				
                        case REG_DISPA_BG0CNT :
				//GPULOG("MAIN BG0 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(state->MainScreen->gpu, 0, val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x8, val);
				return;
                        case REG_DISPA_BG1CNT :
				//GPULOG("MAIN BG1 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(state->MainScreen->gpu, 1, val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xA, val);
				return;
                        case REG_DISPA_BG2CNT :
				//GPULOG("MAIN BG2 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(state->MainScreen->gpu, 2, val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xC, val);
				return;
                        case REG_DISPA_BG3CNT :
				//GPULOG("MAIN BG3 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(state->MainScreen->gpu, 3, val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xE, val);
				return;
                        case REG_DISPB_BG0CNT :
				//GPULOG("SUB BG0 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(state->SubScreen->gpu, 0, val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x1008, val);
				return;
                        case REG_DISPB_BG1CNT :
				//GPULOG("SUB BG1 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(state->SubScreen->gpu, 1, val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x100A, val);
				return;
                        case REG_DISPB_BG2CNT :
				//GPULOG("SUB BG2 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(state->SubScreen->gpu, 2, val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x100C, val);
				return;
                        case REG_DISPB_BG3CNT :
				//GPULOG("SUB BG3 SETPROP 16B %08X\r\n", val);
				if(proc == ARMCPU_ARM9) GPU_setBGProp(state->SubScreen->gpu, 3, val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x100E, val);
				return;
                        case REG_IME : {
			        u32 old_val = state->MMU->reg_IME[proc];
				u32 new_val = val & 1;
				state->MMU->reg_IME[proc] = new_val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x208, val);
				if ( new_val && old_val != new_val) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( state->MMU->reg_IE[proc] & state->MMU->reg_IF[proc]) {
				    state->NDS_ARM7->wIRQ = TRUE;
				    state->NDS_ARM7->waitIRQ = FALSE;
				  }
				}
				return;
			}
			case REG_VRAMCNTA:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTC:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTE:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTG:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				return ;
			case REG_VRAMCNTI:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				return ;

			case REG_IE :
				state->MMU->reg_IE[proc] = (state->MMU->reg_IE[proc]&0xFFFF0000) | val;
				if ( state->MMU->reg_IME[proc]) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( state->MMU->reg_IE[proc] & state->MMU->reg_IF[proc]) {
				    state->NDS_ARM7->wIRQ = TRUE;
				    state->NDS_ARM7->waitIRQ = FALSE;
				  }
				}
				return;
			case REG_IE + 2 :
				state->execute = FALSE;
				state->MMU->reg_IE[proc] = (state->MMU->reg_IE[proc]&0xFFFF) | (((u32)val)<<16);
				return;
				
			case REG_IF :
				state->execute = FALSE;
				state->MMU->reg_IF[proc] &= (~((u32)val));
				return;
			case REG_IF + 2 :
				state->execute = FALSE;
				state->MMU->reg_IF[proc] &= (~(((u32)val)<<16));
				return;
				
                        case REG_IPCSYNC :
				{
				u32 remote = (proc+1)&1;
				u16 IPCSYNC_remote = T1ReadWord(state->MMU->MMU_MEM[remote][0x40], 0x180);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x180, (val&0xFFF0)|((IPCSYNC_remote>>8)&0xF));
				T1WriteWord(state->MMU->MMU_MEM[remote][0x40], 0x180, (IPCSYNC_remote&0xFFF0)|((val>>8)&0xF));
				state->MMU->reg_IF[remote] |= ((IPCSYNC_remote & (1<<14))<<2) & ((val & (1<<13))<<3);// & (MMU->reg_IME[remote] << 16);// & (MMU->reg_IE[remote] & (1<<16));//
				//execute = FALSE;
				}
				return;
                        case REG_IPCFIFOCNT :
				{
					u32 cnt_l = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x184) ;
					u32 cnt_r = T1ReadWord(state->MMU->MMU_MEM[(proc+1) & 1][0x40], 0x184) ;
					if ((val & 0x8000) && !(cnt_l & 0x8000))
					{
						/* this is the first init, the other side didnt init yet */
						/* so do a complete init */
						FIFOInit(state->MMU->fifos + (IPCFIFO+proc));
						T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184,0x8101) ;
						/* and then handle it as usual */
					}

				if(val & 0x4008)
				{
					FIFOInit(state->MMU->fifos + (IPCFIFO+((proc+1)&1)));
					T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184, (cnt_l & 0x0301) | (val & 0x8404) | 1);
					T1WriteWord(state->MMU->MMU_MEM[proc^1][0x40], 0x184, (cnt_r & 0xC507) | 0x100);
					state->MMU->reg_IF[proc] |= ((val & 4)<<15);// & (MMU->reg_IME[proc]<<17);// & (MMU->reg_IE[proc]&0x20000);//
					return;
				}
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184, T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x184) | (val & 0xBFF4));
				}
				return;
                        case REG_TM0CNTL :
                        case REG_TM1CNTL :
                        case REG_TM2CNTL :
                        case REG_TM3CNTL :
				state->MMU->timerReload[proc][(adr>>2)&3] = val;
				return;
                        case REG_TM0CNTH :
                        case REG_TM1CNTH :
                        case REG_TM2CNTH :
                        case REG_TM3CNTH :
				if(val&0x80)
				{
				  state->MMU->timer[proc][((adr-2)>>2)&0x3] = state->MMU->timerReload[proc][((adr-2)>>2)&0x3];
				}
				state->MMU->timerON[proc][((adr-2)>>2)&0x3] = val & 0x80;
				switch(val&7)
				{
				case 0 :
					state->MMU->timerMODE[proc][((adr-2)>>2)&0x3] = 0+1;//proc;
					break;
				case 1 :
					state->MMU->timerMODE[proc][((adr-2)>>2)&0x3] = 6+1;//proc;
					break;
				case 2 :
					state->MMU->timerMODE[proc][((adr-2)>>2)&0x3] = 8+1;//proc;
					break;
				case 3 :
					state->MMU->timerMODE[proc][((adr-2)>>2)&0x3] = 10+1;//proc;
					break;
				default :
					state->MMU->timerMODE[proc][((adr-2)>>2)&0x3] = 0xFFFF;
					break;
				}
				if(!(val & 0x80))
				state->MMU->timerRUN[proc][((adr-2)>>2)&0x3] = FALSE;
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], adr & 0xFFF, val);
				return;
                        case REG_DISPA_DISPCNT+2 : 
				{
				//execute = FALSE;
				u32 v = (T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0) & 0xFFFF) | ((u32) val << 16);
				GPU_setVideoProp(state->MainScreen->gpu, v);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0, v);
				}
				return;
                        case REG_DISPA_DISPCNT :
				if(proc == ARMCPU_ARM9)
				{
				u32 v = (T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0) & 0xFFFF0000) | val;
				GPU_setVideoProp(state->MainScreen->gpu, v);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0, v);
				}
				return;
                        case REG_DISPA_DISPCAPCNT :
				if(proc == ARMCPU_ARM9)
				{
					GPU_set_DISPCAPCNT(state->MainScreen->gpu,val);
				}
				return;
                        case REG_DISPB_DISPCNT+2 : 
				if(proc == ARMCPU_ARM9)
				{
				//execute = FALSE;
				u32 v = (T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x1000) & 0xFFFF) | ((u32) val << 16);
				GPU_setVideoProp(state->SubScreen->gpu, v);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x1000, v);
				}
				return;
                        case REG_DISPB_DISPCNT :
				{
				u32 v = (T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x1000) & 0xFFFF0000) | val;
				GPU_setVideoProp(state->SubScreen->gpu, v);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x1000, v);
				}
				return;
			//case 0x020D8460 :
			/*case 0x0235A904 :
				LOG("ECRIRE %d %04X\r\n", proc, val);
				execute = FALSE;*/
                                case REG_DMA0CNTH :
				{
                                u32 v;

				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma0 %04X\r\n", val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xBA, val);
				state->DMASrc[proc][0] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xB0);
				state->DMADst[proc][0] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xB4);
                                v = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xB8);
				state->MMU->DMAStartTime[proc][0] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				state->MMU->DMACrt[proc][0] = v;
				if(state->MMU->DMAStartTime[proc][0] == 0)
					MMU_doDMA(state, proc, 0);
				#ifdef LOG_DMA2
				//else
				{
					LOG("proc %d, dma %d src %08X dst %08X %s\r\n", proc, 0, state->DMASrc[proc][0], state->DMADst[proc][0], (val&(1<<25))?"ON":"OFF");
				}
				#endif
				}
				return;
                                case REG_DMA1CNTH :
				{
                                u32 v;
				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma1 %04X\r\n", val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xC6, val);
				state->DMASrc[proc][1] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xBC);
				state->DMADst[proc][1] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xC0);
                                v = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xC4);
				state->MMU->DMAStartTime[proc][1] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				state->MMU->DMACrt[proc][1] = v;
				if(state->MMU->DMAStartTime[proc][1] == 0)
					MMU_doDMA(state, proc, 1);
				#ifdef LOG_DMA2
				//else
				{
					LOG("proc %d, dma %d src %08X dst %08X %s\r\n", proc, 1, state->DMASrc[proc][1], state->DMADst[proc][1], (val&(1<<25))?"ON":"OFF");
				}
				#endif
				}
				return;
                                case REG_DMA2CNTH :
				{
                                u32 v;
				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma2 %04X\r\n", val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xD2, val);
				state->DMASrc[proc][2] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xC8);
				state->DMADst[proc][2] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xCC);
                                v = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xD0);
				state->MMU->DMAStartTime[proc][2] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				state->MMU->DMACrt[proc][2] = v;
				if(state->MMU->DMAStartTime[proc][2] == 0)
					MMU_doDMA(state, proc, 2);
				#ifdef LOG_DMA2
				//else
				{
					LOG("proc %d, dma %d src %08X dst %08X %s\r\n", proc, 2, state->DMASrc[proc][2], state->DMADst[proc][2], (val&(1<<25))?"ON":"OFF");
				}
				#endif
				}
				return;
                                case REG_DMA3CNTH :
				{
                                u32 v;
				//if(val&0x8000) execute = FALSE;
				//LOG("16 bit dma3 %04X\r\n", val);
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0xDE, val);
				state->DMASrc[proc][3] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xD4);
				state->DMADst[proc][3] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xD8);
                                v = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xDC);
				state->MMU->DMAStartTime[proc][3] = (proc ? (v>>28) & 0x3 : (v>>27) & 0x7);
				state->MMU->DMACrt[proc][3] = v;
		
				if(state->MMU->DMAStartTime[proc][3] == 0)
					MMU_doDMA(state, proc, 3);
				#ifdef LOG_DMA2
				//else
				{
					LOG("proc %d, dma %d src %08X dst %08X %s\r\n", proc, 3, state->DMASrc[proc][3], state->DMADst[proc][3], (val&(1<<25))?"ON":"OFF");
				}
				#endif
				}
				return;
                        //case REG_AUXSPICNT : execute = FALSE;
			default :
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], adr&state->MMU->MMU_MASK[proc][(adr>>20)&0xFF], val);
				return;
		}
	}
	T1WriteWord(state->MMU->MMU_MEM[proc][(adr>>20)&0xFF], adr&state->MMU->MMU_MASK[proc][(adr>>20)&0xFF], val);
} 


void FASTCALL MMU_write32(NDS_state *state, u32 proc, u32 adr, u32 val)
{
#ifdef INTERNAL_DTCM_WRITE
	if((proc==ARMCPU_ARM9)&((adr&(~0x3FFF))==state->MMU->DTCMRegion))
	{
		T1WriteLong(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF, val);
		return ;
	}
#endif
	
	// CFlash writing, Mic
	if ((adr>=0x9000000)&&(adr<0x9900000)) {
	   cflash_write(state,adr,val);
	   return;
	}

	adr &= 0x0FFFFFFF;

        // This is bad, remove it
        if(proc == ARMCPU_ARM7)
        {
           if ((adr>=0x04000400)&&(adr<0x0400051D))
           {
              SPU_WriteLong(state, adr, val);
              return;
           }
        }

		if ((adr & 0xFF800000) == 0x04800000) {
		/* access to non regular hw registers */
		/* return to not overwrite valid data */
			return ;
		} ;


	if((adr>>24)==4)
	{
		if (adr >= 0x04000400 && adr < 0x04000440)
		{
			// Geometry commands (aka Dislay Lists) - Parameters:X
			((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x400>>2] = val;
#if VIO2SF_GPU_ENABLE
			if(proc==ARMCPU_ARM9)
			{
				state->gpu3D->NDS_3D_CallList(state->gpu3D, val);
			}
#endif
		}
		else
		switch(adr)
		{
#if VIO2SF_GPU_ENABLE
			// Alpha test reference value - Parameters:1
			case 0x04000340:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x340>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_AlphaFunc(state->gpu3D, val);
				}
				return;
			}
			// Clear background color setup - Parameters:2
			case 0x04000350:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x350>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_ClearColor(state->gpu3D, val);
				}
				return;
			}
			// Clear background depth setup - Parameters:2
			case 0x04000354:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x354>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_ClearDepth(state->gpu3D, val);
				}
				return;
			}
			// Fog Color - Parameters:4b
			case 0x04000358:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x358>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_FogColor(state->gpu3D, val);
				}
				return;
			}
			case 0x0400035C:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x35C>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_FogOffset(state->gpu3D, val);
				}
				return;
			}
			// Matrix mode - Parameters:1
			case 0x04000440:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x440>>2] = val;

				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_MatrixMode(state->gpu3D, val);
				}
				return;
			}
			// Push matrix - Parameters:0
			case 0x04000444:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x444>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_PushMatrix(state->gpu3D);
				}
				return;
			}
			// Pop matrix/es - Parameters:1
			case 0x04000448:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x448>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_PopMatrix(state->gpu3D, val);
				}
				return;
			}
			// Store matrix in the stack - Parameters:1
			case 0x0400044C:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x44C>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_StoreMatrix(state->gpu3D, val);
				}
				return;
			}
			// Restore matrix from the stack - Parameters:1
			case 0x04000450:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x450>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_RestoreMatrix(state->gpu3D, val);
				}
				return;
			}
			// Load Identity matrix - Parameters:0
			case 0x04000454:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x454>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_LoadIdentity(state->gpu3D);
				}
				return;
			}
			// Load 4x4 matrix - Parameters:16
			case 0x04000458:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x458>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_LoadMatrix4x4(state->gpu3D, val);
				}
				return;
			}
			// Load 4x3 matrix - Parameters:12
			case 0x0400045C:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x45C>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_LoadMatrix4x3(state->gpu3D, val);
				}
				return;
			}
			// Multiply 4x4 matrix - Parameters:16
			case 0x04000460:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x460>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_MultMatrix4x4(state->gpu3D, val);
				}
				return;
			}
			// Multiply 4x4 matrix - Parameters:12
			case 0x04000464:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x464>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_MultMatrix4x3(state->gpu3D, val);
				}
				return;
			}
			// Multiply 3x3 matrix - Parameters:9
			case 0x04000468 :
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x468>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_MultMatrix3x3(state->gpu3D, val);
				}
				return;
			}
			// Multiply current matrix by scaling matrix - Parameters:3
			case 0x0400046C:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x46C>>2] = val;
				if(proc==ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_Scale(state->gpu3D, val);
				}
				return;
			}
			// Multiply current matrix by translation matrix - Parameters:3
			case 0x04000470:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x470>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_Translate(state->gpu3D, val);
				}
				return;
			}
			// Set vertex color - Parameters:1
			case 0x04000480:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x480>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_Color3b(state->gpu3D, val);
				}
				return;
			}
			// Set vertex normal - Parameters:1
			case 0x04000484:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x484>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_Normal(state->gpu3D, val);
				}
				return;
			}
			// Set vertex texture coordinate - Parameters:1
			case 0x04000488:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x488>>2] = val;
				if(proc==ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_TexCoord(state->gpu3D, val);
				}
				return;
			}
			// Set vertex position 16b/coordinate - Parameters:2
			case 0x0400048C:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x48C>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_Vertex16b(state->gpu3D, val);
				}
				return;
			}
			// Set vertex position 10b/coordinate - Parameters:1
			case 0x04000490:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x490>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_Vertex10b(state->gpu3D, val);
				}
				return;
			}
			// Set vertex XY position - Parameters:1
			case 0x04000494:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x494>>2] = val;
				if(proc==ARMCPU_ARM9)
				{
                    state->gpu3D->NDS_3D_Vertex3_cord(state->gpu3D,0,1,val);
				}
				return;
			}
			// Set vertex XZ position - Parameters:1
			case 0x04000498:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x498>>2] = val;
				if(proc==ARMCPU_ARM9)
				{
                    state->gpu3D->NDS_3D_Vertex3_cord(state->gpu3D,0,2,val);
				}
				return;
			}
			// Set vertex YZ position - Parameters:1
			case 0x0400049C:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x49C>>2] = val;
				if(proc==ARMCPU_ARM9)
				{
                    state->gpu3D->NDS_3D_Vertex3_cord(state->gpu3D,1,2,val);
				}
				return;
			}
			// Set vertex difference position (offset from the last vertex) - Parameters:1
			case 0x040004A0:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x4A0>>2] = val;
				if(proc==ARMCPU_ARM9)
				{
                    state->gpu3D->NDS_3D_Vertex_rel (state->gpu3D,val);
				}
				return;
			}
			// Set polygon attributes - Parameters:1
			case 0x040004A4:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x4A4>>2] = val;
                if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_PolygonAttrib(state->gpu3D,val);
				}
				return;
			}
			// Set texture parameteres - Parameters:1
			case 0x040004A8:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x4A8>>2] = val;
				if(proc==ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_TexImage(state->gpu3D,val);
				}
				return;
			}
			// Set palette base address - Parameters:1
			case 0x040004AC:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x4AC>>2] = val&0x1FFF;
				if(proc==ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_TexPalette(state->gpu3D, val&0x1FFFF);
				}
				return;
			}
			// Set material diffuse/ambient parameters - Parameters:1
			case 0x040004C0:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x4C0>>2] = val;
                if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_Material0 (state->gpu3D, val);
				}
				return;
			}
			// Set material reflection/emission parameters - Parameters:1
			case 0x040004C4:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x4C4>>2] = val;
                if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_Material1 (state->gpu3D, val);
				}
				return;
			}
			// Light direction vector - Parameters:1
			case 0x040004C8:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x4C8>>2] = val;
                if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_LightDirection (state->gpu3D, val);
				}
				return;
			}
			// Light color - Parameters:1
			case 0x040004CC:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x4CC>>2] = val;
                if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_LightColor(state->gpu3D, val);
				}
				return;
			}
			// Material Shininess - Parameters:32
			case 0x040004D0:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x4D0>>2] = val;
                if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_Shininess(state->gpu3D, val);
				}
				return;
			}
			// Begin vertex list - Parameters:1
			case 0x04000500:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x500>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_Begin(state->gpu3D, val);
				}
				return;
			}
			// End vertex list - Parameters:0
			case 0x04000504:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x504>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_End(state->gpu3D);
				}
				return;
			}
			// Swap rendering engine buffers - Parameters:1
			case 0x04000540:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x540>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_Flush(state->gpu3D, val);
				}
				return;
			}
			// Set viewport coordinates - Parameters:1
			case 0x04000580:
			{
				((u32 *)(state->MMU->MMU_MEM[proc][0x40]))[0x580>>2] = val;
				if(proc == ARMCPU_ARM9)
				{
					state->gpu3D->NDS_3D_ViewPort(state->gpu3D, val);
				}
				return;
			}
#endif

			case REG_DISPA_WININ: 	 
			{
	            if(proc == ARMCPU_ARM9) 	 
	            { 	 
	                    GPU_setWININ	(state->MainScreen->gpu, val & 0xFFFF) ;
	                    GPU_setWINOUT16	(state->MainScreen->gpu, (val >> 16) & 0xFFFF) ;
	            } 	 
	            break;
			}
			case REG_DISPB_WININ:
			{
	            if(proc == ARMCPU_ARM9) 	 
	            { 	 
	                    GPU_setWININ	(state->SubScreen->gpu, val & 0xFFFF) ;
	                    GPU_setWINOUT16	(state->SubScreen->gpu, (val >> 16) & 0xFFFF) ;
	            } 	 
	            break;
			}

			case REG_DISPA_BLDCNT:
			{
				if (proc == ARMCPU_ARM9) 	 
				{ 	 
					GPU_setBLDCNT   (state->MainScreen->gpu,val&0xffff);
					GPU_setBLDALPHA (state->MainScreen->gpu,val>>16);
				}
				break;
			}
			case REG_DISPB_BLDCNT:
			{
				if (proc == ARMCPU_ARM9) 	 
				{ 	 
					GPU_setBLDCNT   (state->SubScreen->gpu,val&0xffff);
					GPU_setBLDALPHA (state->SubScreen->gpu,val>>16);
				}
				break;
			}
/*
 // Commented out, as this doesn't use the plug-in system, neither works
			case cmd_3D_MTX_MODE        // 0x04000440 :
				if (proc == ARMCPU_ARM9) gl_MTX_MODE(val);
				return;
			case cmd_3D_MTX_PUSH        // 0x04000444  :
			case cmd_3D_MTX_POP         // 0x04000448  :
			case cmd_3D_MTX_STORE       // 0x0400044C  :
			case cmd_3D_MTX_RESTORE     // 0x04000450  :
				if (proc == ARMCPU_ARM9) gl_print_cmd(adr);
				return;
			case cmd_3D_MTX_IDENTITY    // 0x04000454  :
				if (proc == ARMCPU_ARM9) gl_MTX_IDENTITY();
				return;
			case cmd_3D_MTX_LOAD_4x4    // 0x04000458  :
				if (proc == ARMCPU_ARM9) gl_MTX_LOAD_4x4(val);
				return;
			case cmd_3D_MTX_LOAD_4x3    // 0x0400045C  :
				if (proc == ARMCPU_ARM9) gl_MTX_LOAD_4x3(val);
				return;
			case cmd_3D_MTX_MULT_4x4    // 0x04000460  :
				if (proc == ARMCPU_ARM9) gl_MTX_MULT_4x4(val);
				return;
			case cmd_3D_MTX_MULT_4x3    // 0x04000464  :
				if (proc == ARMCPU_ARM9) gl_MTX_MULT_4x3(val);
				return;
			case cmd_3D_MTX_MULT_3x3    // 0x04000468  :
				if (proc == ARMCPU_ARM9) gl_MTX_MULT_3x3(val);
				return;
			case cmd_3D_MTX_SCALE       // 0x0400046C  :
			case cmd_3D_MTX_TRANS       // 0x04000470  :
			case cmd_3D_COLOR           // 0x04000480  :
			case cmd_3D_NORMA           // 0x04000484  :
				if (proc == ARMCPU_ARM9) gl_print_cmd(adr);
				return;
			case cmd_3D_TEXCOORD        // 0x04000488  :
				if (proc == ARMCPU_ARM9) gl_TEXCOORD(val);
				return;
			case cmd_3D_VTX_16          // 0x0400048C  :
				if (proc == ARMCPU_ARM9) gl_VTX_16(val);
				return;
			case cmd_3D_VTX_10          // 0x04000490  :
				if (proc == ARMCPU_ARM9) gl_VTX_10(val);
				return;
			case cmd_3D_VTX_XY          // 0x04000494  :
				if (proc == ARMCPU_ARM9) gl_VTX_XY(val);
				return;
			case cmd_3D_VTX_XZ          // 0x04000498  :
				if (proc == ARMCPU_ARM9) gl_VTX_XZ(val);
				return;
			case cmd_3D_VTX_YZ          // 0x0400049C  :
				if (proc == ARMCPU_ARM9) gl_VTX_YZ(val);
				return;
			case cmd_3D_VTX_DIFF        // 0x040004A0  :
				if (proc == ARMCPU_ARM9) gl_VTX_DIFF(val);
				return;
			case cmd_3D_POLYGON_ATTR    // 0x040004A4  :
			case cmd_3D_TEXIMAGE_PARAM  // 0x040004A8  :
			case cmd_3D_PLTT_BASE       // 0x040004AC  :
			case cmd_3D_DIF_AMB         // 0x040004C0  :
			case cmd_3D_SPE_EMI         // 0x040004C4  :
			case cmd_3D_LIGHT_VECTOR    // 0x040004C8  :
			case cmd_3D_LIGHT_COLOR     // 0x040004CC  :
			case cmd_3D_SHININESS       // 0x040004D0  :
				if (proc == ARMCPU_ARM9) gl_print_cmd(adr);
				return;
			case cmd_3D_BEGIN_VTXS      // 0x04000500  :
				if (proc == ARMCPU_ARM9) gl_VTX_begin(val);
				return;
			case cmd_3D_END_VTXS        // 0x04000504  :
				if (proc == ARMCPU_ARM9) gl_VTX_end();
				return;
			case cmd_3D_SWAP_BUFFERS    // 0x04000540  :
			case cmd_3D_VIEWPORT        // 0x04000580  :
			case cmd_3D_BOX_TEST        // 0x040005C0  :
			case cmd_3D_POS_TEST        // 0x040005C4  :
			case cmd_3D_VEC_TEST        // 0x040005C8  :
				if (proc == ARMCPU_ARM9) gl_print_cmd(adr);
				return;
*/
                        case REG_DISPA_DISPCNT :
								if(proc == ARMCPU_ARM9) GPU_setVideoProp(state->MainScreen->gpu, val);
				
				//GPULOG("MAIN INIT 32B %08X\r\n", val);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0, val);
				return;
				
                        case REG_DISPB_DISPCNT : 
				if (proc == ARMCPU_ARM9) GPU_setVideoProp(state->SubScreen->gpu, val);
				//GPULOG("SUB INIT 32B %08X\r\n", val);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x1000, val);
				return;
			case REG_VRAMCNTA:
			case REG_VRAMCNTE:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				MMU_write8(state,proc,adr+1,val >> 8) ;
				MMU_write8(state,proc,adr+2,val >> 16) ;
				MMU_write8(state,proc,adr+3,val >> 24) ;
				return ;
			case REG_VRAMCNTI:
				MMU_write8(state,proc,adr,val & 0xFF) ;
				return ;

                        case REG_IME : {
			        u32 old_val = state->MMU->reg_IME[proc];
				u32 new_val = val & 1;
				state->MMU->reg_IME[proc] = new_val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x208, val);
				if ( new_val && old_val != new_val) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( state->MMU->reg_IE[proc] & state->MMU->reg_IF[proc]) {
				    state->NDS_ARM7->wIRQ = TRUE;
				    state->NDS_ARM7->waitIRQ = FALSE;
				  }
				}
				return;
			}
				
			case REG_IE :
				state->MMU->reg_IE[proc] = val;
				if ( state->MMU->reg_IME[proc]) {
				  /* raise an interrupt request to the CPU if needed */
				  if ( state->MMU->reg_IE[proc] & state->MMU->reg_IF[proc]) {
				    state->NDS_ARM7->wIRQ = TRUE;
				    state->NDS_ARM7->waitIRQ = FALSE;
				  }
				}
				return;
			
			case REG_IF :
				state->MMU->reg_IF[proc] &= (~val);
				return;
                        case REG_TM0CNTL :
                        case REG_TM1CNTL :
                        case REG_TM2CNTL :
                        case REG_TM3CNTL :
				state->MMU->timerReload[proc][(adr>>2)&0x3] = (u16)val;
				if(val&0x800000)
				{
					state->MMU->timer[proc][(adr>>2)&0x3] = state->MMU->timerReload[proc][(adr>>2)&0x3];
				}
				state->MMU->timerON[proc][(adr>>2)&0x3] = val & 0x800000;
				switch((val>>16)&7)
				{
					case 0 :
					state->MMU->timerMODE[proc][(adr>>2)&0x3] = 0+1;//proc;
					break;
					case 1 :
					state->MMU->timerMODE[proc][(adr>>2)&0x3] = 6+1;//proc;
					break;
					case 2 :
					state->MMU->timerMODE[proc][(adr>>2)&0x3] = 8+1;//proc;
					break;
					case 3 :
					state->MMU->timerMODE[proc][(adr>>2)&0x3] = 10+1;//proc;
					break;
					default :
					state->MMU->timerMODE[proc][(adr>>2)&0x3] = 0xFFFF;
					break;
				}
				if(!(val & 0x800000))
				{
					state->MMU->timerRUN[proc][(adr>>2)&0x3] = FALSE;
				}
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], adr & 0xFFF, val);
				return;
                        case REG_DIVDENOM :
				{
                                        u16 cnt;
					s64 num = 0;
					s64 den = 1;
					s64 res;
					s64 mod;
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x298, val);
                                        cnt = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x280);
					switch(cnt&3)
					{
					case 0:
					{
						num = (s64) (s32) T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x290);
						den = (s64) (s32) T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x298);
					}
					break;
					case 1:
					{
						num = (s64) T1ReadQuad(state->MMU->MMU_MEM[proc][0x40], 0x290);
						den = (s64) (s32) T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x298);
					}
					break;
					case 2:
					{
						return;
					}
					break;
					default: 
						break;
					}
					if(den==0)
					{
						res = 0;
						mod = 0;
						cnt |= 0x4000;
						cnt &= 0x7FFF;
					}
					else
					{
						res = num / den;
						mod = num % den;
						cnt &= 0x3FFF;
					}
					DIVLOG("BOUT1 %08X%08X / %08X%08X = %08X%08X\r\n", (u32)(num>>32), (u32)num, 
											(u32)(den>>32), (u32)den, 
											(u32)(res>>32), (u32)res);
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2A0, (u32) res);
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2A4, (u32) (res >> 32));
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2A8, (u32) mod);
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2AC, (u32) (mod >> 32));
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x280, cnt);
				}
				return;
                        case REG_DIVDENOM+4 :
			{
                                u16 cnt;
				s64 num = 0;
				s64 den = 1;
				s64 res;
				s64 mod;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x29C, val);
                                cnt = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x280);
				switch(cnt&3)
				{
				case 0:
				{
					return;
				}
				break;
				case 1:
				{
					return;
				}
				break;
				case 2:
				{
					num = (s64) T1ReadQuad(state->MMU->MMU_MEM[proc][0x40], 0x290);
					den = (s64) T1ReadQuad(state->MMU->MMU_MEM[proc][0x40], 0x298);
				}
				break;
				default: 
					break;
				}
				if(den==0)
				{
					res = 0;
					mod = 0;
					cnt |= 0x4000;
					cnt &= 0x7FFF;
				}
				else
				{
					res = num / den;
					mod = num % den;
					cnt &= 0x3FFF;
				}
				DIVLOG("BOUT2 %08X%08X / %08X%08X = %08X%08X\r\n", (u32)(num>>32), (u32)num, 
										(u32)(den>>32), (u32)den, 
										(u32)(res>>32), (u32)res);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2A0, (u32) res);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2A4, (u32) (res >> 32));
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2A8, (u32) mod);
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2AC, (u32) (mod >> 32));
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x280, cnt);
			}
			return;
                        case REG_SQRTPARAM :
				{
                                        u16 cnt;
					u64 v = 1;
					//execute = FALSE;
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2B8, val);
                                        cnt = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x2B0);
					switch(cnt&1)
					{
					case 0:
						v = (u64) T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x2B8);
						break;
					case 1:
						return;
					}
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2B4, (u32) isqrt64(v));
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2B0, cnt & 0x7FFF);
					SQRTLOG("BOUT1 sqrt(%08X%08X) = %08X\r\n", (u32)(v>>32), (u32)v, 
										T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x2B4));
				}
				return;
                        case REG_SQRTPARAM+4 :
				{
                                        u16 cnt;
					u64 v = 1;
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2BC, val);
                                        cnt = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x2B0);
					switch(cnt&1)
					{
					case 0:
						return;
						//break;
					case 1:
						v = T1ReadQuad(state->MMU->MMU_MEM[proc][0x40], 0x2B8);
						break;
					}
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2B4, (u32) isqrt64(v));
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x2B0, cnt & 0x7FFF);
					SQRTLOG("BOUT2 sqrt(%08X%08X) = %08X\r\n", (u32)(v>>32), (u32)v, 
										T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0x2B4));
				}
				return;
                        case REG_IPCSYNC :
				{
					//execute=FALSE;
					u32 remote = (proc+1)&1;
					u32 IPCSYNC_remote = T1ReadLong(state->MMU->MMU_MEM[remote][0x40], 0x180);
					T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0x180, (val&0xFFF0)|((IPCSYNC_remote>>8)&0xF));
					T1WriteLong(state->MMU->MMU_MEM[remote][0x40], 0x180, (IPCSYNC_remote&0xFFF0)|((val>>8)&0xF));
					state->MMU->reg_IF[remote] |= ((IPCSYNC_remote & (1<<14))<<2) & ((val & (1<<13))<<3);// & (MMU->reg_IME[remote] << 16);// & (MMU->reg_IE[remote] & (1<<16));//
				}
				return;
                        case REG_IPCFIFOCNT :
							{
					u32 cnt_l = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x184) ;
					u32 cnt_r = T1ReadWord(state->MMU->MMU_MEM[(proc+1) & 1][0x40], 0x184) ;
					if ((val & 0x8000) && !(cnt_l & 0x8000))
					{
						/* this is the first init, the other side didnt init yet */
						/* so do a complete init */
						FIFOInit(state->MMU->fifos + (IPCFIFO+proc));
						T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184,0x8101) ;
						/* and then handle it as usual */
					}
				if(val & 0x4008)
				{
					FIFOInit(state->MMU->fifos + (IPCFIFO+((proc+1)&1)));
					T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184, (cnt_l & 0x0301) | (val & 0x8404) | 1);
					T1WriteWord(state->MMU->MMU_MEM[proc^1][0x40], 0x184, (cnt_r & 0xC507) | 0x100);
					state->MMU->reg_IF[proc] |= ((val & 4)<<15);// & (MMU->reg_IME[proc]<<17);// & (MMU->reg_IE[proc]&0x20000);//
					return;
				}
				T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184, val & 0xBFF4);
				//execute = FALSE;
				return;
							}
                        case REG_IPCFIFOSEND :
				{
					u16 IPCFIFO_CNT = T1ReadWord(state->MMU->MMU_MEM[proc][0x40], 0x184);
					if(IPCFIFO_CNT&0x8000)
					{
					//if(val==43) execute = FALSE;
					u32 remote = (proc+1)&1;
					u32 fifonum = IPCFIFO+remote;
                                        u16 IPCFIFO_CNT_remote;
					FIFOAdd(state->MMU->fifos + fifonum, val);
					IPCFIFO_CNT = (IPCFIFO_CNT & 0xFFFC) | (state->MMU->fifos[fifonum].full<<1);
                                        IPCFIFO_CNT_remote = T1ReadWord(state->MMU->MMU_MEM[remote][0x40], 0x184);
					IPCFIFO_CNT_remote = (IPCFIFO_CNT_remote & 0xFCFF) | (state->MMU->fifos[fifonum].full<<10);
					T1WriteWord(state->MMU->MMU_MEM[proc][0x40], 0x184, IPCFIFO_CNT);
					T1WriteWord(state->MMU->MMU_MEM[remote][0x40], 0x184, IPCFIFO_CNT_remote);
					state->MMU->reg_IF[remote] |= ((IPCFIFO_CNT_remote & (1<<10))<<8);// & (MMU->reg_IME[remote] << 18);// & (MMU->reg_IE[remote] & 0x40000);//
					//execute = FALSE;
					}
				}
				return;
			case REG_DMA0CNTL :
				//LOG("32 bit dma0 %04X\r\n", val);
				state->DMASrc[proc][0] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xB0);
				state->DMADst[proc][0] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xB4);
				state->MMU->DMAStartTime[proc][0] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				state->MMU->DMACrt[proc][0] = val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0xB8, val);
				if( state->MMU->DMAStartTime[proc][0] == 0 ||
					state->MMU->DMAStartTime[proc][0] == 7)		// Start Immediately
					MMU_doDMA(state, proc, 0);
				#ifdef LOG_DMA2
				else
				{
					LOG("proc %d, dma %d src %08X dst %08X start taille %d %d\r\n", proc, 0, state->DMASrc[proc][0], state->DMADst[proc][0], 0, ((state->MMU->DMACrt[proc][0]>>27)&7));
				}
				#endif
				//execute = FALSE;
				return;
			case REG_DMA1CNTL:
				//LOG("32 bit dma1 %04X\r\n", val);
				state->DMASrc[proc][1] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xBC);
				state->DMADst[proc][1] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xC0);
				state->MMU->DMAStartTime[proc][1] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				state->MMU->DMACrt[proc][1] = val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0xC4, val);
				if(state->MMU->DMAStartTime[proc][1] == 0 ||
					state->MMU->DMAStartTime[proc][1] == 7)		// Start Immediately
					MMU_doDMA(state, proc, 1);
				#ifdef LOG_DMA2
				else
				{
					LOG("proc %d, dma %d src %08X dst %08X start taille %d %d\r\n", proc, 1, state->DMASrc[proc][1], state->DMADst[proc][1], 0, ((state->MMU->DMACrt[proc][1]>>27)&7));
				}
				#endif
				return;
			case REG_DMA2CNTL :
				//LOG("32 bit dma2 %04X\r\n", val);
				state->DMASrc[proc][2] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xC8);
				state->DMADst[proc][2] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xCC);
				state->MMU->DMAStartTime[proc][2] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				state->MMU->DMACrt[proc][2] = val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0xD0, val);
				if(state->MMU->DMAStartTime[proc][2] == 0 ||
					state->MMU->DMAStartTime[proc][2] == 7)		// Start Immediately
					MMU_doDMA(state, proc, 2);
				#ifdef LOG_DMA2
				else
				{
					LOG("proc %d, dma %d src %08X dst %08X start taille %d %d\r\n", proc, 2, state->DMASrc[proc][2], state->DMADst[proc][2], 0, ((state->MMU->DMACrt[proc][2]>>27)&7));
				}
				#endif
				return;
			case 0x040000DC :
				//LOG("32 bit dma3 %04X\r\n", val);
				state->DMASrc[proc][3] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xD4);
				state->DMADst[proc][3] = T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xD8);
				state->MMU->DMAStartTime[proc][3] = (proc ? (val>>28) & 0x3 : (val>>27) & 0x7);
				state->MMU->DMACrt[proc][3] = val;
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0xDC, val);
				if(	state->MMU->DMAStartTime[proc][3] == 0 ||
					state->MMU->DMAStartTime[proc][3] == 7)		// Start Immediately
					MMU_doDMA(state, proc, 3);
				#ifdef LOG_DMA2
				else
				{
					LOG("proc %d, dma %d src %08X dst %08X start taille %d %d\r\n", proc, 3, state->DMASrc[proc][3], state->DMADst[proc][3], 0, ((state->MMU->DMACrt[proc][3]>>27)&7));
				}
				#endif
				return;
                        case REG_GCROMCTRL :
				{
					int i;

                                        if(MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT) == 0xB7)
					{
                                                state->MMU->dscard[proc].adress = (MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT+1) << 24) | (MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT+2) << 16) | (MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT+3) << 8) | (MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT+4));
						state->MMU->dscard[proc].transfer_count = 0x80;// * ((val>>24)&7));
					}
                                        else if (MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT) == 0xB8)
                                        {
                                                // Get ROM chip ID
                                                val |= 0x800000; // Data-Word Status
                                                T1WriteLong(state->MMU->MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, val);
                                                state->MMU->dscard[proc].adress = 0;
                                        }
					else
					{
                                                LOG("CARD command: %02X\n", MEM_8(state->MMU->MMU_MEM[proc], REG_GCCMDOUT));
					}
					
					//CARDLOG("%08X : %08X %08X\r\n", adr, val, adresse[proc]);
                    val |= 0x00800000;
					
					if(state->MMU->dscard[proc].adress == 0)
					{
                                                val &= ~0x80000000; 
                                                T1WriteLong(state->MMU->MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, val);
						return;
					}
                                        T1WriteLong(state->MMU->MMU_MEM[proc][(REG_GCROMCTRL >> 20) & 0xff], REG_GCROMCTRL & 0xfff, val);
										
					/* launch DMA if start flag was set to "DS Cart" */
					if(proc == ARMCPU_ARM7) i = 2;
					else i = 5;
					
					if(proc == ARMCPU_ARM9 && state->MMU->DMAStartTime[proc][0] == i)	/* dma0/1 on arm7 can't start on ds cart event */
					{
						MMU_doDMA(state, proc, 0);
						return;
					}
					else if(proc == ARMCPU_ARM9 && state->MMU->DMAStartTime[proc][1] == i)
					{
						MMU_doDMA(state, proc, 1);
						return;
					}
					else if(state->MMU->DMAStartTime[proc][2] == i)
					{
						MMU_doDMA(state, proc, 2);
						return;
					}
					else if(state->MMU->DMAStartTime[proc][3] == i)
					{
						MMU_doDMA(state, proc, 3);
						return;
					}
					return;

				}
				return;
								case REG_DISPA_DISPCAPCNT :
				if(proc == ARMCPU_ARM9)
				{
					GPU_set_DISPCAPCNT(state->MainScreen->gpu,val);
					T1WriteLong(state->ARM9Mem->ARM9_REG, 0x64, val);
				}
				return;
				
                        case REG_DISPA_BG0CNT :
				if (proc == ARMCPU_ARM9)
				{
					GPU_setBGProp(state->MainScreen->gpu, 0, (val&0xFFFF));
					GPU_setBGProp(state->MainScreen->gpu, 1, (val>>16));
				}
				//if((val>>16)==0x400) execute = FALSE;
				T1WriteLong(state->ARM9Mem->ARM9_REG, 8, val);
				return;
                        case REG_DISPA_BG2CNT :
				if (proc == ARMCPU_ARM9)
				{
					GPU_setBGProp(state->MainScreen->gpu, 2, (val&0xFFFF));
					GPU_setBGProp(state->MainScreen->gpu, 3, (val>>16));
				}
				T1WriteLong(state->ARM9Mem->ARM9_REG, 0xC, val);
				return;
                        case REG_DISPB_BG0CNT :
				if (proc == ARMCPU_ARM9)
				{
					GPU_setBGProp(state->SubScreen->gpu, 0, (val&0xFFFF));
					GPU_setBGProp(state->SubScreen->gpu, 1, (val>>16));
				}
				T1WriteLong(state->ARM9Mem->ARM9_REG, 0x1008, val);
				return;
                        case REG_DISPB_BG2CNT :
				if (proc == ARMCPU_ARM9)
				{
					GPU_setBGProp(state->SubScreen->gpu, 2, (val&0xFFFF));
					GPU_setBGProp(state->SubScreen->gpu, 3, (val>>16));
				}
				T1WriteLong(state->ARM9Mem->ARM9_REG, 0x100C, val);
				return;
			case REG_DISPA_DISPMMEMFIFO:
			{
				// NOTE: right now, the capture unit is not taken into account,
				//       I don't know is it should be handled here or 
			
				FIFOAdd(state->MMU->fifos + MAIN_MEMORY_DISP_FIFO, val);
				break;
			}
			//case 0x21FDFF0 :  if(val==0) execute = FALSE;
			//case 0x21FDFB0 :  if(val==0) execute = FALSE;
			default :
				T1WriteLong(state->MMU->MMU_MEM[proc][0x40], adr & state->MMU->MMU_MASK[proc][(adr>>20)&0xFF], val);
				return;
		}
	}
	T1WriteLong(state->MMU->MMU_MEM[proc][(adr>>20)&0xFF], adr&state->MMU->MMU_MASK[proc][(adr>>20)&0xFF], val);
}


void FASTCALL MMU_doDMA(NDS_state *state, u32 proc, u32 num)
{
	u32 src = state->DMASrc[proc][num];
	u32 dst = state->DMADst[proc][num];
        u32 taille;

	if(src==dst)
	{
		T1WriteLong(state->MMU->MMU_MEM[proc][0x40], 0xB8 + (0xC*num), T1ReadLong(state->MMU->MMU_MEM[proc][0x40], 0xB8 + (0xC*num)) & 0x7FFFFFFF);
		return;
	}
	
	if((!(state->MMU->DMACrt[proc][num]&(1<<31)))&&(!(state->MMU->DMACrt[proc][num]&(1<<25))))
	{       /* not enabled and not to be repeated */
		state->MMU->DMAStartTime[proc][num] = 0;
		state->MMU->DMACycle[proc][num] = 0;
		//MMU->DMAing[proc][num] = FALSE;
		return;
	}
	
	
	/* word count */
	taille = (state->MMU->DMACrt[proc][num]&0xFFFF);
	
	// If we are in "Main memory display" mode just copy an entire 
	// screen (256x192 pixels). 
	//    Reference:  http://nocash.emubase.de/gbatek.htm#dsvideocaptureandmainmemorydisplaymode
	//       (under DISP_MMEM_FIFO)
	if ((state->MMU->DMAStartTime[proc][num]==4) &&		// Must be in main memory display mode
		(taille==4) &&							// Word must be 4
		(((state->MMU->DMACrt[proc][num]>>26)&1) == 1))	// Transfer mode must be 32bit wide
		taille = 256*192/2;
	
	if(state->MMU->DMAStartTime[proc][num] == 5)
		taille *= 0x80;
	
	state->MMU->DMACycle[proc][num] = taille + state->nds->cycles;
	state->MMU->DMAing[proc][num] = TRUE;
	
	DMALOG("proc %d, dma %d src %08X dst %08X start %d taille %d repeat %s %08X\r\n",
		proc, num, src, dst, state->MMU->DMAStartTime[proc][num], taille,
		(state->MMU->DMACrt[proc][num]&(1<<25))?"on":"off",state->MMU->DMACrt[proc][num]);
	
	if(!(state->MMU->DMACrt[proc][num]&(1<<25)))
		state->MMU->DMAStartTime[proc][num] = 0;
	
	// transfer
	{
		u32 i=0;
		// 32 bit or 16 bit transfer ?
		int sz = ((state->MMU->DMACrt[proc][num]>>26)&1)? 4 : 2;
		int dstinc,srcinc;
		int u=(state->MMU->DMACrt[proc][num]>>21);
		switch(u & 0x3) {
			case 0 :  dstinc =  sz; break;
			case 1 :  dstinc = -sz; break;
			case 2 :  dstinc =   0; break;
			case 3 :  dstinc =  sz; break; //reload
		}
		switch((u >> 2)&0x3) {
			case 0 :  srcinc =  sz; break;
			case 1 :  srcinc = -sz; break;
			case 2 :  srcinc =   0; break;
			case 3 :  // reserved
				return;
		}
		if ((state->MMU->DMACrt[proc][num]>>26)&1)
			for(; i < taille; ++i)
			{
				MMU_write32(state, proc, dst, MMU_read32(state, proc, src));
				dst += dstinc;
				src += srcinc;
			}
		else
			for(; i < taille; ++i)
			{
				MMU_write16(state, proc, dst, MMU_read16(state, proc, src));
				dst += dstinc;
				src += srcinc;
			}
	}
}

#ifdef MMU_ENABLE_ACL

INLINE void check_access(NDS_state *state, u32 adr, u32 access) {
	/* every other mode: sys */
	access |= 1;
	if ((state->NDS_ARM9->CPSR.val & 0x1F) == 0x10) {
		/* is user mode access */
		access ^= 1 ;
	}
	if (armcp15_isAccessAllowed((armcp15_t *)state->NDS_ARM9->coproc[15],adr,access)==FALSE) {
		state->execute = FALSE ;
	}
}
INLINE void check_access_write(NDS_State *state, u32 adr) {
	u32 access = CP15_ACCESS_WRITE;
	check_access(state, adr, access)
}

u8 FASTCALL MMU_read8_acl(NDS_state *state, u32 proc, u32 adr, u32 access)
{
	/* on arm9 we need to check the MPU regions */
	if (proc == ARMCPU_ARM9)
		check_access(state, adr, access);
	return MMU_read8(state,proc,adr);
}
u16 FASTCALL MMU_read16_acl(NDS_state *state, u32 proc, u32 adr, u32 access)
{
	/* on arm9 we need to check the MPU regions */
	if (proc == ARMCPU_ARM9)
		check_access(state, adr, access);
	return MMU_read16(state,proc,adr);
}
u32 FASTCALL MMU_read32_acl(NDS_state *state, u32 proc, u32 adr, u32 access)
{
	/* on arm9 we need to check the MPU regions */
	if (proc == ARMCPU_ARM9)
		check_access(state, adr, access);
	return MMU_read32(state,proc,adr);
}

void FASTCALL MMU_write8_acl(NDS_state *state, u32 proc, u32 adr, u8 val)
{
	/* check MPU region on ARM9 */
	if (proc == ARMCPU_ARM9)
		check_access_write(state,adr);
	MMU_write8(state,proc,adr,val);
}
void FASTCALL MMU_write16_acl(NDS_state *state, u32 proc, u32 adr, u16 val)
{
	/* check MPU region on ARM9 */
	if (proc == ARMCPU_ARM9)
		check_access_write(state,adr);
	MMU_write16(state,proc,adr,val) ;
}
void FASTCALL MMU_write32_acl(NDS_state *state, u32 proc, u32 adr, u32 val)
{
	/* check MPU region on ARM9 */
	if (proc == ARMCPU_ARM9)
		check_access_write(state,adr);
	MMU_write32(state,proc,adr,val) ;
}
#endif



#ifdef PROFILE_MEMORY_ACCESS

#define PROFILE_PREFETCH 0
#define PROFILE_READ 1
#define PROFILE_WRITE 2

struct mem_access_profile {
  u64 num_accesses;
  u32 address_mask;
  u32 masked_value;
};

#define PROFILE_NUM_MEM_ACCESS_PROFILES 4

static u64 profile_num_accesses[2][3];
static u64 profile_unknown_addresses[2][3];
static struct mem_access_profile
profile_memory_accesses[2][3][PROFILE_NUM_MEM_ACCESS_PROFILES];

static void
setup_profiling( void) {
  int i;

  for ( i = 0; i < 2; i++) {
    int access_type;

    for ( access_type = 0; access_type < 3; access_type++) {
      profile_num_accesses[i][access_type] = 0;
      profile_unknown_addresses[i][access_type] = 0;

      /*
       * Setup the access testing structures
       */
      profile_memory_accesses[i][access_type][0].address_mask = 0x0e000000;
      profile_memory_accesses[i][access_type][0].masked_value = 0x00000000;
      profile_memory_accesses[i][access_type][0].num_accesses = 0;

      /* main memory */
      profile_memory_accesses[i][access_type][1].address_mask = 0x0f000000;
      profile_memory_accesses[i][access_type][1].masked_value = 0x02000000;
      profile_memory_accesses[i][access_type][1].num_accesses = 0;

      /* shared memory */
      profile_memory_accesses[i][access_type][2].address_mask = 0x0f800000;
      profile_memory_accesses[i][access_type][2].masked_value = 0x03000000;
      profile_memory_accesses[i][access_type][2].num_accesses = 0;

      /* arm7 memory */
      profile_memory_accesses[i][access_type][3].address_mask = 0x0f800000;
      profile_memory_accesses[i][access_type][3].masked_value = 0x03800000;
      profile_memory_accesses[i][access_type][3].num_accesses = 0;
    }
  }
}

static void
profile_memory_access( int arm9, u32 adr, int access_type) {
  static int first = 1;
  int mem_profile;
  int address_found = 0;

  if ( first) {
    setup_profiling();
    first = 0;
  }

  profile_num_accesses[arm9][access_type] += 1;

  for ( mem_profile = 0;
        mem_profile < PROFILE_NUM_MEM_ACCESS_PROFILES &&
          !address_found;
        mem_profile++) {
    if ( (adr & profile_memory_accesses[arm9][access_type][mem_profile].address_mask) ==
         profile_memory_accesses[arm9][access_type][mem_profile].masked_value) {
      /*printf( "adr %08x mask %08x res %08x expected %08x\n",
              adr,
              profile_memory_accesses[arm9][access_type][mem_profile].address_mask,
              adr & profile_memory_accesses[arm9][access_type][mem_profile].address_mask,
              profile_memory_accesses[arm9][access_type][mem_profile].masked_value);*/
      address_found = 1;
      profile_memory_accesses[arm9][access_type][mem_profile].num_accesses += 1;
    }
  }

  if ( !address_found) {
    profile_unknown_addresses[arm9][access_type] += 1;
  }
}


static const char *access_type_strings[] = {
  "prefetch",
  "read    ",
  "write   "
};

void
print_memory_profiling( void) {
  int arm;

  printf("------ Memory access profile ------\n");

  for ( arm = 0; arm < 2; arm++) {
    int access_type;

    for ( access_type = 0; access_type < 3; access_type++) {
      int mem_profile;
      printf("ARM%c: num of %s %lld\n",
             arm ? '9' : '7',
             access_type_strings[access_type],
             profile_num_accesses[arm][access_type]);

      for ( mem_profile = 0;
            mem_profile < PROFILE_NUM_MEM_ACCESS_PROFILES;
            mem_profile++) {
        printf( "address %08x: %lld\n",
                profile_memory_accesses[arm][access_type][mem_profile].masked_value,
                profile_memory_accesses[arm][access_type][mem_profile].num_accesses);
      }
              
      printf( "unknown addresses %lld\n",
              profile_unknown_addresses[arm][access_type]);

      printf( "\n");
    }
  }

  printf("------ End of Memory access profile ------\n\n");
}
#else
void
print_memory_profiling( void) {
}
#endif /* End of PROFILE_MEMORY_ACCESS area */

#ifdef GDB_STUB
static u16 FASTCALL
arm9_prefetch16( NDS_state *state, void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_PREFETCH);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if((adr & ~0x3FFF) == state->MMU->DTCMRegion)
    {
      /* Returns data from DTCM (ARM9 only) */
      return T1ReadWord(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF);
    }
  /* access to main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    return T1ReadWord( state->MMU->MMU_MEM[ARMCPU_ARM9][(adr >> 20) & 0xFF],
                       adr & state->MMU->MMU_MASK[ARMCPU_ARM9][(adr >> 20) & 0xFF]);
  }
#endif

  return MMU_read16( state, ARMCPU_ARM9, adr);
}
static u32 FASTCALL
arm9_prefetch32( NDS_state *state, void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_PREFETCH);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if((adr & ~0x3FFF) == state->MMU->DTCMRegion)
    {
      /* Returns data from DTCM (ARM9 only) */
      return T1ReadLong(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF);
    }
  /* access to main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    return T1ReadLong( state->MMU->MMU_MEM[ARMCPU_ARM9][(adr >> 20) & 0xFF],
                       adr & state->MMU->MMU_MASK[ARMCPU_ARM9][(adr >> 20) & 0xFF]);
  }
#endif

  return MMU_read32( state, ARMCPU_ARM9, adr);
}

static u8 FASTCALL
arm9_read8( NDS_state *state, void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_READ);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if( (adr&(~0x3FFF)) == state->MMU->DTCMRegion)
    {
      return state->ARM9Mem->ARM9_DTCM[adr&0x3FFF];
    }
  /* access to main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    return state->MMU->MMU_MEM[ARMCPU_ARM9][(adr >> 20) & 0xFF]
      [adr & state->MMU->MMU_MASK[ARMCPU_ARM9][(adr >> 20) & 0xFF]];
  }
#endif

  return MMU_read8( state, ARMCPU_ARM9, adr);
}
static u16 FASTCALL
arm9_read16( NDS_state *state, void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_READ);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if((adr & ~0x3FFF) == state->MMU->DTCMRegion)
    {
      /* Returns data from DTCM (ARM9 only) */
      return T1ReadWord(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF);
    }

  /* access to main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    return T1ReadWord( state->MMU->MMU_MEM[ARMCPU_ARM9][(adr >> 20) & 0xFF],
                       adr & state->MMU->MMU_MASK[ARMCPU_ARM9][(adr >> 20) & 0xFF]);
  }
#endif

  return MMU_read16( state, ARMCPU_ARM9, adr);
}
static u32 FASTCALL
arm9_read32( NDS_state *state, void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_READ);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if((adr & ~0x3FFF) == state->MMU->DTCMRegion)
    {
      /* Returns data from DTCM (ARM9 only) */
      return T1ReadLong(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF);
    }
  /* access to main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    return T1ReadLong( state->MMU->MMU_MEM[ARMCPU_ARM9][(adr >> 20) & 0xFF],
                       adr & state->MMU->MMU_MASK[ARMCPU_ARM9][(adr >> 20) & 0xFF]);
  }
#endif

  return MMU_read32( state, ARMCPU_ARM9, adr);
}


static void FASTCALL
arm9_write8(NDS_state *state, void *data, u32 adr, u8 val) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_WRITE);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if( (adr & ~0x3FFF) == state->MMU->DTCMRegion)
    {
      /* Writes data in DTCM (ARM9 only) */
      state->ARM9Mem->ARM9_DTCM[adr&0x3FFF] = val;
      return ;
    }
  /* main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    state->MMU->MMU_MEM[ARMCPU_ARM9][(adr>>20)&0xFF]
      [adr&state->MMU->MMU_MASK[ARMCPU_ARM9][(adr>>20)&0xFF]] = val;
    return;
  }
#endif

  MMU_write8( state, ARMCPU_ARM9, adr, val);
}
static void FASTCALL
arm9_write16(NDS_state *state, void *data, u32 adr, u16 val) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_WRITE);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if((adr & ~0x3FFF) == state->MMU->DTCMRegion)
    {
      /* Writes in DTCM (ARM9 only) */
      T1WriteWord(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF, val);
      return;
    }
  /* main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    T1WriteWord( state->MMU->MMU_MEM[ARMCPU_ARM9][(adr>>20)&0xFF],
                 adr&state->MMU->MMU_MASK[ARMCPU_ARM9][(adr>>20)&0xFF], val);
    return;
  }
#endif

  MMU_write16( state, ARMCPU_ARM9, adr, val);
}
static void FASTCALL
arm9_write32(NDS_state *state, void *data, u32 adr, u32 val) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 1, adr, PROFILE_WRITE);
#endif

#ifdef EARLY_MEMORY_ACCESS
  if((adr & ~0x3FFF) == state->MMU->DTCMRegion)
    {
      /* Writes in DTCM (ARM9 only) */
      T1WriteLong(state->ARM9Mem->ARM9_DTCM, adr & 0x3FFF, val);
      return;
    }
  /* main memory */
  if ( (adr & 0x0f000000) == 0x02000000) {
    T1WriteLong( state->MMU->MMU_MEM[ARMCPU_ARM9][(adr>>20)&0xFF],
                 adr&state->MMU->MMU_MASK[ARMCPU_ARM9][(adr>>20)&0xFF], val);
    return;
  }
#endif

  MMU_write32( state, ARMCPU_ARM9, adr, val);
}




static u16 FASTCALL
arm7_prefetch16( NDS_state *state, void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_PREFETCH);
#endif

#ifdef EARLY_MEMORY_ACCESS
  /* ARM7 private memory */
  if ( (adr & 0x0f800000) == 0x03800000) {
    T1ReadWord(state->MMU->MMU_MEM[ARMCPU_ARM7][(adr >> 20) & 0xFF],
               adr & state->MMU->MMU_MASK[ARMCPU_ARM7][(adr >> 20) & 0xFF]);
  }
#endif

  return MMU_read16( state, ARMCPU_ARM7, adr);
}
static u32 FASTCALL
arm7_prefetch32( NDS_state *state, void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_PREFETCH);
#endif

#ifdef EARLY_MEMORY_ACCESS
  /* ARM7 private memory */
  if ( (adr & 0x0f800000) == 0x03800000) {
    T1ReadLong(state->MMU->MMU_MEM[ARMCPU_ARM7][(adr >> 20) & 0xFF],
               adr & state->MMU->MMU_MASK[ARMCPU_ARM7][(adr >> 20) & 0xFF]);
  }
#endif

  return MMU_read32( state, ARMCPU_ARM7, adr);
}

static u8 FASTCALL
arm7_read8( NDS_state *state, void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_READ);
#endif

  return MMU_read8( state, ARMCPU_ARM7, adr);
}
static u16 FASTCALL
arm7_read16( NDS_state *state, void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_READ);
#endif

  return MMU_read16( state, ARMCPU_ARM7, adr);
}
static u32 FASTCALL
arm7_read32( NDS_state *state, void *data, u32 adr) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_READ);
#endif

  return MMU_read32( state, ARMCPU_ARM7, adr);
}


static void FASTCALL
arm7_write8(NDS_state *state, void *data, u32 adr, u8 val) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_WRITE);
#endif

  MMU_write8( state, ARMCPU_ARM7, adr, val);
}
static void FASTCALL
arm7_write16(NDS_state *state, void *data, u32 adr, u16 val) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_WRITE);
#endif

  MMU_write16( state, ARMCPU_ARM7, adr, val);
}
static void FASTCALL
arm7_write32(NDS_state *state, void *data, u32 adr, u32 val) {
#ifdef PROFILE_MEMORY_ACCESS
  profile_memory_access( 0, adr, PROFILE_WRITE);
#endif

  MMU_write32( state, ARMCPU_ARM7, adr, val);
}


/*
 * the base memory interfaces
 */
struct armcpu_memory_iface arm9_base_memory_iface = {
#ifdef __GNUC__
  .prefetch32 = arm9_prefetch32,
  .prefetch16 = arm9_prefetch16,

  .read8 = arm9_read8,
  .read16 = arm9_read16,
  .read32 = arm9_read32,

  .write8 = arm9_write8,
  .write16 = arm9_write16,
  .write32 = arm9_write32
#else
  arm9_prefetch32,
  arm9_prefetch16,

  arm9_read8,
  arm9_read16,
  arm9_read32,

  arm9_write8,
  arm9_write16,
  arm9_write32
#endif
};

struct armcpu_memory_iface arm7_base_memory_iface = {
#ifdef __GNUC__
  .prefetch32 = arm7_prefetch32,
  .prefetch16 = arm7_prefetch16,

  .read8 = arm7_read8,
  .read16 = arm7_read16,
  .read32 = arm7_read32,

  .write8 = arm7_write8,
  .write16 = arm7_write16,
  .write32 = arm7_write32
#else
  arm7_prefetch32,
  arm7_prefetch16,

  arm7_read8,
  arm7_read16,
  arm7_read32,

  arm7_write8,
  arm7_write16,
  arm7_write32
#endif
};

/*
 * The direct memory interface for the ARM9.
 * This avoids the ARM9 protection unit when accessing
 * memory.
 */
struct armcpu_memory_iface arm9_direct_memory_iface = {
#ifdef __GNUC__
  /* the prefetch is not used */
  .prefetch32 = NULL,
  .prefetch16 = NULL,

  .read8 = arm9_read8,
  .read16 = arm9_read16,
  .read32 = arm9_read32,

  .write8 = arm9_write8,
  .write16 = arm9_write16,
  .write32 = arm9_write32
#else
  NULL,
  NULL,

  arm9_read8,
  arm9_read16,
  arm9_read32,

  arm9_write8,
  arm9_write16,
  arm9_write32
#endif
};
#endif
