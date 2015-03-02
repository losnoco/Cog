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

static void *malloc_exec(usf_state_t *, size_t size);
static void free_exec(void *ptr, size_t length);



static void RSV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.RESERVED;
   state->recomp_func = genreserved;
}

static void RFIN_BLOCK(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FIN_BLOCK;
   state->recomp_func = genfin_block;
}

static void RNOTCOMPILED(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NOTCOMPILED;
   state->recomp_func = gennotcompiled;
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
   state->recomp_func = gennop;
}

static void RSLL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLL;
   state->recomp_func = gensll;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSRL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SRL;
   state->recomp_func = gensrl;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSRA(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SRA;
   state->recomp_func = gensra;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSLLV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLLV;
   state->recomp_func = gensllv;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSRLV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SRLV;
   state->recomp_func = gensrlv;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSRAV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SRAV;
   state->recomp_func = gensrav;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RJR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.JR;
   state->recomp_func = genjr;
   recompile_standard_i_type(state);
}

static void RJALR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.JALR;
   state->recomp_func = genjalr;
   recompile_standard_r_type(state);
}

static void RSYSCALL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SYSCALL;
   state->recomp_func = gensyscall;
}

/* Idle loop hack from 64th Note */
static void RBREAK(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.BREAK;
   state->recomp_func = genbreak;
}

static void RSYNC(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SYNC;
   state->recomp_func = gensync;
}

static void RMFHI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MFHI;
   state->recomp_func = genmfhi;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RMTHI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MTHI;
   state->recomp_func = genmthi;
   recompile_standard_r_type(state);
}

static void RMFLO(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MFLO;
   state->recomp_func = genmflo;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RMTLO(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MTLO;
   state->recomp_func = genmtlo;
   recompile_standard_r_type(state);
}

static void RDSLLV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSLLV;
   state->recomp_func = gendsllv;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRLV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRLV;
   state->recomp_func = gendsrlv;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRAV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRAV;
   state->recomp_func = gendsrav;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RMULT(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MULT;
   state->recomp_func = genmult;
   recompile_standard_r_type(state);
}

static void RMULTU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MULTU;
   state->recomp_func = genmultu;
   recompile_standard_r_type(state);
}

static void RDIV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DIV;
   state->recomp_func = gendiv;
   recompile_standard_r_type(state);
}

static void RDIVU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DIVU;
   state->recomp_func = gendivu;
   recompile_standard_r_type(state);
}

static void RDMULT(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DMULT;
   state->recomp_func = gendmult;
   recompile_standard_r_type(state);
}

static void RDMULTU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DMULTU;
   state->recomp_func = gendmultu;
   recompile_standard_r_type(state);
}

static void RDDIV(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DDIV;
   state->recomp_func = genddiv;
   recompile_standard_r_type(state);
}

static void RDDIVU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DDIVU;
   state->recomp_func = genddivu;
   recompile_standard_r_type(state);
}

static void RADD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADD;
   state->recomp_func = genadd;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RADDU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADDU;
   state->recomp_func = genaddu;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSUB(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SUB;
   state->recomp_func = gensub;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSUBU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SUBU;
   state->recomp_func = gensubu;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RAND(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.AND;
   state->recomp_func = genand;
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void ROR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.OR;
   state->recomp_func = genor;
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RXOR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.XOR;
   state->recomp_func = genxor;
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RNOR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NOR;
   state->recomp_func = gennor;
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSLT(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLT;
   state->recomp_func = genslt;
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RSLTU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLTU;
   state->recomp_func = gensltu;
   recompile_standard_r_type(state);
   if(state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDADD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DADD;
   state->recomp_func = gendadd;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDADDU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DADDU;
   state->recomp_func = gendaddu;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSUB(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSUB;
   state->recomp_func = gendsub;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSUBU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSUBU;
   state->recomp_func = gendsubu;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RTGE(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
   state->recomp_func = genni;
}

static void RTGEU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
   state->recomp_func = genni;
}

static void RTLT(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
   state->recomp_func = genni;
}

static void RTLTU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
   state->recomp_func = genni;
}

static void RTEQ(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TEQ;
   state->recomp_func = genteq;
   recompile_standard_r_type(state);
}

static void RTNE(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
   state->recomp_func = genni;
}

static void RDSLL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSLL;
   state->recomp_func = gendsll;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRL;
   state->recomp_func = gendsrl;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRA(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRA;
   state->recomp_func = gendsra;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSLL32(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSLL32;
   state->recomp_func = gendsll32;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRL32(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRL32;
   state->recomp_func = gendsrl32;
   recompile_standard_r_type(state);
   if (state->dst->f.r.rd == state->reg) RNOP(state);
}

static void RDSRA32(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DSRA32;
   state->recomp_func = gendsra32;
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
   state->recomp_func = genbltz;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLTZ_IDLE;
         state->recomp_func = genbltz_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLTZ_OUT;
      state->recomp_func = genbltz_out;
   }
}

static void RBGEZ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGEZ;
   state->recomp_func = genbgez;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGEZ_IDLE;
         state->recomp_func = genbgez_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGEZ_OUT;
      state->recomp_func = genbgez_out;
   }
}

static void RBLTZL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLTZL;
   state->recomp_func = genbltzl;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLTZL_IDLE;
         state->recomp_func = genbltzl_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLTZL_OUT;
      state->recomp_func = genbltzl_out;
   }
}

static void RBGEZL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGEZL;
   state->recomp_func = genbgezl;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGEZL_IDLE;
         state->recomp_func = genbgezl_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGEZL_OUT;
      state->recomp_func = genbgezl_out;
   }
}

static void RTGEI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
   state->recomp_func = genni;
}

static void RTGEIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
   state->recomp_func = genni;
}

static void RTLTI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
   state->recomp_func = genni;
}

static void RTLTIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
   state->recomp_func = genni;
}

static void RTEQI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
   state->recomp_func = genni;
}

static void RTNEI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
   state->recomp_func = genni;
}

static void RBLTZAL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLTZAL;
   state->recomp_func = genbltzal;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLTZAL_IDLE;
         state->recomp_func = genbltzal_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLTZAL_OUT;
      state->recomp_func = genbltzal_out;
   }
}

static void RBGEZAL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGEZAL;
   state->recomp_func = genbgezal;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGEZAL_IDLE;
         state->recomp_func = genbgezal_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGEZAL_OUT;
      state->recomp_func = genbgezal_out;
   }
}

static void RBLTZALL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLTZALL;
   state->recomp_func = genbltzall;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLTZALL_IDLE;
         state->recomp_func = genbltzall_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLTZALL_OUT;
      state->recomp_func = genbltzall_out;
   }
}

static void RBGEZALL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGEZALL;
   state->recomp_func = genbgezall;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGEZALL_IDLE;
         state->recomp_func = genbgezall_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGEZALL_OUT;
      state->recomp_func = genbgezall_out;
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
   state->recomp_func = gentlbr;
}

static void RTLBWI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TLBWI;
   state->recomp_func = gentlbwi;
}

static void RTLBWR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TLBWR;
   state->recomp_func = gentlbwr;
}

static void RTLBP(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TLBP;
   state->recomp_func = gentlbp;
}

static void RERET(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ERET;
   state->recomp_func = generet;
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
   state->recomp_func = genmfc0;
   recompile_standard_r_type(state);
   state->dst->f.r.rd = (long long*)(state->g_cp0_regs + ((state->src >> 11) & 0x1F));
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
   if (state->dst->f.r.rt == state->reg) RNOP(state);
}

static void RMTC0(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MTC0;
   state->recomp_func = genmtc0;
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
   state->recomp_func = genbc1f;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BC1F_IDLE;
         state->recomp_func = genbc1f_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BC1F_OUT;
      state->recomp_func = genbc1f_out;
   }
}

static void RBC1T(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BC1T;
   state->recomp_func = genbc1t;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BC1T_IDLE;
         state->recomp_func = genbc1t_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BC1T_OUT;
      state->recomp_func = genbc1t_out;
   }
}

static void RBC1FL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BC1FL;
   state->recomp_func = genbc1fl;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BC1FL_IDLE;
         state->recomp_func = genbc1fl_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BC1FL_OUT;
      state->recomp_func = genbc1fl_out;
   }
}

static void RBC1TL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BC1TL;
   state->recomp_func = genbc1tl;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BC1TL_IDLE;
         state->recomp_func = genbc1tl_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BC1TL_OUT;
      state->recomp_func = genbc1tl_out;
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
   state->recomp_func = genadd_s;
   recompile_standard_cf_type(state);
}

static void RSUB_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SUB_S;
   state->recomp_func = gensub_s;
   recompile_standard_cf_type(state);
}

static void RMUL_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MUL_S;
   state->recomp_func = genmul_s;
   recompile_standard_cf_type(state);
}

static void RDIV_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DIV_S;
   state->recomp_func = gendiv_s;
   recompile_standard_cf_type(state);
}

static void RSQRT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SQRT_S;
   state->recomp_func = gensqrt_s;
   recompile_standard_cf_type(state);
}

static void RABS_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ABS_S;
   state->recomp_func = genabs_s;
   recompile_standard_cf_type(state);
}

static void RMOV_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MOV_S;
   state->recomp_func = genmov_s;
   recompile_standard_cf_type(state);
}

static void RNEG_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NEG_S;
   state->recomp_func = genneg_s;
   recompile_standard_cf_type(state);
}

static void RROUND_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ROUND_L_S;
   state->recomp_func = genround_l_s;
   recompile_standard_cf_type(state);
}

static void RTRUNC_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TRUNC_L_S;
   state->recomp_func = gentrunc_l_s;
   recompile_standard_cf_type(state);
}

static void RCEIL_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CEIL_L_S;
   state->recomp_func = genceil_l_s;
   recompile_standard_cf_type(state);
}

static void RFLOOR_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FLOOR_L_S;
   state->recomp_func = genfloor_l_s;
   recompile_standard_cf_type(state);
}

static void RROUND_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ROUND_W_S;
   state->recomp_func = genround_w_s;
   recompile_standard_cf_type(state);
}

static void RTRUNC_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TRUNC_W_S;
   state->recomp_func = gentrunc_w_s;
   recompile_standard_cf_type(state);
}

static void RCEIL_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CEIL_W_S;
   state->recomp_func = genceil_w_s;
   recompile_standard_cf_type(state);
}

static void RFLOOR_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FLOOR_W_S;
   state->recomp_func = genfloor_w_s;
   recompile_standard_cf_type(state);
}

static void RCVT_D_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_D_S;
   state->recomp_func = gencvt_d_s;
   recompile_standard_cf_type(state);
}

static void RCVT_W_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_W_S;
   state->recomp_func = gencvt_w_s;
   recompile_standard_cf_type(state);
}

static void RCVT_L_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_L_S;
   state->recomp_func = gencvt_l_s;
   recompile_standard_cf_type(state);
}

static void RC_F_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_F_S;
   state->recomp_func = genc_f_s;
   recompile_standard_cf_type(state);
}

static void RC_UN_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_UN_S;
   state->recomp_func = genc_un_s;
   recompile_standard_cf_type(state);
}

static void RC_EQ_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_EQ_S;
   state->recomp_func = genc_eq_s;
   recompile_standard_cf_type(state);
}

static void RC_UEQ_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_UEQ_S;
   state->recomp_func = genc_ueq_s;
   recompile_standard_cf_type(state);
}

static void RC_OLT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_OLT_S;
   state->recomp_func = genc_olt_s;
   recompile_standard_cf_type(state);
}

static void RC_ULT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_ULT_S;
   state->recomp_func = genc_ult_s;
   recompile_standard_cf_type(state);
}

static void RC_OLE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_OLE_S;
   state->recomp_func = genc_ole_s;
   recompile_standard_cf_type(state);
}

static void RC_ULE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_ULE_S;
   state->recomp_func = genc_ule_s;
   recompile_standard_cf_type(state);
}

static void RC_SF_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_SF_S;
   state->recomp_func = genc_sf_s;
   recompile_standard_cf_type(state);
}

static void RC_NGLE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGLE_S;
   state->recomp_func = genc_ngle_s;
   recompile_standard_cf_type(state);
}

static void RC_SEQ_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_SEQ_S;
   state->recomp_func = genc_seq_s;
   recompile_standard_cf_type(state);
}

static void RC_NGL_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGL_S;
   state->recomp_func = genc_ngl_s;
   recompile_standard_cf_type(state);
}

static void RC_LT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_LT_S;
   state->recomp_func = genc_lt_s;
   recompile_standard_cf_type(state);
}

static void RC_NGE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGE_S;
   state->recomp_func = genc_nge_s;
   recompile_standard_cf_type(state);
}

static void RC_LE_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_LE_S;
   state->recomp_func = genc_le_s;
   recompile_standard_cf_type(state);
}

static void RC_NGT_S(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGT_S;
   state->recomp_func = genc_ngt_s;
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
   state->recomp_func = genadd_d;
   recompile_standard_cf_type(state);
}

static void RSUB_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SUB_D;
   state->recomp_func = gensub_d;
   recompile_standard_cf_type(state);
}

static void RMUL_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MUL_D;
   state->recomp_func = genmul_d;
   recompile_standard_cf_type(state);
}

static void RDIV_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DIV_D;
   state->recomp_func = gendiv_d;
   recompile_standard_cf_type(state);
}

static void RSQRT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SQRT_D;
   state->recomp_func = gensqrt_d;
   recompile_standard_cf_type(state);
}

static void RABS_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ABS_D;
   state->recomp_func = genabs_d;
   recompile_standard_cf_type(state);
}

static void RMOV_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MOV_D;
   state->recomp_func = genmov_d;
   recompile_standard_cf_type(state);
}

static void RNEG_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NEG_D;
   state->recomp_func = genneg_d;
   recompile_standard_cf_type(state);
}

static void RROUND_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ROUND_L_D;
   state->recomp_func = genround_l_d;
   recompile_standard_cf_type(state);
}

static void RTRUNC_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TRUNC_L_D;
   state->recomp_func = gentrunc_l_d;
   recompile_standard_cf_type(state);
}

static void RCEIL_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CEIL_L_D;
   state->recomp_func = genceil_l_d;
   recompile_standard_cf_type(state);
}

static void RFLOOR_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FLOOR_L_D;
   state->recomp_func = genfloor_l_d;
   recompile_standard_cf_type(state);
}

static void RROUND_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ROUND_W_D;
   state->recomp_func = genround_w_d;
   recompile_standard_cf_type(state);
}

static void RTRUNC_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.TRUNC_W_D;
   state->recomp_func = gentrunc_w_d;
   recompile_standard_cf_type(state);
}

static void RCEIL_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CEIL_W_D;
   state->recomp_func = genceil_w_d;
   recompile_standard_cf_type(state);
}

static void RFLOOR_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.FLOOR_W_D;
   state->recomp_func = genfloor_w_d;
   recompile_standard_cf_type(state);
}

static void RCVT_S_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_S_D;
   state->recomp_func = gencvt_s_d;
   recompile_standard_cf_type(state);
}

static void RCVT_W_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_W_D;
   state->recomp_func = gencvt_w_d;
   recompile_standard_cf_type(state);
}

static void RCVT_L_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_L_D;
   state->recomp_func = gencvt_l_d;
   recompile_standard_cf_type(state);
}

static void RC_F_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_F_D;
   state->recomp_func = genc_f_d;
   recompile_standard_cf_type(state);
}

static void RC_UN_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_UN_D;
   state->recomp_func = genc_un_d;
   recompile_standard_cf_type(state);
}

static void RC_EQ_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_EQ_D;
   state->recomp_func = genc_eq_d;
   recompile_standard_cf_type(state);
}

static void RC_UEQ_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_UEQ_D;
   state->recomp_func = genc_ueq_d;
   recompile_standard_cf_type(state);
}

static void RC_OLT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_OLT_D;
   state->recomp_func = genc_olt_d;
   recompile_standard_cf_type(state);
}

static void RC_ULT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_ULT_D;
   state->recomp_func = genc_ult_d;
   recompile_standard_cf_type(state);
}

static void RC_OLE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_OLE_D;
   state->recomp_func = genc_ole_d;
   recompile_standard_cf_type(state);
}

static void RC_ULE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_ULE_D;
   state->recomp_func = genc_ule_d;
   recompile_standard_cf_type(state);
}

static void RC_SF_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_SF_D;
   state->recomp_func = genc_sf_d;
   recompile_standard_cf_type(state);
}

static void RC_NGLE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGLE_D;
   state->recomp_func = genc_ngle_d;
   recompile_standard_cf_type(state);
}

static void RC_SEQ_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_SEQ_D;
   state->recomp_func = genc_seq_d;
   recompile_standard_cf_type(state);
}

static void RC_NGL_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGL_D;
   state->recomp_func = genc_ngl_d;
   recompile_standard_cf_type(state);
}

static void RC_LT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_LT_D;
   state->recomp_func = genc_lt_d;
   recompile_standard_cf_type(state);
}

static void RC_NGE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGE_D;
   state->recomp_func = genc_nge_d;
   recompile_standard_cf_type(state);
}

static void RC_LE_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_LE_D;
   state->recomp_func = genc_le_d;
   recompile_standard_cf_type(state);
}

static void RC_NGT_D(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.C_NGT_D;
   state->recomp_func = genc_ngt_d;
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
   state->recomp_func = gencvt_s_w;
   recompile_standard_cf_type(state);
}

static void RCVT_D_W(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_D_W;
   state->recomp_func = gencvt_d_w;
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
   state->recomp_func = gencvt_s_l;
   recompile_standard_cf_type(state);
}

static void RCVT_D_L(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CVT_D_L;
   state->recomp_func = gencvt_d_l;
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
   state->recomp_func = genmfc1;
   recompile_standard_r_type(state);
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
   if (state->dst->f.r.rt == state->reg) RNOP(state);
}

static void RDMFC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DMFC1;
   state->recomp_func = gendmfc1;
   recompile_standard_r_type(state);
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
   if (state->dst->f.r.rt == state->reg) RNOP(state);
}

static void RCFC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CFC1;
   state->recomp_func = gencfc1;
   recompile_standard_r_type(state);
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
   if (state->dst->f.r.rt == state->reg) RNOP(state);
}

static void RMTC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.MTC1;
   recompile_standard_r_type(state);
   state->recomp_func = genmtc1;
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
}

static void RDMTC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DMTC1;
   recompile_standard_r_type(state);
   state->recomp_func = gendmtc1;
   state->dst->f.r.nrd = (state->src >> 11) & 0x1F;
}

static void RCTC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.CTC1;
   recompile_standard_r_type(state);
   state->recomp_func = genctc1;
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
   state->recomp_func = genj;
   recompile_standard_j_type(state);
   target = (state->dst->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.J_IDLE;
         state->recomp_func = genj_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.J_OUT;
      state->recomp_func = genj_out;
   }
}

static void RJAL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.JAL;
   state->recomp_func = genjal;
   recompile_standard_j_type(state);
   target = (state->dst->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.JAL_IDLE;
         state->recomp_func = genjal_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.JAL_OUT;
      state->recomp_func = genjal_out;
   }
}

static void RBEQ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BEQ;
   state->recomp_func = genbeq;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BEQ_IDLE;
         state->recomp_func = genbeq_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BEQ_OUT;
      state->recomp_func = genbeq_out;
   }
}

static void RBNE(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BNE;
   state->recomp_func = genbne;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BNE_IDLE;
         state->recomp_func = genbne_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BNE_OUT;
      state->recomp_func = genbne_out;
   }
}

static void RBLEZ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLEZ;
   state->recomp_func = genblez;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLEZ_IDLE;
         state->recomp_func = genblez_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLEZ_OUT;
      state->recomp_func = genblez_out;
   }
}

static void RBGTZ(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGTZ;
   state->recomp_func = genbgtz;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGTZ_IDLE;
         state->recomp_func = genbgtz_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGTZ_OUT;
      state->recomp_func = genbgtz_out;
   }
}

static void RADDI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADDI;
   state->recomp_func = genaddi;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RADDIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ADDIU;
   state->recomp_func = genaddiu;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSLTI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLTI;
   state->recomp_func = genslti;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSLTIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SLTIU;
   state->recomp_func = gensltiu;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RANDI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ANDI;
   state->recomp_func = genandi;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RORI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.ORI;
   state->recomp_func = genori;
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RXORI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.XORI;
   state->recomp_func = genxori;
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLUI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LUI;
   state->recomp_func = genlui;
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
   state->recomp_func = genbeql;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BEQL_IDLE;
         state->recomp_func = genbeql_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BEQL_OUT;
      state->recomp_func = genbeql_out;
   }
}

static void RBNEL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BNEL;
   state->recomp_func = genbnel;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BNEL_IDLE;
         state->recomp_func = genbnel_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BNEL_OUT;
      state->recomp_func = genbnel_out;
   }
}

static void RBLEZL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BLEZL;
   state->recomp_func = genblezl;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BLEZL_IDLE;
         state->recomp_func = genblezl_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BLEZL_OUT;
      state->recomp_func = genblezl_out;
   }
}

static void RBGTZL(usf_state_t * state)
{
   unsigned int target;
   state->dst->ops = state->current_instruction_table.BGTZL;
   state->recomp_func = genbgtzl;
   recompile_standard_i_type(state);
   target = state->dst->addr + state->dst->f.i.immediate*4 + 4;
   if (target == state->dst->addr)
   {
      if (state->check_nop)
      {
         state->dst->ops = state->current_instruction_table.BGTZL_IDLE;
         state->recomp_func = genbgtzl_idle;
      }
   }
   else if (target < state->dst_block->start || target >= state->dst_block->end || state->dst->addr == (state->dst_block->end-4))
   {
      state->dst->ops = state->current_instruction_table.BGTZL_OUT;
      state->recomp_func = genbgtzl_out;
   }
}

static void RDADDI(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DADDI;
   state->recomp_func = gendaddi;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RDADDIU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.DADDIU;
   state->recomp_func = gendaddiu;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLDL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LDL;
   state->recomp_func = genldl;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLDR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LDR;
   state->recomp_func = genldr;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLB(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LB;
   state->recomp_func = genlb;
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLH(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LH;
   state->recomp_func = genlh;
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLWL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LWL;
   state->recomp_func = genlwl;
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLW(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LW;
   state->recomp_func = genlw;
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLBU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LBU;
   state->recomp_func = genlbu;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLHU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LHU;
   state->recomp_func = genlhu;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLWR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LWR;
   state->recomp_func = genlwr;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLWU(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LWU;
   state->recomp_func = genlwu;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSB(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SB;
   state->recomp_func = gensb;
   recompile_standard_i_type(state);
}

static void RSH(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SH;
   state->recomp_func = gensh;
   recompile_standard_i_type(state);
}

static void RSWL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SWL;
   state->recomp_func = genswl;
   recompile_standard_i_type(state);
}

static void RSW(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SW;
   state->recomp_func = gensw;
   recompile_standard_i_type(state);
}

static void RSDL(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SDL;
   state->recomp_func = gensdl;
   recompile_standard_i_type(state);
}

static void RSDR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SDR;
   state->recomp_func = gensdr;
   recompile_standard_i_type(state);
}

static void RSWR(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SWR;
   state->recomp_func = genswr;
   recompile_standard_i_type(state);
}

static void RCACHE(usf_state_t * state)
{
   state->recomp_func = gencache;
   state->dst->ops = state->current_instruction_table.CACHE;
}

static void RLL(usf_state_t * state)
{
   state->recomp_func = genll;
   state->dst->ops = state->current_instruction_table.LL;
   recompile_standard_i_type(state);
   if(state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RLWC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LWC1;
   state->recomp_func = genlwc1;
   recompile_standard_lf_type(state);
}

static void RLLD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
   state->recomp_func = genni;
   recompile_standard_i_type(state);
}

static void RLDC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LDC1;
   state->recomp_func = genldc1;
   recompile_standard_lf_type(state);
}

static void RLD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.LD;
   state->recomp_func = genld;
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSC(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SC;
   state->recomp_func = gensc;
   recompile_standard_i_type(state);
   if (state->dst->f.i.rt == state->reg) RNOP(state);
}

static void RSWC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SWC1;
   state->recomp_func = genswc1;
   recompile_standard_lf_type(state);
}

static void RSCD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.NI;
   state->recomp_func = genni;
   recompile_standard_i_type(state);
}

static void RSDC1(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SDC1;
   state->recomp_func = gensdc1;
   recompile_standard_lf_type(state);
}

static void RSD(usf_state_t * state)
{
   state->dst->ops = state->current_instruction_table.SD;
   state->recomp_func = gensd;
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
    if (state->r4300emu == CORE_DYNAREC) {
        block->block = (precomp_instr *) malloc_exec(state, memsize);
        if (!block->block) {
            DebugMessage(state, M64MSG_ERROR, "Memory error: couldn't allocate executable memory for dynamic recompiler. Try to use an interpreter mode.");
            return;
        }
    }
    else {
        block->block = (precomp_instr *) malloc(memsize);
        if (!block->block) {
            DebugMessage(state, M64MSG_ERROR, "Memory error: couldn't allocate memory for cached interpreter.");
            return;
        }
    }

    memset(block->block, 0, memsize);
    already_exist = 0;
  }

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
   
  if (!already_exist)
  {
    for (i=0; i<length; i++)
    {
      state->dst = block->block + i;
      state->dst->addr = block->start + i*4;
      state->dst->reg_cache_infos.need_map = 0;
      state->dst->local_addr = state->code_length;
      RNOTCOMPILED(state);
      if (state->r4300emu == CORE_DYNAREC) state->recomp_func(state);
    }
  state->init_length = state->code_length;
  }
  else
  {
    state->code_length = state->init_length; /* recompile everything, overwrite old recompiled instructions */
    for (i=0; i<length; i++)
    {
      state->dst = block->block + i;
      state->dst->reg_cache_infos.need_map = 0;
      state->dst->local_addr = i * (state->init_length / length);
      state->dst->ops = state->current_instruction_table.NOTCOMPILED;
    }
  }
   
  if (state->r4300emu == CORE_DYNAREC)
  {
    free_all_registers(state);
    /* calling pass2 of the assembler is not necessary here because all of the code emitted by
       gennotcompiled() and gendebug() is position-independent and contains no jumps . */
    block->code_length = state->code_length;
    block->max_code_length = state->max_code_length;
    free_assembler(state, &block->jumps_table, &block->jumps_number, &block->riprel_table, &block->riprel_number);
  }
   
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
    size_t memsize = get_block_memsize(block);

    if (block->block) {
        if (state->r4300emu == CORE_DYNAREC)
            free_exec(block->block, memsize);
        else
            free(block->block);
        block->block = NULL;
    }
    if (block->code) { free_exec(block->code, block->max_code_length); block->code = NULL; }
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
   
   if (state->r4300emu == CORE_DYNAREC)
     {
    state->code_length = block->code_length;
    state->max_code_length = block->max_code_length;
    state->inst_pointer = &block->code;
    init_assembler(state, block->jumps_table, block->jumps_number, block->riprel_table, block->riprel_number);
    init_cache(state, block->block + (func & 0xFFF) / 4);
     }

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
    state->dst->reg_cache_infos.need_map = 0;
    state->dst->local_addr = state->code_length;
    state->recomp_func = NULL;
    recomp_ops[((state->src >> 26) & 0x3F)](state);
    if (state->r4300emu == CORE_DYNAREC) state->recomp_func(state);
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
         free_all_registers(state);
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
    state->dst->reg_cache_infos.need_map = 0;
    state->dst->local_addr = state->code_length;
    RFIN_BLOCK(state);
    if (state->r4300emu == CORE_DYNAREC) state->recomp_func(state);
    i++;
    if (i < length-1+(length>>2)) // useful when last opcode is a jump
      {
         state->dst = block->block + i;
         state->dst->addr = block->start + i*4;
         state->dst->reg_cache_infos.need_map = 0;
         state->dst->local_addr = state->code_length;
         RFIN_BLOCK(state);
         if (state->r4300emu == CORE_DYNAREC) state->recomp_func(state);
         i++;
      }
     }
   else if (state->r4300emu == CORE_DYNAREC) genlink_subblock(state);

   if (state->r4300emu == CORE_DYNAREC)
     {
    free_all_registers(state);
    passe2(state, block->block, (func&0xFFF)/4, i, block);
    block->code_length = state->code_length;
    block->max_code_length = state->max_code_length;
    free_assembler(state, &block->jumps_table, &block->jumps_number, &block->riprel_table, &block->riprel_number);
     }
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
   state->dst->reg_cache_infos.need_map = 0;
   if(!is_jump(state))
   {
     state->recomp_func = NULL;
     recomp_ops[((state->src >> 26) & 0x3F)](state);
     if (state->r4300emu == CORE_DYNAREC) state->recomp_func(state);
   }
   else
   {
     RNOP(state);
     if (state->r4300emu == CORE_DYNAREC) state->recomp_func(state);
   }
   state->delay_slot_compiled = 2;
}

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

   void *block = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
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
