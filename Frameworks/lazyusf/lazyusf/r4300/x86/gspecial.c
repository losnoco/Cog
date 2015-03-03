/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - gspecial.c                                              *
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
#include "r4300/recomp.h"
#include "r4300/r4300.h"
#include "r4300/ops.h"
#include "r4300/cp0.h"
#include "r4300/exception.h"

void gensll(usf_state_t * state)
{
#ifdef INTERPRET_SLL
   gencallinterp(state, (unsigned int)state->current_instruction_table.SLL, 0);
#else
   int rt = allocate_register(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_w(state, (unsigned int *)state->dst->f.r.rd);
   
   mov_reg32_reg32(state, rd, rt);
   shl_reg32_imm8(state, rd, state->dst->f.r.sa);
#endif
}

void gensrl(usf_state_t * state)
{
#ifdef INTERPRET_SRL
   gencallinterp(state, (unsigned int)state->current_instruction_table.SRL, 0);
#else
   int rt = allocate_register(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_w(state, (unsigned int *)state->dst->f.r.rd);
   
   mov_reg32_reg32(state, rd, rt);
   shr_reg32_imm8(state, rd, state->dst->f.r.sa);
#endif
}

void gensra(usf_state_t * state)
{
#ifdef INTERPRET_SRA
   gencallinterp(state, (unsigned int)state->current_instruction_table.SRA, 0);
#else
   int rt = allocate_register(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_w(state, (unsigned int *)state->dst->f.r.rd);
   
   mov_reg32_reg32(state, rd, rt);
   sar_reg32_imm8(state, rd, state->dst->f.r.sa);
#endif
}

void gensllv(usf_state_t * state)
{
#ifdef INTERPRET_SLLV
   gencallinterp(state, (unsigned int)state->current_instruction_table.SLLV, 0);
#else
   int rt, rd;
   allocate_register_manually(state, ECX, (unsigned int *)state->dst->f.r.rs);
   
   rt = allocate_register(state, (unsigned int *)state->dst->f.r.rt);
   rd = allocate_register_w(state, (unsigned int *)state->dst->f.r.rd);
   
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
#ifdef INTERPRET_SRLV
   gencallinterp(state, (unsigned int)state->current_instruction_table.SRLV, 0);
#else
   int rt, rd;
   allocate_register_manually(state, ECX, (unsigned int *)state->dst->f.r.rs);
   
   rt = allocate_register(state, (unsigned int *)state->dst->f.r.rt);
   rd = allocate_register_w(state, (unsigned int *)state->dst->f.r.rd);
   
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
#ifdef INTERPRET_SRAV
   gencallinterp(state, (unsigned int)state->current_instruction_table.SRAV, 0);
#else
   int rt, rd;
   allocate_register_manually(state, ECX, (unsigned int *)state->dst->f.r.rs);
   
   rt = allocate_register(state, (unsigned int *)state->dst->f.r.rt);
   rd = allocate_register_w(state, (unsigned int *)state->dst->f.r.rd);
   
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
#ifdef INTERPRET_JR
   gencallinterp(state, (unsigned int)state->current_instruction_table.JR, 1);
#else
   unsigned int diff =
     (unsigned int)(&state->dst->local_addr) - (unsigned int)(state->dst);
   unsigned int diff_need =
     (unsigned int)(&state->dst->reg_cache_infos.need_map) - (unsigned int)(state->dst);
   unsigned int diff_wrap =
     (unsigned int)(&state->dst->reg_cache_infos.jump_wrapper) - (unsigned int)(state->dst);
   
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.JR, 1);
    return;
     }
   
   free_all_registers(state);
   simplify_access(state);
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.i.rs);
   mov_memoffs32_eax(state, (unsigned int *)&state->local_rs);
   
   gendelayslot(state);
   
   mov_eax_memoffs32(state, (unsigned int *)&state->local_rs);
   mov_memoffs32_eax(state, (unsigned int *)&state->last_addr);
   
   gencheck_interupt_reg(state);
   
   mov_eax_memoffs32(state, (unsigned int *)&state->local_rs);
   mov_reg32_reg32(state, EBX, EAX);
   and_eax_imm32(state, 0xFFFFF000);
   cmp_eax_imm32(state, state->dst_block->start & 0xFFFFF000);
   je_near_rj(state, 0);

   jump_start_rel32(state);
   
   mov_m32_reg32(state, &state->jump_to_address, EBX);
   mov_m32_imm32(state, (unsigned int*)(&state->PC), (unsigned int)(state->dst+1));
   mov_reg32_imm32(state, EBX, (unsigned int)jump_to_func);
   mov_reg32_reg32(state, RP0, ESI);
   call_reg32(state, EBX);
   
   jump_end_rel32(state);
   
   mov_reg32_reg32(state, EAX, EBX);
   sub_eax_imm32(state, state->dst_block->start);
   shr_reg32_imm8(state, EAX, 2);
   mul_m32(state, (unsigned int *)(&state->precomp_instr_size));
   
   mov_reg32_preg32pimm32(state, EBX, EAX, (unsigned int)(state->dst_block->block)+diff_need);
   cmp_reg32_imm32(state, EBX, 1);
   jne_rj(state, 7);
   
   add_eax_imm32(state, (unsigned int)(state->dst_block->block)+diff_wrap); // 5
   jmp_reg32(state, EAX); // 2
   
   mov_reg32_preg32pimm32(state, EAX, EAX, (unsigned int)(state->dst_block->block)+diff);
   add_reg32_m32(state, EAX, (unsigned int *)(&state->dst_block->code));
   
   jmp_reg32(state, EAX);
#endif
}

void genjalr(usf_state_t * state)
{
#ifdef INTERPRET_JALR
   gencallinterp(state, (unsigned int)state->current_instruction_table.JALR, 0);
#else
   unsigned int diff =
     (unsigned int)(&state->dst->local_addr) - (unsigned int)(state->dst);
   unsigned int diff_need =
     (unsigned int)(&state->dst->reg_cache_infos.need_map) - (unsigned int)(state->dst);
   unsigned int diff_wrap =
     (unsigned int)(&state->dst->reg_cache_infos.jump_wrapper) - (unsigned int)(state->dst);
   
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.JALR, 1);
    return;
     }
   
   free_all_registers(state);
   simplify_access(state);
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.r.rs);
   mov_memoffs32_eax(state, (unsigned int *)&state->local_rs);
   
   gendelayslot(state);
   
   mov_m32_imm32(state, (unsigned int *)(state->dst-1)->f.r.rd, state->dst->addr+4);
   if ((state->dst->addr+4) & 0x80000000)
     mov_m32_imm32(state, ((unsigned int *)(state->dst-1)->f.r.rd)+1, 0xFFFFFFFF);
   else
     mov_m32_imm32(state, ((unsigned int *)(state->dst-1)->f.r.rd)+1, 0);
   
   mov_eax_memoffs32(state, (unsigned int *)&state->local_rs);
   mov_memoffs32_eax(state, (unsigned int *)&state->last_addr);
   
   gencheck_interupt_reg(state);
   
   mov_eax_memoffs32(state, (unsigned int *)&state->local_rs);
   mov_reg32_reg32(state, EBX, EAX);
   and_eax_imm32(state, 0xFFFFF000);
   cmp_eax_imm32(state, state->dst_block->start & 0xFFFFF000);
   je_near_rj(state, 0);

   jump_start_rel32(state);
   
   mov_m32_reg32(state, &state->jump_to_address, EBX);
   mov_m32_imm32(state, (unsigned int*)(&state->PC), (unsigned int)(state->dst+1));
   mov_reg32_imm32(state, EBX, (unsigned int)jump_to_func);
   mov_reg32_reg32(state, RP0, ESI);
   call_reg32(state, EBX);

   jump_end_rel32(state);
   
   mov_reg32_reg32(state, EAX, EBX);
   sub_eax_imm32(state, state->dst_block->start);
   shr_reg32_imm8(state, EAX, 2);
   mul_m32(state, (unsigned int *)(&state->precomp_instr_size));
   
   mov_reg32_preg32pimm32(state, EBX, EAX, (unsigned int)(state->dst_block->block)+diff_need);
   cmp_reg32_imm32(state, EBX, 1);
   jne_rj(state, 7);
   
   add_eax_imm32(state, (unsigned int)(state->dst_block->block)+diff_wrap); // 5
   jmp_reg32(state, EAX); // 2
   
   mov_reg32_preg32pimm32(state, EAX, EAX, (unsigned int)(state->dst_block->block)+diff);
   add_reg32_m32(state, EAX, (unsigned int *)(&state->dst_block->code));
   
   jmp_reg32(state, EAX);
#endif
}

void gensyscall(usf_state_t * state)
{
#ifdef INTERPRET_SYSCALL
   gencallinterp(state, (unsigned int)state->current_instruction_table.SYSCALL, 0);
#else
   free_all_registers(state);
   simplify_access(state);
   mov_m32_imm32(state, &state->g_cp0_regs[CP0_CAUSE_REG], 8 << 2);
   gencallinterp(state, (unsigned int)exception_general, 0);
#endif
}

void gensync(usf_state_t * state)
{
}

void genmfhi(usf_state_t * state)
{
#ifdef INTERPRET_MFHI
   gencallinterp(state, (unsigned int)state->current_instruction_table.MFHI, 0);
#else
   int rd1 = allocate_64_register1_w(state, (unsigned int*)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int*)state->dst->f.r.rd);
   int hi1 = allocate_64_register1(state, (unsigned int*)&state->hi);
   int hi2 = allocate_64_register2(state, (unsigned int*)&state->hi);
   
   mov_reg32_reg32(state, rd1, hi1);
   mov_reg32_reg32(state, rd2, hi2);
#endif
}

void genmthi(usf_state_t * state)
{
#ifdef INTERPRET_MTHI
   gencallinterp(state, (unsigned int)state->current_instruction_table.MTHI, 0);
#else
   int hi1 = allocate_64_register1_w(state, (unsigned int*)&state->hi);
   int hi2 = allocate_64_register2_w(state, (unsigned int*)&state->hi);
   int rs1 = allocate_64_register1(state, (unsigned int*)state->dst->f.r.rs);
   int rs2 = allocate_64_register2(state, (unsigned int*)state->dst->f.r.rs);
   
   mov_reg32_reg32(state, hi1, rs1);
   mov_reg32_reg32(state, hi2, rs2);
#endif
}

void genmflo(usf_state_t * state)
{
#ifdef INTERPRET_MFLO
   gencallinterp(state, (unsigned int)state->current_instruction_table.MFLO, 0);
#else
   int rd1 = allocate_64_register1_w(state, (unsigned int*)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int*)state->dst->f.r.rd);
   int lo1 = allocate_64_register1(state, (unsigned int*)&state->lo);
   int lo2 = allocate_64_register2(state, (unsigned int*)&state->lo);
   
   mov_reg32_reg32(state, rd1, lo1);
   mov_reg32_reg32(state, rd2, lo2);
#endif
}

void genmtlo(usf_state_t * state)
{
#ifdef INTERPRET_MTLO
   gencallinterp(state, (unsigned int)state->current_instruction_table.MTLO, 0);
#else
   int lo1 = allocate_64_register1_w(state, (unsigned int*)&state->lo);
   int lo2 = allocate_64_register2_w(state, (unsigned int*)&state->lo);
   int rs1 = allocate_64_register1(state, (unsigned int*)state->dst->f.r.rs);
   int rs2 = allocate_64_register2(state, (unsigned int*)state->dst->f.r.rs);
   
   mov_reg32_reg32(state, lo1, rs1);
   mov_reg32_reg32(state, lo2, rs2);
#endif
}

void gendsllv(usf_state_t * state)
{
#ifdef INTERPRET_DSLLV
   gencallinterp(state, (unsigned int)state->current_instruction_table.DSLLV, 0);
#else
   int rt1, rt2, rd1, rd2;
   allocate_register_manually(state, ECX, (unsigned int *)state->dst->f.r.rs);
   
   rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rd1 != ECX && rd2 != ECX)
     {
    mov_reg32_reg32(state, rd1, rt1);
    mov_reg32_reg32(state, rd2, rt2);
    shld_reg32_reg32_cl(state, rd2,rd1);
    shl_reg32_cl(state, rd1);
    test_reg32_imm32(state, ECX, 0x20);
    je_rj(state, 4);
    mov_reg32_reg32(state, rd2, rd1); // 2
    xor_reg32_reg32(state, rd1, rd1); // 2
     }
   else
     {
    int temp1, temp2;
    force_32(state, ECX);
    temp1 = lru_register(state);
    temp2 = lru_register_exc1(state, temp1);
    free_register(state, temp1);
    free_register(state, temp2);
    
    mov_reg32_reg32(state, temp1, rt1);
    mov_reg32_reg32(state, temp2, rt2);
    shld_reg32_reg32_cl(state, temp2, temp1);
    shl_reg32_cl(state, temp1);
    test_reg32_imm32(state, ECX, 0x20);
    je_rj(state, 4);
    mov_reg32_reg32(state, temp2, temp1); // 2
    xor_reg32_reg32(state, temp1, temp1); // 2
    
    mov_reg32_reg32(state, rd1, temp1);
    mov_reg32_reg32(state, rd2, temp2);
     }
#endif
}

void gendsrlv(usf_state_t * state)
{
#ifdef INTERPRET_DSRLV
   gencallinterp(state, (unsigned int)state->current_instruction_table.DSRLV, 0);
#else
   int rt1, rt2, rd1, rd2;
   allocate_register_manually(state, ECX, (unsigned int *)state->dst->f.r.rs);
   
   rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rd1 != ECX && rd2 != ECX)
     {
    mov_reg32_reg32(state, rd1, rt1);
    mov_reg32_reg32(state, rd2, rt2);
    shrd_reg32_reg32_cl(state, rd1,rd2);
    shr_reg32_cl(state, rd2);
    test_reg32_imm32(state, ECX, 0x20);
    je_rj(state, 4);
    mov_reg32_reg32(state, rd1, rd2); // 2
    xor_reg32_reg32(state, rd2, rd2); // 2
     }
   else
     {
    int temp1, temp2;
    force_32(state, ECX);
    temp1 = lru_register(state);
    temp2 = lru_register_exc1(state, temp1);
    free_register(state, temp1);
    free_register(state, temp2);
    
    mov_reg32_reg32(state, temp1, rt1);
    mov_reg32_reg32(state, temp2, rt2);
    shrd_reg32_reg32_cl(state, temp1, temp2);
    shr_reg32_cl(state, temp2);
    test_reg32_imm32(state, ECX, 0x20);
    je_rj(state, 4);
    mov_reg32_reg32(state, temp1, temp2); // 2
    xor_reg32_reg32(state, temp2, temp2); // 2
    
    mov_reg32_reg32(state, rd1, temp1);
    mov_reg32_reg32(state, rd2, temp2);
     }
#endif
}

void gendsrav(usf_state_t * state)
{
#ifdef INTERPRET_DSRAV
   gencallinterp(state, (unsigned int)state->current_instruction_table.DSRAV, 0);
#else
   int rt1, rt2, rd1, rd2;
   allocate_register_manually(state, ECX, (unsigned int *)state->dst->f.r.rs);
   
   rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rd1 != ECX && rd2 != ECX)
     {
    mov_reg32_reg32(state, rd1, rt1);
    mov_reg32_reg32(state, rd2, rt2);
    shrd_reg32_reg32_cl(state, rd1,rd2);
    sar_reg32_cl(state, rd2);
    test_reg32_imm32(state, ECX, 0x20);
    je_rj(state, 5);
    mov_reg32_reg32(state, rd1, rd2); // 2
    sar_reg32_imm8(state, rd2, 31); // 3
     }
   else
     {
    int temp1, temp2;
    force_32(state, ECX);
    temp1 = lru_register(state);
    temp2 = lru_register_exc1(state, temp1);
    free_register(state, temp1);
    free_register(state, temp2);
    
    mov_reg32_reg32(state, temp1, rt1);
    mov_reg32_reg32(state, temp2, rt2);
    shrd_reg32_reg32_cl(state, temp1, temp2);
    sar_reg32_cl(state, temp2);
    test_reg32_imm32(state, ECX, 0x20);
    je_rj(state, 5);
    mov_reg32_reg32(state, temp1, temp2); // 2
    sar_reg32_imm8(state, temp2, 31); // 3
    
    mov_reg32_reg32(state, rd1, temp1);
    mov_reg32_reg32(state, rd2, temp2);
     }
#endif
}

void genmult(usf_state_t * state)
{
#ifdef INTERPRET_MULT
   gencallinterp(state, (unsigned int)state->current_instruction_table.MULT, 0);
#else
   int rs, rt;
   allocate_register_manually_w(state, EAX, (unsigned int *)&state->lo, 0);
   allocate_register_manually_w(state, EDX, (unsigned int *)&state->hi, 0);
   rs = allocate_register(state, (unsigned int*)state->dst->f.r.rs);
   rt = allocate_register(state, (unsigned int*)state->dst->f.r.rt);
   mov_reg32_reg32(state, EAX, rs);
   imul_reg32(state, rt);
#endif
}

void genmultu(usf_state_t * state)
{
#ifdef INTERPRET_MULTU
   gencallinterp(state, (unsigned int)state->current_instruction_table.MULTU, 0);
#else
   int rs, rt;
   allocate_register_manually_w(state, EAX, (unsigned int *)&state->lo, 0);
   allocate_register_manually_w(state, EDX, (unsigned int *)&state->hi, 0);
   rs = allocate_register(state, (unsigned int*)state->dst->f.r.rs);
   rt = allocate_register(state, (unsigned int*)state->dst->f.r.rt);
   mov_reg32_reg32(state, EAX, rs);
   mul_reg32(state, rt);
#endif
}

void gendiv(usf_state_t * state)
{
#ifdef INTERPRET_DIV
   gencallinterp(state, (unsigned int)state->current_instruction_table.DIV, 0);
#else
   int rs, rt;
   allocate_register_manually_w(state, EAX, (unsigned int *)&state->lo, 0);
   allocate_register_manually_w(state, EDX, (unsigned int *)&state->hi, 0);
   rs = allocate_register(state, (unsigned int*)state->dst->f.r.rs);
   rt = allocate_register(state, (unsigned int*)state->dst->f.r.rt);
   cmp_reg32_imm32(state, rt, 0);
   je_rj(state, (rs == EAX ? 0 : 2) + 1 + 2);
   mov_reg32_reg32(state, EAX, rs); // 0 or 2
   cdq(state); // 1
   idiv_reg32(state, rt); // 2
#endif
}

void gendivu(usf_state_t * state)
{
#ifdef INTERPRET_DIVU
   gencallinterp(state, (unsigned int)state->current_instruction_table.DIVU, 0);
#else
   int rs, rt;
   allocate_register_manually_w(state, EAX, (unsigned int *)&state->lo, 0);
   allocate_register_manually_w(state, EDX, (unsigned int *)&state->hi, 0);
   rs = allocate_register(state, (unsigned int*)state->dst->f.r.rs);
   rt = allocate_register(state, (unsigned int*)state->dst->f.r.rt);
   cmp_reg32_imm32(state, rt, 0);
   je_rj(state, (rs == EAX ? 0 : 2) + 2 + 2);
   mov_reg32_reg32(state, EAX, rs); // 0 or 2
   xor_reg32_reg32(state, EDX, EDX); // 2
   div_reg32(state, rt); // 2
#endif
}

void gendmult(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.DMULT, 0);
}

void gendmultu(usf_state_t * state)
{
#ifdef INTERPRET_DMULTU
   gencallinterp(state, (unsigned int)state->current_instruction_table.DMULTU, 0);
#else
   free_all_registers(state);
   simplify_access(state);
   
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.r.rs);
   mul_m32(state, (unsigned int *)state->dst->f.r.rt); // EDX:EAX = temp1
   mov_memoffs32_eax(state, (unsigned int *)(&state->lo));
   
   mov_reg32_reg32(state, EBX, EDX); // EBX = temp1>>32
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.r.rs);
   mul_m32(state, (unsigned int *)(state->dst->f.r.rt)+1);
   add_reg32_reg32(state, EBX, EAX);
   adc_reg32_imm32(state, EDX, 0);
   mov_reg32_reg32(state, ECX, EDX); // ECX:EBX = temp2
   
   mov_eax_memoffs32(state, (unsigned int *)(state->dst->f.r.rs)+1);
   mul_m32(state, (unsigned int *)state->dst->f.r.rt); // EDX:EAX = temp3
   
   add_reg32_reg32(state, EBX, EAX);
   adc_reg32_imm32(state, ECX, 0); // ECX:EBX = result2
   mov_m32_reg32(state, (unsigned int*)(&state->lo)+1, EBX);
   
   mov_reg32_reg32(state, EDI, EDX); // EDI = temp3>>32
   mov_eax_memoffs32(state, (unsigned int *)(state->dst->f.r.rs)+1);
   mul_m32(state, (unsigned int *)(state->dst->f.r.rt)+1);
   add_reg32_reg32(state, EAX, EDI);
   adc_reg32_imm32(state, EDX, 0); // EDX:EAX = temp4
   
   add_reg32_reg32(state, EAX, ECX);
   adc_reg32_imm32(state, EDX, 0); // EDX:EAX = result3
   mov_memoffs32_eax(state,(unsigned int *)(&state->hi));
   mov_m32_reg32(state, (unsigned int *)(&state->hi)+1, EDX);
#endif
}

void genddiv(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.DDIV, 0);
}

void genddivu(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.DDIVU, 0);
}

void genadd(usf_state_t * state)
{
#ifdef INTERPRET_ADD
   gencallinterp(state, (unsigned int)state->current_instruction_table.ADD, 0);
#else
   int rs = allocate_register(state, (unsigned int *)state->dst->f.r.rs);
   int rt = allocate_register(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rt != rd && rs != rd)
     {
    mov_reg32_reg32(state, rd, rs);
    add_reg32_reg32(state, rd, rt);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rs);
    add_reg32_reg32(state, temp, rt);
    mov_reg32_reg32(state, rd, temp);
     }
#endif
}

void genaddu(usf_state_t * state)
{
#ifdef INTERPRET_ADDU
   gencallinterp(state, (unsigned int)state->current_instruction_table.ADDU, 0);
#else
   int rs = allocate_register(state, (unsigned int *)state->dst->f.r.rs);
   int rt = allocate_register(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rt != rd && rs != rd)
     {
    mov_reg32_reg32(state, rd, rs);
    add_reg32_reg32(state, rd, rt);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rs);
    add_reg32_reg32(state, temp, rt);
    mov_reg32_reg32(state, rd, temp);
     }
#endif
}

void gensub(usf_state_t * state)
{
#ifdef INTERPRET_SUB
   gencallinterp(state, (unsigned int)state->current_instruction_table.SUB, 0);
#else
   int rs = allocate_register(state, (unsigned int *)state->dst->f.r.rs);
   int rt = allocate_register(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rt != rd && rs != rd)
     {
    mov_reg32_reg32(state, rd, rs);
    sub_reg32_reg32(state, rd, rt);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rs);
    sub_reg32_reg32(state, temp, rt);
    mov_reg32_reg32(state, rd, temp);
     }
#endif
}

void gensubu(usf_state_t * state)
{
#ifdef INTERPRET_SUBU
   gencallinterp(state, (unsigned int)state->current_instruction_table.SUBU, 0);
#else
   int rs = allocate_register(state, (unsigned int *)state->dst->f.r.rs);
   int rt = allocate_register(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rt != rd && rs != rd)
     {
    mov_reg32_reg32(state, rd, rs);
    sub_reg32_reg32(state, rd, rt);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rs);
    sub_reg32_reg32(state, temp, rt);
    mov_reg32_reg32(state, rd, temp);
     }
#endif
}

void genand(usf_state_t * state)
{
#ifdef INTERPRET_AND
   gencallinterp((unsigned int)state->current_instruction_table.AND, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rs);
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rt1 != rd1 && rs1 != rd1)
     {
    mov_reg32_reg32(state, rd1, rs1);
    mov_reg32_reg32(state, rd2, rs2);
    and_reg32_reg32(state, rd1, rt1);
    and_reg32_reg32(state, rd2, rt2);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rs1);
    and_reg32_reg32(state, temp, rt1);
    mov_reg32_reg32(state, rd1, temp);
    mov_reg32_reg32(state, temp, rs2);
    and_reg32_reg32(state, temp, rt2);
    mov_reg32_reg32(state, rd2, temp);
     }
#endif
}

void genor(usf_state_t * state)
{
#ifdef INTERPRET_OR
   gencallinterp(state, (unsigned int)state->current_instruction_table.OR, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rs);
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rt1 != rd1 && rs1 != rd1)
     {
    mov_reg32_reg32(state, rd1, rs1);
    mov_reg32_reg32(state, rd2, rs2);
    or_reg32_reg32(state, rd1, rt1);
    or_reg32_reg32(state, rd2, rt2);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rs1);
    or_reg32_reg32(state, temp, rt1);
    mov_reg32_reg32(state, rd1, temp);
    mov_reg32_reg32(state, temp, rs2);
    or_reg32_reg32(state, temp, rt2);
    mov_reg32_reg32(state, rd2, temp);
     }
#endif
}

void genxor(usf_state_t * state)
{
#ifdef INTERPRET_XOR
   gencallinterp(state, (unsigned int)state->current_instruction_table.XOR, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rs);
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rt1 != rd1 && rs1 != rd1)
     {
    mov_reg32_reg32(state, rd1, rs1);
    mov_reg32_reg32(state, rd2, rs2);
    xor_reg32_reg32(state, rd1, rt1);
    xor_reg32_reg32(state, rd2, rt2);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rs1);
    xor_reg32_reg32(state, temp, rt1);
    mov_reg32_reg32(state, rd1, temp);
    mov_reg32_reg32(state, temp, rs2);
    xor_reg32_reg32(state, temp, rt2);
    mov_reg32_reg32(state, rd2, temp);
     }
#endif
}

void gennor(usf_state_t * state)
{
#ifdef INTERPRET_NOR
   gencallinterp(state, (unsigned int)state->current_instruction_table.NOR, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rs);
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rt1 != rd1 && rs1 != rd1)
     {
    mov_reg32_reg32(state, rd1, rs1);
    mov_reg32_reg32(state, rd2, rs2);
    or_reg32_reg32(state, rd1, rt1);
    or_reg32_reg32(state, rd2, rt2);
    not_reg32(state, rd1);
    not_reg32(state, rd2);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rs1);
    or_reg32_reg32(state, temp, rt1);
    mov_reg32_reg32(state, rd1, temp);
    mov_reg32_reg32(state, temp, rs2);
    or_reg32_reg32(state, temp, rt2);
    mov_reg32_reg32(state, rd2, temp);
    not_reg32(state, rd1);
    not_reg32(state, rd2);
     }
#endif
}

void genslt(usf_state_t * state)
{
#ifdef INTERPRET_SLT
   gencallinterp(state, (unsigned int)state->current_instruction_table.SLT, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rs);
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_w(state, (unsigned int *)state->dst->f.r.rd);
   
   cmp_reg32_reg32(state, rs2, rt2);
   jl_rj(state, 13);
   jne_rj(state, 4); // 2
   cmp_reg32_reg32(state, rs1, rt1); // 2
   jl_rj(state, 7); // 2
   mov_reg32_imm32(state, rd, 0); // 5
   jmp_imm_short(state, 5); // 2
   mov_reg32_imm32(state, rd, 1); // 5
#endif
}

void gensltu(usf_state_t * state)
{
#ifdef INTERPRET_SLTU
   gencallinterp(state, (unsigned int)state->current_instruction_table.SLTU, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rs);
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_w(state, (unsigned int *)state->dst->f.r.rd);
   
   cmp_reg32_reg32(state, rs2, rt2);
   jb_rj(state, 13);
   jne_rj(state, 4); // 2
   cmp_reg32_reg32(state, rs1, rt1); // 2
   jb_rj(state, 7); // 2
   mov_reg32_imm32(state, rd, 0); // 5
   jmp_imm_short(state, 5); // 2
   mov_reg32_imm32(state, rd, 1); // 5
#endif
}

void gendadd(usf_state_t * state)
{
#ifdef INTERPRET_DADD
   gencallinterp(state, (unsigned int)state->current_instruction_table.DADD, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rs);
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rt1 != rd1 && rs1 != rd1)
     {
    mov_reg32_reg32(state, rd1, rs1);
    mov_reg32_reg32(state, rd2, rs2);
    add_reg32_reg32(state, rd1, rt1);
    adc_reg32_reg32(state, rd2, rt2);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rs1);
    add_reg32_reg32(state, temp, rt1);
    mov_reg32_reg32(state, rd1, temp);
    mov_reg32_reg32(state, temp, rs2);
    adc_reg32_reg32(state, temp, rt2);
    mov_reg32_reg32(state, rd2, temp);
     }
#endif
}

void gendaddu(usf_state_t * state)
{
#ifdef INTERPRET_DADDU
   gencallinterp(state, (unsigned int)state->current_instruction_table.DADDU, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rs);
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rt1 != rd1 && rs1 != rd1)
     {
    mov_reg32_reg32(state, rd1, rs1);
    mov_reg32_reg32(state, rd2, rs2);
    add_reg32_reg32(state, rd1, rt1);
    adc_reg32_reg32(state, rd2, rt2);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rs1);
    add_reg32_reg32(state, temp, rt1);
    mov_reg32_reg32(state, rd1, temp);
    mov_reg32_reg32(state, temp, rs2);
    adc_reg32_reg32(state, temp, rt2);
    mov_reg32_reg32(state, rd2, temp);
     }
#endif
}

void gendsub(usf_state_t * state)
{
#ifdef INTERPRET_DSUB
   gencallinterp(state, (unsigned int)state->current_instruction_table.DSUB, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rs);
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rt1 != rd1 && rs1 != rd1)
     {
    mov_reg32_reg32(state, rd1, rs1);
    mov_reg32_reg32(state, rd2, rs2);
    sub_reg32_reg32(state, rd1, rt1);
    sbb_reg32_reg32(state, rd2, rt2);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rs1);
    sub_reg32_reg32(state, temp, rt1);
    mov_reg32_reg32(state, rd1, temp);
    mov_reg32_reg32(state, temp, rs2);
    sbb_reg32_reg32(state, temp, rt2);
    mov_reg32_reg32(state, rd2, temp);
     }
#endif
}

void gendsubu(usf_state_t * state)
{
#ifdef INTERPRET_DSUBU
   gencallinterp(state, (unsigned int)state->current_instruction_table.DSUBU, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rs);
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   if (rt1 != rd1 && rs1 != rd1)
     {
    mov_reg32_reg32(state, rd1, rs1);
    mov_reg32_reg32(state, rd2, rs2);
    sub_reg32_reg32(state, rd1, rt1);
    sbb_reg32_reg32(state, rd2, rt2);
     }
   else
     {
    int temp = lru_register(state);
    free_register(state, temp);
    mov_reg32_reg32(state, temp, rs1);
    sub_reg32_reg32(state, temp, rt1);
    mov_reg32_reg32(state, rd1, temp);
    mov_reg32_reg32(state, temp, rs2);
    sbb_reg32_reg32(state, temp, rt2);
    mov_reg32_reg32(state, rd2, temp);
     }
#endif
}

void genteq(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.TEQ, 0);
}

void gendsll(usf_state_t * state)
{
#ifdef INTERPRET_DSLL
   gencallinterp(state, (unsigned int)state->current_instruction_table.DSLL, 0);
#else
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   mov_reg32_reg32(state, rd1, rt1);
   mov_reg32_reg32(state, rd2, rt2);
   shld_reg32_reg32_imm8(state, rd2, rd1, state->dst->f.r.sa);
   shl_reg32_imm8(state, rd1, state->dst->f.r.sa);
   if (state->dst->f.r.sa & 0x20)
     {
    mov_reg32_reg32(state, rd2, rd1);
    xor_reg32_reg32(state, rd1, rd1);
     }
#endif
}

void gendsrl(usf_state_t * state)
{
#ifdef INTERPRET_DSRL
   gencallinterp(state, (unsigned int)state->current_instruction_table.DSRL, 0);
#else
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   mov_reg32_reg32(state, rd1, rt1);
   mov_reg32_reg32(state, rd2, rt2);
   shrd_reg32_reg32_imm8(state, rd1, rd2, state->dst->f.r.sa);
   shr_reg32_imm8(state, rd2, state->dst->f.r.sa);
   if (state->dst->f.r.sa & 0x20)
     {
    mov_reg32_reg32(state, rd1, rd2);
    xor_reg32_reg32(state, rd2, rd2);
     }
#endif
}

void gendsra(usf_state_t * state)
{
#ifdef INTERPRET_DSRA
   gencallinterp(state, (unsigned int)state->current_instruction_table.DSRA, 0);
#else
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   mov_reg32_reg32(state, rd1, rt1);
   mov_reg32_reg32(state, rd2, rt2);
   shrd_reg32_reg32_imm8(state, rd1, rd2, state->dst->f.r.sa);
   sar_reg32_imm8(state, rd2, state->dst->f.r.sa);
   if (state->dst->f.r.sa & 0x20)
     {
    mov_reg32_reg32(state, rd1, rd2);
    sar_reg32_imm8(state, rd2, 31);
     }
#endif
}

void gendsll32(usf_state_t * state)
{
#ifdef INTERPRET_DSLL32
   gencallinterp(state, (unsigned int)state->current_instruction_table.DSLL32, 0);
#else
   int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.r.rt);
   int rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   mov_reg32_reg32(state, rd2, rt1);
   shl_reg32_imm8(state, rd2, state->dst->f.r.sa);
   xor_reg32_reg32(state, rd1, rd1);
#endif
}

void gendsrl32(usf_state_t * state)
{
#ifdef INTERPRET_DSRL32
   gencallinterp(state, (unsigned int)state->current_instruction_table.DSRL32, 0);
#else
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.r.rd);
   int rd2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.r.rd);
   
   mov_reg32_reg32(state, rd1, rt2);
   shr_reg32_imm8(state, rd1, state->dst->f.r.sa);
   xor_reg32_reg32(state, rd2, rd2);
#endif
}

void gendsra32(usf_state_t * state)
{
#ifdef INTERPRET_DSRA32
   gencallinterp(state, (unsigned int)state->current_instruction_table.DSRA32, 0);
#else
   int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.r.rt);
   int rd = allocate_register_w(state, (unsigned int *)state->dst->f.r.rd);
   
   mov_reg32_reg32(state, rd, rt2);
   sar_reg32_imm8(state, rd, state->dst->f.r.sa);
#endif
}

void genbreak(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.BREAK, 0);
}

