/*  Copyright (C) 2006 yopyop
    yopyop156@ifrance.com
    yopyop156.ifrance.com

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

#ifndef NDSSYSTEM_H
#define NDSSYSTEM_H

#include "armcpu.h"
#include "MMU.h"

#include "GPU.h"

#include "mem.h"
//#include "wifi.h"

#include "ARM9.h"

#include "state.h"

#ifdef __cplusplus
extern "C" {
#endif


extern volatile BOOL execute;
extern BOOL click;

/*
 * The firmware language values
 */
#define NDS_FW_LANG_JAP 0
#define NDS_FW_LANG_ENG 1
#define NDS_FW_LANG_FRE 2
#define NDS_FW_LANG_GER 3
#define NDS_FW_LANG_ITA 4
#define NDS_FW_LANG_SPA 5
#define NDS_FW_LANG_CHI 6
#define NDS_FW_LANG_RES 7


//#define LOG_ARM9
//#define LOG_ARM7

typedef struct
{
       char     gameTile[12];
       char     gameCode[4];
       u16      makerCode;
       u8       unitCode;
       u8       deviceCode;
       u8       cardSize;
       u8       cardInfo[8];
       u8       flags;
       
       u32      ARM9src;
       u32      ARM9exe;
       u32      ARM9cpy;
       u32      ARM9binSize;
       
       u32      ARM7src;
       u32      ARM7exe;
       u32      ARM7cpy;
       u32      ARM7binSize;
       
       u32      FNameTblOff;
       u32      FNameTblSize;
       
       u32      FATOff;
       u32      FATSize;
       
       u32     ARM9OverlayOff;
       u32     ARM9OverlaySize;
       u32     ARM7OverlayOff;
       u32     ARM7OverlaySize;
       
       u32     unknown2a;
       u32     unknown2b;
       
       u32     IconOff;
       u16     CRC16;
       u16     ROMtimeout;
       u32     ARM9unk;
       u32     ARM7unk;
       
       u8      unknown3c[8];
       u32     ROMSize;
       u32     HeaderSize;
       u8      unknown5[56];
       u8      logo[156];
       u16     logoCRC16;
       u16     headerCRC16;
       u8      reserved[160];
} NDS_header;

typedef struct NDSSystem
{
       s32 ARM9Cycle;
       s32 ARM7Cycle;
       s32 cycles;
       s32 timerCycle[2][4];
       BOOL timerOver[2][4];
       s32 nextHBlank;
       u32 VCount;
       u32 old;
       s32 diff;
       BOOL lignerendu;
       
       u16 touchX;
       u16 touchY;
} NDSSystem;

/** /brief A touchscreen calibration point.
 */
struct NDS_fw_touchscreen_cal {
  u16 adc_x;
  u16 adc_y;

  u8 screen_x;
  u8 screen_y;
};

/** /brief The type of DS
 */
enum nds_fw_ds_type {
  NDS_FW_DS_TYPE_FAT,
  NDS_FW_DS_TYPE_LITE
};

#define MAX_FW_NICKNAME_LENGTH 10
#define MAX_FW_MESSAGE_LENGTH 26

struct NDS_fw_config_data {
  enum nds_fw_ds_type ds_type;

  u8 fav_colour;
  u8 birth_month;
  u8 birth_day;

  u16 nickname[MAX_FW_NICKNAME_LENGTH];
  u8 nickname_len;

  u16 message[MAX_FW_MESSAGE_LENGTH];
  u8 message_len;

  u8 language;

  /* touchscreen calibration */
  struct NDS_fw_touchscreen_cal touch_cal[2];
};

#ifdef GDB_STUB
int NDS_Init( NDS_state *,
              struct armcpu_memory_iface *arm9_mem_if,
              struct armcpu_ctrl_iface **arm9_ctrl_iface,
              struct armcpu_memory_iface *arm7_mem_if,
              struct armcpu_ctrl_iface **arm7_ctrl_iface);
#else
int NDS_Init ( NDS_state * );
#endif

void NDS_DeInit(NDS_state *);
void
NDS_FillDefaultFirmwareConfigData( NDS_state *, struct NDS_fw_config_data *fw_config);

BOOL NDS_SetROM(NDS_state *, u8 * rom, u32 mask);
NDS_header * NDS_getROMHeader(NDS_state *);
 
void NDS_setTouchPos(NDS_state *, u16 x, u16 y);
void NDS_releasTouch(NDS_state *);

void NDS_FreeROM(NDS_state *);
void NDS_Reset(NDS_state *);

int NDS_CreateDummyFirmware( NDS_state *, struct NDS_fw_config_data *user_settings);
u32
NDS_exec(NDS_state *, s32 nb, BOOL force);

       static INLINE void NDS_ARM9HBlankInt(NDS_state *state)
       {
            if(T1ReadWord(state->ARM9Mem->ARM9_REG, 4) & 0x10)
            {
                 state->MMU->reg_IF[0] |= 2;// & (MMU->reg_IME[0] << 1);// (MMU->reg_IE[0] & (1<<1));
                 state->NDS_ARM9->wIRQ = TRUE;
            }
       }
       
       static INLINE void NDS_ARM7HBlankInt(NDS_state *state)
       {
            if(T1ReadWord(state->MMU->ARM7_REG, 4) & 0x10)
            {
                 state->MMU->reg_IF[1] |= 2;// & (MMU->reg_IME[1] << 1);// (MMU->reg_IE[1] & (1<<1));
                 state->NDS_ARM7->wIRQ = TRUE;
            }
       }
       
       static INLINE void NDS_ARM9VBlankInt(NDS_state *state)
       {
            if(T1ReadWord(state->ARM9Mem->ARM9_REG, 4) & 0x8)
            {
                 state->MMU->reg_IF[0] |= 1;// & (MMU->reg_IME[0]);// (MMU->reg_IE[0] & 1);
                 state->NDS_ARM9->wIRQ = TRUE;
                      //execute = FALSE;
                      /*logcount++;*/
            }
       }
       
       static INLINE void NDS_ARM7VBlankInt(NDS_state *state)
       {
            if(T1ReadWord(state->MMU->ARM7_REG, 4) & 0x8)
                 state->MMU->reg_IF[1] |= 1;// & (MMU->reg_IME[1]);// (MMU->reg_IE[1] & 1);
                 state->NDS_ARM7->wIRQ = TRUE;
                 //execute = FALSE;
       }
       
       static INLINE void NDS_swapScreen(NDS_state *state)
       {
	       u16 tmp = state->MainScreen->offset;
	       state->MainScreen->offset = state->SubScreen->offset;
	       state->SubScreen->offset = tmp;
       }
 
 

void NDS_exec_frame(NDS_state *, int cpu_clockdown_level_arm9, int cpu_clockdown_level_arm7);
void NDS_exec_hframe(NDS_state *, int cpu_clockdown_level_arm9, int cpu_clockdown_level_arm7);

#ifdef __cplusplus
}
#endif

#endif

 	  	 
