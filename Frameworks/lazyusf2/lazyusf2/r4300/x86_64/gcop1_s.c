/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - gcop1_s.c                                               *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2007 Richard Goedeken (Richard42)                       *
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
#include "r4300/r4300.h"
#include "r4300/ops.h"
#include "r4300/macros.h"
#include "r4300/cp1.h"

#if defined(COUNT_INSTR)
#include "r4300/instr_counters.h"
#endif

void genadd_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[119]);
#endif
#ifdef INTERPRET_ADD_S
    gencallinterp(state, (unsigned long long)state->current_instruction_table.ADD_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fadd_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg64_dword(state, RAX);
#endif
}

void gensub_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[120]);
#endif
#ifdef INTERPRET_SUB_S
    gencallinterp(state, (unsigned long long)state->current_instruction_table.SUB_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fsub_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg64_dword(state, RAX);
#endif
}

void genmul_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[121]);
#endif
#ifdef INTERPRET_MUL_S
    gencallinterp(state, (unsigned long long)state->current_instruction_table.MUL_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fmul_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg64_dword(state, RAX);
#endif
}

void gendiv_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[122]);
#endif
#ifdef INTERPRET_DIV_S
    gencallinterp(state, (unsigned long long)state->current_instruction_table.DIV_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fdiv_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg64_dword(state, RAX);
#endif
}

void gensqrt_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[123]);
#endif
#ifdef INTERPRET_SQRT_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SQRT_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fsqrt(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg64_dword(state, RAX);
#endif
}

void genabs_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[124]);
#endif
#ifdef INTERPRET_ABS_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.ABS_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fabs_(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg64_dword(state, RAX);
#endif
}

void genmov_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[125]);
#endif
#ifdef INTERPRET_MOV_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.MOV_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   mov_reg32_preg64(state, EBX, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   mov_preg64_reg32(state, RAX, EBX);
#endif
}

void genneg_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[126]);
#endif
#ifdef INTERPRET_NEG_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.NEG_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fchs(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg64_dword(state, RAX);
#endif
}

void genround_l_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[127]);
#endif
#ifdef INTERPRET_ROUND_L_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.ROUND_L_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16rel(state, (unsigned short*)&state->round_mode);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_double[state->dst->f.cf.fd]));
   fistp_preg64_qword(state, RAX);
   fldcw_m16rel(state, (unsigned short*)&state->rounding_mode);
#endif
}

void gentrunc_l_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[128]);
#endif
#ifdef INTERPRET_TRUNC_L_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.TRUNC_L_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16rel(state, (unsigned short*)&state->trunc_mode);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_double[state->dst->f.cf.fd]));
   fistp_preg64_qword(state, RAX);
   fldcw_m16rel(state, (unsigned short*)&state->rounding_mode);
#endif
}

void genceil_l_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[129]);
#endif
#ifdef INTERPRET_CEIL_L_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.CEIL_L_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16rel(state, (unsigned short*)&state->ceil_mode);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_double[state->dst->f.cf.fd]));
   fistp_preg64_qword(state, RAX);
   fldcw_m16rel(state, (unsigned short*)&state->rounding_mode);
#endif
}

void genfloor_l_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[130]);
#endif
#ifdef INTERPRET_FLOOR_L_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.FLOOR_L_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16rel(state, (unsigned short*)&state->floor_mode);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_double[state->dst->f.cf.fd]));
   fistp_preg64_qword(state, RAX);
   fldcw_m16rel(state, (unsigned short*)&state->rounding_mode);
#endif
}

void genround_w_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[127]);
#endif
#ifdef INTERPRET_ROUND_W_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.ROUND_W_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16rel(state, (unsigned short*)&state->round_mode);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fistp_preg64_dword(state, RAX);
   fldcw_m16rel(state, (unsigned short*)&state->rounding_mode);
#endif
}

void gentrunc_w_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[128]);
#endif
#ifdef INTERPRET_TRUNC_W_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.TRUNC_W_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16rel(state, (unsigned short*)&state->trunc_mode);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fistp_preg64_dword(state, RAX);
   fldcw_m16rel(state, (unsigned short*)&state->rounding_mode);
#endif
}

void genceil_w_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[129]);
#endif
#ifdef INTERPRET_CEIL_W_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.CEIL_W_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16rel(state, (unsigned short*)&state->ceil_mode);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fistp_preg64_dword(state, RAX);
   fldcw_m16rel(state, (unsigned short*)&state->rounding_mode);
#endif
}

void genfloor_w_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[130]);
#endif
#ifdef INTERPRET_FLOOR_W_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.FLOOR_W_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16rel(state, (unsigned short*)&state->floor_mode);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fistp_preg64_dword(state, RAX);
   fldcw_m16rel(state, (unsigned short*)&state->rounding_mode);
#endif
}

void gencvt_d_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[117]);
#endif
#ifdef INTERPRET_CVT_D_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.CVT_D_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_double[state->dst->f.cf.fd]));
   fstp_preg64_qword(state, RAX);
#endif
}

void gencvt_w_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[117]);
#endif
#ifdef INTERPRET_CVT_W_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.CVT_W_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fistp_preg64_dword(state, RAX);
#endif
}

void gencvt_l_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[117]);
#endif
#ifdef INTERPRET_CVT_L_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.CVT_L_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_double[state->dst->f.cf.fd]));
   fistp_preg64_qword(state, RAX);
#endif
}

void genc_f_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_F_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_F_S, 0);
#else
   gencheck_cop1_unusable(state);
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000);
#endif
}

void genc_un_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_UN_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_UN_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 13);
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
   jmp_imm_short(state, 11); // 2
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
#endif
}

void genc_eq_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_EQ_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_EQ_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jne_rj(state, 13);
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
   jmp_imm_short(state, 11); // 2
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
#endif
}

void genc_ueq_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_UEQ_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_UEQ_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 15);
   jne_rj(state, 13);
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
   jmp_imm_short(state, 11); // 2
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
#endif
}

void genc_olt_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_OLT_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_OLT_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jae_rj(state, 13);
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
   jmp_imm_short(state, 11); // 2
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
#endif
}

void genc_ult_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_ULT_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_ULT_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 15);
   jae_rj(state, 13);
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
   jmp_imm_short(state, 11); // 2
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
#endif
}

void genc_ole_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_OLE_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_OLE_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   ja_rj(state, 13);
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
   jmp_imm_short(state, 11); // 2
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
#endif
}

void genc_ule_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_ULE_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_ULE_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 15);
   ja_rj(state, 13);
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
   jmp_imm_short(state, 11); // 2
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
#endif
}

void genc_sf_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_SF_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_SF_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000);
#endif
}

void genc_ngle_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_NGLE_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_NGLE_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 13);
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
   jmp_imm_short(state, 11); // 2
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
#endif
}

void genc_seq_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_SEQ_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_SEQ_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jne_rj(state, 13);
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
   jmp_imm_short(state, 11); // 2
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
#endif
}

void genc_ngl_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_NGL_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_NGL_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 15);
   jne_rj(state, 13);
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
   jmp_imm_short(state, 11); // 2
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
#endif
}

void genc_lt_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_LT_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_LT_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jae_rj(state, 13);
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
   jmp_imm_short(state, 11); // 2
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
#endif
}

void genc_nge_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_NGE_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_NGE_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 15);
   jae_rj(state, 13);
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
   jmp_imm_short(state, 11); // 2
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
#endif
}

void genc_le_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_LE_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_LE_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   ja_rj(state, 13);
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
   jmp_imm_short(state, 11); // 2
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
#endif
}

void genc_ngt_s(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[118]);
#endif
#ifdef INTERPRET_C_NGT_S
   gencallinterp(state, (unsigned long long)state->current_instruction_table.C_NGT_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg64_dword(state, RAX);
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg64_dword(state, RAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 15);
   ja_rj(state, 13);
   or_m32rel_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 11
   jmp_imm_short(state, 11); // 2
   and_m32rel_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 11
#endif
}

