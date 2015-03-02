/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - empty_dynarec.c                                         *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2002 Richard42, Nmn                                     *
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

#include "recomp.h"

/* From assemble.c */

void init_assembler(usf_state_t * state, void *block_jumps_table, int block_jumps_number, void *block_riprel_table, int block_riprel_number)
{
}

void free_assembler(usf_state_t * state, void **block_jumps_table, int *block_jumps_number, void **block_riprel_table, int *block_riprel_number)
{
}

void passe2(usf_state_t * state, precomp_instr *dest, int start, int end, precomp_block *block)
{
}

/* From gbc.c */

void genbc1f(usf_state_t * state)
{
}

void genbc1f_out(usf_state_t * state)
{
}

void genbc1f_idle(usf_state_t * state)
{
}

void genbc1t(usf_state_t * state)
{
}

void genbc1t_out(usf_state_t * state)
{
}

void genbc1t_idle(usf_state_t * state)
{
}

void genbc1fl(usf_state_t * state)
{
}

void genbc1fl_out(usf_state_t * state)
{
}

void genbc1fl_idle(usf_state_t * state)
{
}

void genbc1tl(usf_state_t * state)
{
}

void genbc1tl_out(usf_state_t * state)
{
}

void genbc1tl_idle(usf_state_t * state)
{
}

/* From gcop0.c */

void genmfc0(usf_state_t * state)
{
}

void genmtc0(usf_state_t * state)
{
}

/* From gcop1.c */

void genmfc1(usf_state_t * state)
{
}

void gendmfc1(usf_state_t * state)
{
}

void gencfc1(usf_state_t * state)
{
}

void genmtc1(usf_state_t * state)
{
}

void gendmtc1(usf_state_t * state)
{
}

void genctc1(usf_state_t * state)
{
}

/* From gcop1_d.c */

void genadd_d(usf_state_t * state)
{
}

void gensub_d(usf_state_t * state)
{
}

void genmul_d(usf_state_t * state)
{
}

void gendiv_d(usf_state_t * state)
{
}

void gensqrt_d(usf_state_t * state)
{
}

void genabs_d(usf_state_t * state)
{
}

void genmov_d(usf_state_t * state)
{
}

void genneg_d(usf_state_t * state)
{
}

void genround_l_d(usf_state_t * state)
{
}

void gentrunc_l_d(usf_state_t * state)
{
}

void genceil_l_d(usf_state_t * state)
{
}

void genfloor_l_d(usf_state_t * state)
{
}

void genround_w_d(usf_state_t * state)
{
}

void gentrunc_w_d(usf_state_t * state)
{
}

void genceil_w_d(usf_state_t * state)
{
}

void genfloor_w_d(usf_state_t * state)
{
}

void gencvt_s_d(usf_state_t * state)
{
}

void gencvt_w_d(usf_state_t * state)
{
}

void gencvt_l_d(usf_state_t * state)
{
}

void genc_f_d(usf_state_t * state)
{
}

void genc_un_d(usf_state_t * state)
{
}

void genc_eq_d(usf_state_t * state)
{
}

void genc_ueq_d(usf_state_t * state)
{
}

void genc_olt_d(usf_state_t * state)
{
}

void genc_ult_d(usf_state_t * state)
{
}

void genc_ole_d(usf_state_t * state)
{
}

void genc_ule_d(usf_state_t * state)
{
}

void genc_sf_d(usf_state_t * state)
{
}

void genc_ngle_d(usf_state_t * state)
{
}

void genc_seq_d(usf_state_t * state)
{
}

void genc_ngl_d(usf_state_t * state)
{
}

void genc_lt_d(usf_state_t * state)
{
}

void genc_nge_d(usf_state_t * state)
{
}

void genc_le_d(usf_state_t * state)
{
}

void genc_ngt_d(usf_state_t * state)
{
}

/* From gcop1_l.c */

void gencvt_s_l(usf_state_t * state)
{
}

void gencvt_d_l(usf_state_t * state)
{
}

/* From gcop1_s.c */

void genadd_s(usf_state_t * state)
{
}

void gensub_s(usf_state_t * state)
{
}

void genmul_s(usf_state_t * state)
{
}

void gendiv_s(usf_state_t * state)
{
}

void gensqrt_s(usf_state_t * state)
{
}

void genabs_s(usf_state_t * state)
{
}

void genmov_s(usf_state_t * state)
{
}

void genneg_s(usf_state_t * state)
{
}

void genround_l_s(usf_state_t * state)
{
}

void gentrunc_l_s(usf_state_t * state)
{
}

void genceil_l_s(usf_state_t * state)
{
}

void genfloor_l_s(usf_state_t * state)
{
}

void genround_w_s(usf_state_t * state)
{
}

void gentrunc_w_s(usf_state_t * state)
{
}

void genceil_w_s(usf_state_t * state)
{
}

void genfloor_w_s(usf_state_t * state)
{
}

void gencvt_d_s(usf_state_t * state)
{
}

void gencvt_w_s(usf_state_t * state)
{
}

void gencvt_l_s(usf_state_t * state)
{
}

void genc_f_s(usf_state_t * state)
{
}

void genc_un_s(usf_state_t * state)
{
}

void genc_eq_s(usf_state_t * state)
{
}

void genc_ueq_s(usf_state_t * state)
{
}

void genc_olt_s(usf_state_t * state)
{
}

void genc_ult_s(usf_state_t * state)
{
}

void genc_ole_s(usf_state_t * state)
{
}

void genc_ule_s(usf_state_t * state)
{
}

void genc_sf_s(usf_state_t * state)
{
}

void genc_ngle_s(usf_state_t * state)
{
}

void genc_seq_s(usf_state_t * state)
{
}

void genc_ngl_s(usf_state_t * state)
{
}

void genc_lt_s(usf_state_t * state)
{
}

void genc_nge_s(usf_state_t * state)
{
}

void genc_le_s(usf_state_t * state)
{
}

void genc_ngt_s(usf_state_t * state)
{
}

/* From gcop1_w.c */

void gencvt_s_w(usf_state_t * state)
{
}

void gencvt_d_w(usf_state_t * state)
{
}

/* From gr4300.c */

void gennotcompiled(usf_state_t * state)
{
}

void genlink_subblock(usf_state_t * state)
{
}

void genni(usf_state_t * state)
{
}

void genreserved(usf_state_t * state)
{
}

void genfin_block(usf_state_t * state)
{
}

void gennop(usf_state_t * state)
{
}

void genj(usf_state_t * state)
{
}

void genj_out(usf_state_t * state)
{
}

void genj_idle(usf_state_t * state)
{
}

void genjal(usf_state_t * state)
{
}

void genjal_out(usf_state_t * state)
{
}

void genjal_idle(usf_state_t * state)
{
}

void genbne(usf_state_t * state)
{
}

void genbne_out(usf_state_t * state)
{
}

void genbne_idle(usf_state_t * state)
{
}

void genblez(usf_state_t * state)
{
}

void genblez_idle(usf_state_t * state)
{
}

void genbgtz(usf_state_t * state)
{
}

void genbgtz_out(usf_state_t * state)
{
}

void genbgtz_idle(usf_state_t * state)
{
}

void genaddi(usf_state_t * state)
{
}

void genaddiu(usf_state_t * state)
{
}

void genslti(usf_state_t * state)
{
}

void gensltiu(usf_state_t * state)
{
}

void genandi(usf_state_t * state)
{
}

void genori(usf_state_t * state)
{
}

void genxori(usf_state_t * state)
{
}

void genlui(usf_state_t * state)
{
}

void genbeql(usf_state_t * state)
{
}

void genbeql_out(usf_state_t * state)
{
}

void genbeql_idle(usf_state_t * state)
{
}

void genbeq(usf_state_t * state)
{
}

void genbeq_out(usf_state_t * state)
{
}

void genbeq_idle(usf_state_t * state)
{
}

void genbnel(usf_state_t * state)
{
}

void genbnel_out(usf_state_t * state)
{
}

void genbnel_idle(usf_state_t * state)
{
}

void genblezl(usf_state_t * state)
{
}

void genblezl_out(usf_state_t * state)
{
}

void genblezl_idle(usf_state_t * state)
{
}

void genbgtzl(usf_state_t * state)
{
}

void genbgtzl_out(usf_state_t * state)
{
}

void genbgtzl_idle(usf_state_t * state)
{
}

void gendaddi(usf_state_t * state)
{
}

void gendaddiu(usf_state_t * state)
{
}

void genldl(usf_state_t * state)
{
}

void genldr(usf_state_t * state)
{
}

void genlb(usf_state_t * state)
{
}

void genlh(usf_state_t * state)
{
}

void genlwl(usf_state_t * state)
{
}

void genlw(usf_state_t * state)
{
}

void genlbu(usf_state_t * state)
{
}

void genlhu(usf_state_t * state)
{
}

void genlwr(usf_state_t * state)
{
}

void genlwu(usf_state_t * state)
{
}

void gensb(usf_state_t * state)
{
}

void gensh(usf_state_t * state)
{
}

void genswl(usf_state_t * state)
{
}

void gensw(usf_state_t * state)
{
}

void gensdl(usf_state_t * state)
{
}

void gensdr(usf_state_t * state)
{
}

void genswr(usf_state_t * state)
{
}

void genlwc1(usf_state_t * state)
{
}

void genldc1(usf_state_t * state)
{
}

void gencache(usf_state_t * state)
{
}

void genld(usf_state_t * state)
{
}

void genswc1(usf_state_t * state)
{
}

void gensdc1(usf_state_t * state)
{
}

void gensd(usf_state_t * state)
{
}

void genll(usf_state_t * state)
{
}

void gensc(usf_state_t * state)
{
}

void genblez_out(usf_state_t * state)
{
}

/* From gregimm.c */

void genbltz(usf_state_t * state)
{
}

void genbltz_out(usf_state_t * state)
{
}

void genbltz_idle(usf_state_t * state)
{
}

void genbgez(usf_state_t * state)
{
}

void genbgez_out(usf_state_t * state)
{
}

void genbgez_idle(usf_state_t * state)
{
}

void genbltzl(usf_state_t * state)
{
}

void genbltzl_out(usf_state_t * state)
{
}

void genbltzl_idle(usf_state_t * state)
{
}

void genbgezl(usf_state_t * state)
{
}

void genbgezl_out(usf_state_t * state)
{
}

void genbgezl_idle(usf_state_t * state)
{
}

void genbltzal(usf_state_t * state)
{
}

void genbltzal_out(usf_state_t * state)
{
}

void genbltzal_idle(usf_state_t * state)
{
}

void genbgezal(usf_state_t * state)
{
}

void genbgezal_out(usf_state_t * state)
{
}

void genbgezal_idle(usf_state_t * state)
{
}

void genbltzall(usf_state_t * state)
{
}

void genbltzall_out(usf_state_t * state)
{
}

void genbltzall_idle(usf_state_t * state)
{
}

void genbgezall(usf_state_t * state)
{
}

void genbgezall_out(usf_state_t * state)
{
}

void genbgezall_idle(usf_state_t * state)
{
}

/* From gspecial.c */

void gensll(usf_state_t * state)
{
}

void gensrl(usf_state_t * state)
{
}

void gensra(usf_state_t * state)
{
}

void gensllv(usf_state_t * state)
{
}

void gensrlv(usf_state_t * state)
{
}

void gensrav(usf_state_t * state)
{
}

void genjr(usf_state_t * state)
{
}

void genjalr(usf_state_t * state)
{
}

void gensyscall(usf_state_t * state)
{
}

void gensync(usf_state_t * state)
{
}

void genmfhi(usf_state_t * state)
{
}

void genmthi(usf_state_t * state)
{
}

void genmflo(usf_state_t * state)
{
}

void genmtlo(usf_state_t * state)
{
}

void gendsllv(usf_state_t * state)
{
}

void gendsrlv(usf_state_t * state)
{
}

void gendsrav(usf_state_t * state)
{
}

void genmult(usf_state_t * state)
{
}

void genmultu(usf_state_t * state)
{
}

void gendiv(usf_state_t * state)
{
}

void gendivu(usf_state_t * state)
{
}

void gendmult(usf_state_t * state)
{
}

void gendmultu(usf_state_t * state)
{
}

void genddiv(usf_state_t * state)
{
}

void genddivu(usf_state_t * state)
{
}

void genadd(usf_state_t * state)
{
}

void genaddu(usf_state_t * state)
{
}

void gensub(usf_state_t * state)
{
}

void gensubu(usf_state_t * state)
{
}

void genand(usf_state_t * state)
{
}

void genor(usf_state_t * state)
{
}

void genxor(usf_state_t * state)
{
}

void gennor(usf_state_t * state)
{
}

void genslt(usf_state_t * state)
{
}

void gensltu(usf_state_t * state)
{
}

void gendadd(usf_state_t * state)
{
}

void gendaddu(usf_state_t * state)
{
}

void gendsub(usf_state_t * state)
{
}

void gendsubu(usf_state_t * state)
{
}

void genteq(usf_state_t * state)
{
}

void gendsll(usf_state_t * state)
{
}

void gendsrl(usf_state_t * state)
{
}

void gendsra(usf_state_t * state)
{
}

void gendsll32(usf_state_t * state)
{
}

void gendsrl32(usf_state_t * state)
{
}

void gendsra32(usf_state_t * state)
{
}

void genbreak(usf_state_t * state)
{
}

/* From gtlb.c */

void gentlbwi(usf_state_t * state)
{
}

void gentlbp(usf_state_t * state)
{
}

void gentlbr(usf_state_t * state)
{
}

void generet(usf_state_t * state)
{
}

void gentlbwr(usf_state_t * state)
{
}

/* From regcache.c */

void init_cache(usf_state_t * state, precomp_instr* start)
{
}

void free_all_registers(usf_state_t * state)
{
}

/* From rjump.c */

void dyna_jump(usf_state_t * state)
{
}

void dyna_stop(usf_state_t * state)
{
}

