/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - recomp.c                                                *
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
#include <string.h>

#if defined(__GNUC__)
#include <unistd.h>
#ifndef __MINGW32__
#include <sys/mman.h>
#if defined(__APPLE__)
#include <stdio.h>
#include <sys/sysctl.h>
#endif
#endif
#endif

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "memory/memory.h"

#include "cached_interp.h"
#include "recomp.h"
#include "recomph.h" //include for function prototypes
#include "cp0.h"
#include "r4300.h"
#include "ops.h"
#include "tlb.h"

#ifdef DYNAREC
static void *malloc_exec(usf_state_t *, size_t size);
static void free_exec(void *ptr, size_t length);
#endif


static void RSV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.RESERVED;
#ifdef DYNAREC
    state->recomp_func = genreserved;
#endif
}

static void RFIN_BLOCK(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FIN_BLOCK;
#ifdef DYNAREC
   state->recomp_func = genfin_block;
#endif
}

static void RNOTCOMPILED(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NOTCOMPILED;
#ifdef DYNAREC
   state->recomp_func = gennotcompiled;
#endif
}

static void recompile_standard_i_type(usf_state_t * state)
{
   state->dst->f.i.rs = state->reg + ((state->src >> 21) & 0x1F);
   state->dst->f.i.rt = state->reg + ((state->src >> 16) & 0x1F);
   state->dst->f.i.immediate = state->src & 0xFFFF;
}

static void recompile_standard_j_type(usf_state_t * state)
{
   state->dst->f.j.inst_index = state->src & 0x3FFFFFF;
}

static void recompile_standard_r_type(usf_state_t * state)
{
   state->dst->f.r.rs = state->reg + ((state->src >> 21) & 0x1F);
   state->dst->f.r.rt = state->reg + ((state->src >> 16) & 0x1F);
   state->dst->f.r.rd = state->reg + ((state->src >> 11) & 0x1F);
   state->dst->f.r.sa = (state->src >>  6) & 0x1F;
}

static void recompile_standard_lf_type(usf_state_t * state)
{
   state->dst->f.lf.base = (state->src >> 21) & 0x1F;
   state->dst->f.lf.ft = (state->src >> 16) & 0x1F;
   state->dst->f.lf.offset = state->src & 0xFFFF;
}

static void recompile_standard_cf_type(usf_state_t * state)
{
   state->dst->f.cf.ft = (state->src >> 16) & 0x1F;
   state->dst->f.cf.fs = (state->src >> 11) & 0x1F;
   state->dst->f.cf.fd = (state->src >>  6) & 0x1F;
}

//-------------------------------------------------------------------------
//                                  SPECIAL                                
//-------------------------------------------------------------------------

static void RNOP(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NOP;
#ifdef DYNAREC
   state->recomp_func = gennop;
#endif
}

static void RSLL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLL;
#ifdef DYNAREC
   state->recomp_func = gensll;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSRL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SRL;
#ifdef DYNAREC
   state->recomp_func = gensrl;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSRA(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SRA;
#ifdef DYNAREC
   state->recomp_func = gensra;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSLLV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLLV;
#ifdef DYNAREC
   state->recomp_func = gensllv;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSRLV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SRLV;
#ifdef DYNAREC
   state->recomp_func = gensrlv;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSRAV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SRAV;
#ifdef DYNAREC
   state->recomp_func = gensrav;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RJR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.JR;
#ifdef DYNAREC
   state->recomp_func = genjr;
#endif
   recompile_standard_i_type(state);
}

static void RJALR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.JALR;
#ifdef DYNAREC
   state->recomp_func = genjalr;
#endif
   recompile_standard_r_type(state);
}

static void RSYSCALL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SYSCALL;
#ifdef DYNAREC
   state->recomp_func = gensyscall;
#endif
}

/* Idle loop hack from 64th Note */
static void RBREAK(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.BREAK;
#ifdef DYNAREC
   state->recomp_func = genbreak;
#endif
}

static void RSYNC(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SYNC;
#ifdef DYNAREC
   state->recomp_func = gensync;
#endif
}

static void RMFHI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MFHI;
#ifdef DYNAREC
   state->recomp_func = genmfhi;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RMTHI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MTHI;
#ifdef DYNAREC
   state->recomp_func = genmthi;
#endif
   recompile_standard_r_type(state);
}

static void RMFLO(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MFLO;
#ifdef DYNAREC
   state->recomp_func = genmflo;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RMTLO(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MTLO;
#ifdef DYNAREC
   state->recomp_func = genmtlo;
#endif
   recompile_standard_r_type(state);
}

static void RDSLLV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSLLV;
#ifdef DYNAREC
   state->recomp_func = gendsllv;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRLV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRLV;
#ifdef DYNAREC
   state->recomp_func = gendsrlv;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRAV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRAV;
#ifdef DYNAREC
   state->recomp_func = gendsrav;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RMULT(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MULT;
#ifdef DYNAREC
   state->recomp_func = genmult;
#endif
   recompile_standard_r_type(state);
}

static void RMULTU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MULTU;
#ifdef DYNAREC
   state->recomp_func = genmultu;
#endif
   recompile_standard_r_type(state);
}

static void RDIV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DIV;
#ifdef DYNAREC
   state->recomp_func = gendiv;
#endif
   recompile_standard_r_type(state);
}

static void RDIVU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DIVU;
#ifdef DYNAREC
   state->recomp_func = gendivu;
#endif
   recompile_standard_r_type(state);
}

static void RDMULT(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DMULT;
#ifdef DYNAREC
   state->recomp_func = gendmult;
#endif
   recompile_standard_r_type(state);
}

static void RDMULTU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DMULTU;
#ifdef DYNAREC
   state->recomp_func = gendmultu;
#endif
   recompile_standard_r_type(state);
}

static void RDDIV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DDIV;
#ifdef DYNAREC
   state->recomp_func = genddiv;
#endif
   recompile_standard_r_type(state);
}

static void RDDIVU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DDIVU;
#ifdef DYNAREC
   state->recomp_func = genddivu;
#endif
   recompile_standard_r_type(state);
}

static void RADD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADD;
#ifdef DYNAREC
   state->recomp_func = genadd;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RADDU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADDU;
#ifdef DYNAREC
   state->recomp_func = genaddu;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSUB(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SUB;
#ifdef DYNAREC
   state->recomp_func = gensub;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSUBU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SUBU;
#ifdef DYNAREC
   state->recomp_func = gensubu;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RAND(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.AND;
#ifdef DYNAREC
   state->recomp_func = genand;
#endif
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void ROR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.OR;
#ifdef DYNAREC
   state->recomp_func = genor;
#endif
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RXOR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.XOR;
#ifdef DYNAREC
   state->recomp_func = genxor;
#endif
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RNOR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NOR;
#ifdef DYNAREC
   state->recomp_func = gennor;
#endif
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSLT(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLT;
#ifdef DYNAREC
   state->recomp_func = genslt;
#endif
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSLTU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLTU;
#ifdef DYNAREC
   state->recomp_func = gensltu;
#endif
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDADD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DADD;
#ifdef DYNAREC
   state->recomp_func = gendadd;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDADDU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DADDU;
#ifdef DYNAREC
   state->recomp_func = gendaddu;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSUB(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSUB;
#ifdef DYNAREC
   state->recomp_func = gendsub;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSUBU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSUBU;
#ifdef DYNAREC
   state->recomp_func = gendsubu;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RTGE(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
#ifdef DYNAREC
   state->recomp_func = genni;
#endif
}

static void RTGEU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
#ifdef DYNAREC
   state->recomp_func = genni;
#endif
}

static void RTLT(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
#ifdef DYNAREC
   state->recomp_func = genni;
#endif
}

static void RTLTU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
#ifdef DYNAREC
   state->recomp_func = genni;
#endif
}

static void RTEQ(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TEQ;
#ifdef DYNAREC
   state->recomp_func = genteq;
#endif
   recompile_standard_r_type(state);
}

static void RTNE(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
#ifdef DYNAREC
   state->recomp_func = genni;
#endif
}

static void RDSLL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSLL;
#ifdef DYNAREC
   state->recomp_func = gendsll;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRL;
#ifdef DYNAREC
   state->recomp_func = gendsrl;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRA(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRA;
#ifdef DYNAREC
   state->recomp_func = gendsra;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSLL32(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSLL32;
#ifdef DYNAREC
   state->recomp_func = gendsll32;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRL32(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRL32;
#ifdef DYNAREC
   state->recomp_func = gendsrl32;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRA32(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRA32;
#ifdef DYNAREC
   state->recomp_func = gendsra32;
#endif
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void (*recomp_special[64])(usf_state_t *) =
{
   RSLL , RSV   , RSRL , RSRA , RSLLV   , RSV    , RSRLV  , RSRAV  ,
   RJR  , RJALR , RSV  , RSV  , RSYSCALL, RBREAK , RSV    , RSYNC  ,
   RMFHI, RMTHI , RMFLO, RMTLO, RDSLLV  , RSV    , RDSRLV , RDSRAV ,
   RMULT, RMULTU, RDIV , RDIVU, RDMULT  , RDMULTU, RDDIV  , RDDIVU ,
   RADD , RADDU , RSUB , RSUBU, RAND    , ROR    , RXOR   , RNOR   ,
   RSV  , RSV   , RSLT , RSLTU, RDADD   , RDADDU , RDSUB  , RDSUBU ,
   RTGE , RTGEU , RTLT , RTLTU, RTEQ    , RSV    , RTNE   , RSV    ,
   RDSLL, RSV   , RDSRL, RDSRA, RDSLL32 , RSV    , RDSRL32, RDSRA32
};

//-------------------------------------------------------------------------
//                                   REGIMM                                
//-------------------------------------------------------------------------

static void RBLTZ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLTZ;
#ifdef DYNAREC
   state->recomp_func = genbltz;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLTZ_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbltz_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLTZ_OUT;
#ifdef DYNAREC
      state->recomp_func = genbltz_out;
#endif
   }
}

static void RBGEZ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGEZ;
#ifdef DYNAREC
   state->recomp_func = genbgez;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGEZ_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbgez_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGEZ_OUT;
#ifdef DYNAREC
      state->recomp_func = genbgez_out;
#endif
   }
}

static void RBLTZL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLTZL;
#ifdef DYNAREC
   state->recomp_func = genbltzl;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLTZL_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbltzl_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLTZL_OUT;
#ifdef DYNAREC
      state->recomp_func = genbltzl_out;
#endif
   }
}

static void RBGEZL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGEZL;
#ifdef DYNAREC
   state->recomp_func = genbgezl;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGEZL_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbgezl_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGEZL_OUT;
#ifdef DYNAREC
      state->recomp_func = genbgezl_out;
#endif
   }
}

static void RTGEI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
#ifdef DYNAREC
   state->recomp_func = genni;
#endif
}

static void RTGEIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
#ifdef DYNAREC
   state->recomp_func = genni;
#endif
}

static void RTLTI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
#ifdef DYNAREC
   state->recomp_func = genni;
#endif
}

static void RTLTIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
#ifdef DYNAREC
   state->recomp_func = genni;
#endif
}

static void RTEQI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
#ifdef DYNAREC
   state->recomp_func = genni;
#endif
}

static void RTNEI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
#ifdef DYNAREC
   state->recomp_func = genni;
#endif
}

static void RBLTZAL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLTZAL;
#ifdef DYNAREC
   state->recomp_func = genbltzal;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLTZAL_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbltzal_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLTZAL_OUT;
#ifdef DYNAREC
      state->recomp_func = genbltzal_out;
#endif
   }
}

static void RBGEZAL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGEZAL;
#ifdef DYNAREC
   state->recomp_func = genbgezal;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGEZAL_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbgezal_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGEZAL_OUT;
#ifdef DYNAREC
      state->recomp_func = genbgezal_out;
#endif
   }
}

static void RBLTZALL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLTZALL;
#ifdef DYNAREC
   state->recomp_func = genbltzall;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLTZALL_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbltzall_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLTZALL_OUT;
#ifdef DYNAREC
      state->recomp_func = genbltzall_out;
#endif
   }
}

static void RBGEZALL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGEZALL;
#ifdef DYNAREC
   state->recomp_func = genbgezall;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGEZALL_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbgezall_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGEZALL_OUT;
#ifdef DYNAREC
      state->recomp_func = genbgezall_out;
#endif
   }
}

static void (*recomp_regimm[32])(usf_state_t *) =
{
   RBLTZ  , RBGEZ  , RBLTZL  , RBGEZL  , RSV  , RSV, RSV  , RSV,
   RTGEI  , RTGEIU , RTLTI   , RTLTIU  , RTEQI, RSV, RTNEI, RSV,
   RBLTZAL, RBGEZAL, RBLTZALL, RBGEZALL, RSV  , RSV, RSV  , RSV,
   RSV    , RSV    , RSV     , RSV     , RSV  , RSV, RSV  , RSV
};

//-------------------------------------------------------------------------
//                                     TLB                                 
//-------------------------------------------------------------------------

static void RTLBR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TLBR;
#ifdef DYNAREC
   state->recomp_func = gentlbr;
#endif
}

static void RTLBWI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TLBWI;
#ifdef DYNAREC
   state->recomp_func = gentlbwi;
#endif
}

static void RTLBWR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TLBWR;
#ifdef DYNAREC
   state->recomp_func = gentlbwr;
#endif
}

static void RTLBP(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TLBP;
#ifdef DYNAREC
   state->recomp_func = gentlbp;
#endif
}

static void RERET(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ERET;
#ifdef DYNAREC
   state->recomp_func = generet;
#endif
}

static void (*recomp_tlb[64])(usf_state_t *) =
{
   RSV  , RTLBR, RTLBWI, RSV, RSV, RSV, RTLBWR, RSV, 
   RTLBP, RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RERET, RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV, 
   RSV  , RSV  , RSV   , RSV, RSV, RSV, RSV   , RSV
};

//-------------------------------------------------------------------------
//                                    COP0                                 
//-------------------------------------------------------------------------

static void RMFC0(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MFC0;
#ifdef DYNAREC
   state->recomp_func = genmfc0;
#endif
   recompile_standard_r_type(state);
   state->dst->f.r.rd = (long long*)(state->g_cp0_regs + ((state->src >> 11) & 0x1F));
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
   if (state->dst->f.r.rt == state->reg) RNOP(state);
}

static void RMTC0(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MTC0;
#ifdef DYNAREC
   state->recomp_func = genmtc0;
#endif
   recompile_standard_r_type(state);
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
}

static void RTLB(usf_state_t * state)
{
   recomp_tlb[(state->src & 0x3F)](state);
}

static void (*recomp_cop0[32])(usf_state_t *) =
{
   RMFC0, RSV, RSV, RSV, RMTC0, RSV, RSV, RSV,
   RSV  , RSV, RSV, RSV, RSV  , RSV, RSV, RSV,
   RTLB , RSV, RSV, RSV, RSV  , RSV, RSV, RSV,
   RSV  , RSV, RSV, RSV, RSV  , RSV, RSV, RSV
};

//-------------------------------------------------------------------------
//                                     BC                                  
//-------------------------------------------------------------------------

static void RBC1F(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BC1F;
#ifdef DYNAREC
   state->recomp_func = genbc1f;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BC1F_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbc1f_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BC1F_OUT;
#ifdef DYNAREC
      state->recomp_func = genbc1f_out;
#endif
   }
}

static void RBC1T(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BC1T;
#ifdef DYNAREC
   state->recomp_func = genbc1t;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BC1T_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbc1t_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BC1T_OUT;
#ifdef DYNAREC
      state->recomp_func = genbc1t_out;
#endif
   }
}

static void RBC1FL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BC1FL;
#ifdef DYNAREC
   state->recomp_func = genbc1fl;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BC1FL_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbc1fl_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BC1FL_OUT;
#ifdef DYNAREC
      state->recomp_func = genbc1fl_out;
#endif
   }
}

static void RBC1TL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BC1TL;
#ifdef DYNAREC
   state->recomp_func = genbc1tl;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BC1TL_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbc1tl_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BC1TL_OUT;
#ifdef DYNAREC
      state->recomp_func = genbc1tl_out;
#endif
   }
}

static void (*recomp_bc[4])(usf_state_t *) =
{
   RBC1F , RBC1T ,
   RBC1FL, RBC1TL
};

//-------------------------------------------------------------------------
//                                     S                                   
//-------------------------------------------------------------------------

static void RADD_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADD_S;
#ifdef DYNAREC
   state->recomp_func = genadd_s;
#endif
   recompile_standard_cf_type(state);
}

static void RSUB_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SUB_S;
#ifdef DYNAREC
   state->recomp_func = gensub_s;
#endif
   recompile_standard_cf_type(state);
}

static void RMUL_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MUL_S;
#ifdef DYNAREC
   state->recomp_func = genmul_s;
#endif
   recompile_standard_cf_type(state);
}

static void RDIV_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DIV_S;
#ifdef DYNAREC
   state->recomp_func = gendiv_s;
#endif
   recompile_standard_cf_type(state);
}

static void RSQRT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SQRT_S;
#ifdef DYNAREC
   state->recomp_func = gensqrt_s;
#endif
   recompile_standard_cf_type(state);
}

static void RABS_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ABS_S;
#ifdef DYNAREC
   state->recomp_func = genabs_s;
#endif
   recompile_standard_cf_type(state);
}

static void RMOV_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MOV_S;
#ifdef DYNAREC
   state->recomp_func = genmov_s;
#endif
   recompile_standard_cf_type(state);
}

static void RNEG_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NEG_S;
#ifdef DYNAREC
   state->recomp_func = genneg_s;
#endif
   recompile_standard_cf_type(state);
}

static void RROUND_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ROUND_L_S;
#ifdef DYNAREC
   state->recomp_func = genround_l_s;
#endif
   recompile_standard_cf_type(state);
}

static void RTRUNC_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TRUNC_L_S;
#ifdef DYNAREC
   state->recomp_func = gentrunc_l_s;
#endif
   recompile_standard_cf_type(state);
}

static void RCEIL_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CEIL_L_S;
#ifdef DYNAREC
   state->recomp_func = genceil_l_s;
#endif
   recompile_standard_cf_type(state);
}

static void RFLOOR_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FLOOR_L_S;
#ifdef DYNAREC
   state->recomp_func = genfloor_l_s;
#endif
   recompile_standard_cf_type(state);
}

static void RROUND_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ROUND_W_S;
#ifdef DYNAREC
   state->recomp_func = genround_w_s;
#endif
   recompile_standard_cf_type(state);
}

static void RTRUNC_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TRUNC_W_S;
#ifdef DYNAREC
   state->recomp_func = gentrunc_w_s;
#endif
   recompile_standard_cf_type(state);
}

static void RCEIL_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CEIL_W_S;
#ifdef DYNAREC
   state->recomp_func = genceil_w_s;
#endif
   recompile_standard_cf_type(state);
}

static void RFLOOR_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FLOOR_W_S;
#ifdef DYNAREC
   state->recomp_func = genfloor_w_s;
#endif
   recompile_standard_cf_type(state);
}

static void RCVT_D_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_D_S;
#ifdef DYNAREC
   state->recomp_func = gencvt_d_s;
#endif
   recompile_standard_cf_type(state);
}

static void RCVT_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_W_S;
#ifdef DYNAREC
   state->recomp_func = gencvt_w_s;
#endif
   recompile_standard_cf_type(state);
}

static void RCVT_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_L_S;
#ifdef DYNAREC
   state->recomp_func = gencvt_l_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_F_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_F_S;
#ifdef DYNAREC
   state->recomp_func = genc_f_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_UN_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_UN_S;
#ifdef DYNAREC
   state->recomp_func = genc_un_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_EQ_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_EQ_S;
#ifdef DYNAREC
   state->recomp_func = genc_eq_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_UEQ_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_UEQ_S;
#ifdef DYNAREC
   state->recomp_func = genc_ueq_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_OLT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_OLT_S;
#ifdef DYNAREC
   state->recomp_func = genc_olt_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_ULT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_ULT_S;
#ifdef DYNAREC
   state->recomp_func = genc_ult_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_OLE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_OLE_S;
#ifdef DYNAREC
   state->recomp_func = genc_ole_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_ULE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_ULE_S;
#ifdef DYNAREC
   state->recomp_func = genc_ule_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_SF_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_SF_S;
#ifdef DYNAREC
   state->recomp_func = genc_sf_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_NGLE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGLE_S;
#ifdef DYNAREC
   state->recomp_func = genc_ngle_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_SEQ_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_SEQ_S;
#ifdef DYNAREC
   state->recomp_func = genc_seq_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_NGL_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGL_S;
#ifdef DYNAREC
   state->recomp_func = genc_ngl_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_LT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_LT_S;
#ifdef DYNAREC
   state->recomp_func = genc_lt_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_NGE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGE_S;
#ifdef DYNAREC
   state->recomp_func = genc_nge_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_LE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_LE_S;
#ifdef DYNAREC
   state->recomp_func = genc_le_s;
#endif
   recompile_standard_cf_type(state);
}

static void RC_NGT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGT_S;
#ifdef DYNAREC
   state->recomp_func = genc_ngt_s;
#endif
   recompile_standard_cf_type(state);
}

static void (*recomp_s[64])(usf_state_t *) =
{
   RADD_S    , RSUB_S    , RMUL_S   , RDIV_S    , RSQRT_S   , RABS_S    , RMOV_S   , RNEG_S    , 
   RROUND_L_S, RTRUNC_L_S, RCEIL_L_S, RFLOOR_L_S, RROUND_W_S, RTRUNC_W_S, RCEIL_W_S, RFLOOR_W_S, 
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       , 
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       , 
   RSV       , RCVT_D_S  , RSV      , RSV       , RCVT_W_S  , RCVT_L_S  , RSV      , RSV       , 
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       , 
   RC_F_S    , RC_UN_S   , RC_EQ_S  , RC_UEQ_S  , RC_OLT_S  , RC_ULT_S  , RC_OLE_S , RC_ULE_S  , 
   RC_SF_S   , RC_NGLE_S , RC_SEQ_S , RC_NGL_S  , RC_LT_S   , RC_NGE_S  , RC_LE_S  , RC_NGT_S
};

//-------------------------------------------------------------------------
//                                     D                                   
//-------------------------------------------------------------------------

static void RADD_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADD_D;
#ifdef DYNAREC
   state->recomp_func = genadd_d;
#endif
   recompile_standard_cf_type(state);
}

static void RSUB_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SUB_D;
#ifdef DYNAREC
   state->recomp_func = gensub_d;
#endif
   recompile_standard_cf_type(state);
}

static void RMUL_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MUL_D;
#ifdef DYNAREC
   state->recomp_func = genmul_d;
#endif
   recompile_standard_cf_type(state);
}

static void RDIV_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DIV_D;
#ifdef DYNAREC
   state->recomp_func = gendiv_d;
#endif
   recompile_standard_cf_type(state);
}

static void RSQRT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SQRT_D;
#ifdef DYNAREC
   state->recomp_func = gensqrt_d;
#endif
   recompile_standard_cf_type(state);
}

static void RABS_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ABS_D;
#ifdef DYNAREC
   state->recomp_func = genabs_d;
#endif
   recompile_standard_cf_type(state);
}

static void RMOV_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MOV_D;
#ifdef DYNAREC
   state->recomp_func = genmov_d;
#endif
   recompile_standard_cf_type(state);
}

static void RNEG_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NEG_D;
#ifdef DYNAREC
   state->recomp_func = genneg_d;
#endif
   recompile_standard_cf_type(state);
}

static void RROUND_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ROUND_L_D;
#ifdef DYNAREC
   state->recomp_func = genround_l_d;
#endif
   recompile_standard_cf_type(state);
}

static void RTRUNC_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TRUNC_L_D;
#ifdef DYNAREC
   state->recomp_func = gentrunc_l_d;
#endif
   recompile_standard_cf_type(state);
}

static void RCEIL_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CEIL_L_D;
#ifdef DYNAREC
   state->recomp_func = genceil_l_d;
#endif
   recompile_standard_cf_type(state);
}

static void RFLOOR_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FLOOR_L_D;
#ifdef DYNAREC
   state->recomp_func = genfloor_l_d;
#endif
   recompile_standard_cf_type(state);
}

static void RROUND_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ROUND_W_D;
#ifdef DYNAREC
   state->recomp_func = genround_w_d;
#endif
   recompile_standard_cf_type(state);
}

static void RTRUNC_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TRUNC_W_D;
#ifdef DYNAREC
   state->recomp_func = gentrunc_w_d;
#endif
   recompile_standard_cf_type(state);
}

static void RCEIL_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CEIL_W_D;
#ifdef DYNAREC
   state->recomp_func = genceil_w_d;
#endif
   recompile_standard_cf_type(state);
}

static void RFLOOR_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FLOOR_W_D;
#ifdef DYNAREC
   state->recomp_func = genfloor_w_d;
#endif
   recompile_standard_cf_type(state);
}

static void RCVT_S_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_S_D;
#ifdef DYNAREC
   state->recomp_func = gencvt_s_d;
#endif
   recompile_standard_cf_type(state);
}

static void RCVT_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_W_D;
#ifdef DYNAREC
   state->recomp_func = gencvt_w_d;
#endif
   recompile_standard_cf_type(state);
}

static void RCVT_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_L_D;
#ifdef DYNAREC
   state->recomp_func = gencvt_l_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_F_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_F_D;
#ifdef DYNAREC
   state->recomp_func = genc_f_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_UN_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_UN_D;
#ifdef DYNAREC
   state->recomp_func = genc_un_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_EQ_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_EQ_D;
#ifdef DYNAREC
   state->recomp_func = genc_eq_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_UEQ_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_UEQ_D;
#ifdef DYNAREC
   state->recomp_func = genc_ueq_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_OLT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_OLT_D;
#ifdef DYNAREC
   state->recomp_func = genc_olt_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_ULT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_ULT_D;
#ifdef DYNAREC
   state->recomp_func = genc_ult_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_OLE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_OLE_D;
#ifdef DYNAREC
   state->recomp_func = genc_ole_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_ULE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_ULE_D;
#ifdef DYNAREC
   state->recomp_func = genc_ule_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_SF_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_SF_D;
#ifdef DYNAREC
   state->recomp_func = genc_sf_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_NGLE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGLE_D;
#ifdef DYNAREC
   state->recomp_func = genc_ngle_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_SEQ_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_SEQ_D;
#ifdef DYNAREC
   state->recomp_func = genc_seq_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_NGL_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGL_D;
#ifdef DYNAREC
   state->recomp_func = genc_ngl_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_LT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_LT_D;
#ifdef DYNAREC
   state->recomp_func = genc_lt_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_NGE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGE_D;
#ifdef DYNAREC
   state->recomp_func = genc_nge_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_LE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_LE_D;
#ifdef DYNAREC
   state->recomp_func = genc_le_d;
#endif
   recompile_standard_cf_type(state);
}

static void RC_NGT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGT_D;
#ifdef DYNAREC
   state->recomp_func = genc_ngt_d;
#endif
   recompile_standard_cf_type(state);
}

static void (*recomp_d[64])(usf_state_t *) =
{
   RADD_D    , RSUB_D    , RMUL_D   , RDIV_D    , RSQRT_D   , RABS_D    , RMOV_D   , RNEG_D    ,
   RROUND_L_D, RTRUNC_L_D, RCEIL_L_D, RFLOOR_L_D, RROUND_W_D, RTRUNC_W_D, RCEIL_W_D, RFLOOR_W_D,
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       ,
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       ,
   RCVT_S_D  , RSV       , RSV      , RSV       , RCVT_W_D  , RCVT_L_D  , RSV      , RSV       ,
   RSV       , RSV       , RSV      , RSV       , RSV       , RSV       , RSV      , RSV       ,
   RC_F_D    , RC_UN_D   , RC_EQ_D  , RC_UEQ_D  , RC_OLT_D  , RC_ULT_D  , RC_OLE_D , RC_ULE_D  ,
   RC_SF_D   , RC_NGLE_D , RC_SEQ_D , RC_NGL_D  , RC_LT_D   , RC_NGE_D  , RC_LE_D  , RC_NGT_D
};

//-------------------------------------------------------------------------
//                                     W                                   
//-------------------------------------------------------------------------

static void RCVT_S_W(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_S_W;
#ifdef DYNAREC
   state->recomp_func = gencvt_s_w;
#endif
   recompile_standard_cf_type(state);
}

static void RCVT_D_W(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_D_W;
#ifdef DYNAREC
   state->recomp_func = gencvt_d_w;
#endif
   recompile_standard_cf_type(state);
}

static void (*recomp_w[64])(usf_state_t *) =
{
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RCVT_S_W, RCVT_D_W, RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV
};

//-------------------------------------------------------------------------
//                                     L                                   
//-------------------------------------------------------------------------

static void RCVT_S_L(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_S_L;
#ifdef DYNAREC
   state->recomp_func = gencvt_s_l;
#endif
   recompile_standard_cf_type(state);
}

static void RCVT_D_L(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_D_L;
#ifdef DYNAREC
   state->recomp_func = gencvt_d_l;
#endif
   recompile_standard_cf_type(state);
}

static void (*recomp_l[64])(usf_state_t *) =
{
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV,
   RCVT_S_L, RCVT_D_L, RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
   RSV     , RSV     , RSV, RSV, RSV, RSV, RSV, RSV, 
};

//-------------------------------------------------------------------------
//                                    COP1                                 
//-------------------------------------------------------------------------

static void RMFC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MFC1;
#ifdef DYNAREC
   state->recomp_func = genmfc1;
#endif
   recompile_standard_r_type(state);
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
   if (state->dst->f.r.rt == state->reg) RNOP(state);
}

static void RDMFC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DMFC1;
#ifdef DYNAREC
   state->recomp_func = gendmfc1;
#endif
   recompile_standard_r_type(state);
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
   if (state->dst->f.r.rt == state->reg) RNOP(state);
}

static void RCFC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CFC1;
#ifdef DYNAREC
   state->recomp_func = gencfc1;
#endif
   recompile_standard_r_type(state);
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
   if (state->dst->f.r.rt == state->reg) RNOP(state);
}

static void RMTC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MTC1;
   recompile_standard_r_type(state);
#ifdef DYNAREC
   state->recomp_func = genmtc1;
#endif
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
}

static void RDMTC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DMTC1;
   recompile_standard_r_type(state);
#ifdef DYNAREC
   state->recomp_func = gendmtc1;
#endif
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
}

static void RCTC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CTC1;
   recompile_standard_r_type(state);
#ifdef DYNAREC
   state->recomp_func = genctc1;
#endif
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
}

static void RBC(usf_state_t * state)
{
   recomp_bc[((state->src >> 16) & 3)](state);
}

static void RS(usf_state_t * state)
{
   recomp_s[(state->src & 0x3F)](state);
}

static void RD(usf_state_t * state)
{
   recomp_d[(state->src & 0x3F)](state);
}

static void RW(usf_state_t * state)
{
   recomp_w[(state->src & 0x3F)](state);
}

static void RL(usf_state_t * state)
{
   recomp_l[(state->src & 0x3F)](state);
}

static void (*recomp_cop1[32])(usf_state_t *) =
{
   RMFC1, RDMFC1, RCFC1, RSV, RMTC1, RDMTC1, RCTC1, RSV,
   RBC  , RSV   , RSV  , RSV, RSV  , RSV   , RSV  , RSV,
   RS   , RD    , RSV  , RSV, RW   , RL    , RSV  , RSV,
   RSV  , RSV   , RSV  , RSV, RSV  , RSV   , RSV  , RSV
};

//-------------------------------------------------------------------------
//                                   R4300                                 
//-------------------------------------------------------------------------

static void RSPECIAL(usf_state_t * state)
{
   recomp_special[(state->src & 0x3F)](state);
}

static void RREGIMM(usf_state_t * state)
{
   recomp_regimm[((state->src >> 16) & 0x1F)](state);
}

static void RJ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.J;
#ifdef DYNAREC
   state->recomp_func = genj;
#endif
   recompile_standard_j_type(state);
   target = (state->dst->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.J_IDLE;
#ifdef DYNAREC
         state->recomp_func = genj_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.J_OUT;
#ifdef DYNAREC
      state->recomp_func = genj_out;
#endif
   }
}

static void RJAL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.JAL;
#ifdef DYNAREC
   state->recomp_func = genjal;
#endif
   recompile_standard_j_type(state);
   target = (state->dst->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.JAL_IDLE;
#ifdef DYNAREC
         state->recomp_func = genjal_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.JAL_OUT;
#ifdef DYNAREC
      state->recomp_func = genjal_out;
#endif
   }
}

static void RBEQ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BEQ;
#ifdef DYNAREC
   state->recomp_func = genbeq;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BEQ_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbeq_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BEQ_OUT;
#ifdef DYNAREC
      state->recomp_func = genbeq_out;
#endif
   }
}

static void RBNE(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BNE;
#ifdef DYNAREC
   state->recomp_func = genbne;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BNE_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbne_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BNE_OUT;
#ifdef DYNAREC
      state->recomp_func = genbne_out;
#endif
   }
}

static void RBLEZ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLEZ;
#ifdef DYNAREC
   state->recomp_func = genblez;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLEZ_IDLE;
#ifdef DYNAREC
         state->recomp_func = genblez_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLEZ_OUT;
#ifdef DYNAREC
      state->recomp_func = genblez_out;
#endif
   }
}

static void RBGTZ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGTZ;
#ifdef DYNAREC
   state->recomp_func = genbgtz;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGTZ_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbgtz_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGTZ_OUT;
#ifdef DYNAREC
      state->recomp_func = genbgtz_out;
#endif
   }
}

static void RADDI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADDI;
#ifdef DYNAREC
   state->recomp_func = genaddi;
#endif
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RADDIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADDIU;
#ifdef DYNAREC
   state->recomp_func = genaddiu;
#endif
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSLTI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLTI;
#ifdef DYNAREC
   state->recomp_func = genslti;
#endif
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSLTIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLTIU;
#ifdef DYNAREC
   state->recomp_func = gensltiu;
#endif
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RANDI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ANDI;
#ifdef DYNAREC
   state->recomp_func = genandi;
#endif
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RORI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ORI;
#ifdef DYNAREC
   state->recomp_func = genori;
#endif
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RXORI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.XORI;
#ifdef DYNAREC
   state->recomp_func = genxori;
#endif
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLUI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LUI;
#ifdef DYNAREC
   state->recomp_func = genlui;
#endif
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RCOP0(usf_state_t * state)
{
   recomp_cop0[((state->src >> 21) & 0x1F)](state);
}

static void RCOP1(usf_state_t * state)
{
   recomp_cop1[((state->src >> 21) & 0x1F)](state);
}

static void RBEQL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BEQL;
#ifdef DYNAREC
   state->recomp_func = genbeql;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BEQL_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbeql_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BEQL_OUT;
#ifdef DYNAREC
      state->recomp_func = genbeql_out;
#endif
   }
}

static void RBNEL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BNEL;
#ifdef DYNAREC
   state->recomp_func = genbnel;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BNEL_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbnel_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BNEL_OUT;
#ifdef DYNAREC
      state->recomp_func = genbnel_out;
#endif
   }
}

static void RBLEZL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLEZL;
#ifdef DYNAREC
   state->recomp_func = genblezl;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLEZL_IDLE;
#ifdef DYNAREC
         state->recomp_func = genblezl_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLEZL_OUT;
#ifdef DYNAREC
      state->recomp_func = genblezl_out;
#endif
   }
}

static void RBGTZL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGTZL;
#ifdef DYNAREC
   state->recomp_func = genbgtzl;
#endif
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGTZL_IDLE;
#ifdef DYNAREC
         state->recomp_func = genbgtzl_idle;
#endif
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGTZL_OUT;
#ifdef DYNAREC
      state->recomp_func = genbgtzl_out;
#endif
   }
}

static void RDADDI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DADDI;
#ifdef DYNAREC
   state->recomp_func = gendaddi;
#endif
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RDADDIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DADDIU;
#ifdef DYNAREC
   state->recomp_func = gendaddiu;
#endif
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLDL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LDL;
#ifdef DYNAREC
   state->recomp_func = genldl;
#endif
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLDR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LDR;
#ifdef DYNAREC
   state->recomp_func = genldr;
#endif
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLB(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LB;
#ifdef DYNAREC
   state->recomp_func = genlb;
#endif
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLH(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LH;
#ifdef DYNAREC
   state->recomp_func = genlh;
#endif
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLWL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LWL;
#ifdef DYNAREC
   state->recomp_func = genlwl;
#endif
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLW(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LW;
#ifdef DYNAREC
   state->recomp_func = genlw;
#endif
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLBU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LBU;
#ifdef DYNAREC
   state->recomp_func = genlbu;
#endif
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLHU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LHU;
#ifdef DYNAREC
   state->recomp_func = genlhu;
#endif
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLWR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LWR;
#ifdef DYNAREC
   state->recomp_func = genlwr;
#endif
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLWU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LWU;
#ifdef DYNAREC
   state->recomp_func = genlwu;
#endif
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSB(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SB;
#ifdef DYNAREC
   state->recomp_func = gensb;
#endif
   recompile_standard_i_type(state);
}

static void RSH(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SH;
#ifdef DYNAREC
   state->recomp_func = gensh;
#endif
   recompile_standard_i_type(state);
}

static void RSWL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SWL;
#ifdef DYNAREC
   state->recomp_func = genswl;
#endif
   recompile_standard_i_type(state);
}

static void RSW(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SW;
#ifdef DYNAREC
   state->recomp_func = gensw;
#endif
   recompile_standard_i_type(state);
}

static void RSDL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SDL;
#ifdef DYNAREC
   state->recomp_func = gensdl;
#endif
   recompile_standard_i_type(state);
}

static void RSDR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SDR;
#ifdef DYNAREC
   state->recomp_func = gensdr;
#endif
   recompile_standard_i_type(state);
}

static void RSWR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SWR;
#ifdef DYNAREC
   state->recomp_func = genswr;
#endif
   recompile_standard_i_type(state);
}

static void RCACHE(usf_state_t * state)
{
#ifdef DYNAREC
   state->recomp_func = gencache;
#endif
   state->dst->ops = state->current_instruction_table.CACHE;
}

static void RLL(usf_state_t * state)
{
#ifdef DYNAREC
   state->recomp_func = genll;
#endif
   state->dst->ops = state->current_instruction_table.LL;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLWC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LWC1;
#ifdef DYNAREC
   state->recomp_func = genlwc1;
#endif
   recompile_standard_lf_type(state);
}

static void RLLD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
#ifdef DYNAREC
   state->recomp_func = genni;
#endif
   recompile_standard_i_type(state);
}

static void RLDC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LDC1;
#ifdef DYNAREC
   state->recomp_func = genldc1;
#endif
   recompile_standard_lf_type(state);
}

static void RLD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LD;
#ifdef DYNAREC
   state->recomp_func = genld;
#endif
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSC(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SC;
#ifdef DYNAREC
   state->recomp_func = gensc;
#endif
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSWC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SWC1;
#ifdef DYNAREC
   state->recomp_func = genswc1;
#endif
   recompile_standard_lf_type(state);
}

static void RSCD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
#ifdef DYNAREC
   state->recomp_func = genni;
#endif
   recompile_standard_i_type(state);
}

static void RSDC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SDC1;
#ifdef DYNAREC
   state->recomp_func = gensdc1;
#endif
   recompile_standard_lf_type(state);
}

static void RSD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SD;
#ifdef DYNAREC
   state->recomp_func = gensd;
#endif
   recompile_standard_i_type(state);
}

static void (*recomp_ops[64])(usf_state_t *) =
{
   RSPECIAL, RREGIMM, RJ   , RJAL  , RBEQ , RBNE , RBLEZ , RBGTZ ,
   RADDI   , RADDIU , RSLTI, RSLTIU, RANDI, RORI , RXORI , RLUI  ,
   RCOP0   , RCOP1  , RSV  , RSV   , RBEQL, RBNEL, RBLEZL, RBGTZL,
   RDADDI  , RDADDIU, RLDL , RLDR  , RSV  , RSV  , RSV   , RSV   ,
   RLB     , RLH    , RLWL , RLW   , RLBU , RLHU , RLWR  , RLWU  ,
   RSB     , RSH    , RSWL , RSW   , RSDL , RSDR , RSWR  , RCACHE,
   RLL     , RLWC1  , RSV  , RSV   , RLLD , RLDC1, RSV   , RLD   ,
   RSC     , RSWC1  , RSV  , RSV   , RSCD , RSDC1, RSV   , RSD
};

static int get_block_length(const precomp_block *block)
{
  return (block->end-block->start)/4;
}

static size_t get_block_memsize(const precomp_block *block)
{
  int length = get_block_length(block);
  return ((length+1)+(length>>2)) * sizeof(precomp_instr);
}

/**********************************************************************
 ******************** initialize an empty block ***********************
 **********************************************************************/
void init_block(usf_state_t * state, precomp_block *block)
{
  int i, length, already_exist = 1;

  length = get_block_length(block);
   
  if (!block->block)
  {
    size_t memsize = get_block_memsize(block);
#ifdef DYNAREC
    if (state->r4300emu == CORE_DYNAREC) {
        block->block = (precomp_instr *) malloc_exec(state, memsize);
        if (!block->block) {
            DebugMessage(state, M64MSG_ERROR, "Memory error: couldn't allocate executable memory for dynamic recompiler. Try to use an interpreter mode.");
            return;
        }
    }
    else
#endif
    {
        block->block = (precomp_instr *) malloc(memsize);
        if (!block->block) {
            DebugMessage(state, M64MSG_ERROR, "Memory error: couldn't allocate memory for cached interpreter.");
            return;
        }
    }

    memset(block->block, 0, memsize);
    already_exist = 0;
  }

#ifdef DYNAREC
  if (state->r4300emu == CORE_DYNAREC)
  {
    if (!block->code)
    {
      state->max_code_length = 32768;
      block->code = (unsigned char *) malloc_exec(state, state->max_code_length);
    }
    else
    {
      state->max_code_length = block->max_code_length;
    }
    state->code_length = 0;
    state->inst_pointer = &block->code;
    
    if (block->jumps_table)
    {
      free(block->jumps_table);
      block->jumps_table = NULL;
    }
    if (block->riprel_table)
    {
      free(block->riprel_table);
      block->riprel_table = NULL;
    }
    init_assembler(state, NULL, 0, NULL, 0);
    init_cache(state, block->block);
  }
#endif
   
  if (!already_exist)
  {
    for (i=0; i<length; i++)
    {
      state->dst = block->block + i;
      state->dst->addr = block->start + i*4;
#ifdef DYNAREC
      state->dst->reg_cache_infos.need_map = 0;
#endif
      state->dst->local_addr = state->code_length;
      RNOTCOMPILED(state);
#ifdef DYNAREC
      if (state->r4300emu == CORE_DYNAREC) state->recomp_func(state);
#endif
    }
  state->init_length = state->code_length;
  }
  else
  {
    state->code_length = state->init_length; /* recompile everything, overwrite old recompiled instructions */
    for (i=0; i<length; i++)
    {
      state->dst = block->block + i;
#ifdef DYNAREC
      state->dst->reg_cache_infos.need_map = 0;
#endif
      state->dst->local_addr = i * (state->init_length / length);
      state->dst->ops = state->current_instruction_table.NOTCOMPILED;
    }
  }
   
#ifdef DYNAREC
  if (state->r4300emu == CORE_DYNAREC)
  {
    free_all_registers(state);
    /* calling pass2 of the assembler is not necessary here because all of the code emitted by
       gennotcompiled() and gendebug() is position-independent and contains no jumps . */
    block->code_length = state->code_length;
    block->max_code_length = state->max_code_length;
    free_assembler(state, &block->jumps_table, &block->jumps_number, &block->riprel_table, &block->riprel_number);
  }
#endif
   
  /* here we're marking the block as a valid code even if it's not compiled
   * yet as the game should have already set up the code correctly.
   */
  state->invalid_code[block->start>>12] = 0;
  if (block->end < 0x80000000 || block->start >= 0xc0000000)
  { 
    unsigned int paddr;
    
    paddr = virtual_to_physical_address(state, block->start, 2);
    state->invalid_code[paddr>>12] = 0;
    if (!state->blocks[paddr>>12])
    {
      state->blocks[paddr>>12] = (precomp_block *) malloc(sizeof(precomp_block));
      state->blocks[paddr>>12]->code = NULL;
      state->blocks[paddr>>12]->block = NULL;
      state->blocks[paddr>>12]->jumps_table = NULL;
      state->blocks[paddr>>12]->riprel_table = NULL;
      state->blocks[paddr>>12]->start = paddr & ~0xFFF;
      state->blocks[paddr>>12]->end = (paddr & ~0xFFF) + 0x1000;
    }
    init_block(state, state->blocks[paddr>>12]);
    
    paddr += block->end - block->start - 4;
    state->invalid_code[paddr>>12] = 0;
    if (!state->blocks[paddr>>12])
    {
      state->blocks[paddr>>12] = (precomp_block *) malloc(sizeof(precomp_block));
      state->blocks[paddr>>12]->code = NULL;
      state->blocks[paddr>>12]->block = NULL;
      state->blocks[paddr>>12]->jumps_table = NULL;
      state->blocks[paddr>>12]->riprel_table = NULL;
      state->blocks[paddr>>12]->start = paddr & ~0xFFF;
      state->blocks[paddr>>12]->end = (paddr & ~0xFFF) + 0x1000;
    }
    init_block(state, state->blocks[paddr>>12]);
  }
  else
  {
    if (block->start >= 0x80000000 && block->end < 0xa0000000 && state->invalid_code[(block->start+0x20000000)>>12])
    {
      if (!state->blocks[(block->start+0x20000000)>>12])
      {
        state->blocks[(block->start+0x20000000)>>12] = (precomp_block *) malloc(sizeof(precomp_block));
        state->blocks[(block->start+0x20000000)>>12]->code = NULL;
        state->blocks[(block->start+0x20000000)>>12]->block = NULL;
        state->blocks[(block->start+0x20000000)>>12]->jumps_table = NULL;
        state->blocks[(block->start+0x20000000)>>12]->riprel_table = NULL;
        state->blocks[(block->start+0x20000000)>>12]->start = (block->start+0x20000000) & ~0xFFF;
        state->blocks[(block->start+0x20000000)>>12]->end = ((block->start+0x20000000) & ~0xFFF) + 0x1000;
      }
      init_block(state, state->blocks[(block->start+0x20000000)>>12]);
    }
    if (block->start >= 0xa0000000 && block->end < 0xc0000000 && state->invalid_code[(block->start-0x20000000)>>12])
    {
      if (!state->blocks[(block->start-0x20000000)>>12])
      {
        state->blocks[(block->start-0x20000000)>>12] = (precomp_block *) malloc(sizeof(precomp_block));
        state->blocks[(block->start-0x20000000)>>12]->code = NULL;
        state->blocks[(block->start-0x20000000)>>12]->block = NULL;
        state->blocks[(block->start-0x20000000)>>12]->jumps_table = NULL;
        state->blocks[(block->start-0x20000000)>>12]->riprel_table = NULL;
        state->blocks[(block->start-0x20000000)>>12]->start = (block->start-0x20000000) & ~0xFFF;
        state->blocks[(block->start-0x20000000)>>12]->end = ((block->start-0x20000000) & ~0xFFF) + 0x1000;
      }
      init_block(state, state->blocks[(block->start-0x20000000)>>12]);
    }
  }
}

void free_block(usf_state_t * state, precomp_block *block)
{
#ifdef DYNAREC
    size_t memsize = get_block_memsize(block);
#endif

    if (block->block) {
#ifdef DYNAREC
        if (state->r4300emu == CORE_DYNAREC)
            free_exec(block->block, memsize);
        else
#endif
            free(block->block);
        block->block = NULL;
    }
#ifdef DYNAREC
    if (block->code) { free_exec(block->code, block->max_code_length); block->code = NULL; }
#endif
    if (block->jumps_table) { free(block->jumps_table); block->jumps_table = NULL; }
    if (block->riprel_table) { free(block->riprel_table); block->riprel_table = NULL; }
}

/**********************************************************************
 ********************* recompile a block of code **********************
 **********************************************************************/
void recompile_block(usf_state_t * state, int *source, precomp_block *block, unsigned int func)
{
   int i, length, finished=0;
   length = (block->end-block->start)/4;
   state->dst_block = block;
   
   //for (i=0; i<16; i++) block->md5[i] = 0;
   block->adler32 = 0;
   
#ifdef DYNAREC
   if (state->r4300emu == CORE_DYNAREC)
     {
    state->code_length = block->code_length;
    state->max_code_length = block->max_code_length;
    state->inst_pointer = &block->code;
    init_assembler(state, block->jumps_table, block->jumps_number, block->riprel_table, block->riprel_number);
    init_cache(state, block->block + (func & 0xFFF) / 4);
     }
#endif

   for (i = (func & 0xFFF) / 4; finished != 2; i++)
     {
    if(block->start < 0x80000000 || block->start >= 0xc0000000)
      {
          unsigned int address2 =
           virtual_to_physical_address(state, block->start + i*4, 0);
         if(state->blocks[address2>>12]->block[(address2&0xFFF)/4].ops == state->current_instruction_table.NOTCOMPILED)
           state->blocks[address2>>12]->block[(address2&0xFFF)/4].ops = state->current_instruction_table.NOTCOMPILED2;
      }
    
    state->SRC = source + i;
    state->src = source[i];
    state->check_nop = source[i+1] == 0;
    state->dst = block->block + i;
    state->dst->addr = block->start + i*4;
#ifdef DYNAREC
    state->dst->reg_cache_infos.need_map = 0;
#endif
    state->dst->local_addr = state->code_length;
    state->recomp_func = NULL;
    recomp_ops[((state->src >> 26) & 0x3F)](state);
#ifdef DYNAREC
    if (state->r4300emu == CORE_DYNAREC) state->recomp_func(state);
#endif
    state->dst = block->block + i;

    /*if ((dst+1)->ops != NOTCOMPILED && !delay_slot_compiled &&
        i < length)
      {
         if (r4300emu == CORE_DYNAREC) genlink_subblock();
         finished = 2;
      }*/
    if (state->delay_slot_compiled)
      {
         state->delay_slot_compiled--;
#ifdef DYNAREC
         free_all_registers(state);
#endif
      }
    
    if (i >= length-2+(length>>2)) finished = 2;
    if (i >= (length-1) && (block->start == 0xa4000000 ||
                block->start >= 0xc0000000 ||
                block->end   <  0x80000000)) finished = 2;
    if (state->dst->ops == state->current_instruction_table.ERET || finished == 1) finished = 2;
    if (/*i >= length &&*/ 
        (state->dst->ops == state->current_instruction_table.J ||
         state->dst->ops == state->current_instruction_table.J_OUT ||
         state->dst->ops == state->current_instruction_table.JR) &&
        !(i >= (length-1) && (block->start >= 0xc0000000 ||
                  block->end   <  0x80000000)))
      finished = 1;
     }

   if (i >= length)
     {
    state->dst = block->block + i;
    state->dst->addr = block->start + i*4;
#ifdef DYNAREC
    state->dst->reg_cache_infos.need_map = 0;
#endif
    state->dst->local_addr = state->code_length;
    RFIN_BLOCK(state);
    if (state->r4300emu == CORE_DYNAREC) state->recomp_func(state);
    i++;
    if (i < length-1+(length>>2)) // useful when last opcode is a jump
      {
         state->dst = block->block + i;
         state->dst->addr = block->start + i*4;
#ifdef DYNAREC
         state->dst->reg_cache_infos.need_map = 0;
#endif
         state->dst->local_addr = state->code_length;
         RFIN_BLOCK(state);
#ifdef DYNAREC
         if (state->r4300emu == CORE_DYNAREC) state->recomp_func(state);
#endif
         i++;
      }
     }
#ifdef DYNAREC
   else if (state->r4300emu == CORE_DYNAREC) genlink_subblock(state);

   if (state->r4300emu == CORE_DYNAREC)
     {
    free_all_registers(state);
    passe2(state, block->block, (func&0xFFF)/4, i, block);
    block->code_length = state->code_length;
    block->max_code_length = state->max_code_length;
    free_assembler(state, &block->jumps_table, &block->jumps_number, &block->riprel_table, &block->riprel_number);
     }
#endif
}

static int is_jump(usf_state_t * state)
{
   recomp_ops[((state->src >> 26) & 0x3F)](state);
   return
      (state->dst->ops == state->current_instruction_table.J ||
       state->dst->ops == state->current_instruction_table.J_OUT ||
       state->dst->ops == state->current_instruction_table.J_IDLE ||
       state->dst->ops == state->current_instruction_table.JAL ||
       state->dst->ops == state->current_instruction_table.JAL_OUT ||
       state->dst->ops == state->current_instruction_table.JAL_IDLE ||
       state->dst->ops == state->current_instruction_table.BEQ ||
       state->dst->ops == state->current_instruction_table.BEQ_OUT ||
       state->dst->ops == state->current_instruction_table.BEQ_IDLE ||
       state->dst->ops == state->current_instruction_table.BNE ||
       state->dst->ops == state->current_instruction_table.BNE_OUT ||
       state->dst->ops == state->current_instruction_table.BNE_IDLE ||
       state->dst->ops == state->current_instruction_table.BLEZ ||
       state->dst->ops == state->current_instruction_table.BLEZ_OUT ||
       state->dst->ops == state->current_instruction_table.BLEZ_IDLE ||
       state->dst->ops == state->current_instruction_table.BGTZ ||
       state->dst->ops == state->current_instruction_table.BGTZ_OUT ||
       state->dst->ops == state->current_instruction_table.BGTZ_IDLE ||
       state->dst->ops == state->current_instruction_table.BEQL ||
       state->dst->ops == state->current_instruction_table.BEQL_OUT ||
       state->dst->ops == state->current_instruction_table.BEQL_IDLE ||
       state->dst->ops == state->current_instruction_table.BNEL ||
       state->dst->ops == state->current_instruction_table.BNEL_OUT ||
       state->dst->ops == state->current_instruction_table.BNEL_IDLE ||
       state->dst->ops == state->current_instruction_table.BLEZL ||
       state->dst->ops == state->current_instruction_table.BLEZL_OUT ||
       state->dst->ops == state->current_instruction_table.BLEZL_IDLE ||
       state->dst->ops == state->current_instruction_table.BGTZL ||
       state->dst->ops == state->current_instruction_table.BGTZL_OUT ||
       state->dst->ops == state->current_instruction_table.BGTZL_IDLE ||
       state->dst->ops == state->current_instruction_table.JR ||
       state->dst->ops == state->current_instruction_table.JALR ||
       state->dst->ops == state->current_instruction_table.BLTZ ||
       state->dst->ops == state->current_instruction_table.BLTZ_OUT ||
       state->dst->ops == state->current_instruction_table.BLTZ_IDLE ||
       state->dst->ops == state->current_instruction_table.BGEZ ||
       state->dst->ops == state->current_instruction_table.BGEZ_OUT ||
       state->dst->ops == state->current_instruction_table.BGEZ_IDLE ||
       state->dst->ops == state->current_instruction_table.BLTZL ||
       state->dst->ops == state->current_instruction_table.BLTZL_OUT ||
       state->dst->ops == state->current_instruction_table.BLTZL_IDLE ||
       state->dst->ops == state->current_instruction_table.BGEZL ||
       state->dst->ops == state->current_instruction_table.BGEZL_OUT ||
       state->dst->ops == state->current_instruction_table.BGEZL_IDLE ||
       state->dst->ops == state->current_instruction_table.BLTZAL ||
       state->dst->ops == state->current_instruction_table.BLTZAL_OUT ||
       state->dst->ops == state->current_instruction_table.BLTZAL_IDLE ||
       state->dst->ops == state->current_instruction_table.BGEZAL ||
       state->dst->ops == state->current_instruction_table.BGEZAL_OUT ||
       state->dst->ops == state->current_instruction_table.BGEZAL_IDLE ||
       state->dst->ops == state->current_instruction_table.BLTZALL ||
       state->dst->ops == state->current_instruction_table.BLTZALL_OUT ||
       state->dst->ops == state->current_instruction_table.BLTZALL_IDLE ||
       state->dst->ops == state->current_instruction_table.BGEZALL ||
       state->dst->ops == state->current_instruction_table.BGEZALL_OUT ||
       state->dst->ops == state->current_instruction_table.BGEZALL_IDLE ||
       state->dst->ops == state->current_instruction_table.BC1F ||
       state->dst->ops == state->current_instruction_table.BC1F_OUT ||
       state->dst->ops == state->current_instruction_table.BC1F_IDLE ||
       state->dst->ops == state->current_instruction_table.BC1T ||
       state->dst->ops == state->current_instruction_table.BC1T_OUT ||
       state->dst->ops == state->current_instruction_table.BC1T_IDLE ||
       state->dst->ops == state->current_instruction_table.BC1FL ||
       state->dst->ops == state->current_instruction_table.BC1FL_OUT ||
       state->dst->ops == state->current_instruction_table.BC1FL_IDLE ||
       state->dst->ops == state->current_instruction_table.BC1TL ||
       state->dst->ops == state->current_instruction_table.BC1TL_OUT ||
       state->dst->ops == state->current_instruction_table.BC1TL_IDLE);
}

/**********************************************************************
 ************ recompile only one opcode (use for delay slot) **********
 **********************************************************************/
void recompile_opcode(usf_state_t * state)
{
   state->SRC++;
   state->src = *state->SRC;
   state->dst++;
   state->dst->addr = (state->dst-1)->addr + 4;
#ifdef DYNAREC
   state->dst->reg_cache_infos.need_map = 0;
#endif
   if(!is_jump(state))
   {
     state->recomp_func = NULL;
     recomp_ops[((state->src >> 26) & 0x3F)](state);
#ifdef DYNAREC
     if (state->r4300emu == CORE_DYNAREC) state->recomp_func(state);
#endif
   }
   else
   {
     RNOP(state);
#ifdef DYNAREC
     if (state->r4300emu == CORE_DYNAREC) state->recomp_func(state);
#endif
   }
   state->delay_slot_compiled = 2;
}

#ifdef DYNAREC
#if defined(__APPLE__)
static inline int macos_release()
{
   char buf[64];
   size_t size = sizeof(buf);
   int err = sysctlbyname("kern.osrelease", buf, &size, NULL, 0);
   if (err != 0) return 0;
   int major;
   if (sscanf(buf, "%d", &major) != 1) return 0;
   return major;
}
#endif

/**********************************************************************
 ************** allocate memory with executable bit set ***************
 **********************************************************************/
static void *malloc_exec(usf_state_t * state, size_t size)
{
#if defined(WIN32)
   return VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#elif defined(__GNUC__)

   #ifndef  MAP_ANONYMOUS
      #ifdef MAP_ANON
         #define MAP_ANONYMOUS MAP_ANON
      #endif
   #endif
    
   #ifndef MAP_JIT
      #define MAP_JIT 0
   #endif

   #if defined(__APPLE__)
   static int flags;
   /* Don't use MAP_JIT unless running on Mojave (release 18) or later */
   if (!flags) flags = MAP_PRIVATE | MAP_ANONYMOUS | (macos_release() >= 18 ? MAP_JIT : 0);

   void *block = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, flags, -1, 0);
   #else
   void *block = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_JIT, -1, 0);
   #endif

   if (block == MAP_FAILED)
       { DebugMessage(state, M64MSG_ERROR, "Memory error: couldn't allocate %zi byte block of aligned RWX memory.", size); return NULL; }

   return block;
#else
   return malloc(size);
#endif
}

/**********************************************************************
 ************* reallocate memory with executable bit set **************
 **********************************************************************/
void *realloc_exec(usf_state_t * state, void *ptr, size_t oldsize, size_t newsize)
{
   void* block = malloc_exec(state, newsize);
   if (block != NULL)
   {
      size_t copysize;
      if (oldsize < newsize)
         copysize = oldsize;
      else
         copysize = newsize;
      memcpy(block, ptr, copysize);
   }
   free_exec(ptr, oldsize);
   return block;
}

/**********************************************************************
 **************** frees memory with executable bit set ****************
 **********************************************************************/
static void free_exec(void *ptr, size_t length)
{
#if defined(WIN32)
   VirtualFree(ptr, 0, MEM_RELEASE);
#elif defined(__GNUC__)
   munmap(ptr, length);
#else
   free(ptr);
#endif
}
#endif
