/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - exception.c                                             *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2002 Hacktarux                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "memory/memory.h"

#include "exception.h"
#include "r4300.h"
#include "cp0.h"
#include "recomph.h"
#include "tlb.h"

void TLB_refill_exception(usf_state_t * state, unsigned int address, int w)
{
   int usual_handler = 0, i;

   if (state->r4300emu != CORE_DYNAREC && w != 2) update_count(state);
   if (w == 1) state->g_cp0_regs[CP0_CAUSE_REG] = (3 << 2);
   else state->g_cp0_regs[CP0_CAUSE_REG] = (2 << 2);
   state->g_cp0_regs[CP0_BADVADDR_REG] = address;
   state->g_cp0_regs[CP0_CONTEXT_REG] = (state->g_cp0_regs[CP0_CONTEXT_REG] & 0xFF80000F) | ((address >> 9) & 0x007FFFF0);
   state->g_cp0_regs[CP0_ENTRYHI_REG] = address & 0xFFFFE000;
   if (state->g_cp0_regs[CP0_STATUS_REG] & 0x2) // Test de EXL
     {
    generic_jump_to(state, 0x80000180);
    if(state->delay_slot==1 || state->delay_slot==3) state->g_cp0_regs[CP0_CAUSE_REG] |= 0x80000000;
    else state->g_cp0_regs[CP0_CAUSE_REG] &= 0x7FFFFFFF;
     }
   else
     {
    if (state->r4300emu != CORE_PURE_INTERPRETER)
      {
         if (w!=2)
           state->g_cp0_regs[CP0_EPC_REG] = state->PC->addr;
         else
           state->g_cp0_regs[CP0_EPC_REG] = address;
      }
    else state->g_cp0_regs[CP0_EPC_REG] = state->PC->addr;
         
    state->g_cp0_regs[CP0_CAUSE_REG] &= ~0x80000000;
    state->g_cp0_regs[CP0_STATUS_REG] |= 0x2; //EXL=1
    
    if (address >= 0x80000000 && address < 0xc0000000)
      usual_handler = 1;
    for (i=0; i<32; i++)
      {
         if (/*state->tlb_e[i].v_even &&*/ address >= state->tlb_e[i].start_even &&
         address <= state->tlb_e[i].end_even)
           usual_handler = 1;
         if (/*state->tlb_e[i].v_odd &&*/ address >= state->tlb_e[i].start_odd &&
         address <= state->tlb_e[i].end_odd)
           usual_handler = 1;
      }
    if (usual_handler)
      {
         generic_jump_to(state, 0x80000180);
      }
    else
      {
         generic_jump_to(state, 0x80000000);
      }
     }
   if(state->delay_slot==1 || state->delay_slot==3)
     {
    state->g_cp0_regs[CP0_CAUSE_REG] |= 0x80000000;
    state->g_cp0_regs[CP0_EPC_REG]-=4;
     }
   else
     {
    state->g_cp0_regs[CP0_CAUSE_REG] &= 0x7FFFFFFF;
     }
   if(w != 2) state->g_cp0_regs[CP0_EPC_REG]-=4;
   
   state->last_addr = state->PC->addr;
   
#ifdef DYNAREC
   if (state->r4300emu == CORE_DYNAREC)
     {
    dyna_jump(state);
    if (!state->dyna_interp) state->delay_slot = 0;
     }
#endif
   
   if (state->r4300emu != CORE_DYNAREC || state->dyna_interp)
     {
    state->dyna_interp = 0;
    if (state->delay_slot)
      {
         state->skip_jump = state->PC->addr;
         state->next_interupt = 0;
         state->cycle_count = 0;
      }
     }
}

void osal_fastcall exception_general(usf_state_t * state)
{
   update_count(state);
   state->g_cp0_regs[CP0_STATUS_REG] |= 2;
   
   state->g_cp0_regs[CP0_EPC_REG] = state->PC->addr;
   
   if(state->delay_slot==1 || state->delay_slot==3)
     {
    state->g_cp0_regs[CP0_CAUSE_REG] |= 0x80000000;
    state->g_cp0_regs[CP0_EPC_REG]-=4;
     }
   else
     {
    state->g_cp0_regs[CP0_CAUSE_REG] &= 0x7FFFFFFF;
     }
   generic_jump_to(state, 0x80000180);
   state->last_addr = state->PC->addr;
#ifdef DYNAREC
   if (state->r4300emu == CORE_DYNAREC)
     {
    dyna_jump(state);
    if (!state->dyna_interp) state->delay_slot = 0;
     }
#endif
   if (state->r4300emu != CORE_DYNAREC || state->dyna_interp)
     {
    state->dyna_interp = 0;
    if (state->delay_slot)
      {
         state->skip_jump = state->PC->addr;
         state->next_interupt = 0;
         state->cycle_count = 0;
      }
     }
}

