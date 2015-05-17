/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - gr4300.c                                                *
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

#include "assemble.h"
#include "interpret.h"
#include "regcache.h"

#include "main/main.h"
#include "memory/memory.h"
#include "r4300/r4300.h"
#include "r4300/cached_interp.h"
#include "r4300/cp0.h"
#include "r4300/cp1.h"
#include "r4300/interupt.h"
#include "r4300/ops.h"
#include "r4300/recomph.h"
#include "r4300/exception.h"

/* static functions */

static void genupdate_count(usf_state_t * state, unsigned int addr)
{
   mov_reg32_imm32(state, EAX, addr);
   sub_reg32_m32(state, EAX, (unsigned int*)(&state->last_addr));
   shr_reg32_imm8(state, EAX, 2);
   mov_reg32_m32(state, EDX, &state->count_per_op);
   mul_reg32(state, EDX);
   add_m32_reg32(state, (unsigned int*)(&state->g_cp0_regs[CP0_COUNT_REG]), EAX);
}

static void gencheck_interupt(usf_state_t * state, unsigned int instr_structure)
{
   free_register(state, EBX);
   mov_eax_memoffs32(state, &state->next_interupt);
   cmp_reg32_m32(state, EAX, &state->g_cp0_regs[CP0_COUNT_REG]);
   ja_rj(state, 19);
   mov_m32_imm32(state, (unsigned int*)(&state->PC), instr_structure); // 10
   mov_reg32_imm32(state, EBX, (unsigned int)gen_interupt); // 5
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
}

static void gencheck_interupt_out(usf_state_t * state, unsigned int addr)
{
   free_register(state, EBX);
   mov_eax_memoffs32(state, &state->next_interupt);
   cmp_reg32_m32(state, EAX, &state->g_cp0_regs[CP0_COUNT_REG]);
   ja_rj(state, 29);
   mov_m32_imm32(state, (unsigned int*)(&state->fake_instr.addr), addr); // 10
   mov_m32_imm32(state, (unsigned int*)(&state->PC), (unsigned int)(&state->fake_instr)); // 10
   mov_reg32_imm32(state, EBX, (unsigned int)gen_interupt); // 5
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
}

static void genbeq_test(usf_state_t * state)
{
   int rs_64bit = is64(state, (unsigned int *)state->dst->f.i.rs);
   int rt_64bit = is64(state, (unsigned int *)state->dst->f.i.rt);
   
   if (!rs_64bit && !rt_64bit)
     {
    int rs = allocate_register(state, (unsigned int *)state->dst->f.i.rs);
    int rt = allocate_register(state, (unsigned int *)state->dst->f.i.rt);
    
    cmp_reg32_reg32(state, rs, rt);
    jne_rj(state, 12);
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
     }
   else if (rs_64bit == -1)
     {
    int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rt);
    int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rt);
    
    cmp_reg32_m32(state, rt1, (unsigned int *)state->dst->f.i.rs);
    jne_rj(state, 20);
    cmp_reg32_m32(state, rt2, ((unsigned int *)state->dst->f.i.rs)+1); // 6
    jne_rj(state, 12); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
     }
   else if (rt_64bit == -1)
     {
    int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
    int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
    
    cmp_reg32_m32(state, rs1, (unsigned int *)state->dst->f.i.rt);
    jne_rj(state, 20);
    cmp_reg32_m32(state, rs2, ((unsigned int *)state->dst->f.i.rt)+1); // 6
    jne_rj(state, 12); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
     }
   else
     {
    int rs1, rs2, rt1, rt2;
    if (!rs_64bit)
      {
         rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rt);
         rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rt);
         rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
         rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
      }
    else
      {
         rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
         rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
         rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rt);
         rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rt);
      }
    cmp_reg32_reg32(state, rs1, rt1);
    jne_rj(state, 16);
    cmp_reg32_reg32(state, rs2, rt2); // 2
    jne_rj(state, 12); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
     }
}

static void genbne_test(usf_state_t * state)
{
   int rs_64bit = is64(state, (unsigned int *)state->dst->f.i.rs);
   int rt_64bit = is64(state, (unsigned int *)state->dst->f.i.rt);
   
   if (!rs_64bit && !rt_64bit)
     {
    int rs = allocate_register(state, (unsigned int *)state->dst->f.i.rs);
    int rt = allocate_register(state, (unsigned int *)state->dst->f.i.rt);
    
    cmp_reg32_reg32(state, rs, rt);
    je_rj(state, 12);
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
     }
   else if (rs_64bit == -1)
     {
    int rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rt);
    int rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rt);
    
    cmp_reg32_m32(state, rt1, (unsigned int *)state->dst->f.i.rs);
    jne_rj(state, 20);
    cmp_reg32_m32(state, rt2, ((unsigned int *)state->dst->f.i.rs)+1); // 6
    jne_rj(state, 12); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
     }
   else if (rt_64bit == -1)
     {
    int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
    int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
    
    cmp_reg32_m32(state, rs1, (unsigned int *)state->dst->f.i.rt);
    jne_rj(state, 20);
    cmp_reg32_m32(state, rs2, ((unsigned int *)state->dst->f.i.rt)+1); // 6
    jne_rj(state, 12); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
     }
   else
     {
    int rs1, rs2, rt1, rt2;
    if (!rs_64bit)
      {
         rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rt);
         rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rt);
         rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
         rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
      }
    else
      {
         rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
         rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
         rt1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rt);
         rt2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rt);
      }
    cmp_reg32_reg32(state, rs1, rt1);
    jne_rj(state, 16);
    cmp_reg32_reg32(state, rs2, rt2); // 2
    jne_rj(state, 12); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
     }
}

static void genblez_test(usf_state_t * state)
{
   int rs_64bit = is64(state, (unsigned int *)state->dst->f.i.rs);
   
   if (!rs_64bit)
     {
    int rs = allocate_register(state, (unsigned int *)state->dst->f.i.rs);
    
    cmp_reg32_imm32(state, rs, 0);
    jg_rj(state, 12);
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
     }
   else if (rs_64bit == -1)
     {
    cmp_m32_imm32(state, ((unsigned int *)state->dst->f.i.rs)+1, 0);
    jg_rj(state, 14);
    jne_rj(state, 24); // 2
    cmp_m32_imm32(state, (unsigned int *)state->dst->f.i.rs, 0); // 10
    je_rj(state, 12); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
     }
   else
     {
    int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
    int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
    
    cmp_reg32_imm32(state, rs2, 0);
    jg_rj(state, 10);
    jne_rj(state, 20); // 2
    cmp_reg32_imm32(state, rs1, 0); // 6
    je_rj(state, 12); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
     }
}

static void genbgtz_test(usf_state_t * state)
{
   int rs_64bit = is64(state, (unsigned int *)state->dst->f.i.rs);
   
   if (!rs_64bit)
     {
    int rs = allocate_register(state, (unsigned int *)state->dst->f.i.rs);
    
    cmp_reg32_imm32(state, rs, 0);
    jle_rj(state, 12);
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
     }
   else if (rs_64bit == -1)
     {
    cmp_m32_imm32(state, ((unsigned int *)state->dst->f.i.rs)+1, 0);
    jl_rj(state, 14);
    jne_rj(state, 24); // 2
    cmp_m32_imm32(state, (unsigned int *)state->dst->f.i.rs, 0); // 10
    jne_rj(state, 12); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
     }
   else
     {
    int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
    int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
    
    cmp_reg32_imm32(state, rs2, 0);
    jl_rj(state, 10);
    jne_rj(state, 20); // 2
    cmp_reg32_imm32(state, rs1, 0); // 6
    jne_rj(state, 12); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0); // 10
    jmp_imm_short(state, 10); // 2
    mov_m32_imm32(state, (unsigned int *)(&state->branch_taken), 1); // 10
     }
}


/* global functions */

void gennotcompiled(usf_state_t * state)
{
    free_all_registers(state);
    simplify_access(state);

    mov_m32_imm32(state, (unsigned int*)(&state->PC), (unsigned int)(state->dst));
    mov_reg32_imm32(state, EBX, (unsigned int)state->current_instruction_table.NOTCOMPILED);
    mov_reg32_reg32(state, RP0, ESI);
    call_reg32(state, EBX);
}

void genlink_subblock(usf_state_t * state)
{
   free_all_registers(state);
   jmp(state, state->dst->addr+4);
}

void gencallinterp(usf_state_t * state, unsigned long addr, int jump)
{
   free_all_registers(state);
   simplify_access(state);
   if (jump)
     mov_m32_imm32(state, (unsigned int*)(&state->dyna_interp), 1);
   mov_m32_imm32(state, (unsigned int*)(&state->PC), (unsigned int)(state->dst));
   mov_reg32_imm32(state, EBX, addr);
   mov_reg32_reg32(state, RP0, ESI);
   call_reg32(state, EBX);
   if (jump)
     {
    mov_m32_imm32(state, (unsigned int*)(&state->dyna_interp), 0);
    mov_reg32_imm32(state, EBX, (unsigned int)dyna_jump);
    mov_reg32_reg32(state, RP0, ESI);
    call_reg32(state, EBX);
     }
}

void gendelayslot(usf_state_t * state)
{
   mov_m32_imm32(state, &state->delay_slot, 1);
   recompile_opcode(state);
   
   free_all_registers(state);
   genupdate_count(state, state->dst->addr+4);
   
   mov_m32_imm32(state, &state->delay_slot, 0);
}

void genni(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.NI, 0);
}

void genreserved(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.RESERVED, 0);
}

void genfin_block(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.FIN_BLOCK, 0);
}

void gencheck_interupt_reg(usf_state_t * state) // addr is in EAX
{
   free_register(state, ECX);
   mov_reg32_m32(state, EBX, &state->next_interupt);
   cmp_reg32_m32(state, EBX, &state->g_cp0_regs[CP0_COUNT_REG]);
   ja_rj(state, 24);
   mov_memoffs32_eax(state, (unsigned int*)(&state->fake_instr.addr)); // 5
   mov_m32_imm32(state, (unsigned int*)(&state->PC), (unsigned int)(&state->fake_instr)); // 10
   mov_reg32_imm32(state, EBX, (unsigned int)gen_interupt); // 5
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
}

void gennop(usf_state_t * state)
{
}

void genj(usf_state_t * state)
{
#ifdef INTERPRET_J
   gencallinterp(state, (unsigned int)state->current_instruction_table.J, 1);
#else
   unsigned int naddr;
   
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.J, 1);
    return;
     }
   
   gendelayslot(state);
   naddr = ((state->dst-1)->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);
   
   mov_m32_imm32(state, &state->last_addr, naddr);
   gencheck_interupt(state, (unsigned int)&state->actual->block[(naddr-state->actual->start)/4]);
   jmp(state, naddr);
#endif
}

void genj_out(usf_state_t * state)
{
#ifdef INTERPRET_J_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.J_OUT, 1);
#else
   unsigned int naddr;
   
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.J_OUT, 1);
    return;
     }
   
   gendelayslot(state);
   naddr = ((state->dst-1)->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);
   
   mov_m32_imm32(state, &state->last_addr, naddr);
   gencheck_interupt_out(state, naddr);
   mov_m32_imm32(state, &state->jump_to_address, naddr);
   mov_m32_imm32(state, (unsigned int*)(&state->PC), (unsigned int)(state->dst+1));
   free_register(state, EBX);
   mov_reg32_imm32(state, EBX, (unsigned int)jump_to_func);
   free_register(state, RP0);
   mov_reg32_reg32(state, RP0, ESI);
   call_reg32(state, EBX);
#endif
}

void genj_idle(usf_state_t * state)
{
#ifdef INTERPRET_J_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.J_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.J_IDLE, 1);
    return;
     }
   
   mov_eax_memoffs32(state, (unsigned int *)(&state->next_interupt));
   sub_reg32_m32(state, EAX, (unsigned int *)(&state->g_cp0_regs[CP0_COUNT_REG]));
   cmp_reg32_imm8(state, EAX, 3);
   jbe_rj(state, 11);
   
   and_eax_imm32(state, 0xFFFFFFFC);  // 5
   add_m32_reg32(state, (unsigned int *)(&state->g_cp0_regs[CP0_COUNT_REG]), EAX); // 6
  
   genj(state);
#endif
}

void genjal(usf_state_t * state)
{
#ifdef INTERPRET_JAL
   gencallinterp(state, (unsigned int)state->current_instruction_table.JAL, 1);
#else
   unsigned int naddr;
   
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.JAL, 1);
    return;
     }
   
   gendelayslot(state);
   
   mov_m32_imm32(state, (unsigned int *)(state->reg + 31), state->dst->addr + 4);
   if (((state->dst->addr + 4) & 0x80000000))
     mov_m32_imm32(state, (unsigned int *)(&state->reg[31])+1, 0xFFFFFFFF);
   else
     mov_m32_imm32(state, (unsigned int *)(&state->reg[31])+1, 0);
   
   naddr = ((state->dst-1)->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);
   
   mov_m32_imm32(state, &state->last_addr, naddr);
   gencheck_interupt(state, (unsigned int)&state->actual->block[(naddr-state->actual->start)/4]);
   jmp(state, naddr);
#endif
}

void genjal_out(usf_state_t * state)
{
#ifdef INTERPRET_JAL_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.JAL_OUT, 1);
#else
   unsigned int naddr;
   
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.JAL_OUT, 1);
    return;
     }
   
   gendelayslot(state);
   
   mov_m32_imm32(state, (unsigned int *)(state->reg + 31), state->dst->addr + 4);
   if (((state->dst->addr + 4) & 0x80000000))
     mov_m32_imm32(state, (unsigned int *)(&state->reg[31])+1, 0xFFFFFFFF);
   else
     mov_m32_imm32(state, (unsigned int *)(&state->reg[31])+1, 0);
   
   naddr = ((state->dst-1)->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);
   
   mov_m32_imm32(state, &state->last_addr, naddr);
   gencheck_interupt_out(state, naddr);
   mov_m32_imm32(state, &state->jump_to_address, naddr);
   mov_m32_imm32(state, (unsigned int*)(&state->PC), (unsigned int)(state->dst+1));
   free_register(state, EBX);
   mov_reg32_imm32(state, EBX, (unsigned int)jump_to_func);
   free_register(state, RP0);
   mov_reg32_reg32(state, RP0, ESI);
   call_reg32(state, EBX);
#endif
}

void genjal_idle(usf_state_t * state)
{
#ifdef INTERPRET_JAL_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.JAL_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.JAL_IDLE, 1);
    return;
     }
   
   mov_eax_memoffs32(state, (unsigned int *)(&state->next_interupt));
   sub_reg32_m32(state, EAX, (unsigned int *)(&state->g_cp0_regs[CP0_COUNT_REG]));
   cmp_reg32_imm8(state, EAX, 3);
   jbe_rj(state, 11);
   
   and_eax_imm32(state, 0xFFFFFFFC);
   add_m32_reg32(state, (unsigned int *)(&state->g_cp0_regs[CP0_COUNT_REG]), EAX);
  
   genjal(state);
#endif
}

void gentest(usf_state_t * state)
{
   cmp_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0);
   je_near_rj(state, 0);

   jump_start_rel32(state);

   mov_m32_imm32(state, &state->last_addr, state->dst->addr + (state->dst-1)->f.i.immediate*4);
   gencheck_interupt(state, (unsigned int)(state->dst + (state->dst-1)->f.i.immediate));
   jmp(state, state->dst->addr + (state->dst-1)->f.i.immediate*4);

   jump_end_rel32(state);

   mov_m32_imm32(state, &state->last_addr, state->dst->addr + 4);
   gencheck_interupt(state, (unsigned int)(state->dst + 1));
   jmp(state, state->dst->addr + 4);
}

void genbeq(usf_state_t * state)
{
#ifdef INTERPRET_BEQ
   gencallinterp(state, (unsigned int)state->current_instruction_table.BEQ, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BEQ, 1);
    return;
     }
   
   genbeq_test(state);
   gendelayslot(state);
   gentest(state);
#endif
}

void gentest_out(usf_state_t * state)
{
   cmp_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0);
   je_near_rj(state, 0);

   jump_start_rel32(state);

   mov_m32_imm32(state, &state->last_addr, state->dst->addr + (state->dst-1)->f.i.immediate*4);
   gencheck_interupt_out(state, state->dst->addr + (state->dst-1)->f.i.immediate*4);
   mov_m32_imm32(state, &state->jump_to_address, state->dst->addr + (state->dst-1)->f.i.immediate*4);
   mov_m32_imm32(state, (unsigned int*)(&state->PC), (unsigned int)(state->dst+1));
   free_register(state, EBX);
   mov_reg32_imm32(state, EBX, (unsigned int)jump_to_func);
   free_register(state, RP0);
   mov_reg32_reg32(state, RP0, ESI);
   call_reg32(state, EBX);
   
   jump_end_rel32(state);

   mov_m32_imm32(state, &state->last_addr, state->dst->addr + 4);
   gencheck_interupt(state, (unsigned int)(state->dst + 1));
   jmp(state, state->dst->addr + 4);
}

void genbeq_out(usf_state_t * state)
{
#ifdef INTERPRET_BEQ_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.BEQ_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BEQ_OUT, 1);
    return;
     }
   
   genbeq_test(state);
   gendelayslot(state);
   gentest_out(state);
#endif
}

void gentest_idle(usf_state_t * state)
{
   int reg;
   
   reg = lru_register(state);
   free_register(state, reg);
   
   cmp_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0);
   je_near_rj(state, 0);

   jump_start_rel32(state);
   
   mov_reg32_m32(state, reg, (unsigned int *)(&state->next_interupt));
   sub_reg32_m32(state, reg, (unsigned int *)(&state->g_cp0_regs[CP0_COUNT_REG]));
   cmp_reg32_imm8(state, reg, 3);
   jbe_rj(state, 12);
   
   //sub_reg32_imm32(state, reg, 2); // 6
   and_reg32_imm32(state, reg, 0xFFFFFFFC); // 6
   add_m32_reg32(state, (unsigned int *)(&state->g_cp0_regs[CP0_COUNT_REG]), reg); // 6
   
   jump_end_rel32(state);
}

void genbeq_idle(usf_state_t * state)
{
#ifdef INTERPRET_BEQ_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.BEQ_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BEQ_IDLE, 1);
    return;
     }
   
   genbeq_test(state);
   gentest_idle(state);
   genbeq(state);
#endif
}

void genbne(usf_state_t * state)
{
#ifdef INTERPRET_BNE
   gencallinterp(state, (unsigned int)state->current_instruction_table.BNE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BNE, 1);
    return;
     }
   
   genbne_test(state);
   gendelayslot(state);
   gentest(state);
#endif
}

void genbne_out(usf_state_t * state)
{
#ifdef INTERPRET_BNE_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.BNE_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BNE_OUT, 1);
    return;
     }
   
   genbne_test(state);
   gendelayslot(state);
   gentest_out(state);
#endif
}

void genbne_idle(usf_state_t * state)
{
#ifdef INTERPRET_BNE_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.BNE_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BNE_IDLE, 1);
    return;
     }
   
   genbne_test(state);
   gentest_idle(state);
   genbne(state);
#endif
}

void genblez(usf_state_t * state)
{
#ifdef INTERPRET_BLEZ
   gencallinterp(state, (unsigned int)state->current_instruction_table.BLEZ, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BLEZ, 1);
    return;
     }
   
   genblez_test(state);
   gendelayslot(state);
   gentest(state);
#endif
}

void genblez_out(usf_state_t * state)
{
#ifdef INTERPRET_BLEZ_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.BLEZ_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BLEZ_OUT, 1);
    return;
     }
   
   genblez_test(state);
   gendelayslot(state);
   gentest_out(state);
#endif
}

void genblez_idle(usf_state_t * state)
{
#ifdef INTERPRET_BLEZ_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.BLEZ_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BLEZ_IDLE, 1);
    return;
     }
   
   genblez_test(state);
   gentest_idle(state);
   genblez(state);
#endif
}

void genbgtz(usf_state_t * state)
{
#ifdef INTERPRET_BGTZ
   gencallinterp(state, (unsigned int)state->current_instruction_table.BGTZ, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BGTZ, 1);
    return;
     }
   
   genbgtz_test(state);
   gendelayslot(state);
   gentest(state);
#endif
}

void genbgtz_out(usf_state_t * state)
{
#ifdef INTERPRET_BGTZ_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.BGTZ_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BGTZ_OUT, 1);
    return;
     }
   
   genbgtz_test(state);
   gendelayslot(state);
   gentest_out(state);
#endif
}

void genbgtz_idle(usf_state_t * state)
{
#ifdef INTERPRET_BGTZ_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.BGTZ_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BGTZ_IDLE, 1);
    return;
     }
   
   genbgtz_test(state);
   gentest_idle(state);
   genbgtz(state);
#endif
}

void genaddi(usf_state_t * state)
{
#ifdef INTERPRET_ADDI
   gencallinterp(state, (unsigned int)state->current_instruction_table.ADDI, 0);
#else
   int rs = allocate_register(state, (unsigned int *)state->dst->f.i.rs);
   int rt = allocate_register_w(state, (unsigned int *)state->dst->f.i.rt);
   
   mov_reg32_reg32(state, rt, rs);
   add_reg32_imm32(state, rt,(int)state->dst->f.i.immediate);
#endif
}

void genaddiu(usf_state_t * state)
{
#ifdef INTERPRET_ADDIU
   gencallinterp(state, (unsigned int)state->current_instruction_table.ADDIU, 0);
#else
   int rs = allocate_register(state, (unsigned int *)state->dst->f.i.rs);
   int rt = allocate_register_w(state, (unsigned int *)state->dst->f.i.rt);
   
   mov_reg32_reg32(state, rt, rs);
   add_reg32_imm32(state, rt,(int)state->dst->f.i.immediate);
#endif
}

void genslti(usf_state_t * state)
{
#ifdef INTERPRET_SLTI
   gencallinterp(state, (unsigned int)state->current_instruction_table.SLTI, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
   int rt = allocate_register_w(state, (unsigned int *)state->dst->f.i.rt);
   long long imm = (long long)state->dst->f.i.immediate;
   
   cmp_reg32_imm32(state, rs2, (unsigned int)(imm >> 32));
   jl_rj(state, 17);
   jne_rj(state, 8); // 2
   cmp_reg32_imm32(state, rs1, (unsigned int)imm); // 6
   jl_rj(state, 7); // 2
   mov_reg32_imm32(state, rt, 0); // 5
   jmp_imm_short(state, 5); // 2
   mov_reg32_imm32(state, rt, 1); // 5
#endif
}

void gensltiu(usf_state_t * state)
{
#ifdef INTERPRET_SLTIU
   gencallinterp(state, (unsigned int)state->current_instruction_table.SLTIU, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
   int rt = allocate_register_w(state, (unsigned int *)state->dst->f.i.rt);
   long long imm = (long long)state->dst->f.i.immediate;
   
   cmp_reg32_imm32(state, rs2, (unsigned int)(imm >> 32));
   jb_rj(state, 17);
   jne_rj(state, 8); // 2
   cmp_reg32_imm32(state, rs1, (unsigned int)imm); // 6
   jb_rj(state, 7); // 2
   mov_reg32_imm32(state, rt, 0); // 5
   jmp_imm_short(state, 5); // 2
   mov_reg32_imm32(state, rt, 1); // 5
#endif
}

void genandi(usf_state_t * state)
{
#ifdef INTERPRET_ANDI
   gencallinterp(state, (unsigned int)state->current_instruction_table.ANDI, 0);
#else
   int rs = allocate_register(state, (unsigned int *)state->dst->f.i.rs);
   int rt = allocate_register_w(state, (unsigned int *)state->dst->f.i.rt);
   
   mov_reg32_reg32(state, rt, rs);
   and_reg32_imm32(state, rt, (unsigned short)state->dst->f.i.immediate);
#endif
}

void genori(usf_state_t * state)
{
#ifdef INTERPRET_ORI
   gencallinterp(state, (unsigned int)state->current_instruction_table.ORI, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
   int rt1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.i.rt);
   int rt2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.i.rt);
   
   mov_reg32_reg32(state, rt1, rs1);
   mov_reg32_reg32(state, rt2, rs2);
   or_reg32_imm32(state, rt1, (unsigned short)state->dst->f.i.immediate);
#endif
}

void genxori(usf_state_t * state)
{
#ifdef INTERPRET_XORI
   gencallinterp(state, (unsigned int)state->current_instruction_table.XORI, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
   int rt1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.i.rt);
   int rt2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.i.rt);
   
   mov_reg32_reg32(state, rt1, rs1);
   mov_reg32_reg32(state, rt2, rs2);
   xor_reg32_imm32(state, rt1, (unsigned short)state->dst->f.i.immediate);
#endif
}

void genlui(usf_state_t * state)
{
#ifdef INTERPRET_LUI
   gencallinterp(state, (unsigned int)state->current_instruction_table.LUI, 0);
#else
   int rt = allocate_register_w(state, (unsigned int *)state->dst->f.i.rt);
   
   mov_reg32_imm32(state, rt, (unsigned int)state->dst->f.i.immediate << 16);
#endif
}

void gentestl(usf_state_t * state)
{
   cmp_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0);
   je_near_rj(state, 0);

   jump_start_rel32(state);

   gendelayslot(state);
   mov_m32_imm32(state, &state->last_addr, state->dst->addr + (state->dst-1)->f.i.immediate*4);
   gencheck_interupt(state, (unsigned int)(state->dst + (state->dst-1)->f.i.immediate));
   jmp(state, state->dst->addr + (state->dst-1)->f.i.immediate*4);
   
   jump_end_rel32(state);

   genupdate_count(state, state->dst->addr+4);
   mov_m32_imm32(state, &state->last_addr, state->dst->addr + 4);
   gencheck_interupt(state, (unsigned int)(state->dst + 1));
   jmp(state, state->dst->addr + 4);
}

void genbeql(usf_state_t * state)
{
#ifdef INTERPRET_BEQL
   gencallinterp(state, (unsigned int)state->current_instruction_table.BEQL, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BEQL, 1);
    return;
     }
   
   genbeq_test(state);
   free_all_registers(state);
   gentestl(state);
#endif
}

void gentestl_out(usf_state_t * state)
{
   cmp_m32_imm32(state, (unsigned int *)(&state->branch_taken), 0);
   je_near_rj(state, 0);

   jump_start_rel32(state);

   gendelayslot(state);
   mov_m32_imm32(state, &state->last_addr, state->dst->addr + (state->dst-1)->f.i.immediate*4);
   gencheck_interupt_out(state, state->dst->addr + (state->dst-1)->f.i.immediate*4);
   mov_m32_imm32(state, &state->jump_to_address, state->dst->addr + (state->dst-1)->f.i.immediate*4);
   mov_m32_imm32(state, (unsigned int*)(&state->PC), (unsigned int)(state->dst+1));
   free_register(state, EBX);
   mov_reg32_imm32(state, EBX, (unsigned int)jump_to_func);
   free_register(state, RP0);
   mov_reg32_reg32(state, RP0, ESI);
   call_reg32(state, EBX);
   
   jump_end_rel32(state);

   genupdate_count(state, state->dst->addr+4);
   mov_m32_imm32(state, &state->last_addr, state->dst->addr + 4);
   gencheck_interupt(state, (unsigned int)(state->dst + 1));
   jmp(state, state->dst->addr + 4);
}

void genbeql_out(usf_state_t * state)
{
#ifdef INTERPRET_BEQL_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.BEQL_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BEQL_OUT, 1);
    return;
     }
   
   genbeq_test(state);
   free_all_registers(state);
   gentestl_out(state);
#endif
}

void genbeql_idle(usf_state_t * state)
{
#ifdef INTERPRET_BEQL_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.BEQL_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BEQL_IDLE, 1);
    return;
     }
   
   genbeq_test(state);
   gentest_idle(state);
   genbeql(state);
#endif
}

void genbnel(usf_state_t * state)
{
#ifdef INTERPRET_BNEL
   gencallinterp(state, (unsigned int)state->current_instruction_table.BNEL, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BNEL, 1);
    return;
     }
   
   genbne_test(state);
   free_all_registers(state);
   gentestl(state);
#endif
}

void genbnel_out(usf_state_t * state)
{
#ifdef INTERPRET_BNEL_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.BNEL_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BNEL_OUT, 1);
    return;
     }
   
   genbne_test(state);
   free_all_registers(state);
   gentestl_out(state);
#endif
}

void genbnel_idle(usf_state_t * state)
{
#ifdef INTERPRET_BNEL_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.BNEL_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BNEL_IDLE, 1);
    return;
     }
   
   genbne_test(state);
   gentest_idle(state);
   genbnel(state);
#endif
}

void genblezl(usf_state_t * state)
{
#ifdef INTERPRET_BLEZL
   gencallinterp(state, (unsigned int)state->current_instruction_table.BLEZL, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BLEZL, 1);
    return;
     }
   
   genblez_test(state);
   free_all_registers(state);
   gentestl(state);
#endif
}

void genblezl_out(usf_state_t * state)
{
#ifdef INTERPRET_BLEZL_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.BLEZL_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BLEZL_OUT, 1);
    return;
     }
   
   genblez_test(state);
   free_all_registers(state);
   gentestl_out(state);
#endif
}

void genblezl_idle(usf_state_t * state)
{
#ifdef INTERPRET_BLEZL_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.BLEZL_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BLEZL_IDLE, 1);
    return;
     }
   
   genblez_test(state);
   gentest_idle(state);
   genblezl(state);
#endif
}

void genbgtzl(usf_state_t * state)
{
#ifdef INTERPRET_BGTZL
   gencallinterp(state, (unsigned int)state->current_instruction_table.BGTZL, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BGTZL, 1);
    return;
     }
   
   genbgtz_test(state);
   free_all_registers(state);
   gentestl(state);
#endif
}

void genbgtzl_out(usf_state_t * state)
{
#ifdef INTERPRET_BGTZL_OUT
   gencallinterp(state, (unsigned int)state->current_instruction_table.BGTZL_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BGTZL_OUT, 1);
    return;
     }
   
   genbgtz_test(state);
   free_all_registers(state);
   gentestl_out(state);
#endif
}

void genbgtzl_idle(usf_state_t * state)
{
#ifdef INTERPRET_BGTZL_IDLE
   gencallinterp(state, (unsigned int)state->current_instruction_table.BGTZL_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned int)state->current_instruction_table.BGTZL_IDLE, 1);
    return;
     }
   
   genbgtz_test(state);
   gentest_idle(state);
   genbgtzl(state);
#endif
}

void gendaddi(usf_state_t * state)
{
#ifdef INTERPRET_DADDI
   gencallinterp(state, (unsigned int)state->current_instruction_table.DADDI, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
   int rt1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.i.rt);
   int rt2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.i.rt);
   
   mov_reg32_reg32(state, rt1, rs1);
   mov_reg32_reg32(state, rt2, rs2);
   add_reg32_imm32(state, rt1, state->dst->f.i.immediate);
   adc_reg32_imm32(state, rt2, (int)state->dst->f.i.immediate>>31);
#endif
}

void gendaddiu(usf_state_t * state)
{
#ifdef INTERPRET_DADDIU
   gencallinterp(state, (unsigned int)state->current_instruction_table.DADDIU, 0);
#else
   int rs1 = allocate_64_register1(state, (unsigned int *)state->dst->f.i.rs);
   int rs2 = allocate_64_register2(state, (unsigned int *)state->dst->f.i.rs);
   int rt1 = allocate_64_register1_w(state, (unsigned int *)state->dst->f.i.rt);
   int rt2 = allocate_64_register2_w(state, (unsigned int *)state->dst->f.i.rt);
   
   mov_reg32_reg32(state, rt1, rs1);
   mov_reg32_reg32(state, rt2, rs2);
   add_reg32_imm32(state, rt1, state->dst->f.i.immediate);
   adc_reg32_imm32(state, rt2, (int)state->dst->f.i.immediate>>31);
#endif
}

void genldl(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.LDL, 0);
}

void genldr(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.LDR, 0);
}

void genlb(usf_state_t * state)
{
#ifdef INTERPRET_LB
   gencallinterp(state, (unsigned int)state->current_instruction_table.LB, 0);
#else
   free_all_registers(state);
   simplify_access(state);
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->readmemb);
    cmp_reg32_imm32(state, EAX, (unsigned int)read_rdramb);
     }
   je_rj(state, 49);
   
   mov_m32_imm32(state, (unsigned int *)&state->PC, (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_m32_imm32(state, (unsigned int *)(&state->rdword), (unsigned int)state->dst->f.i.rt); // 10
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->readmemb); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   movsx_reg32_m8(state, EAX, (unsigned char *)state->dst->f.i.rt); // 7
   jmp_imm_short(state, 16); // 2
   
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(state, BL, 3); // 3
   movsx_reg32_8preg32pimm32(state, EAX, EBX, (unsigned int)state->g_rdram); // 7
   
   set_register_state(state, EAX, (unsigned int*)state->dst->f.i.rt, 1);
#endif
}

void genlh(usf_state_t * state)
{
#ifdef INTERPRET_LH
   gencallinterp(state, (unsigned int)state->current_instruction_table.LH, 0);
#else
   free_all_registers(state);
   simplify_access(state);
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->readmemh);
    cmp_reg32_imm32(state, EAX, (unsigned int)read_rdramh);
     }
   je_rj(state, 49);
   
   mov_m32_imm32(state, (unsigned int *)&state->PC, (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_m32_imm32(state, (unsigned int *)(&state->rdword), (unsigned int)state->dst->f.i.rt); // 10
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->readmemh); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   movsx_reg32_m16(state, EAX, (unsigned short *)state->dst->f.i.rt); // 7
   jmp_imm_short(state, 16); // 2
   
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(state, BL, 2); // 3
   movsx_reg32_16preg32pimm32(state, EAX, EBX, (unsigned int)state->g_rdram); // 7
   
   set_register_state(state, EAX, (unsigned int*)state->dst->f.i.rt, 1);
#endif
}

void genlwl(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.LWL, 0);
}

void genlw(usf_state_t * state)
{
#ifdef INTERPRET_LW
   gencallinterp(state, (unsigned int)state->current_instruction_table.LW, 0);
#else
   free_all_registers(state);
   simplify_access(state);
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->readmem);
    cmp_reg32_imm32(state, EAX, (unsigned int)read_rdram);
     }
   je_rj(state, 47);
   
   mov_m32_imm32(state, (unsigned int *)&state->PC, (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_m32_imm32(state, (unsigned int *)(&state->rdword), (unsigned int)state->dst->f.i.rt); // 10
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->readmem); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   mov_eax_memoffs32(state, (unsigned int *)(state->dst->f.i.rt)); // 5
   jmp_imm_short(state, 12); // 2
   
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_reg32_preg32pimm32(state, EAX, EBX, (unsigned int)state->g_rdram); // 6
   
   set_register_state(state, EAX, (unsigned int*)state->dst->f.i.rt, 1);
#endif
}

void genlbu(usf_state_t * state)
{
#ifdef INTERPRET_LBU
   gencallinterp(state, (unsigned int)state->current_instruction_table.LBU, 0);
#else
   free_all_registers(state);
   simplify_access(state);
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->readmemb);
    cmp_reg32_imm32(state, EAX, (unsigned int)read_rdramb);
     }
   je_rj(state, 48);
   
   mov_m32_imm32(state, (unsigned int *)&state->PC, (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_m32_imm32(state, (unsigned int *)(&state->rdword), (unsigned int)state->dst->f.i.rt); // 10
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->readmemb); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   mov_reg32_m32(state, EAX, (unsigned int *)state->dst->f.i.rt); // 6
   jmp_imm_short(state, 15); // 2
   
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(state, BL, 3); // 3
   mov_reg32_preg32pimm32(state, EAX, EBX, (unsigned int)state->g_rdram); // 6
   
   and_eax_imm32(state, 0xFF);
   
   set_register_state(state, EAX, (unsigned int*)state->dst->f.i.rt, 1);
#endif
}

void genlhu(usf_state_t * state)
{
#ifdef INTERPRET_LHU
   gencallinterp(state, (unsigned int)state->current_instruction_table.LHU, 0);
#else
   free_all_registers(state);
   simplify_access(state);
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->readmemh);
    cmp_reg32_imm32(state, EAX, (unsigned int)read_rdramh);
     }
   je_rj(state, 48);
   
   mov_m32_imm32(state, (unsigned int *)&state->PC, (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_m32_imm32(state, (unsigned int *)(&state->rdword), (unsigned int)state->dst->f.i.rt); // 10
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->readmemh); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   mov_reg32_m32(state, EAX, (unsigned int *)state->dst->f.i.rt); // 6
   jmp_imm_short(state, 15); // 2
   
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(state, BL, 2); // 3
   mov_reg32_preg32pimm32(state, EAX, EBX, (unsigned int)state->g_rdram); // 6
   
   and_eax_imm32(state, 0xFFFF);
   
   set_register_state(state, EAX, (unsigned int*)state->dst->f.i.rt, 1);
#endif
}

void genlwr(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.LWR, 0);
}

void genlwu(usf_state_t * state)
{
#ifdef INTERPRET_LWU
   gencallinterp(state, (unsigned int)state->current_instruction_table.LWU, 0);
#else
   free_all_registers(state);
   simplify_access(state);
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->readmem);
    cmp_reg32_imm32(state, EAX, (unsigned int)read_rdram);
     }
   je_rj(state, 47);
   
   mov_m32_imm32(state, (unsigned int *)(&state->PC), (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_m32_imm32(state, (unsigned int *)(&state->rdword), (unsigned int)state->dst->f.i.rt); // 10
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->readmem); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   mov_eax_memoffs32(state, (unsigned int *)(state->dst->f.i.rt)); // 5
   jmp_imm_short(state, 12); // 2
   
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_reg32_preg32pimm32(state, EAX, EBX, (unsigned int)state->g_rdram); // 6
   
   xor_reg32_reg32(state, EBX, EBX);
   
   set_64_register_state(state, EAX, EBX, (unsigned int*)state->dst->f.i.rt, 1);
#endif
}

void gensb(usf_state_t * state)
{
#ifdef INTERPRET_SB
   gencallinterp(state, (unsigned int)state->current_instruction_table.SB, 0);
#else
   free_all_registers(state);
   simplify_access(state);
   mov_reg8_m8(state, CL, (unsigned char *)state->dst->f.i.rt);
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->writememb);
    cmp_reg32_imm32(state, EAX, (unsigned int)write_rdramb);
     }
   je_rj(state, 43);
   
   mov_m32_imm32(state, (unsigned int *)(&state->PC), (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_m8_reg8(state, (unsigned char *)(&state->cpu_byte), CL); // 6
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->writememb); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   mov_eax_memoffs32(state, (unsigned int *)(&state->address)); // 5
   jmp_imm_short(state, 17); // 2
   
   mov_reg32_reg32(state, EAX, EBX); // 2
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(state, BL, 3); // 3
   mov_preg32pimm32_reg8(state, EBX, (unsigned int)state->g_rdram, CL); // 6
   
   mov_reg32_reg32(state, EBX, EAX);
   shr_reg32_imm8(state, EBX, 12);
   cmp_preg32pimm32_imm8(state, EBX, (unsigned int)state->invalid_code, 0);
   jne_rj(state, 54);
   mov_reg32_reg32(state, ECX, EBX); // 2
   shl_reg32_imm8(state, EBX, 2); // 3
   mov_reg32_preg32pimm32(state, EBX, EBX, (unsigned int)state->blocks); // 6
   mov_reg32_preg32pimm32(state, EBX, EBX, (int)&state->actual->block - (int)state->actual); // 6
   and_eax_imm32(state, 0xFFF); // 5
   shr_reg32_imm8(state, EAX, 2); // 3
   mov_reg32_imm32(state, EDX, sizeof(precomp_instr)); // 5
   mul_reg32(state, EDX); // 2
   mov_reg32_preg32preg32pimm32(state, EAX, EAX, EBX, (int)&state->dst->ops - (int)state->dst); // 7
   cmp_reg32_imm32(state, EAX, (unsigned int)state->current_instruction_table.NOTCOMPILED); // 6
   je_rj(state, 7); // 2
   mov_preg32pimm32_imm8(state, ECX, (unsigned int)state->invalid_code, 1); // 7
#endif
}

void gensh(usf_state_t * state)
{
#ifdef INTERPRET_SH
   gencallinterp(state, (unsigned int)state->current_instruction_table.SH, 0);
#else
   free_all_registers(state);
   simplify_access(state);
   mov_reg16_m16(state, CX, (unsigned short *)state->dst->f.i.rt);
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->writememh);
    cmp_reg32_imm32(state, EAX, (unsigned int)write_rdramh);
     }
   je_rj(state, 44);
   
   mov_m32_imm32(state, (unsigned int *)(&state->PC), (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_m16_reg16(state, (unsigned short *)(&state->cpu_hword), CX); // 7
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->writememh); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   mov_eax_memoffs32(state, (unsigned int *)(&state->address)); // 5
   jmp_imm_short(state, 18); // 2
   
   mov_reg32_reg32(state, EAX, EBX); // 2
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(state, BL, 2); // 3
   mov_preg32pimm32_reg16(state, EBX, (unsigned int)state->g_rdram, CX); // 7
   
   mov_reg32_reg32(state, EBX, EAX);
   shr_reg32_imm8(state, EBX, 12);
   cmp_preg32pimm32_imm8(state, EBX, (unsigned int)state->invalid_code, 0);
   jne_rj(state, 54);
   mov_reg32_reg32(state, ECX, EBX); // 2
   shl_reg32_imm8(state, EBX, 2); // 3
   mov_reg32_preg32pimm32(state, EBX, EBX, (unsigned int)state->blocks); // 6
   mov_reg32_preg32pimm32(state, EBX, EBX, (int)&state->actual->block - (int)state->actual); // 6
   and_eax_imm32(state, 0xFFF); // 5
   shr_reg32_imm8(state, EAX, 2); // 3
   mov_reg32_imm32(state, EDX, sizeof(precomp_instr)); // 5
   mul_reg32(state, EDX); // 2
   mov_reg32_preg32preg32pimm32(state, EAX, EAX, EBX, (int)&state->dst->ops - (int)state->dst); // 7
   cmp_reg32_imm32(state, EAX, (unsigned int)state->current_instruction_table.NOTCOMPILED); // 6
   je_rj(state, 7); // 2
   mov_preg32pimm32_imm8(state, ECX, (unsigned int)state->invalid_code, 1); // 7
#endif
}

void genswl(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.SWL, 0);
}

void gensw(usf_state_t * state)
{
#ifdef INTERPRET_SW
   gencallinterp(state, (unsigned int)state->current_instruction_table.SW, 0);
#else
   free_all_registers(state);
   simplify_access(state);
   mov_reg32_m32(state, ECX, (unsigned int *)state->dst->f.i.rt);
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->writemem);
    cmp_reg32_imm32(state, EAX, (unsigned int)write_rdram);
     }
   je_rj(state, 43);
   
   mov_m32_imm32(state, (unsigned int *)(&state->PC), (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_m32_reg32(state, (unsigned int *)(&state->cpu_word), ECX); // 6
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->writemem); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   mov_eax_memoffs32(state, (unsigned int *)(&state->address)); // 5
   jmp_imm_short(state, 14); // 2
   
   mov_reg32_reg32(state, EAX, EBX); // 2
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_preg32pimm32_reg32(state, EBX, (unsigned int)state->g_rdram, ECX); // 6
   
   mov_reg32_reg32(state, EBX, EAX);
   shr_reg32_imm8(state, EBX, 12);
   cmp_preg32pimm32_imm8(state, EBX, (unsigned int)state->invalid_code, 0);
   jne_rj(state, 54);
   mov_reg32_reg32(state, ECX, EBX); // 2
   shl_reg32_imm8(state, EBX, 2); // 3
   mov_reg32_preg32pimm32(state, EBX, EBX, (unsigned int)state->blocks); // 6
   mov_reg32_preg32pimm32(state, EBX, EBX, (int)&state->actual->block - (int)state->actual); // 6
   and_eax_imm32(state, 0xFFF); // 5
   shr_reg32_imm8(state, EAX, 2); // 3
   mov_reg32_imm32(state, EDX, sizeof(precomp_instr)); // 5
   mul_reg32(state, EDX); // 2
   mov_reg32_preg32preg32pimm32(state, EAX, EAX, EBX, (int)&state->dst->ops - (int)state->dst); // 7
   cmp_reg32_imm32(state, EAX, (unsigned int)state->current_instruction_table.NOTCOMPILED); // 6
   je_rj(state, 7); // 2
   mov_preg32pimm32_imm8(state, ECX, (unsigned int)state->invalid_code, 1); // 7
#endif
}

void gensdl(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.SDL, 0);
}

void gensdr(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.SDR, 0);
}

void genswr(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.SWR, 0);
}

void gencheck_cop1_unusable(usf_state_t * state)
{
   free_all_registers(state);
   simplify_access(state);
   test_m32_imm32(state, (unsigned int*)&state->g_cp0_regs[CP0_STATUS_REG], 0x20000000);
   jne_rj(state, 0);

   jump_start_rel8(state);
   
   gencallinterp(state, (unsigned int)check_cop1_unusable, 0);
   
   jump_end_rel8(state);
}

void genlwc1(usf_state_t * state)
{
#ifdef INTERPRET_LWC1
   gencallinterp(state, (unsigned int)state->current_instruction_table.LWC1, 0);
#else
   gencheck_cop1_unusable(state);
   
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg[state->dst->f.lf.base]));
   add_eax_imm32(state, (int)state->dst->f.lf.offset);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->readmem);
    cmp_reg32_imm32(state, EAX, (unsigned int)read_rdram);
     }
   je_rj(state, 44);
   
   mov_m32_imm32(state, (unsigned int *)(&state->PC), (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_reg32_m32(state, EDX, (unsigned int*)(&state->reg_cop1_simple[state->dst->f.lf.ft])); // 6
   mov_m32_reg32(state, (unsigned int *)(&state->rdword), EDX); // 6
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->readmem); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   jmp_imm_short(state, 20); // 2
   
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_reg32_preg32pimm32(state, EAX, EBX, (unsigned int)state->g_rdram); // 6
   mov_reg32_m32(state, EBX, (unsigned int*)(&state->reg_cop1_simple[state->dst->f.lf.ft])); // 6
   mov_preg32_reg32(state, EBX, EAX); // 2
#endif
}

void genldc1(usf_state_t * state)
{
#ifdef INTERPRET_LDC1
   gencallinterp(state, (unsigned int)state->current_instruction_table.LDC1, 0);
#else
   gencheck_cop1_unusable(state);
   
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg[state->dst->f.lf.base]));
   add_eax_imm32(state, (int)state->dst->f.lf.offset);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->readmemd);
    cmp_reg32_imm32(state, EAX, (unsigned int)read_rdramd);
     }
   je_rj(state, 44);
   
   mov_m32_imm32(state, (unsigned int *)(&state->PC), (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_reg32_m32(state, EDX, (unsigned int*)(&state->reg_cop1_double[state->dst->f.lf.ft])); // 6
   mov_m32_reg32(state, (unsigned int *)(&state->rdword), EDX); // 6
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->readmemd); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   jmp_imm_short(state, 32); // 2
   
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_reg32_preg32pimm32(state, EAX, EBX, ((unsigned int)state->g_rdram)+4); // 6
   mov_reg32_preg32pimm32(state, ECX, EBX, ((unsigned int)state->g_rdram)); // 6
   mov_reg32_m32(state, EBX, (unsigned int*)(&state->reg_cop1_double[state->dst->f.lf.ft])); // 6
   mov_preg32_reg32(state, EBX, EAX); // 2
   mov_preg32pimm32_reg32(state, EBX, 4, ECX); // 6
#endif
}

void gencache(usf_state_t * state)
{
}

void genld(usf_state_t * state)
{
#ifdef INTERPRET_LD
   gencallinterp(state, (unsigned int)state->current_instruction_table.LD, 0);
#else
   free_all_registers(state);
   simplify_access(state);
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->readmemd);
    cmp_reg32_imm32(state, EAX, (unsigned int)read_rdramd);
     }
   je_rj(state, 53);
   
   mov_m32_imm32(state, (unsigned int *)(&state->PC), (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_m32_imm32(state, (unsigned int *)(&state->rdword), (unsigned int)state->dst->f.i.rt); // 10
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->readmemd); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   mov_eax_memoffs32(state, (unsigned int *)(state->dst->f.i.rt)); // 5
   mov_reg32_m32(state, ECX, (unsigned int *)(state->dst->f.i.rt)+1); // 6
   jmp_imm_short(state, 18); // 2
   
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_reg32_preg32pimm32(state, EAX, EBX, ((unsigned int)state->g_rdram)+4); // 6
   mov_reg32_preg32pimm32(state, ECX, EBX, ((unsigned int)state->g_rdram)); // 6
   
   set_64_register_state(state, EAX, ECX, (unsigned int*)state->dst->f.i.rt, 1);
#endif
}

void genswc1(usf_state_t * state)
{
#ifdef INTERPRET_SWC1
   gencallinterp(state, (unsigned int)state->current_instruction_table.SWC1, 0);
#else
   gencheck_cop1_unusable(state);
   
   mov_reg32_m32(state, EDX, (unsigned int*)(&state->reg_cop1_simple[state->dst->f.lf.ft]));
   mov_reg32_preg32(state, ECX, EDX);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg[state->dst->f.lf.base]));
   add_eax_imm32(state, (int)state->dst->f.lf.offset);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->writemem);
    cmp_reg32_imm32(state, EAX, (unsigned int)write_rdram);
     }
   je_rj(state, 43);
   
   mov_m32_imm32(state, (unsigned int *)(&state->PC), (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_m32_reg32(state, (unsigned int *)(&state->cpu_word), ECX); // 6
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->writemem); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   mov_eax_memoffs32(state, (unsigned int *)(&state->address)); // 5
   jmp_imm_short(state, 14); // 2
   
   mov_reg32_reg32(state, EAX, EBX); // 2
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_preg32pimm32_reg32(state, EBX, (unsigned int)state->g_rdram, ECX); // 6
   
   mov_reg32_reg32(state, EBX, EAX);
   shr_reg32_imm8(state, EBX, 12);
   cmp_preg32pimm32_imm8(state, EBX, (unsigned int)state->invalid_code, 0);
   jne_rj(state, 54);
   mov_reg32_reg32(state, ECX, EBX); // 2
   shl_reg32_imm8(state, EBX, 2); // 3
   mov_reg32_preg32pimm32(state, EBX, EBX, (unsigned int)state->blocks); // 6
   mov_reg32_preg32pimm32(state, EBX, EBX, (int)&state->actual->block - (int)state->actual); // 6
   and_eax_imm32(state, 0xFFF); // 5
   shr_reg32_imm8(state, EAX, 2); // 3
   mov_reg32_imm32(state, EDX, sizeof(precomp_instr)); // 5
   mul_reg32(state, EDX); // 2
   mov_reg32_preg32preg32pimm32(state, EAX, EAX, EBX, (int)&state->dst->ops - (int)state->dst); // 7
   cmp_reg32_imm32(state, EAX, (unsigned int)state->current_instruction_table.NOTCOMPILED); // 6
   je_rj(state, 7); // 2
   mov_preg32pimm32_imm8(state, ECX, (unsigned int)state->invalid_code, 1); // 7
#endif
}

void gensdc1(usf_state_t * state)
{
#ifdef INTERPRET_SDC1
   gencallinterp(state, (unsigned int)state->current_instruction_table.SDC1, 0);
#else
   gencheck_cop1_unusable(state);
   
   mov_reg32_m32(state, EDI, (unsigned int*)(&state->reg_cop1_double[state->dst->f.lf.ft]));
   mov_reg32_preg32(state, ECX, EDI);
   mov_reg32_preg32pimm32(state, EDX, EDI, 4);
   mov_eax_memoffs32(state, (unsigned int *)(&state->reg[state->dst->f.lf.base]));
   add_eax_imm32(state, (int)state->dst->f.lf.offset);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->writememd);
    cmp_reg32_imm32(state, EAX, (unsigned int)write_rdramd);
     }
   je_rj(state, 49);
   
   mov_m32_imm32(state, (unsigned int *)(&state->PC), (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_m32_reg32(state, (unsigned int *)(&state->cpu_dword), ECX); // 6
   mov_m32_reg32(state, (unsigned int *)(&state->cpu_dword)+1, EDX); // 6
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->writememd); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   mov_eax_memoffs32(state, (unsigned int *)(&state->address)); // 5
   jmp_imm_short(state, 20); // 2
   
   mov_reg32_reg32(state, EAX, EBX); // 2
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_preg32pimm32_reg32(state, EBX, ((unsigned int)state->g_rdram)+4, ECX); // 6
   mov_preg32pimm32_reg32(state, EBX, ((unsigned int)state->g_rdram)+0, EDX); // 6
   
   mov_reg32_reg32(state, EBX, EAX);
   shr_reg32_imm8(state, EBX, 12);
   cmp_preg32pimm32_imm8(state, EBX, (unsigned int)state->invalid_code, 0);
   jne_rj(state, 54);
   mov_reg32_reg32(state, ECX, EBX); // 2
   shl_reg32_imm8(state, EBX, 2); // 3
   mov_reg32_preg32pimm32(state, EBX, EBX, (unsigned int)state->blocks); // 6
   mov_reg32_preg32pimm32(state, EBX, EBX, (int)&state->actual->block - (int)state->actual); // 6
   and_eax_imm32(state, 0xFFF); // 5
   shr_reg32_imm8(state, EAX, 2); // 3
   mov_reg32_imm32(state, EDX, sizeof(precomp_instr)); // 5
   mul_reg32(state, EDX); // 2
   mov_reg32_preg32preg32pimm32(state, EAX, EAX, EBX, (int)&state->dst->ops - (int)state->dst); // 7
   cmp_reg32_imm32(state, EAX, (unsigned int)state->current_instruction_table.NOTCOMPILED); // 6
   je_rj(state, 7); // 2
   mov_preg32pimm32_imm8(state, ECX, (unsigned int)state->invalid_code, 1); // 7
#endif
}

void gensd(usf_state_t * state)
{
#ifdef INTERPRET_SD
   gencallinterp(state, (unsigned int)state->current_instruction_table.SD, 0);
#else
   free_all_registers(state);
   simplify_access(state);
   
   mov_reg32_m32(state, ECX, (unsigned int *)state->dst->f.i.rt);
   mov_reg32_m32(state, EDX, ((unsigned int *)state->dst->f.i.rt)+1);
   mov_eax_memoffs32(state, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    shr_reg32_imm8(state, EAX, 16);
    mov_reg32_preg32x4pimm32(state, EAX, EAX, (unsigned int)state->writememd);
    cmp_reg32_imm32(state, EAX, (unsigned int)write_rdramd);
     }
   je_rj(state, 49);
   
   mov_m32_imm32(state, (unsigned int *)(&state->PC), (unsigned int)(state->dst+1)); // 10
   mov_m32_reg32(state, (unsigned int *)(&state->address), EBX); // 6
   mov_m32_reg32(state, (unsigned int *)(&state->cpu_dword), ECX); // 6
   mov_m32_reg32(state, (unsigned int *)(&state->cpu_dword)+1, EDX); // 6
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg32_preg32x4pimm32(state, EBX, EBX, (unsigned int)state->writememd); // 7
   mov_reg32_reg32(state, RP0, ESI); // 2
   call_reg32(state, EBX); // 2
   mov_eax_memoffs32(state, (unsigned int *)(&state->address)); // 5
   jmp_imm_short(state, 20); // 2
   
   mov_reg32_reg32(state, EAX, EBX); // 2
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_preg32pimm32_reg32(state, EBX, ((unsigned int)state->g_rdram)+4, ECX); // 6
   mov_preg32pimm32_reg32(state, EBX, ((unsigned int)state->g_rdram)+0, EDX); // 6
   
   mov_reg32_reg32(state, EBX, EAX);
   shr_reg32_imm8(state, EBX, 12);
   cmp_preg32pimm32_imm8(state, EBX, (unsigned int)state->invalid_code, 0);
   jne_rj(state, 54);
   mov_reg32_reg32(state, ECX, EBX); // 2
   shl_reg32_imm8(state, EBX, 2); // 3
   mov_reg32_preg32pimm32(state, EBX, EBX, (unsigned int)state->blocks); // 6
   mov_reg32_preg32pimm32(state, EBX, EBX, (int)&state->actual->block - (int)state->actual); // 6
   and_eax_imm32(state, 0xFFF); // 5
   shr_reg32_imm8(state, EAX, 2); // 3
   mov_reg32_imm32(state, EDX, sizeof(precomp_instr)); // 5
   mul_reg32(state, EDX); // 2
   mov_reg32_preg32preg32pimm32(state, EAX, EAX, EBX, (int)&state->dst->ops - (int)state->dst); // 7
   cmp_reg32_imm32(state, EAX, (unsigned int)state->current_instruction_table.NOTCOMPILED); // 6
   je_rj(state, 7); // 2
   mov_preg32pimm32_imm8(state, ECX, (unsigned int)state->invalid_code, 1); // 7
#endif
}

void genll(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.LL, 0);
}

void gensc(usf_state_t * state)
{
   gencallinterp(state, (unsigned int)state->current_instruction_table.SC, 0);
}

