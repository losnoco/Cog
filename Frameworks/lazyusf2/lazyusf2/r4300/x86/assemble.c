/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - assemble.c                                              *
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

#include <stdlib.h>
#include <stdio.h>

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "assemble.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "osal/preproc.h"
#include "r4300/recomph.h"
#include "r4300/recomp.h"
#include "r4300/r4300.h"

void init_assembler(usf_state_t * state, void *block_jumps_table, int block_jumps_number, void *block_riprel_table, int block_riprel_number)
{
   if (block_jumps_table)
   {
     state->jumps_table = (jump_table *) block_jumps_table;
     state->jumps_number = block_jumps_number;
     state->max_jumps_number = state->jumps_number;
   }
   else
   {
     state->jumps_table = (jump_table *) malloc(1000*sizeof(jump_table));
     state->jumps_number = 0;
     state->max_jumps_number = 1000;
   }
}

void free_assembler(usf_state_t * state, void **block_jumps_table, int *block_jumps_number, void **block_riprel_table, int *block_riprel_number)
{
   *block_jumps_table = state->jumps_table;
   *block_jumps_number = state->jumps_number;
   *block_riprel_table = NULL;  /* RIP-relative addressing is only for x86-64 */
   *block_riprel_number = 0;
}

void add_jump(usf_state_t * state, unsigned int pc_addr, unsigned int mi_addr)
{
   if (state->jumps_number == state->max_jumps_number)
   {
     state->max_jumps_number += 1000;
     state->jumps_table = (jump_table *) realloc(state->jumps_table, state->max_jumps_number*sizeof(jump_table));
   }
   state->jumps_table[state->jumps_number].pc_addr = pc_addr;
   state->jumps_table[state->jumps_number].mi_addr = mi_addr;
   state->jumps_number++;
}

void passe2(usf_state_t * state, precomp_instr *dest, int start, int end, precomp_block *block)
{
   unsigned int real_code_length, addr_dest;
   int i;
   build_wrappers(state, dest, start, end, block);
   real_code_length = state->code_length;
   
   for (i=0; i < state->jumps_number; i++)
   {
     state->code_length = state->jumps_table[i].pc_addr;
     if (dest[(state->jumps_table[i].mi_addr - dest[0].addr)/4].reg_cache_infos.need_map)
     {
       addr_dest = (unsigned int)dest[(state->jumps_table[i].mi_addr - dest[0].addr)/4].reg_cache_infos.jump_wrapper;
       put32(state, addr_dest-((unsigned int)block->code+state->code_length)-4);
     }
     else
     {
       addr_dest = dest[(state->jumps_table[i].mi_addr - dest[0].addr)/4].local_addr;
       put32(state, addr_dest-state->code_length-4);
     }
   }
   state->code_length = real_code_length;
}

void jump_start_rel8(usf_state_t * state)
{
  state->g_jump_start8 = state->code_length;
}

void jump_start_rel32(usf_state_t * state)
{
  state->g_jump_start32 = state->code_length;
}

void jump_end_rel8(usf_state_t * state)
{
  unsigned int jump_end = state->code_length;
  int jump_vec = jump_end - state->g_jump_start8;

  if (jump_vec > 127 || jump_vec < -128)
  {
    DebugMessage(state, M64MSG_ERROR, "8-bit relative jump too long! From %x to %x", state->g_jump_start8, jump_end);
    OSAL_BREAKPOINT_INTERRUPT;
  }

  state->code_length = state->g_jump_start8 - 1;
  put8(state, jump_vec);
  state->code_length = jump_end;
}

void jump_end_rel32(usf_state_t * state)
{
  unsigned int jump_end = state->code_length;
  int jump_vec = jump_end - state->g_jump_start32;

  state->code_length = state->g_jump_start32 - 4;
  put32(state, jump_vec);
  state->code_length = jump_end;
}
