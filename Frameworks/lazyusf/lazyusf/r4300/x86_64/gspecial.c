/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - gspecial.c                                              *
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

#include "r4300/cached_interp.h"
#include "r4300/recomph.h"
#include "r4300/recomp.h"
#include "r4300/r4300.h"
#include "r4300/ops.h"
#include "r4300/cp0.h"
#include "r4300/exception.h"

#if defined(COUNT_INSTR)
#include "r4300/instr_counters.h"
#endif

#if !defined(offsetof)
#   define offsetof(TYPE,MEMBER) ((unsigned int) &((TYPE*)0)->MEMBER)
#endif

void gensll(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[55]);
#endif
#ifdef INTERPRET_SLL
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SLL, 0);
#else
   int rt = allocate_register_32(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_32_w(state, (unsigned int *)state->dst->f.r.rd);
   
   mov_reg32_reg32(state, rd, rt);
   shl_reg32_imm8(state, rd, state->dst->f.r.sa);
#endif
}

void gensrl(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[56]);
#endif
#ifdef INTERPRET_SRL
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SRL, 0);
#else
   int rt = allocate_register_32(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_32_w(state, (unsigned int *)state->dst->f.r.rd);
   
   mov_reg32_reg32(state, rd, rt);
   shr_reg32_imm8(state, rd, state->dst->f.r.sa);
#endif
}

void gensra(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[57]);
#endif
#ifdef INTERPRET_SRA
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SRA, 0);
#else
   int rt = allocate_register_32(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_32_w(state, (unsigned int *)state->dst->f.r.rd);
   
   mov_reg32_reg32(state, rd, rt);
   sar_reg32_imm8(state, rd, state->dst->f.r.sa);
#endif
}

void gensllv(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[58]);
#endif
#ifdef INTERPRET_SLLV
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SLLV, 0);
#else
   int rt, rd;
   allocate_register_32_manually(state, ECX, (unsigned int *)state->dst->f.r.rs);
   
   rt = allocate_register_32(state, (unsigned int *)state->dst->f.r.rt);
   rd = allocate_register_32_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rd != ECX)
     {
    mov_reg32_reg32(state, rd, rt);
    shl_reg32_cl(state, rd);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rt);
    shl_reg32_cl(state, temp);
    mov_reg32_reg32(state, rd, temp);
     }
#endif
}

void gensrlv(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[59]);
#endif
#ifdef INTERPRET_SRLV
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SRLV, 0);
#else
   int rt, rd;
   allocate_register_32_manually(state, ECX, (unsigned int *)state->dst->f.r.rs);
   
   rt = allocate_register_32(state, (unsigned int *)state->dst->f.r.rt);
   rd = allocate_register_32_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rd != ECX)
     {
    mov_reg32_reg32(state, rd, rt);
    shr_reg32_cl(state, rd);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rt);
    shr_reg32_cl(state, temp);
    mov_reg32_reg32(state, rd, temp);
     }
#endif
}

void gensrav(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[60]);
#endif
#ifdef INTERPRET_SRAV
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SRAV, 0);
#else
   int rt, rd;
   allocate_register_32_manually(state, ECX, (unsigned int *)state->dst->f.r.rs);
   
   rt = allocate_register_32(state, (unsigned int *)state->dst->f.r.rt);
   rd = allocate_register_32_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rd != ECX)
     {
    mov_reg32_reg32(state, rd, rt);
    sar_reg32_cl(state, rd);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rt);
    sar_reg32_cl(state, temp);
    mov_reg32_reg32(state, rd, temp);
     }
#endif
}

void genjr(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[61]);
#endif
#ifdef INTERPRET_JR
   gencallinterp(state, (unsigned long long)state->current_instruction_table.JR, 1);
#else
   unsigned int diff = (unsigned int) offsetof(precomp_instr, local_addr);
   unsigned int diff_need = (unsigned int) offsetof(precomp_instr, reg_cache_infos.need_map);
   unsigned int diff_wrap = (unsigned int) offsetof(precomp_instr, reg_cache_infos.jump_wrapper);
   
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.JR, 1);
    return;
     }
   
   free_registers_move_start(state);

   mov_xreg32_m32rel(state, EAX, (unsigned int *)state->dst->f.i.rs);
   mov_m32rel_xreg32(state, (unsigned int *)&state->local_rs, EAX);
   
   gendelayslot(state);
   
   mov_xreg32_m32rel(state, EAX, (unsigned int *)&state->local_rs);
   mov_m32rel_xreg32(state, (unsigned int *)&state->last_addr, EAX);
   
   gencheck_interupt_reg(state);
   
   mov_xreg32_m32rel(state, EAX, (unsigned int *)&state->local_rs);
   mov_reg32_reg32(state, EBX, EAX);
   and_eax_imm32(state, 0xFFFFF000);
   cmp_eax_imm32(state, state->dst_block->start & 0xFFFFF000);
   je_near_rj(state, 0);

   jump_start_rel32(state);
   
   mov_m32rel_xreg32(state, &state->jump_to_address, EBX);
   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX);
   mov_reg64_imm64(state, RAX, (unsigned long long) jump_to_func);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, RAX);  /* will never return from call */

   jump_end_rel32(state);

   mov_reg64_imm64(state, RSI, (unsigned long long) state->dst_block->block);
   mov_reg32_reg32(state, EAX, EBX);
   sub_eax_imm32(state, state->dst_block->start);
   shr_reg32_imm8(state, EAX, 2);
   mul_m32rel(state, (unsigned int *)(&state->precomp_instr_size));
   
   mov_reg32_preg64preg64pimm32(state, EBX, RAX, RSI, diff_need);
   cmp_reg32_imm32(state, EBX, 1);
   jne_rj(state, 11);

   add_reg32_imm32(state, EAX, diff_wrap); // 6
   add_reg64_reg64(state, RAX, RSI); // 3
   jmp_reg64(state, RAX); // 2

   mov_reg32_preg64preg64pimm32(state, EBX, RAX, RSI, diff);
   mov_rax_memoffs64(state, (unsigned long long *) &state->dst_block->code);
   add_reg64_reg64(state, RAX, RBX);
   jmp_reg64(state, RAX);
#endif
}

void genjalr(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[62]);
#endif
#ifdef INTERPRET_JALR
   gencallinterp(state, (unsigned long long)state->current_instruction_table.JALR, 0);
#else
   unsigned int diff = (unsigned int) offsetof(precomp_instr, local_addr);
   unsigned int diff_need = (unsigned int) offsetof(precomp_instr, reg_cache_infos.need_map);
   unsigned int diff_wrap = (unsigned int) offsetof(precomp_instr, reg_cache_infos.jump_wrapper);
   
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.JALR, 1);
    return;
     }
   
   free_registers_move_start(state);

   mov_xreg32_m32rel(state, EAX, (unsigned int *)state->dst->f.r.rs);
   mov_m32rel_xreg32(state, (unsigned int *)&state->local_rs, EAX);
   
   gendelayslot(state);
   
   mov_m32rel_imm32(state, (unsigned int *)(state->dst-1)->f.r.rd, state->dst->addr+4);
   if ((state->dst->addr+4) & 0x80000000)
     mov_m32rel_imm32(state, ((unsigned int *)(state->dst-1)->f.r.rd)+1, 0xFFFFFFFF);
   else
     mov_m32rel_imm32(state, ((unsigned int *)(state->dst-1)->f.r.rd)+1, 0);
   
   mov_xreg32_m32rel(state, EAX, (unsigned int *)&state->local_rs);
   mov_m32rel_xreg32(state, (unsigned int *)&state->last_addr, EAX);
   
   gencheck_interupt_reg(state);
   
   mov_xreg32_m32rel(state, EAX, (unsigned int *)&state->local_rs);
   mov_reg32_reg32(state, EBX, EAX);
   and_eax_imm32(state, 0xFFFFF000);
   cmp_eax_imm32(state, state->dst_block->start & 0xFFFFF000);
   je_near_rj(state, 0);

   jump_start_rel32(state);
   
   mov_m32rel_xreg32(state, &state->jump_to_address, EBX);
   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX);
   mov_reg64_imm64(state, RAX, (unsigned long long) jump_to_func);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, RAX);  /* will never return from call */

   jump_end_rel32(state);

   mov_reg64_imm64(state, RSI, (unsigned long long) state->dst_block->block);
   mov_reg32_reg32(state, EAX, EBX);
   sub_eax_imm32(state, state->dst_block->start);
   shr_reg32_imm8(state, EAX, 2);
   mul_m32rel(state, (unsigned int *)(&state->precomp_instr_size));

   mov_reg32_preg64preg64pimm32(state, EBX, RAX, RSI, diff_need);
   cmp_reg32_imm32(state, EBX, 1);
   jne_rj(state, 11);

   add_reg32_imm32(state, EAX, diff_wrap); // 6
   add_reg64_reg64(state, RAX, RSI); // 3
   jmp_reg64(state, RAX); // 2

   mov_reg32_preg64preg64pimm32(state, EBX, RAX, RSI, diff);
   mov_rax_memoffs64(state, (unsigned long long *) &state->dst_block->code);
   add_reg64_reg64(state, RAX, RBX);
   jmp_reg64(state, RAX);
#endif
}

void gensyscall(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[63]);
#endif
#ifdef INTERPRET_SYSCALL
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SYSCALL, 0);
#else
   free_registers_move_start(state);

   mov_m32rel_imm32(state, &state->g_cp0_regs[CP0_CAUSE_REG], 8 << 2);
   gencallinterp(state, (unsigned long long)exception_general, 0);
#endif
}

void gensync(usf_state_t * state)
{
    (void)state;
}

void genmfhi(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[64]);
#endif
#ifdef INTERPRET_MFHI
   gencallinterp(state, (unsigned long long)state->current_instruction_table.MFHI, 0);
#else
   int rd = allocate_register_64_w(state, (unsigned long long *) state->dst->f.r.rd);
   int _hi = allocate_register_64(state, (unsigned long long *) &state->hi);
   
   mov_reg64_reg64(state, rd, _hi);
#endif
}

void genmthi(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[65]);
#endif
#ifdef INTERPRET_MTHI
   gencallinterp(state, (unsigned long long)state->current_instruction_table.MTHI, 0);
#else
   int _hi = allocate_register_64_w(state, (unsigned long long *) &state->hi);
   int rs = allocate_register_64(state, (unsigned long long *) state->dst->f.r.rs);

   mov_reg64_reg64(state, _hi, rs);
#endif
}

void genmflo(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[66]);
#endif
#ifdef INTERPRET_MFLO
   gencallinterp(state, (unsigned long long)state->current_instruction_table.MFLO, 0);
#else
   int rd = allocate_register_64_w(state, (unsigned long long *) state->dst->f.r.rd);
   int _lo = allocate_register_64(state, (unsigned long long *) &state->lo);
   
   mov_reg64_reg64(state, rd, _lo);
#endif
}

void genmtlo(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[67]);
#endif
#ifdef INTERPRET_MTLO
   gencallinterp(state, (unsigned long long)state->current_instruction_table.MTLO, 0);
#else
   int _lo = allocate_register_64_w(state, (unsigned long long *)&state->lo);
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rs);

   mov_reg64_reg64(state, _lo, rs);
#endif
}

void gendsllv(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[68]);
#endif
#ifdef INTERPRET_DSLLV
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DSLLV, 0);
#else
   int rt, rd;
   allocate_register_32_manually(state, ECX, (unsigned int *)state->dst->f.r.rs);
   
   rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);
   
   if (rd != ECX)
     {
    mov_reg64_reg64(state, rd, rt);
    shl_reg64_cl(state, rd);
     }
   else
     {
    int temp;
    temp = lru_register(state);
    free_register(state, temp);
    
    mov_reg64_reg64(state, temp, rt);
    shl_reg64_cl(state, temp);
    mov_reg64_reg64(state, rd, temp);
     }
#endif
}

void gendsrlv(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[69]);
#endif
#ifdef INTERPRET_DSRLV
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DSRLV, 0);
#else
   int rt, rd;
   allocate_register_32_manually(state, ECX, (unsigned int *)state->dst->f.r.rs);
   
   rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);
   
   if (rd != ECX)
     {
    mov_reg64_reg64(state, rd, rt);
    shr_reg64_cl(state, rd);
     }
   else
     {
    int temp;
    temp = lru_register(state);
    free_register(state, temp);
    
    mov_reg64_reg64(state, temp, rt);
    shr_reg64_cl(state, temp);
    mov_reg64_reg64(state, rd, temp);
     }
#endif
}

void gendsrav(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[70]);
#endif
#ifdef INTERPRET_DSRAV
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DSRAV, 0);
#else
   int rt, rd;
   allocate_register_32_manually(state, ECX, (unsigned int *)state->dst->f.r.rs);
   
   rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);
   
   if (rd != ECX)
     {
    mov_reg64_reg64(state, rd, rt);
    sar_reg64_cl(state, rd);
     }
   else
     {
    int temp;
    temp = lru_register(state);
    free_register(state, temp);
    
    mov_reg64_reg64(state, temp, rt);
    sar_reg64_cl(state, temp);
    mov_reg64_reg64(state, rd, temp);
     }
#endif
}

void genmult(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[71]);
#endif
#ifdef INTERPRET_MULT
   gencallinterp(state, (unsigned long long)state->current_instruction_table.MULT, 0);
#else
   int rs, rt;
   allocate_register_32_manually_w(state, EAX, (unsigned int *)&state->lo); /* these must be done first so they are not assigned by allocate_register() */
   allocate_register_32_manually_w(state, EDX, (unsigned int *)&state->hi);
   rs = allocate_register_32(state, (unsigned int*)state->dst->f.r.rs);
   rt = allocate_register_32(state, (unsigned int*)state->dst->f.r.rt);
   mov_reg32_reg32(state, EAX, rs);
   imul_reg32(state, rt);
#endif
}

void genmultu(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[72]);
#endif
#ifdef INTERPRET_MULTU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.MULTU, 0);
#else
   int rs, rt;
   allocate_register_32_manually_w(state, EAX, (unsigned int *)&state->lo);
   allocate_register_32_manually_w(state, EDX, (unsigned int *)&state->hi);
   rs = allocate_register_32(state, (unsigned int*)state->dst->f.r.rs);
   rt = allocate_register_32(state, (unsigned int*)state->dst->f.r.rt);
   mov_reg32_reg32(state, EAX, rs);
   mul_reg32(state, rt);
#endif
}

void gendiv(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[73]);
#endif
#ifdef INTERPRET_DIV
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DIV, 0);
#else
   int rs, rt;
   allocate_register_32_manually_w(state, EAX, (unsigned int *)&state->lo);
   allocate_register_32_manually_w(state, EDX, (unsigned int *)&state->hi);
   rs = allocate_register_32(state, (unsigned int*)state->dst->f.r.rs);
   rt = allocate_register_32(state, (unsigned int*)state->dst->f.r.rt);
   cmp_reg32_imm32(state, rt, 0);
   je_rj(state, (rs == EAX ? 0 : 2) + 1 + 2);
   mov_reg32_reg32(state, EAX, rs); // 0 or 2
   cdq(state); // 1
   idiv_reg32(state, rt); // 2
#endif
}

void gendivu(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[74]);
#endif
#ifdef INTERPRET_DIVU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DIVU, 0);
#else
   int rs, rt;
   allocate_register_32_manually_w(state, EAX, (unsigned int *)&state->lo);
   allocate_register_32_manually_w(state, EDX, (unsigned int *)&state->hi);
   rs = allocate_register_32(state, (unsigned int*)state->dst->f.r.rs);
   rt = allocate_register_32(state, (unsigned int*)state->dst->f.r.rt);
   cmp_reg32_imm32(state, rt, 0);
   je_rj(state, (rs == EAX ? 0 : 2) + 2 + 2);
   mov_reg32_reg32(state, EAX, rs); // 0 or 2
   xor_reg32_reg32(state, EDX, EDX); // 2
   div_reg32(state, rt); // 2
#endif
}

void gendmult(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[75]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DMULT, 0);
}

void gendmultu(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[76]);
#endif
#ifdef INTERPRET_DMULTU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DMULTU, 0);
#else
   free_registers_move_start(state);
   
   mov_xreg64_m64rel(state, RAX, (unsigned long long *) state->dst->f.r.rs);
   mov_xreg64_m64rel(state, RDX, (unsigned long long *) state->dst->f.r.rt);
   mul_reg64(state, RDX);
   mov_m64rel_xreg64(state, (unsigned long long *) &state->lo, RAX);
   mov_m64rel_xreg64(state, (unsigned long long *) &state->hi, RDX);
#endif
}

void genddiv(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[77]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DDIV, 0);
}

void genddivu(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[78]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DDIVU, 0);
}

void genadd(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[79]);
#endif
#ifdef INTERPRET_ADD
   gencallinterp(state, (unsigned long long)state->current_instruction_table.ADD, 0);
#else
   int rs = allocate_register_32(state, (unsigned int *)state->dst->f.r.rs);
   int rt = allocate_register_32(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_32_w(state, (unsigned int *)state->dst->f.r.rd);

   if (rs == rd)
     add_reg32_reg32(state, rd, rt);
   else if (rt == rd)
     add_reg32_reg32(state, rd, rs);
   else
     {
    mov_reg32_reg32(state, rd, rs);
    add_reg32_reg32(state, rd, rt);
     }
#endif
}

void genaddu(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[80]);
#endif
#ifdef INTERPRET_ADDU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.ADDU, 0);
#else
   int rs = allocate_register_32(state, (unsigned int *)state->dst->f.r.rs);
   int rt = allocate_register_32(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_32_w(state, (unsigned int *)state->dst->f.r.rd);

   if (rs == rd)
     add_reg32_reg32(state, rd, rt);
   else if (rt == rd)
     add_reg32_reg32(state, rd, rs);
   else
     {
    mov_reg32_reg32(state, rd, rs);
    add_reg32_reg32(state, rd, rt);
     }
#endif
}

void gensub(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[81]);
#endif
#ifdef INTERPRET_SUB
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SUB, 0);
#else
   int rs = allocate_register_32(state, (unsigned int *)state->dst->f.r.rs);
   int rt = allocate_register_32(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_32_w(state, (unsigned int *)state->dst->f.r.rd);

   if (rs == rd)
     sub_reg32_reg32(state, rd, rt);
   else if (rt == rd)
   {
     neg_reg32(state, rd);
     add_reg32_reg32(state, rd, rs);
   }
   else
   {
    mov_reg32_reg32(state, rd, rs);
    sub_reg32_reg32(state, rd, rt);
   }
#endif
}

void gensubu(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[82]);
#endif
#ifdef INTERPRET_SUBU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SUBU, 0);
#else
   int rs = allocate_register_32(state, (unsigned int *)state->dst->f.r.rs);
   int rt = allocate_register_32(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_32_w(state, (unsigned int *)state->dst->f.r.rd);

   if (rs == rd)
     sub_reg32_reg32(state, rd, rt);
   else if (rt == rd)
   {
     neg_reg32(state, rd);
     add_reg32_reg32(state, rd, rs);
   }
   else
     {
    mov_reg32_reg32(state, rd, rs);
    sub_reg32_reg32(state, rd, rt);
     }
#endif
}

void genand(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[83]);
#endif
#ifdef INTERPRET_AND
   gencallinterp(state, (unsigned long long)state->current_instruction_table.AND, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rs);
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);

   if (rs == rd)
     and_reg64_reg64(state, rd, rt);
   else if (rt == rd)
     and_reg64_reg64(state, rd, rs);
   else
     {
    mov_reg64_reg64(state, rd, rs);
    and_reg64_reg64(state, rd, rt);
     }
#endif
}

void genor(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[84]);
#endif
#ifdef INTERPRET_OR
   gencallinterp(state, (unsigned long long)state->current_instruction_table.OR, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rs);
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);

   if (rs == rd)
     or_reg64_reg64(state, rd, rt);
   else if (rt == rd)
     or_reg64_reg64(state, rd, rs);
   else
     {
    mov_reg64_reg64(state, rd, rs);
    or_reg64_reg64(state, rd, rt);
     }
#endif
}

void genxor(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[85]);
#endif
#ifdef INTERPRET_XOR
   gencallinterp(state, (unsigned long long)state->current_instruction_table.XOR, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rs);
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);

   if (rs == rd)
     xor_reg64_reg64(state, rd, rt);
   else if (rt == rd)
     xor_reg64_reg64(state, rd, rs);
   else
     {
    mov_reg64_reg64(state, rd, rs);
    xor_reg64_reg64(state, rd, rt);
     }
#endif
}

void gennor(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[86]);
#endif
#ifdef INTERPRET_NOR
   gencallinterp(state, (unsigned long long)state->current_instruction_table.NOR, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rs);
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);
   
   if (rs == rd)
   {
     or_reg64_reg64(state, rd, rt);
     not_reg64(state, rd);
   }
   else if (rt == rd)
   {
     or_reg64_reg64(state, rd, rs);
     not_reg64(state, rd);
   }
   else
   {
     mov_reg64_reg64(state, rd, rs);
     or_reg64_reg64(state, rd, rt);
     not_reg64(state, rd);
   }
#endif
}

void genslt(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[87]);
#endif
#ifdef INTERPRET_SLT
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SLT, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rs);
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);

   cmp_reg64_reg64(state, rs, rt);
   setl_reg8(state, rd);
   and_reg64_imm8(state, rd, 1);
#endif
}

void gensltu(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[88]);
#endif
#ifdef INTERPRET_SLTU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SLTU, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rs);
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);
   
   cmp_reg64_reg64(state, rs, rt);
   setb_reg8(state, rd);
   and_reg64_imm8(state, rd, 1);
#endif
}

void gendadd(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[89]);
#endif
#ifdef INTERPRET_DADD
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DADD, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rs);
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);

   if (rs == rd)
     add_reg64_reg64(state, rd, rt);
   else if (rt == rd)
     add_reg64_reg64(state, rd, rs);
   else
     {
    mov_reg64_reg64(state, rd, rs);
    add_reg64_reg64(state, rd, rt);
     }
#endif
}

void gendaddu(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[90]);
#endif
#ifdef INTERPRET_DADDU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DADDU, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rs);
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);

   if (rs == rd)
     add_reg64_reg64(state, rd, rt);
   else if (rt == rd)
     add_reg64_reg64(state, rd, rs);
   else
     {
    mov_reg64_reg64(state, rd, rs);
    add_reg64_reg64(state, rd, rt);
     }
#endif
}

void gendsub(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[91]);
#endif
#ifdef INTERPRET_DSUB
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DSUB, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rs);
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);

   if (rs == rd)
     sub_reg64_reg64(state, rd, rt);
   else if (rt == rd)
   {
     neg_reg64(state, rd);
     add_reg64_reg64(state, rd, rs);
   }
   else
   {
     mov_reg64_reg64(state, rd, rs);
     sub_reg64_reg64(state, rd, rt);
   }
#endif
}

void gendsubu(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[92]);
#endif
#ifdef INTERPRET_DSUBU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DSUBU, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rs);
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);

   if (rs == rd)
     sub_reg64_reg64(state, rd, rt);
   else if (rt == rd)
   {
     neg_reg64(state, rd);
     add_reg64_reg64(state, rd, rs);
   }
   else
   {
     mov_reg64_reg64(state, rd, rs);
     sub_reg64_reg64(state, rd, rt);
   }
#endif
}

void genteq(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[96]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.TEQ, 0);
}

void gendsll(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[93]);
#endif
#ifdef INTERPRET_DSLL
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DSLL, 0);
#else
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);

   mov_reg64_reg64(state, rd, rt);
   shl_reg64_imm8(state, rd, state->dst->f.r.sa);
#endif
}

void gendsrl(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[94]);
#endif
#ifdef INTERPRET_DSRL
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DSRL, 0);
#else
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);
   
   mov_reg64_reg64(state, rd, rt);
   shr_reg64_imm8(state, rd, state->dst->f.r.sa);
#endif
}

void gendsra(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[95]);
#endif
#ifdef INTERPRET_DSRA
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DSRA, 0);
#else
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);

   mov_reg64_reg64(state, rd, rt);
   sar_reg64_imm8(state, rd, state->dst->f.r.sa);
#endif
}

void gendsll32(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[97]);
#endif
#ifdef INTERPRET_DSLL32
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DSLL32, 0);
#else
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);

   mov_reg64_reg64(state, rd, rt);
   shl_reg64_imm8(state, rd, state->dst->f.r.sa + 32);
#endif
}

void gendsrl32(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[98]);
#endif
#ifdef INTERPRET_DSRL32
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DSRL32, 0);
#else
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);
   
   mov_reg64_reg64(state, rd, rt);
   shr_reg64_imm8(state, rd, state->dst->f.r.sa + 32);
#endif
}

void gendsra32(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[99]);
#endif
#ifdef INTERPRET_DSRA32
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DSRA32, 0);
#else
   int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.r.rt);
   int rd = allocate_register_64_w(state, (unsigned long long *)state->dst->f.r.rd);
   
   mov_reg64_reg64(state, rd, rt);
   sar_reg64_imm8(state, rd, state->dst->f.r.sa + 32);
#endif
}

/* Idle loop hack from 64th Note */
void genbreak(usf_state_t * state)
{
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BREAK, 0);
}

