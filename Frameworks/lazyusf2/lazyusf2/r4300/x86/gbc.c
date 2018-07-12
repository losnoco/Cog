/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - gbc.c                                                   *
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

#include "r4300/cached_interp.h"
#include "r4300/recomph.h"
#include "r4300/r4300.h"
#include "r4300/ops.h"
#include "r4300/cp1.h"

static void genbc1f_test(usf_state_t * state)
{
   test_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000);
   jne_rj(state, 12);
   mov_m32_imm32(state, (unsigned int*)(&state->branch_taken), 1); // 10
   jmp_imm_short(state, 10); // 2
   mov_m32_imm32(state, (unsigned int*)(&state->branch_taken), 0); // 10
}

void genbc1f(usf_state_t * state)
{
#ifdef INTERPRET_BC1F
   gencallinterp(state, (unsigned int)state->current_instruction_table.BC1F, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BC1F, 1);
    return;
     }
   
   gencheck_cop1_unusable(state);
   genbc1f_test(state);
   gendelayslot(state);
   gentest(state);
#endif
}

void genbc1f_out(usf_state_t * state)
{
#ifdef INTERPRET_BC1F_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.BC1F_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BC1F_OUT, 1);
    return;
     }
   
   gencheck_cop1_unusable(state);
   genbc1f_test(state);
   gendelayslot(state);
   gentest_out(state);
#endif
}

void genbc1f_idle(usf_state_t * state)
{
#ifdef INTERPRET_BC1F_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.BC1F_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BC1F_IDLE, 1);
    return;
     }
   
   gencheck_cop1_unusable(state);
   genbc1f_test(state);
   gentest_idle(state);
   genbc1f(state);
#endif
}

static void genbc1t_test(usf_state_t * state)
{
   test_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000);
   je_rj(state, 12);
   mov_m32_imm32(state, (unsigned int*)(&state->branch_taken), 1); // 10
   jmp_imm_short(state, 10); // 2
   mov_m32_imm32(state, (unsigned int*)(&state->branch_taken), 0); // 10
}

void genbc1t(usf_state_t * state)
{
#ifdef INTERPRET_BC1T
   gencallinterp(state, (unsigned int)state->current_instruction_table.BC1T, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BC1T, 1);
    return;
     }
   
   gencheck_cop1_unusable(state);
   genbc1t_test(state);
   gendelayslot(state);
   gentest(state);
#endif
}

void genbc1t_out(usf_state_t * state)
{
#ifdef INTERPRET_BC1T_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.BC1T_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BC1T_OUT, 1);
    return;
     }
   
   gencheck_cop1_unusable(state);
   genbc1t_test(state);
   gendelayslot(state);
   gentest_out(state);
#endif
}

void genbc1t_idle(usf_state_t * state)
{
#ifdef INTERPRET_BC1T_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.BC1T_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BC1T_IDLE, 1);
    return;
     }
   
   gencheck_cop1_unusable(state);
   genbc1t_test(state);
   gentest_idle(state);
   genbc1t(state);
#endif
}

void genbc1fl(usf_state_t * state)
{
#ifdef INTERPRET_BC1FL
   gencallinterp(state, (unsigned int)state->current_instruction_table.BC1FL, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BC1FL, 1);
    return;
     }
   
   gencheck_cop1_unusable(state);
   genbc1f_test(state);
   free_all_registers(state);
   gentestl(state);
#endif
}

void genbc1fl_out(usf_state_t * state)
{
#ifdef INTERPRET_BC1FL_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.BC1FL_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BC1FL_OUT, 1);
    return;
     }
   
   gencheck_cop1_unusable(state);
   genbc1f_test(state);
   free_all_registers(state);
   gentestl_out(state);
#endif
}

void genbc1fl_idle(usf_state_t * state)
{
#ifdef INTERPRET_BC1FL_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.BC1FL_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BC1FL_IDLE, 1);
    return;
     }
   
   gencheck_cop1_unusable(state);
   genbc1f_test(state);
   gentest_idle(state);
   genbc1fl(state);
#endif
}

void genbc1tl(usf_state_t * state)
{
#ifdef INTERPRET_BC1TL
   gencallinterp(state, (unsigned int)state->current_instruction_table.BC1TL, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BC1TL, 1);
    return;
     }
   
   gencheck_cop1_unusable(state);
   genbc1t_test(state);
   free_all_registers(state);
   gentestl(state);
#endif
}

void genbc1tl_out(usf_state_t * state)
{
#ifdef INTERPRET_BC1TL_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.BC1TL_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BC1TL_OUT, 1);
    return;
     }
   
   gencheck_cop1_unusable(state);
   genbc1t_test(state);
   free_all_registers(state);
   gentestl_out(state);
#endif
}

void genbc1tl_idle(usf_state_t * state)
{
#ifdef INTERPRET_BC1TL_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.BC1TL_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BC1TL_IDLE, 1);
    return;
     }
   
   gencheck_cop1_unusable(state);
   genbc1t_test(state);
   gentest_idle(state);
   genbc1tl(state);
#endif
}

