/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - gcop1.c                                                 *
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

#include <stdio.h>

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "assemble.h"
#include "interpret.h"

#include "r4300/recomph.h"
#include "r4300/recomp.h"
#include "r4300/r4300.h"
#include "r4300/ops.h"
#include "r4300/macros.h"
#include "r4300/cp1.h"

#include "memory/memory.h"

void genmfc1(usf_state_t * state)
{
#ifdef INTERPRET_MFC1
   gencallinterp(state, (unsigned int)state->current_instruction_table.MFC1, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int*)(&state->reg_cop1_simple[state->dst->f.r.nrd]));
   mov_reg32_preg32(state, EBX, EAX);
   mov_m32_reg32(state, (unsigned int*)state->dst->f.r.rt, EBX);
   sar_reg32_imm8(state, EBX, 31);
   mov_m32_reg32(state, ((unsigned int*)state->dst->f.r.rt)+1, EBX);
#endif
}

void gendmfc1(usf_state_t * state)
{
#ifdef INTERPRET_DMFC1
   gencallinterp(state, (unsigned int)state->current_instruction_table.DMFC1, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int*)(&state->reg_cop1_double[state->dst->f.r.nrd]));
   mov_reg32_preg32(state, EBX, EAX);
   mov_reg32_preg32pimm32(state, ECX, EAX, 4);
   mov_m32_reg32(state, (unsigned int*)state->dst->f.r.rt, EBX);
   mov_m32_reg32(state, ((unsigned int*)state->dst->f.r.rt)+1, ECX);
#endif
}

void gencfc1(usf_state_t * state)
{
#ifdef INTERPRET_CFC1
   gencallinterp(state, (unsigned int)state->current_instruction_table.CFC1, 0);
#else
   gencheck_cop1_unusable(state);
   if(state->dst->f.r.nrd == 31) mov_eax_memoffs32(state, (unsigned int*)&state->FCR31);
   else mov_eax_memoffs32(state, (unsigned int*)&state->FCR0);
   mov_memoffs32_eax(state, (unsigned int*)state->dst->f.r.rt);
   sar_reg32_imm8(state, EAX, 31);
   mov_memoffs32_eax(state, ((unsigned int*)state->dst->f.r.rt)+1);
#endif
}

void genmtc1(usf_state_t * state)
{
#ifdef INTERPRET_MTC1
   gencallinterp(state, (unsigned int)state->current_instruction_table.MTC1, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int*)state->dst->f.r.rt);
   mov_reg32_m32(state, EBX, (unsigned int*)(&state->reg_cop1_simple[state->dst->f.r.nrd]));
   mov_preg32_reg32(state, EBX, EAX);
#endif
}

void gendmtc1(usf_state_t * state)
{
#ifdef INTERPRET_DMTC1
   gencallinterp(state, (unsigned int)state->current_instruction_table.DMTC1, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int*)state->dst->f.r.rt);
   mov_reg32_m32(state, EBX, ((unsigned int*)state->dst->f.r.rt)+1);
   mov_reg32_m32(state, EDX, (unsigned int*)(&state->reg_cop1_double[state->dst->f.r.nrd]));
   mov_preg32_reg32(state, EDX, EAX);
   mov_preg32pimm32_reg32(state, EDX, 4, EBX);
#endif
}

void genctc1(usf_state_t * state)
{
#ifdef INTERPRET_CTC1
   gencallinterp(state, (unsigned int)state->current_instruction_table.CTC1, 0);
#else
   gencheck_cop1_unusable(state);
   
   if (state->dst->f.r.nrd != 31) return;
   mov_eax_memoffs32(state, (unsigned int*)state->dst->f.r.rt);
   mov_memoffs32_eax(state, (unsigned int*)&state->FCR31);
   and_eax_imm32(state, 3);
   
   cmp_eax_imm32(state, 0);
   jne_rj(state, 12);
   mov_m32_imm32(state, (unsigned int*)&state->rounding_mode, 0x33F); // 10
   jmp_imm_short(state, 48); // 2
   
   cmp_eax_imm32(state, 1); // 5
   jne_rj(state, 12); // 2
   mov_m32_imm32(state, (unsigned int*)&state->rounding_mode, 0xF3F); // 10
   jmp_imm_short(state, 29); // 2
   
   cmp_eax_imm32(state, 2); // 5
   jne_rj(state, 12); // 2
   mov_m32_imm32(state, (unsigned int*)&state->rounding_mode, 0xB3F); // 10
   jmp_imm_short(state, 10); // 2
   
   mov_m32_imm32(state, (unsigned int*)&state->rounding_mode, 0x73F); // 10
   
   fldcw_m16(state, (unsigned short*)&state->rounding_mode);
#endif
}

