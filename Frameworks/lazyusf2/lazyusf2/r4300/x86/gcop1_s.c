/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - gcop1_s.c                                               *
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
#include "r4300/r4300.h"
#include "r4300/ops.h"
#include "r4300/macros.h"
#include "r4300/cp1.h"

void genadd_s(usf_state_t * state)
{
#ifdef INTERPRET_ADD_S
    gencallinterp(state, (unsigned int)state->current_instruction_table.ADD_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fadd_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg32_dword(state, EAX);
#endif
}

void gensub_s(usf_state_t * state)
{
#ifdef INTERPRET_SUB_S
    gencallinterp(state, (unsigned int)state->current_instruction_table.SUB_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fsub_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg32_dword(state, EAX);
#endif
}

void genmul_s(usf_state_t * state)
{
#ifdef INTERPRET_MUL_S
    gencallinterp(state, (unsigned int)state->current_instruction_table.MUL_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fmul_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg32_dword(state, EAX);
#endif
}

void gendiv_s(usf_state_t * state)
{
#ifdef INTERPRET_DIV_S
    gencallinterp(state, (unsigned int)state->current_instruction_table.DIV_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fdiv_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg32_dword(state, EAX);
#endif
}

void gensqrt_s(usf_state_t * state)
{
#ifdef INTERPRET_SQRT_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.SQRT_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fsqrt(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg32_dword(state, EAX);
#endif
}

void genabs_s(usf_state_t * state)
{
#ifdef INTERPRET_ABS_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.ABS_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fabs_(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg32_dword(state, EAX);
#endif
}

void genmov_s(usf_state_t * state)
{
#ifdef INTERPRET_MOV_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.MOV_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   mov_reg32_preg32(state, EBX, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   mov_preg32_reg32(state, EAX, EBX);
#endif
}

void genneg_s(usf_state_t * state)
{
#ifdef INTERPRET_NEG_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.NEG_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fchs(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fstp_preg32_dword(state, EAX);
#endif
}

void genround_l_s(usf_state_t * state)
{
#ifdef INTERPRET_ROUND_L_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.ROUND_L_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16(state, (unsigned short*)&state->round_mode);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_double[state->dst->f.cf.fd]));
   fistp_preg32_qword(state, EAX);
   fldcw_m16(state, (unsigned short*)&state->rounding_mode);
#endif
}

void gentrunc_l_s(usf_state_t * state)
{
#ifdef INTERPRET_TRUNC_L_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.TRUNC_L_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16(state, (unsigned short*)&state->trunc_mode);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_double[state->dst->f.cf.fd]));
   fistp_preg32_qword(state, EAX);
   fldcw_m16(state, (unsigned short*)&state->rounding_mode);
#endif
}

void genceil_l_s(usf_state_t * state)
{
#ifdef INTERPRET_CEIL_L_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.CEIL_L_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16(state, (unsigned short*)&state->ceil_mode);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_double[state->dst->f.cf.fd]));
   fistp_preg32_qword(state, EAX);
   fldcw_m16(state, (unsigned short*)&state->rounding_mode);
#endif
}

void genfloor_l_s(usf_state_t * state)
{
#ifdef INTERPRET_FLOOR_L_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.FLOOR_L_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16(state, (unsigned short*)&state->floor_mode);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_double[state->dst->f.cf.fd]));
   fistp_preg32_qword(state, EAX);
   fldcw_m16(state, (unsigned short*)&state->rounding_mode);
#endif
}

void genround_w_s(usf_state_t * state)
{
#ifdef INTERPRET_ROUND_W_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.ROUND_W_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16(state, (unsigned short*)&state->round_mode);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fistp_preg32_dword(state, EAX);
   fldcw_m16(state, (unsigned short*)&state->rounding_mode);
#endif
}

void gentrunc_w_s(usf_state_t * state)
{
#ifdef INTERPRET_TRUNC_W_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.TRUNC_W_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16(state, (unsigned short*)&state->trunc_mode);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fistp_preg32_dword(state, EAX);
   fldcw_m16(state, (unsigned short*)&state->rounding_mode);
#endif
}

void genceil_w_s(usf_state_t * state)
{
#ifdef INTERPRET_CEIL_W_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.CEIL_W_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16(state, (unsigned short*)&state->ceil_mode);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fistp_preg32_dword(state, EAX);
   fldcw_m16(state, (unsigned short*)&state->rounding_mode);
#endif
}

void genfloor_w_s(usf_state_t * state)
{
#ifdef INTERPRET_FLOOR_W_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.FLOOR_W_S, 0);
#else
   gencheck_cop1_unusable(state);
   fldcw_m16(state, (unsigned short*)&state->floor_mode);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fistp_preg32_dword(state, EAX);
   fldcw_m16(state, (unsigned short*)&state->rounding_mode);
#endif
}

void gencvt_d_s(usf_state_t * state)
{
#ifdef INTERPRET_CVT_D_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.CVT_D_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_double[state->dst->f.cf.fd]));
   fstp_preg32_qword(state, EAX);
#endif
}

void gencvt_w_s(usf_state_t * state)
{
#ifdef INTERPRET_CVT_W_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.CVT_W_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fd]));
   fistp_preg32_dword(state, EAX);
#endif
}

void gencvt_l_s(usf_state_t * state)
{
#ifdef INTERPRET_CVT_L_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.CVT_L_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_double[state->dst->f.cf.fd]));
   fistp_preg32_qword(state, EAX);
#endif
}

void genc_f_s(usf_state_t * state)
{
#ifdef INTERPRET_C_F_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_F_S, 0);
#else
   gencheck_cop1_unusable(state);
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000);
#endif
}

void genc_un_s(usf_state_t * state)
{
#ifdef INTERPRET_C_UN_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_UN_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 12);
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
   jmp_imm_short(state, 10); // 2
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
#endif
}

void genc_eq_s(usf_state_t * state)
{
#ifdef INTERPRET_C_EQ_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_EQ_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jne_rj(state, 12);
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
   jmp_imm_short(state, 10); // 2
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
#endif
}

void genc_ueq_s(usf_state_t * state)
{
#ifdef INTERPRET_C_UEQ_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_UEQ_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 14);
   jne_rj(state, 12);
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
   jmp_imm_short(state, 10); // 2
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
#endif
}

void genc_olt_s(usf_state_t * state)
{
#ifdef INTERPRET_C_OLT_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_OLT_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jae_rj(state, 12);
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
   jmp_imm_short(state, 10); // 2
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
#endif
}

void genc_ult_s(usf_state_t * state)
{
#ifdef INTERPRET_C_ULT_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_ULT_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 14);
   jae_rj(state, 12);
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
   jmp_imm_short(state, 10); // 2
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
#endif
}

void genc_ole_s(usf_state_t * state)
{
#ifdef INTERPRET_C_OLE_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_OLE_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   ja_rj(state, 12);
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
   jmp_imm_short(state, 10); // 2
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
#endif
}

void genc_ule_s(usf_state_t * state)
{
#ifdef INTERPRET_C_ULE_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_ULE_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fucomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 14);
   ja_rj(state, 12);
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
   jmp_imm_short(state, 10); // 2
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
#endif
}

void genc_sf_s(usf_state_t * state)
{
#ifdef INTERPRET_C_SF_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_SF_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000);
#endif
}

void genc_ngle_s(usf_state_t * state)
{
#ifdef INTERPRET_C_NGLE_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_NGLE_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 12);
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
   jmp_imm_short(state, 10); // 2
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
#endif
}

void genc_seq_s(usf_state_t * state)
{
#ifdef INTERPRET_C_SEQ_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_SEQ_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jne_rj(state, 12);
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
   jmp_imm_short(state, 10); // 2
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
#endif
}

void genc_ngl_s(usf_state_t * state)
{
#ifdef INTERPRET_C_NGL_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_NGL_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 14);
   jne_rj(state, 12);
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
   jmp_imm_short(state, 10); // 2
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
#endif
}

void genc_lt_s(usf_state_t * state)
{
#ifdef INTERPRET_C_LT_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_LT_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jae_rj(state, 12);
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
   jmp_imm_short(state, 10); // 2
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
#endif
}

void genc_nge_s(usf_state_t * state)
{
#ifdef INTERPRET_C_NGE_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_NGE_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 14);
   jae_rj(state, 12);
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
   jmp_imm_short(state, 10); // 2
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
#endif
}

void genc_le_s(usf_state_t * state)
{
#ifdef INTERPRET_C_LE_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_LE_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   ja_rj(state, 12);
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
   jmp_imm_short(state, 10); // 2
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
#endif
}

void genc_ngt_s(usf_state_t * state)
{
#ifdef INTERPRET_C_NGT_S
   gencallinterp(state, (unsigned int)state->current_instruction_table.C_NGT_S, 0);
#else
   gencheck_cop1_unusable(state);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.ft]));
   fld_preg32_dword(state, EAX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg_cop1_simple[state->dst->f.cf.fs]));
   fld_preg32_dword(state, EAX);
   fcomip_fpreg(state, 1);
   ffree_fpreg(state, 0);
   jp_rj(state, 14);
   ja_rj(state, 12);
   or_m32_imm32(state, (unsigned int*)&state->FCR31, 0x800000); // 10
   jmp_imm_short(state, 10); // 2
   and_m32_imm32(state, (unsigned int*)&state->FCR31, ~0x800000); // 10
#endif
}

