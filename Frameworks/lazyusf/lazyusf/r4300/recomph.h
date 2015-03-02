/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - recomph.h                                               *
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

#ifndef M64P_R4300_RECOMPH_H
#define M64P_R4300_RECOMPH_H

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "recomp.h"

void passe2(usf_state_t *, precomp_instr *dest, int start, int end, precomp_block* block);
void init_assembler(usf_state_t *, void *block_jumps_table, int block_jumps_number, void *block_riprel_table, int block_riprel_number);
void free_assembler(usf_state_t *, void **block_jumps_table, int *block_jumps_number, void **block_riprel_table, int *block_riprel_number);

#if defined(__x86_64__)
void gencallinterp(usf_state_t *, unsigned long long addr, int jump);
#else
void gencallinterp(usf_state_t *, unsigned long addr, int jump);
#endif

void genupdate_system(usf_state_t *, int type);
void genbnel(usf_state_t *);
void genblezl(usf_state_t *);
void genlw(usf_state_t *);
void genlbu(usf_state_t *);
void genlhu(usf_state_t *);
void gensb(usf_state_t *);
void gensh(usf_state_t *);
void gensw(usf_state_t *);
void gencache(usf_state_t *);
void genlwc1(usf_state_t *);
void genld(usf_state_t *);
void gensd(usf_state_t *);
void genbeq(usf_state_t *);
void genbne(usf_state_t *);
void genblez(usf_state_t *);
void genaddi(usf_state_t *);
void genaddiu(usf_state_t *);
void genslti(usf_state_t *);
void gensltiu(usf_state_t *);
void genandi(usf_state_t *);
void genori(usf_state_t *);
void genxori(usf_state_t *);
void genlui(usf_state_t *);
void genbeql(usf_state_t *);
void genmul_s(usf_state_t *);
void gendiv_s(usf_state_t *);
void gencvt_d_s(usf_state_t *);
void genadd_d(usf_state_t *);
void gentrunc_w_d(usf_state_t *);
void gencvt_s_w(usf_state_t *);
void genmfc1(usf_state_t *);
void gencfc1(usf_state_t *);
void genmtc1(usf_state_t *);
void genctc1(usf_state_t *);
void genj(usf_state_t *);
void genjal(usf_state_t *);
void genslt(usf_state_t *);
void gensltu(usf_state_t *);
void gendsll32(usf_state_t *);
void gendsra32(usf_state_t *);
void genbgez(usf_state_t *);
void genbgezl(usf_state_t *);
void genbgezal(usf_state_t *);
void gentlbwi(usf_state_t *);
void generet(usf_state_t *);
void genmfc0(usf_state_t *);
void genadd_s(usf_state_t *);
void genmult(usf_state_t *);
void genmultu(usf_state_t *);
void genmflo(usf_state_t *);
void genmtlo(usf_state_t *);
void gendiv(usf_state_t *);
void gendmultu(usf_state_t *);
void genddivu(usf_state_t *);
void genadd(usf_state_t *);
void genaddu(usf_state_t *);
void gensubu(usf_state_t *);
void genand(usf_state_t *);
void genor(usf_state_t *);
void genxor(usf_state_t *);
void genreserved(usf_state_t *);
void gennop(usf_state_t *);
void gensll(usf_state_t *);
void gensrl(usf_state_t *);
void gensra(usf_state_t *);
void gensllv(usf_state_t *);
void gensrlv(usf_state_t *);
void genjr(usf_state_t *);
void genni(usf_state_t *);
void genmfhi(usf_state_t *);
void genmthi(usf_state_t *);
void genmtc0(usf_state_t *);
void genbltz(usf_state_t *);
void genlwl(usf_state_t *);
void genswl(usf_state_t *);
void gentlbp(usf_state_t *);
void gentlbr(usf_state_t *);
void genswr(usf_state_t *);
void genlwr(usf_state_t *);
void gensrav(usf_state_t *);
void genbgtz(usf_state_t *);
void genlb(usf_state_t *);
void genswc1(usf_state_t *);
void genldc1(usf_state_t *);
void gencvt_d_w(usf_state_t *);
void genmul_d(usf_state_t *);
void gensub_d(usf_state_t *);
void gendiv_d(usf_state_t *);
void gencvt_s_d(usf_state_t *);
void genmov_s(usf_state_t *);
void genc_le_s(usf_state_t *);
void genbc1t(usf_state_t *);
void gentrunc_w_s(usf_state_t *);
void genbc1tl(usf_state_t *);
void genc_lt_s(usf_state_t *);
void genbc1fl(usf_state_t *);
void genneg_s(usf_state_t *);
void genc_le_d(usf_state_t *);
void genbgezal_idle(usf_state_t *);
void genj_idle(usf_state_t *);
void genbeq_idle(usf_state_t *);
void genlh(usf_state_t *);
void genmov_d(usf_state_t *);
void genc_lt_d(usf_state_t *);
void genbc1f(usf_state_t *);
void gennor(usf_state_t *);
void genneg_d(usf_state_t *);
void gensub(usf_state_t *);
void genblez_idle(usf_state_t *);
void gendivu(usf_state_t *);
void gencvt_w_s(usf_state_t *);
void genbltzl(usf_state_t *);
void gensdc1(usf_state_t *);
void genc_eq_s(usf_state_t *);
void genjalr(usf_state_t *);
void gensub_s(usf_state_t *);
void gensqrt_s(usf_state_t *);
void genc_eq_d(usf_state_t *);
void gencvt_w_d(usf_state_t *);
void genfin_block(usf_state_t *);
void genddiv(usf_state_t *);
void gendaddiu(usf_state_t *);
void genbgtzl(usf_state_t *);
void gendsrav(usf_state_t *);
void gendsllv(usf_state_t *);
void gencvt_s_l(usf_state_t *);
void gendmtc1(usf_state_t *);
void gendsrlv(usf_state_t *);
void gendsra(usf_state_t *);
void gendmult(usf_state_t *);
void gendsll(usf_state_t *);
void genabs_s(usf_state_t *);
void gensc(usf_state_t *);
void gennotcompiled(usf_state_t *);
void genjal_idle(usf_state_t *);
void genjal_out(usf_state_t *);
void genbeq_out(usf_state_t *);
void gensyscall(usf_state_t *);
void gensync(usf_state_t *);
void gendadd(usf_state_t *);
void gendaddu(usf_state_t *);
void gendsub(usf_state_t *);
void gendsubu(usf_state_t *);
void genteq(usf_state_t *);
void gendsrl(usf_state_t *);
void gendsrl32(usf_state_t *);
void genbltz_idle(usf_state_t *);
void genbltz_out(usf_state_t *);
void genbgez_idle(usf_state_t *);
void genbgez_out(usf_state_t *);
void genbltzl_idle(usf_state_t *);
void genbltzl_out(usf_state_t *);
void genbgezl_idle(usf_state_t *);
void genbgezl_out(usf_state_t *);
void genbltzal_idle(usf_state_t *);
void genbltzal_out(usf_state_t *);
void genbltzal(usf_state_t *);
void genbgezal_out(usf_state_t *);
void genbltzall_idle(usf_state_t *);
void genbltzall_out(usf_state_t *);
void genbltzall(usf_state_t *);
void genbgezall_idle(usf_state_t *);
void genbgezall_out(usf_state_t *);
void genbgezall(usf_state_t *);
void gentlbwr(usf_state_t *);
void genbc1f_idle(usf_state_t *);
void genbc1f_out(usf_state_t *);
void genbc1t_idle(usf_state_t *);
void genbc1t_out(usf_state_t *);
void genbc1fl_idle(usf_state_t *);
void genbc1fl_out(usf_state_t *);
void genbc1tl_idle(usf_state_t *);
void genbc1tl_out(usf_state_t *);
void genround_l_s(usf_state_t *);
void gentrunc_l_s(usf_state_t *);
void genceil_l_s(usf_state_t *);
void genfloor_l_s(usf_state_t *);
void genround_w_s(usf_state_t *);
void genceil_w_s(usf_state_t *);
void genfloor_w_s(usf_state_t *);
void gencvt_l_s(usf_state_t *);
void genc_f_s(usf_state_t *);
void genc_un_s(usf_state_t *);
void genc_ueq_s(usf_state_t *);
void genc_olt_s(usf_state_t *);
void genc_ult_s(usf_state_t *);
void genc_ole_s(usf_state_t *);
void genc_ule_s(usf_state_t *);
void genc_sf_s(usf_state_t *);
void genc_ngle_s(usf_state_t *);
void genc_seq_s(usf_state_t *);
void genc_ngl_s(usf_state_t *);
void genc_nge_s(usf_state_t *);
void genc_ngt_s(usf_state_t *);
void gensqrt_d(usf_state_t *);
void genabs_d(usf_state_t *);
void genround_l_d(usf_state_t *);
void gentrunc_l_d(usf_state_t *);
void genceil_l_d(usf_state_t *);
void genfloor_l_d(usf_state_t *);
void genround_w_d(usf_state_t *);
void genceil_w_d(usf_state_t *);
void genfloor_w_d(usf_state_t *);
void gencvt_l_d(usf_state_t *);
void genc_f_d(usf_state_t *);
void genc_un_d(usf_state_t *);
void genc_ueq_d(usf_state_t *);
void genc_olt_d(usf_state_t *);
void genc_ult_d(usf_state_t *);
void genc_ole_d(usf_state_t *);
void genc_ule_d(usf_state_t *);
void genc_sf_d(usf_state_t *);
void genc_ngle_d(usf_state_t *);
void genc_seq_d(usf_state_t *);
void genc_ngl_d(usf_state_t *);
void genc_nge_d(usf_state_t *);
void genc_ngt_d(usf_state_t *);
void gencvt_d_l(usf_state_t *);
void gendmfc1(usf_state_t *);
void genj_out(usf_state_t *);
void genbne_idle(usf_state_t *);
void genbne_out(usf_state_t *);
void genblez_out(usf_state_t *);
void genbgtz_idle(usf_state_t *);
void genbgtz_out(usf_state_t *);
void genbeql_idle(usf_state_t *);
void genbeql_out(usf_state_t *);
void genbnel_idle(usf_state_t *);
void genbnel_out(usf_state_t *);
void genblezl_idle(usf_state_t *);
void genblezl_out(usf_state_t *);
void genbgtzl_idle(usf_state_t *);
void genbgtzl_out(usf_state_t *);
void gendaddi(usf_state_t *);
void genldl(usf_state_t *);
void genldr(usf_state_t *);
void genlwu(usf_state_t *);
void gensdl(usf_state_t *);
void gensdr(usf_state_t *);
void genlink_subblock(usf_state_t *);
void gendelayslot(usf_state_t *);
void gencheck_interupt_reg(usf_state_t *);
void gentest(usf_state_t *);
void gentest_out(usf_state_t *);
void gentest_idle(usf_state_t *);
void gentestl(usf_state_t *);
void gentestl_out(usf_state_t *);
void gencheck_cop1_unusable(usf_state_t *);
void genll(usf_state_t *);
void genbreak(usf_state_t *);

#endif /* M64P_R4300_RECOMPH_H */

