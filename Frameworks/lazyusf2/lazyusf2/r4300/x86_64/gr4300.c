/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - gr4300.c                                                *
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

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "assemble.h"
#include "regcache.h"
#include "interpret.h"

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

#if defined(COUNT_INSTR)
#include "r4300/instr_counters.h"
#endif

#if !defined(offsetof)
#   define offsetof(TYPE,MEMBER) ((unsigned int) &((TYPE*)0)->MEMBER)
#endif

/* static functions */

#ifdef DYNAREC
static void genupdate_count(usf_state_t * state, unsigned int addr)
{
   mov_reg32_imm32(state, EAX, addr);
   sub_xreg32_m32rel(state, EAX, (unsigned int*)(&state->last_addr));
   shr_reg32_imm8(state, EAX, 2);
   mov_xreg32_m32rel(state, EDX, (void*)&state->count_per_op);
   mul_reg32(state, EDX);
   add_m32rel_xreg32(state, (unsigned int*)(&state->g_cp0_regs[CP0_COUNT_REG]), EAX);
   add_m32rel_xreg32(state, (unsigned int*)(&state->cycle_count), EAX);
}

static void gencheck_interupt(usf_state_t * state, unsigned long long instr_structure)
{
   mov_xreg32_m32rel(state, EAX, (void*)(&state->cycle_count));
   test_reg32_reg32(state, EAX, EAX);
   js_rj(state, 0);
   jump_start_rel8(state);

   mov_reg64_imm64(state, RAX, (unsigned long long) instr_structure);
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX);
   mov_reg64_imm64(state, RAX, (unsigned long long) gen_interupt);
   free_register(state, RP0);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, RAX);

   jump_end_rel8(state);
}

static void gencheck_interupt_out(usf_state_t * state, unsigned int addr)
{
   mov_xreg32_m32rel(state, EAX, (void*)(&state->cycle_count));
   test_reg32_reg32(state, EAX, EAX);
   js_rj(state, 0);
   jump_start_rel8(state);

   mov_m32rel_imm32(state, (unsigned int*)(&state->fake_instr.addr), addr);
   mov_reg64_imm64(state, RAX, (unsigned long long) (&state->fake_instr));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX);
   mov_reg64_imm64(state, RAX, (unsigned long long) gen_interupt);
   free_register(state, RP0);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, RAX);

   jump_end_rel8(state);
}

static void genbeq_test(usf_state_t * state)
{
   int rs_64bit = is64(state, (unsigned int *)state->dst->f.i.rs);
   int rt_64bit = is64(state, (unsigned int *)state->dst->f.i.rt);
   
   if (rs_64bit == 0 && rt_64bit == 0)
     {
    int rs = allocate_register_32(state, (unsigned int *)state->dst->f.i.rs);
    int rt = allocate_register_32(state, (unsigned int *)state->dst->f.i.rt);
    
    cmp_reg32_reg32(state, rs, rt);
    sete_m8rel(state, (unsigned char *) &state->branch_taken);
     }
   else if (rs_64bit == -1)
     {
    int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.i.rt);
    
    cmp_xreg64_m64rel(state, rt, (unsigned long long *) state->dst->f.i.rs);
    sete_m8rel(state, (unsigned char *) &state->branch_taken);
     }
   else if (rt_64bit == -1)
     {
    int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.i.rs);
    
    cmp_xreg64_m64rel(state, rs, (unsigned long long *)state->dst->f.i.rt);
    sete_m8rel(state, (unsigned char *) &state->branch_taken);
     }
   else
     {
    int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.i.rs);
    int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.i.rt);
    cmp_reg64_reg64(state, rs, rt);
    sete_m8rel(state, (unsigned char *) &state->branch_taken);
     }
}

static void genbne_test(usf_state_t * state)
{
   int rs_64bit = is64(state, (unsigned int *)state->dst->f.i.rs);
   int rt_64bit = is64(state, (unsigned int *)state->dst->f.i.rt);
   
   if (rs_64bit == 0 && rt_64bit == 0)
     {
    int rs = allocate_register_32(state, (unsigned int *)state->dst->f.i.rs);
    int rt = allocate_register_32(state, (unsigned int *)state->dst->f.i.rt);
    
    cmp_reg32_reg32(state, rs, rt);
    setne_m8rel(state, (unsigned char *) &state->branch_taken);
     }
   else if (rs_64bit == -1)
     {
    int rt = allocate_register_64(state, (unsigned long long *) state->dst->f.i.rt);

    cmp_xreg64_m64rel(state, rt, (unsigned long long *)state->dst->f.i.rs);
    setne_m8rel(state, (unsigned char *) &state->branch_taken);
     }
   else if (rt_64bit == -1)
     {
    int rs = allocate_register_64(state, (unsigned long long *) state->dst->f.i.rs);
    
    cmp_xreg64_m64rel(state, rs, (unsigned long long *)state->dst->f.i.rt);
    setne_m8rel(state, (unsigned char *) &state->branch_taken);
     }
   else
     {
    int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.i.rs);
    int rt = allocate_register_64(state, (unsigned long long *)state->dst->f.i.rt);

    cmp_reg64_reg64(state, rs, rt);
    setne_m8rel(state, (unsigned char *) &state->branch_taken);
     }
}

static void genblez_test(usf_state_t * state)
{
   int rs_64bit = is64(state, (unsigned int *)state->dst->f.i.rs);
   
   if (rs_64bit == 0)
     {
    int rs = allocate_register_32(state, (unsigned int *)state->dst->f.i.rs);
    
    cmp_reg32_imm32(state, rs, 0);
    setle_m8rel(state, (unsigned char *) &state->branch_taken);
     }
   else
     {
    int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.i.rs);
    
    cmp_reg64_imm8(state, rs, 0);
    setle_m8rel(state, (unsigned char *) &state->branch_taken);
     }
}

static void genbgtz_test(usf_state_t * state)
{
   int rs_64bit = is64(state, (unsigned int *)state->dst->f.i.rs);
   
   if (rs_64bit == 0)
     {
    int rs = allocate_register_32(state, (unsigned int *)state->dst->f.i.rs);
    
    cmp_reg32_imm32(state, rs, 0);
    setg_m8rel(state, (unsigned char *) &state->branch_taken);
     }
   else
     {
    int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.i.rs);

    cmp_reg64_imm8(state, rs, 0);
    setg_m8rel(state, (unsigned char *) &state->branch_taken);
     }
}

static void ld_register_alloc(usf_state_t * state, int *pGpr1, int *pGpr2, int *pBase1, int *pBase2)
{
   int gpr1, gpr2, base1, base2 = 0;

   if (state->dst->f.i.rs == state->dst->f.i.rt)
   {
     allocate_register_32(state, (unsigned int*)state->dst->f.r.rs);          // tell regcache we need to read RS register here
     gpr1 = allocate_register_32_w(state, (unsigned int*)state->dst->f.r.rt); // tell regcache we will modify RT register during this instruction
     gpr2 = lock_register(state, lru_register(state));                      // free and lock least recently used register for usage here
     add_reg32_imm32(state, gpr1, (int)state->dst->f.i.immediate);
     mov_reg32_reg32(state, gpr2, gpr1);
   }
   else
   {
     gpr2 = allocate_register_32(state, (unsigned int*)state->dst->f.r.rs);   // tell regcache we need to read RS register here
     gpr1 = allocate_register_32_w(state, (unsigned int*)state->dst->f.r.rt); // tell regcache we will modify RT register during this instruction
     free_register(state, gpr2);                                       // write out gpr2 if dirty because I'm going to trash it right now
     add_reg32_imm32(state, gpr2, (int)state->dst->f.i.immediate);
     mov_reg32_reg32(state, gpr1, gpr2);
     lock_register(state, gpr2);                                       // lock the freed gpr2 it so it doesn't get returned in the lru query
   }
   base1 = lock_register(state, lru_base_register(state));                  // get another lru register
   if (!state->fast_memory)
   {
     base2 = lock_register(state, lru_base_register(state));                // and another one if necessary
     unlock_register(state, base2);
   }
   unlock_register(state, base1);                                      // unlock the locked registers (they are
   unlock_register(state, gpr2);
   set_register_state(state, gpr1, NULL, 0, 0);                        // clear gpr1 state because it hasn't been written yet -
                                                                // we don't want it to be pushed/popped around read_rdramX call
   *pGpr1 = gpr1;
   *pGpr2 = gpr2;
   *pBase1 = base1;
   *pBase2 = base2;
}


/* global functions */

void gennotcompiled(usf_state_t * state)
{
   free_registers_move_start(state);

   mov_reg64_imm64(state, RAX, (unsigned long long) state->dst);
   mov_memoffs64_rax(state, (unsigned long long *) &state->PC); /* RIP-relative will not work here */
   mov_reg64_imm64(state, RAX, (unsigned long long) state->current_instruction_table.NOTCOMPILED);
   free_register(state, RP0);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, RAX);
}

void genlink_subblock(usf_state_t * state)
{
   free_all_registers(state);
   jmp(state, state->dst->addr+4);
}

void gencallinterp(usf_state_t * state, unsigned long long addr, int jump)
{
   free_registers_move_start(state);

   if (jump)
     mov_m32rel_imm32(state, (unsigned int*)(&state->dyna_interp), 1);

   mov_reg64_imm64(state, RAX, (unsigned long long) state->dst);
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX);
   mov_reg64_imm64(state, RAX, addr);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, RAX);

   if (jump)
   {
     mov_m32rel_imm32(state, (unsigned int*)(&state->dyna_interp), 0);
     mov_reg64_imm64(state, RAX, (unsigned long long)dyna_jump);
     mov_reg64_reg64(state, RP0, 15);
     call_reg64(state, RAX);
   }
}

void gendelayslot(usf_state_t * state)
{
   mov_m32rel_imm32(state, (void*)(&state->delay_slot), 1);
   recompile_opcode(state);
   
   free_all_registers(state);
   genupdate_count(state, state->dst->addr+4);
   
   mov_m32rel_imm32(state, (void*)(&state->delay_slot), 0);
}

void genni(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[1]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.NI, 0);
}

void genreserved(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[0]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.RESERVED, 0);
}

void genfin_block(usf_state_t * state)
{
   gencallinterp(state, (unsigned long long)state->current_instruction_table.FIN_BLOCK, 0);
}

void gencheck_interupt_reg(usf_state_t * state) // addr is in EAX
{
   mov_xreg32_m32rel(state, EBX, (void*)(&state->cycle_count));
   test_reg32_reg32(state, EBX, EBX);
   js_rj(state, 0);
   jump_start_rel8(state);

   mov_m32rel_xreg32(state, (unsigned int*)(&state->fake_instr.addr), EAX);
   mov_reg64_imm64(state, RAX, (unsigned long long) (&state->fake_instr));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX);
   mov_reg64_imm64(state, RAX, (unsigned long long) gen_interupt);
   free_register(state, RP0);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, RAX);

   jump_end_rel8(state);
}

void gennop(usf_state_t * state)
{
    (void)state;
}

void genj(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[2]);
#endif
#ifdef INTERPRET_J
   gencallinterp(state, (unsigned long long)state->current_instruction_table.J, 1);
#else
   unsigned int naddr;
   
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.J, 1);
    return;
     }
   
   gendelayslot(state);
   naddr = ((state->dst-1)->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);
   
   mov_m32rel_imm32(state, (void*)(&state->last_addr), naddr);
   gencheck_interupt(state, (unsigned long long) &state->actual->block[(naddr-state->actual->start)/4]);
   jmp(state, naddr);
#endif
}

void genj_out(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[2]);
#endif
#ifdef INTERPRET_J_OUT
   gencallinterp(state, (unsigned long long)state->current_instruction_table.J_OUT, 1);
#else
   unsigned int naddr;
   
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.J_OUT, 1);
    return;
     }
   
   gendelayslot(state);
   naddr = ((state->dst-1)->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);
   
   mov_m32rel_imm32(state, (void*)(&state->last_addr), naddr);
   gencheck_interupt_out(state, naddr);
   mov_m32rel_imm32(state, &state->jump_to_address, naddr);
   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX);
   mov_reg64_imm64(state, RAX, (unsigned long long)jump_to_func);
   free_register(state, RP0);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, RAX);
#endif
}

void genj_idle(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[2]);
#endif
#ifdef INTERPRET_J_IDLE
   gencallinterp(state, (unsigned long long)state->current_instruction_table.J_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.J_IDLE, 1);
    return;
     }
   
   mov_xreg32_m32rel(state, EAX, (unsigned int *)(&state->cycle_count));
   test_reg32_reg32(state, EAX, EAX);
   jns_rj(state, 18);


   sub_m32rel_xreg32(state, (unsigned int *)(&state->g_cp0_regs[CP0_COUNT_REG]), EAX); // 7
   mov_m32rel_imm32(state, (unsigned int *)(&state->cycle_count), 0); // 11

   genj(state);
#endif
}

void genjal(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[3]);
#endif
#ifdef INTERPRET_JAL
   gencallinterp(state, (unsigned long long)state->current_instruction_table.JAL, 1);
#else
   unsigned int naddr;
   
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.JAL, 1);
    return;
     }
   
   gendelayslot(state);
   
   mov_m32rel_imm32(state, (unsigned int *)(state->reg + 31), state->dst->addr + 4);
   if (((state->dst->addr + 4) & 0x80000000))
     mov_m32rel_imm32(state, (unsigned int *)(&state->reg[31])+1, 0xFFFFFFFF);
   else
     mov_m32rel_imm32(state, (unsigned int *)(&state->reg[31])+1, 0);
   
   naddr = ((state->dst-1)->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);

   mov_m32rel_imm32(state, (void*)(&state->last_addr), naddr);
   gencheck_interupt(state, (unsigned long long) &state->actual->block[(naddr-state->actual->start)/4]);
   jmp(state, naddr);
#endif
}

void genjal_out(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[3]);
#endif
#ifdef INTERPRET_JAL_OUT
   gencallinterp(state, (unsigned long long)state->current_instruction_table.JAL_OUT, 1);
#else
   unsigned int naddr;
   
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.JAL_OUT, 1);
    return;
     }
   
   gendelayslot(state);

   mov_m32rel_imm32(state, (unsigned int *)(state->reg + 31), state->dst->addr + 4);
   if (((state->dst->addr + 4) & 0x80000000))
     mov_m32rel_imm32(state, (unsigned int *)(&state->reg[31])+1, 0xFFFFFFFF);
   else
     mov_m32rel_imm32(state, (unsigned int *)(&state->reg[31])+1, 0);
   
   naddr = ((state->dst-1)->f.j.inst_index<<2) | (state->dst->addr & 0xF0000000);

   mov_m32rel_imm32(state, (void*)(&state->last_addr), naddr);
   gencheck_interupt_out(state, naddr);
   mov_m32rel_imm32(state, &state->jump_to_address, naddr);
   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX);
   mov_reg64_imm64(state, RAX, (unsigned long long) jump_to_func);
   free_register(state, RP0);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, RAX);
#endif
}

void genjal_idle(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[3]);
#endif
#ifdef INTERPRET_JAL_IDLE
   gencallinterp(state, (unsigned long long)state->current_instruction_table.JAL_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.JAL_IDLE, 1);
    return;
     }
   
   mov_xreg32_m32rel(state, EAX, (unsigned int *)(&state->cycle_count));
   test_reg32_reg32(state, EAX, EAX);
   jns_rj(state, 18);
   
   sub_m32rel_xreg32(state, (unsigned int *)(&state->g_cp0_regs[CP0_COUNT_REG]), EAX); // 7
   mov_m32rel_imm32(state, (unsigned int *)(&state->cycle_count), 0); // 11
  
   genjal(state);
#endif
}

void gentest(usf_state_t * state)
{
   cmp_m32rel_imm32(state, (unsigned int *)(&state->branch_taken), 0);
   je_near_rj(state, 0);
   jump_start_rel32(state);

   mov_m32rel_imm32(state, (void*)(&state->last_addr), state->dst->addr + (state->dst-1)->f.i.immediate*4);
   gencheck_interupt(state, (unsigned long long) (state->dst + (state->dst-1)->f.i.immediate));
   jmp(state, state->dst->addr + (state->dst-1)->f.i.immediate*4);

   jump_end_rel32(state);

   mov_m32rel_imm32(state, (void*)(&state->last_addr), state->dst->addr + 4);
   gencheck_interupt(state, (unsigned long long)(state->dst + 1));
   jmp(state, state->dst->addr + 4);
}

void genbeq(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[4]);
#endif
#ifdef INTERPRET_BEQ
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BEQ, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BEQ, 1);
    return;
     }
   
   genbeq_test(state);
   gendelayslot(state);
   gentest(state);
#endif
}

void gentest_out(usf_state_t * state)
{
   cmp_m32rel_imm32(state, (unsigned int *)(&state->branch_taken), 0);
   je_near_rj(state, 0);
   jump_start_rel32(state);

   mov_m32rel_imm32(state, (void*)(&state->last_addr), state->dst->addr + (state->dst-1)->f.i.immediate*4);
   gencheck_interupt_out(state, state->dst->addr + (state->dst-1)->f.i.immediate*4);
   mov_m32rel_imm32(state, &state->jump_to_address, state->dst->addr + (state->dst-1)->f.i.immediate*4);
   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX);
   mov_reg64_imm64(state, RAX, (unsigned long long) jump_to_func);
   free_register(state, RP0);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, RAX);
   jump_end_rel32(state);

   mov_m32rel_imm32(state, (void*)(&state->last_addr), state->dst->addr + 4);
   gencheck_interupt(state, (unsigned long long) (state->dst + 1));
   jmp(state, state->dst->addr + 4);
}

void genbeq_out(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[4]);
#endif
#ifdef INTERPRET_BEQ_OUT
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BEQ_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BEQ_OUT, 1);
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
   
   cmp_m32rel_imm32(state, (unsigned int *)(&state->branch_taken), 0);
   je_near_rj(state, 0);
   jump_start_rel32(state);

   mov_xreg32_m32rel(state, reg, (unsigned int *)(&state->cycle_count));
   test_reg32_reg32(state, reg, reg);
   jns_rj(state, 0);

   jump_start_rel8(state);
   
   sub_m32rel_xreg32(state, (unsigned int *)(&state->g_cp0_regs[CP0_COUNT_REG]), reg);
   mov_m32rel_imm32(state, (unsigned int *)(&state->cycle_count), 0);
   
   jump_end_rel8(state);
   jump_end_rel32(state);
}

void genbeq_idle(usf_state_t * state)
{
#ifdef INTERPRET_BEQ_IDLE
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BEQ_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BEQ_IDLE, 1);
    return;
     }
   
   genbeq_test(state);
   gentest_idle(state);
   genbeq(state);
#endif
}

void genbne(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[5]);
#endif
#ifdef INTERPRET_BNE
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BNE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BNE, 1);
    return;
     }
   
   genbne_test(state);
   gendelayslot(state);
   gentest(state);
#endif
}

void genbne_out(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[5]);
#endif
#ifdef INTERPRET_BNE_OUT
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BNE_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BNE_OUT, 1);
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
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BNE_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BNE_IDLE, 1);
    return;
     }
   
   genbne_test(state);
   gentest_idle(state);
   genbne(state);
#endif
}

void genblez(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[6]);
#endif
#ifdef INTERPRET_BLEZ
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BLEZ, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BLEZ, 1);
    return;
     }
   
   genblez_test(state);
   gendelayslot(state);
   gentest(state);
#endif
}

void genblez_out(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[6]);
#endif
#ifdef INTERPRET_BLEZ_OUT
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BLEZ_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BLEZ_OUT, 1);
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
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BLEZ_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BLEZ_IDLE, 1);
    return;
     }
   
   genblez_test(state);
   gentest_idle(state);
   genblez(state);
#endif
}

void genbgtz(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[7]);
#endif
#ifdef INTERPRET_BGTZ
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BGTZ, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BGTZ, 1);
    return;
     }
   
   genbgtz_test(state);
   gendelayslot(state);
   gentest(state);
#endif
}

void genbgtz_out(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[7]);
#endif
#ifdef INTERPRET_BGTZ_OUT
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BGTZ_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BGTZ_OUT, 1);
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
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BGTZ_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BGTZ_IDLE, 1);
    return;
     }
   
   genbgtz_test(state);
   gentest_idle(state);
   genbgtz(state);
#endif
}

void genaddi(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[8]);
#endif
#ifdef INTERPRET_ADDI
   gencallinterp(state, (unsigned long long)state->current_instruction_table.ADDI, 0);
#else
   int rs = allocate_register_32(state, (unsigned int *)state->dst->f.i.rs);
   int rt = allocate_register_32_w(state, (unsigned int *)state->dst->f.i.rt);

   mov_reg32_reg32(state, rt, rs);
   add_reg32_imm32(state, rt,(int)state->dst->f.i.immediate);
#endif
}

void genaddiu(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[9]);
#endif
#ifdef INTERPRET_ADDIU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.ADDIU, 0);
#else
   int rs = allocate_register_32(state, (unsigned int *)state->dst->f.i.rs);
   int rt = allocate_register_32_w(state, (unsigned int *)state->dst->f.i.rt);

   mov_reg32_reg32(state, rt, rs);
   add_reg32_imm32(state, rt,(int)state->dst->f.i.immediate);
#endif
}

void genslti(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[10]);
#endif
#ifdef INTERPRET_SLTI
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SLTI, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *) state->dst->f.i.rs);
   int rt = allocate_register_64_w(state, (unsigned long long *) state->dst->f.i.rt);
   int imm = (int) state->dst->f.i.immediate;
   
   cmp_reg64_imm32(state, rs, imm);
   setl_reg8(state, rt);
   and_reg64_imm8(state, rt, 1);
#endif
}

void gensltiu(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[11]);
#endif
#ifdef INTERPRET_SLTIU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SLTIU, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.i.rs);
   int rt = allocate_register_64_w(state, (unsigned long long *)state->dst->f.i.rt);
   int imm = (int) state->dst->f.i.immediate;
   
   cmp_reg64_imm32(state, rs, imm);
   setb_reg8(state, rt);
   and_reg64_imm8(state, rt, 1);
#endif
}

void genandi(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[12]);
#endif
#ifdef INTERPRET_ANDI
   gencallinterp(state, (unsigned long long)state->current_instruction_table.ANDI, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.i.rs);
   int rt = allocate_register_64_w(state, (unsigned long long *)state->dst->f.i.rt);
   
   mov_reg64_reg64(state, rt, rs);
   and_reg64_imm32(state, rt, (unsigned short)state->dst->f.i.immediate);
#endif
}

void genori(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[13]);
#endif
#ifdef INTERPRET_ORI
   gencallinterp(state, (unsigned long long)state->current_instruction_table.ORI, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *) state->dst->f.i.rs);
   int rt = allocate_register_64_w(state, (unsigned long long *) state->dst->f.i.rt);
   
   mov_reg64_reg64(state, rt, rs);
   or_reg64_imm32(state, rt, (unsigned short)state->dst->f.i.immediate);
#endif
}

void genxori(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[14]);
#endif
#ifdef INTERPRET_XORI
   gencallinterp(state, (unsigned long long)state->current_instruction_table.XORI, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.i.rs);
   int rt = allocate_register_64_w(state, (unsigned long long *)state->dst->f.i.rt);
   
   mov_reg64_reg64(state, rt, rs);
   xor_reg64_imm32(state, rt, (unsigned short)state->dst->f.i.immediate);
#endif
}

void genlui(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[15]);
#endif
#ifdef INTERPRET_LUI
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LUI, 0);
#else
   int rt = allocate_register_32_w(state, (unsigned int *)state->dst->f.i.rt);

   mov_reg32_imm32(state, rt, (unsigned int)state->dst->f.i.immediate << 16);
#endif
}

void gentestl(usf_state_t * state)
{
   cmp_m32rel_imm32(state, (unsigned int *)(&state->branch_taken), 0);
   je_near_rj(state, 0);
   jump_start_rel32(state);

   gendelayslot(state);
   mov_m32rel_imm32(state, (void*)(&state->last_addr), state->dst->addr + (state->dst-1)->f.i.immediate*4);
   gencheck_interupt(state, (unsigned long long) (state->dst + (state->dst-1)->f.i.immediate));
   jmp(state, state->dst->addr + (state->dst-1)->f.i.immediate*4);
   
   jump_end_rel32(state);

   genupdate_count(state, state->dst->addr-4);
   mov_m32rel_imm32(state, (void*)(&state->last_addr), state->dst->addr + 4);
   gencheck_interupt(state, (unsigned long long) (state->dst + 1));
   jmp(state, state->dst->addr + 4);
}

void genbeql(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[16]);
#endif
#ifdef INTERPRET_BEQL
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BEQL, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BEQL, 1);
    return;
     }
   
   genbeq_test(state);
   free_all_registers(state);
   gentestl(state);
#endif
}

void gentestl_out(usf_state_t * state)
{
   cmp_m32rel_imm32(state, (unsigned int *)(&state->branch_taken), 0);
   je_near_rj(state, 0);
   jump_start_rel32(state);

   gendelayslot(state);
   mov_m32rel_imm32(state, (void*)(&state->last_addr), state->dst->addr + (state->dst-1)->f.i.immediate*4);
   gencheck_interupt_out(state, state->dst->addr + (state->dst-1)->f.i.immediate*4);
   mov_m32rel_imm32(state, &state->jump_to_address, state->dst->addr + (state->dst-1)->f.i.immediate*4);

   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX);
   mov_reg64_imm64(state, RAX, (unsigned long long) jump_to_func);
   free_register(state, RP0);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, RAX);
   
   jump_end_rel32(state);

   genupdate_count(state, state->dst->addr-4);
   mov_m32rel_imm32(state, (void*)(&state->last_addr), state->dst->addr + 4);
   gencheck_interupt(state, (unsigned long long) (state->dst + 1));
   jmp(state, state->dst->addr + 4);
}

void genbeql_out(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[16]);
#endif
#ifdef INTERPRET_BEQL_OUT
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BEQL_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BEQL_OUT, 1);
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
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BEQL_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BEQL_IDLE, 1);
    return;
     }
   
   genbeq_test(state);
   gentest_idle(state);
   genbeql(state);
#endif
}

void genbnel(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[17]);
#endif
#ifdef INTERPRET_BNEL
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BNEL, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BNEL, 1);
    return;
     }
   
   genbne_test(state);
   free_all_registers(state);
   gentestl(state);
#endif
}

void genbnel_out(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[17]);
#endif
#ifdef INTERPRET_BNEL_OUT
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BNEL_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BNEL_OUT, 1);
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
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BNEL_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BNEL_IDLE, 1);
    return;
     }
   
   genbne_test(state);
   gentest_idle(state);
   genbnel(state);
#endif
}

void genblezl(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[18]);
#endif
#ifdef INTERPRET_BLEZL
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BLEZL, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BLEZL, 1);
    return;
     }
   
   genblez_test(state);
   free_all_registers(state);
   gentestl(state);
#endif
}

void genblezl_out(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[18]);
#endif
#ifdef INTERPRET_BLEZL_OUT
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BLEZL_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BLEZL_OUT, 1);
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
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BLEZL_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BLEZL_IDLE, 1);
    return;
     }
   
   genblez_test(state);
   gentest_idle(state);
   genblezl(state);
#endif
}

void genbgtzl(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[19]);
#endif
#ifdef INTERPRET_BGTZL
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BGTZL, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BGTZL, 1);
    return;
     }
   
   genbgtz_test(state);
   free_all_registers(state);
   gentestl(state);
#endif
}

void genbgtzl_out(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[19]);
#endif
#ifdef INTERPRET_BGTZL_OUT
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BGTZL_OUT, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BGTZL_OUT, 1);
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
   gencallinterp(state, (unsigned long long)state->current_instruction_table.BGTZL_IDLE, 1);
#else
   if (((state->dst->addr & 0xFFF) == 0xFFC &&
       (state->dst->addr < 0x80000000 || state->dst->addr >= 0xC0000000))||state->no_compiled_jump)
     {
    gencallinterp(state, (unsigned long long)state->current_instruction_table.BGTZL_IDLE, 1);
    return;
     }
   
   genbgtz_test(state);
   gentest_idle(state);
   genbgtzl(state);
#endif
}

void gendaddi(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[20]);
#endif
#ifdef INTERPRET_DADDI
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DADDI, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.i.rs);
   int rt = allocate_register_64_w(state, (unsigned long long *)state->dst->f.i.rt);

   mov_reg64_reg64(state, rt, rs);
   add_reg64_imm32(state, rt, (int) state->dst->f.i.immediate);
#endif
}

void gendaddiu(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[21]);
#endif
#ifdef INTERPRET_DADDIU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.DADDIU, 0);
#else
   int rs = allocate_register_64(state, (unsigned long long *)state->dst->f.i.rs);
   int rt = allocate_register_64_w(state, (unsigned long long *)state->dst->f.i.rt);

   mov_reg64_reg64(state, rt, rs);
   add_reg64_imm32(state, rt, (int) state->dst->f.i.immediate);
#endif
}

void genldl(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[22]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LDL, 0);
}

void genldr(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[23]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LDR, 0);
}

void genlb(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[24]);
#endif
   int gpr1, gpr2, base1, base2 = 0;
#ifdef INTERPRET_LB
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LB, 0);
#else
   free_registers_move_start(state);
    
   lock_register(state, RP0);
    
   ld_register_alloc(state, &gpr1, &gpr2, &base1, &base2);

   mov_reg64_imm64(state, base1, (unsigned long long) state->readmemb);
   if(state->fast_memory)
     {
    and_reg32_imm32(state, gpr1, 0xDF800000);
    cmp_reg32_imm32(state, gpr1, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, base2, (unsigned long long) read_rdramb);
    shr_reg32_imm8(state, gpr1, 16);
    mov_reg64_preg64x8preg64(state, gpr1, gpr1, base1);
    cmp_reg64_reg64(state, gpr1, base2);
     }
   je_rj(state, 0);
   jump_start_rel8(state);

   mov_reg64_imm64(state, gpr1, (unsigned long long) (state->dst+1));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), gpr1);
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), gpr2);
   mov_reg64_imm64(state, gpr1, (unsigned long long) state->dst->f.i.rt);
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->rdword), gpr1);
   shr_reg32_imm8(state, gpr2, 16);
   mov_reg64_preg64x8preg64(state, gpr2, gpr2, base1);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, gpr2);
   unlock_register(state, RP0);
   movsx_xreg32_m8rel(state, gpr1, (unsigned char *)state->dst->f.i.rt);
   jmp_imm_short(state, 24);

   jump_end_rel8(state);
   mov_reg64_imm64(state, base1, (unsigned long long) state->g_rdram); // 10
   and_reg32_imm32(state, gpr2, 0x7FFFFF); // 6
   xor_reg8_imm8(state, gpr2, 3); // 4
   movsx_reg32_8preg64preg64(state, gpr1, gpr2, base1); // 4

   set_register_state(state, gpr1, (unsigned int*)state->dst->f.i.rt, 1, 0);
#endif
}

void genlh(usf_state_t * state)
{
   int gpr1, gpr2, base1, base2 = 0;
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[25]);
#endif
#ifdef INTERPRET_LH
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LH, 0);
#else
   free_registers_move_start(state);
    
   lock_register(state, RP0);

   ld_register_alloc(state, &gpr1, &gpr2, &base1, &base2);

   mov_reg64_imm64(state, base1, (unsigned long long) state->readmemh);
   if(state->fast_memory)
     {
    and_reg32_imm32(state, gpr1, 0xDF800000);
    cmp_reg32_imm32(state, gpr1, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, base2, (unsigned long long) read_rdramh);
    shr_reg32_imm8(state, gpr1, 16);
    mov_reg64_preg64x8preg64(state, gpr1, gpr1, base1);
    cmp_reg64_reg64(state, gpr1, base2);
     }
   je_rj(state, 0);
   jump_start_rel8(state);
   
   mov_reg64_imm64(state, gpr1, (unsigned long long) (state->dst+1));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), gpr1);
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), gpr2);
   mov_reg64_imm64(state, gpr1, (unsigned long long) state->dst->f.i.rt);
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->rdword), gpr1);
   shr_reg32_imm8(state, gpr2, 16);
   mov_reg64_preg64x8preg64(state, gpr2, gpr2, base1);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, gpr2);
   unlock_register(state, RP0);
   movsx_xreg32_m16rel(state, gpr1, (unsigned short *)state->dst->f.i.rt);
   jmp_imm_short(state, 24);

   jump_end_rel8(state);
   mov_reg64_imm64(state, base1, (unsigned long long) state->g_rdram); // 10
   and_reg32_imm32(state, gpr2, 0x7FFFFF); // 6
   xor_reg8_imm8(state, gpr2, 2); // 4
   movsx_reg32_16preg64preg64(state, gpr1, gpr2, base1); // 4

   set_register_state(state, gpr1, (unsigned int*)state->dst->f.i.rt, 1, 0);
#endif
}

void genlwl(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[27]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LWL, 0);
}

void genlw(usf_state_t * state)
{
   int gpr1, gpr2, base1, base2 = 0;
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[26]);
#endif
#ifdef INTERPRET_LW
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LW, 0);
#else
   free_registers_move_start(state);

   lock_register(state, RP0);
    
   ld_register_alloc(state, &gpr1, &gpr2, &base1, &base2);

   mov_reg64_imm64(state, base1, (unsigned long long) state->readmem);
   if(state->fast_memory)
     {
    and_reg32_imm32(state, gpr1, 0xDF800000);
    cmp_reg32_imm32(state, gpr1, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, base2, (unsigned long long) read_rdram);
    shr_reg32_imm8(state, gpr1, 16);
    mov_reg64_preg64x8preg64(state, gpr1, gpr1, base1);
    cmp_reg64_reg64(state, gpr1, base2);
     }
   jne_rj(state, 21);

   mov_reg64_imm64(state, base1, (unsigned long long) state->g_rdram); // 10
   and_reg32_imm32(state, gpr2, 0x7FFFFF); // 6
   mov_reg32_preg64preg64(state, gpr1, gpr2, base1); // 3
   jmp_imm_short(state, 0); // 2
   jump_start_rel8(state);

   mov_reg64_imm64(state, gpr1, (unsigned long long) (state->dst+1));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), gpr1);
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), gpr2);
   mov_reg64_imm64(state, gpr1, (unsigned long long) state->dst->f.i.rt);
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->rdword), gpr1);
   shr_reg32_imm8(state, gpr2, 16);
   mov_reg64_preg64x8preg64(state, gpr1, gpr2, base1);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, gpr1);
   unlock_register(state, RP0);
   mov_xreg32_m32rel(state, gpr1, (unsigned int *)(state->dst->f.i.rt));

   jump_end_rel8(state);

   set_register_state(state, gpr1, (unsigned int*)state->dst->f.i.rt, 1, 0);     // set gpr1 state as dirty, and bound to r4300 reg RT
#endif
}

void genlbu(usf_state_t * state)
{
   int gpr1, gpr2, base1, base2 = 0;
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[28]);
#endif
#ifdef INTERPRET_LBU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LBU, 0);
#else
   free_registers_move_start(state);

   lock_register(state, RP0);

   ld_register_alloc(state, &gpr1, &gpr2, &base1, &base2);

   mov_reg64_imm64(state, base1, (unsigned long long) state->readmemb);
   if(state->fast_memory)
     {
    and_reg32_imm32(state, gpr1, 0xDF800000);
    cmp_reg32_imm32(state, gpr1, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, base2, (unsigned long long) read_rdramb);
    shr_reg32_imm8(state, gpr1, 16);
    mov_reg64_preg64x8preg64(state, gpr1, gpr1, base1);
    cmp_reg64_reg64(state, gpr1, base2);
     }
   je_rj(state, 0);
   jump_start_rel8(state);

   mov_reg64_imm64(state, gpr1, (unsigned long long) (state->dst+1));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), gpr1);
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), gpr2);
   mov_reg64_imm64(state, gpr1, (unsigned long long) state->dst->f.i.rt);
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->rdword), gpr1);
   shr_reg32_imm8(state, gpr2, 16);
   mov_reg64_preg64x8preg64(state, gpr2, gpr2, base1);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, gpr2);
   unlock_register(state, RP0);
   mov_xreg32_m32rel(state, gpr1, (unsigned int *)state->dst->f.i.rt);
   jmp_imm_short(state, 23);

   jump_end_rel8(state);
   mov_reg64_imm64(state, base1, (unsigned long long) state->g_rdram); // 10
   and_reg32_imm32(state, gpr2, 0x7FFFFF); // 6
   xor_reg8_imm8(state, gpr2, 3); // 4
   mov_reg32_preg64preg64(state, gpr1, gpr2, base1); // 3
   
   and_reg32_imm32(state, gpr1, 0xFF);
   set_register_state(state, gpr1, (unsigned int*)state->dst->f.i.rt, 1, 0);
#endif
}

void genlhu(usf_state_t * state)
{
   int gpr1, gpr2, base1, base2 = 0;
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[29]);
#endif
#ifdef INTERPRET_LHU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LHU, 0);
#else
   free_registers_move_start(state);

   lock_register(state, RP0);

   ld_register_alloc(state, &gpr1, &gpr2, &base1, &base2);

   mov_reg64_imm64(state, base1, (unsigned long long) state->readmemh);
   if(state->fast_memory)
     {
    and_reg32_imm32(state, gpr1, 0xDF800000);
    cmp_reg32_imm32(state, gpr1, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, base2, (unsigned long long) read_rdramh);
    shr_reg32_imm8(state, gpr1, 16);
    mov_reg64_preg64x8preg64(state, gpr1, gpr1, base1);
    cmp_reg64_reg64(state, gpr1, base2);
     }
   je_rj(state, 0);
   jump_start_rel8(state);

   mov_reg64_imm64(state, gpr1, (unsigned long long) (state->dst+1));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), gpr1);
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), gpr2);
   mov_reg64_imm64(state, gpr1, (unsigned long long) state->dst->f.i.rt);
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->rdword), gpr1);
   shr_reg32_imm8(state, gpr2, 16);
   mov_reg64_preg64x8preg64(state, gpr2, gpr2, base1);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, gpr2);
   mov_xreg32_m32rel(state, gpr1, (unsigned int *)state->dst->f.i.rt);
   jmp_imm_short(state, 23);

   jump_end_rel8(state);
   mov_reg64_imm64(state, base1, (unsigned long long) state->g_rdram); // 10
   and_reg32_imm32(state, gpr2, 0x7FFFFF); // 6
   xor_reg8_imm8(state, gpr2, 2); // 4
   mov_reg32_preg64preg64(state, gpr1, gpr2, base1); // 3

   and_reg32_imm32(state, gpr1, 0xFFFF);
   set_register_state(state, gpr1, (unsigned int*)state->dst->f.i.rt, 1, 0);
#endif
}

void genlwr(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[31]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LWR, 0);
}

void genlwu(usf_state_t * state)
{
   int gpr1, gpr2, base1, base2 = 0;
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[30]);
#endif
#ifdef INTERPRET_LWU
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LWU, 0);
#else
   free_registers_move_start(state);

   lock_register(state, RP0);

   ld_register_alloc(state, &gpr1, &gpr2, &base1, &base2);

   mov_reg64_imm64(state, base1, (unsigned long long) state->readmem);
   if(state->fast_memory)
     {
    and_reg32_imm32(state, gpr1, 0xDF800000);
    cmp_reg32_imm32(state, gpr1, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, base2, (unsigned long long) read_rdram);
    shr_reg32_imm8(state, gpr1, 16);
    mov_reg64_preg64x8preg64(state, gpr1, gpr1, base1);
    cmp_reg64_reg64(state, gpr1, base2);
     }
   je_rj(state, 0);
   jump_start_rel8(state);

   mov_reg64_imm64(state, gpr1, (unsigned long long) (state->dst+1));
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), gpr1);
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), gpr2);
   mov_reg64_imm64(state, gpr1, (unsigned long long) state->dst->f.i.rt);
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->rdword), gpr1);
   shr_reg32_imm8(state, gpr2, 16);
   mov_reg64_preg64x8preg64(state, gpr2, gpr2, base1);
   mov_reg64_reg64(state, RP0, 15);
   call_reg64(state, gpr2);
   unlock_register(state, RP0);
   mov_xreg32_m32rel(state, gpr1, (unsigned int *)state->dst->f.i.rt);
   jmp_imm_short(state, 19);

   jump_end_rel8(state);
   mov_reg64_imm64(state, base1, (unsigned long long) state->g_rdram); // 10
   and_reg32_imm32(state, gpr2, 0x7FFFFF); // 6
   mov_reg32_preg64preg64(state, gpr1, gpr2, base1); // 3

   set_register_state(state, gpr1, (unsigned int*)state->dst->f.i.rt, 1, 1);
#endif
}

void gensb(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[32]);
#endif
#ifdef INTERPRET_SB
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SB, 0);
#else
   free_registers_move_start(state);

   mov_xreg8_m8rel(state, CL, (unsigned char *)state->dst->f.i.rt);
   mov_xreg32_m32rel(state, EAX, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   mov_reg64_imm64(state, RSI, (unsigned long long) state->writememb);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, RDI, (unsigned long long) write_rdramb);
    shr_reg32_imm8(state, EAX, 16);
    mov_reg64_preg64x8preg64(state, RAX, RAX, RSI);
    cmp_reg64_reg64(state, RAX, RDI);
     }
   je_rj(state, 52);

   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1)); // 10
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), EBX); // 7
   mov_m8rel_xreg8(state, (unsigned char *)(&state->cpu_byte), CL); // 7
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg64_preg64x8preg64(state, RBX, RBX, RSI);  // 4
   mov_reg64_reg64(state, RP0, 15); // 3
   call_reg64(state, RBX); // 2
   mov_xreg32_m32rel(state, EAX, (unsigned int *)(&state->address)); // 7
   jmp_imm_short(state, 25); // 2

   mov_reg64_imm64(state, RSI, (unsigned long long) state->g_rdram); // 10
   mov_reg32_reg32(state, EAX, EBX); // 2
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(state, BL, 3); // 4
   mov_preg64preg64_reg8(state, RBX, RSI, CL); // 3
   
   mov_reg64_imm64(state, RSI, (unsigned long long) state->invalid_code);
   mov_reg32_reg32(state, EBX, EAX);
   shr_reg32_imm8(state, EBX, 12);
   cmp_preg64preg64_imm8(state, RBX, RSI, 0);
   jne_rj(state, 65);

   mov_reg64_imm64(state, RDI, (unsigned long long) state->blocks); // 10
   mov_reg32_reg32(state, ECX, EBX); // 2
   mov_reg64_preg64x8preg64(state, RBX, RBX, RDI);  // 4
   mov_reg64_preg64pimm32(state, RBX, RBX, (int) offsetof(precomp_block, block)); // 7
   mov_reg64_imm64(state, RDI, (unsigned long long) state->current_instruction_table.NOTCOMPILED); // 10
   and_eax_imm32(state, 0xFFF); // 5
   shr_reg32_imm8(state, EAX, 2); // 3
   mov_reg32_imm32(state, EDX, sizeof(precomp_instr)); // 5
   mul_reg32(state, EDX); // 2
   mov_reg64_preg64preg64pimm32(state, RAX, RAX, RBX, (int) offsetof(precomp_instr, ops)); // 8
   cmp_reg64_reg64(state, RAX, RDI); // 3
   je_rj(state, 4); // 2
   mov_preg64preg64_imm8(state, RCX, RSI, 1); // 4
#endif
}

void gensh(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[33]);
#endif
#ifdef INTERPRET_SH
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SH, 0);
#else
   free_registers_move_start(state);

   mov_xreg16_m16rel(state, CX, (unsigned short *)state->dst->f.i.rt);
   mov_xreg32_m32rel(state, EAX, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   mov_reg64_imm64(state, RSI, (unsigned long long) state->writememh);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, RDI, (unsigned long long) write_rdramh);
    shr_reg32_imm8(state, EAX, 16);
    mov_reg64_preg64x8preg64(state, RAX, RAX, RSI);
    cmp_reg64_reg64(state, RAX, RDI);
     }
   je_rj(state, 53);

   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1)); // 10
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), EBX); // 7
   mov_m16rel_xreg16(state, (unsigned short *)(&state->cpu_hword), CX); // 8
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg64_preg64x8preg64(state, RBX, RBX, RSI);  // 4
   mov_reg64_reg64(state, RP0, 15); // 3
   call_reg64(state, RBX); // 2
   mov_xreg32_m32rel(state, EAX, (unsigned int *)(&state->address)); // 7
   jmp_imm_short(state, 26); // 2

   mov_reg64_imm64(state, RSI, (unsigned long long) state->g_rdram); // 10
   mov_reg32_reg32(state, EAX, EBX); // 2
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   xor_reg8_imm8(state, BL, 2); // 4
   mov_preg64preg64_reg16(state, RBX, RSI, CX); // 4

   mov_reg64_imm64(state, RSI, (unsigned long long) state->invalid_code);
   mov_reg32_reg32(state, EBX, EAX);
   shr_reg32_imm8(state, EBX, 12);
   cmp_preg64preg64_imm8(state, RBX, RSI, 0);
   jne_rj(state, 65);

   mov_reg64_imm64(state, RDI, (unsigned long long) state->blocks); // 10
   mov_reg32_reg32(state, ECX, EBX); // 2
   mov_reg64_preg64x8preg64(state, RBX, RBX, RDI);  // 4
   mov_reg64_preg64pimm32(state, RBX, RBX, (int) offsetof(precomp_block, block)); // 7
   mov_reg64_imm64(state, RDI, (unsigned long long) state->current_instruction_table.NOTCOMPILED); // 10
   and_eax_imm32(state, 0xFFF); // 5
   shr_reg32_imm8(state, EAX, 2); // 3
   mov_reg32_imm32(state, EDX, sizeof(precomp_instr)); // 5
   mul_reg32(state, EDX); // 2
   mov_reg64_preg64preg64pimm32(state, RAX, RAX, RBX, (int) offsetof(precomp_instr, ops)); // 8
   cmp_reg64_reg64(state, RAX, RDI); // 3
   je_rj(state, 4); // 2
   mov_preg64preg64_imm8(state, RCX, RSI, 1); // 4
#endif
}

void genswl(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[35]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SWL, 0);
}

void gensw(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[34]);
#endif
#ifdef INTERPRET_SW
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SW, 0);
#else
   free_registers_move_start(state);

   mov_xreg32_m32rel(state, ECX, (unsigned int *)state->dst->f.i.rt);
   mov_xreg32_m32rel(state, EAX, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   mov_reg64_imm64(state, RSI, (unsigned long long) state->writemem);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, RDI, (unsigned long long) write_rdram);
    shr_reg32_imm8(state, EAX, 16);
    mov_reg64_preg64x8preg64(state, RAX, RAX, RSI);
    cmp_reg64_reg64(state, RAX, RDI);
     }
   je_rj(state, 52);

   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1)); // 10
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), EBX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->cpu_word), ECX); // 7
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg64_preg64x8preg64(state, RBX, RBX, RSI);  // 4
   mov_reg64_reg64(state, RP0, 15); // 3
   call_reg64(state, RBX); // 2
   mov_xreg32_m32rel(state, EAX, (unsigned int *)(&state->address)); // 7
   jmp_imm_short(state, 21); // 2

   mov_reg64_imm64(state, RSI, (unsigned long long) state->g_rdram); // 10
   mov_reg32_reg32(state, EAX, EBX); // 2
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_preg64preg64_reg32(state, RBX, RSI, ECX); // 3

   mov_reg64_imm64(state, RSI, (unsigned long long) state->invalid_code);
   mov_reg32_reg32(state, EBX, EAX);
   shr_reg32_imm8(state, EBX, 12);
   cmp_preg64preg64_imm8(state, RBX, RSI, 0);
   jne_rj(state, 65);

   mov_reg64_imm64(state, RDI, (unsigned long long) state->blocks); // 10
   mov_reg32_reg32(state, ECX, EBX); // 2
   mov_reg64_preg64x8preg64(state, RBX, RBX, RDI);  // 4
   mov_reg64_preg64pimm32(state, RBX, RBX, (int) offsetof(precomp_block, block)); // 7
   mov_reg64_imm64(state, RDI, (unsigned long long) state->current_instruction_table.NOTCOMPILED); // 10
   and_eax_imm32(state, 0xFFF); // 5
   shr_reg32_imm8(state, EAX, 2); // 3
   mov_reg32_imm32(state, EDX, sizeof(precomp_instr)); // 5
   mul_reg32(state, EDX); // 2
   mov_reg64_preg64preg64pimm32(state, RAX, RAX, RBX, (int) offsetof(precomp_instr, ops)); // 8
   cmp_reg64_reg64(state, RAX, RDI); // 3
   je_rj(state, 4); // 2
   mov_preg64preg64_imm8(state, RCX, RSI, 1); // 4
#endif
}

void gensdl(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[37]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SDL, 0);
}

void gensdr(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[38]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SDR, 0);
}

void genswr(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[36]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SWR, 0);
}

void gencheck_cop1_unusable(usf_state_t * state)
{
   free_registers_move_start(state);

   test_m32rel_imm32(state, (unsigned int*)&state->g_cp0_regs[CP0_STATUS_REG], 0x20000000);
   jne_rj(state, 0);
   jump_start_rel8(state);

   gencallinterp(state, (unsigned long long)check_cop1_unusable, 0);

   jump_end_rel8(state);
}

void genlwc1(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(&instr_count[39]);
#endif
#ifdef INTERPRET_LWC1
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LWC1, 0);
#else
   gencheck_cop1_unusable(state);
   
   mov_xreg32_m32rel(state, EAX, (unsigned int *)(&state->reg[state->dst->f.lf.base]));
   add_eax_imm32(state, (int)state->dst->f.lf.offset);
   mov_reg32_reg32(state, EBX, EAX);
   mov_reg64_imm64(state, RSI, (unsigned long long) state->readmem);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, RDI, (unsigned long long) read_rdram);
    shr_reg32_imm8(state, EAX, 16);
    mov_reg64_preg64x8preg64(state, RAX, RAX, RSI);
    cmp_reg64_reg64(state, RAX, RDI);
     }
   je_rj(state, 52);

   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1)); // 10
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), EBX); // 7
   mov_xreg64_m64rel(state, RDX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.lf.ft])); // 7
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->rdword), RDX); // 7
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg64_preg64x8preg64(state, RBX, RBX, RSI);  // 4
   mov_reg64_reg64(state, RP0, 15); // 3
   call_reg64(state, RBX); // 2
   jmp_imm_short(state, 28); // 2

   mov_reg64_imm64(state, RSI, (unsigned long long) state->g_rdram); // 10
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_reg32_preg64preg64(state, EAX, RBX, RSI); // 3
   mov_xreg64_m64rel(state, RBX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.lf.ft])); // 7
   mov_preg64_reg32(state, RBX, EAX); // 2
#endif
}

void genldc1(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[40]);
#endif
#ifdef INTERPRET_LDC1
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LDC1, 0);
#else
   gencheck_cop1_unusable(state);
   
   mov_xreg32_m32rel(state, EAX, (unsigned int *)(&state->reg[state->dst->f.lf.base]));
   add_eax_imm32(state, (int)state->dst->f.lf.offset);
   mov_reg32_reg32(state, EBX, EAX);
   mov_reg64_imm64(state, RSI, (unsigned long long) state->readmemd);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, RDI, (unsigned long long) read_rdramd);
    shr_reg32_imm8(state, EAX, 16);
    mov_reg64_preg64x8preg64(state, RAX, RAX, RSI);
    cmp_reg64_reg64(state, RAX, RDI);
     }
   je_rj(state, 52);
   
   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1)); // 10
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), EBX); // 7
   mov_xreg64_m64rel(state, RDX, (unsigned long long *)(&state->reg_cop1_double[state->dst->f.lf.ft])); // 7
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->rdword), RDX); // 7
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg64_preg64x8preg64(state, RBX, RBX, RSI);  // 4
   mov_reg64_reg64(state, RP0, 15); // 3
   call_reg64(state, RBX); // 2
   jmp_imm_short(state, 39); // 2

   mov_reg64_imm64(state, RSI, (unsigned long long) state->g_rdram); // 10
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_reg64_preg64preg64(state, RAX, RBX, RSI); // 4
   mov_xreg64_m64rel(state, RBX, (unsigned long long *)(&state->reg_cop1_double[state->dst->f.lf.ft])); // 7
   mov_preg64pimm32_reg32(state, RBX, 4, EAX); // 6
   shr_reg64_imm8(state, RAX, 32); // 4
   mov_preg64_reg32(state, RBX, EAX); // 2
#endif
}

void gencache(usf_state_t * state)
{
    (void)state;
}

void genld(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[41]);
#endif
#ifdef INTERPRET_LD
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LD, 0);
#else
   free_registers_move_start(state);

   mov_xreg32_m32rel(state, EAX, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   mov_reg64_imm64(state, RSI, (unsigned long long) state->readmemd);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, RDI, (unsigned long long) read_rdramd);
    shr_reg32_imm8(state, EAX, 16);
    mov_reg64_preg64x8preg64(state, RAX, RAX, RSI);
    cmp_reg64_reg64(state, RAX, RDI);
     }
   je_rj(state, 62);

   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1)); // 10
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), EBX); // 7
   mov_reg64_imm64(state, RAX, (unsigned long long) state->dst->f.i.rt); // 10
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->rdword), RAX); // 7
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg64_preg64x8preg64(state, RBX, RBX, RSI);  // 4
   mov_reg64_reg64(state, RP0, 15); // 3
   call_reg64(state, RBX); // 2
   mov_xreg64_m64rel(state, RAX, (unsigned long long *)(state->dst->f.i.rt)); // 7
   jmp_imm_short(state, 33); // 2

   mov_reg64_imm64(state, RSI, (unsigned long long) state->g_rdram); // 10
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6

   mov_reg32_preg64preg64(state, EAX, RBX, RSI); // 3
   mov_reg32_preg64preg64pimm32(state, EBX, RBX, RSI, 4); // 7
   shl_reg64_imm8(state, RAX, 32); // 4
   or_reg64_reg64(state, RAX, RBX); // 3
   
   set_register_state(state, RAX, (unsigned int*)state->dst->f.i.rt, 1, 1);
#endif
}

void genswc1(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[43]);
#endif
#ifdef INTERPRET_SWC1
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SWC1, 0);
#else
   gencheck_cop1_unusable(state);

   mov_xreg64_m64rel(state, RDX, (unsigned long long *)(&state->reg_cop1_simple[state->dst->f.lf.ft]));
   mov_reg32_preg64(state, ECX, RDX);
   mov_xreg32_m32rel(state, EAX, (unsigned int *)(&state->reg[state->dst->f.lf.base]));
   add_eax_imm32(state, (int)state->dst->f.lf.offset);
   mov_reg32_reg32(state, EBX, EAX);
   mov_reg64_imm64(state, RSI, (unsigned long long) state->writemem);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, RDI, (unsigned long long) write_rdram);
    shr_reg32_imm8(state, EAX, 16);
    mov_reg64_preg64x8preg64(state, RAX, RAX, RSI);
    cmp_reg64_reg64(state, RAX, RDI);
     }
   je_rj(state, 52);

   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1)); // 10
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), EBX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->cpu_word), ECX); // 7
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg64_preg64x8preg64(state, RBX, RBX, RSI);  // 4
   mov_reg64_reg64(state, RP0, 15); // 3
   call_reg64(state, RBX); // 2
   mov_xreg32_m32rel(state, EAX, (unsigned int *)(&state->address)); // 7
   jmp_imm_short(state, 21); // 2

   mov_reg64_imm64(state, RSI, (unsigned long long) state->g_rdram); // 10
   mov_reg32_reg32(state, EAX, EBX); // 2
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_preg64preg64_reg32(state, RBX, RSI, ECX); // 3
   
   mov_reg64_imm64(state, RSI, (unsigned long long) state->invalid_code);
   mov_reg32_reg32(state, EBX, EAX);
   shr_reg32_imm8(state, EBX, 12);
   cmp_preg64preg64_imm8(state, RBX, RSI, 0);
   jne_rj(state, 65);

   mov_reg64_imm64(state, RDI, (unsigned long long) state->blocks); // 10
   mov_reg32_reg32(state, ECX, EBX); // 2
   mov_reg64_preg64x8preg64(state, RBX, RBX, RDI);  // 4
   mov_reg64_preg64pimm32(state, RBX, RBX, (int) offsetof(precomp_block, block)); // 7
   mov_reg64_imm64(state, RDI, (unsigned long long) state->current_instruction_table.NOTCOMPILED); // 10
   and_eax_imm32(state, 0xFFF); // 5
   shr_reg32_imm8(state, EAX, 2); // 3
   mov_reg32_imm32(state, EDX, sizeof(precomp_instr)); // 5
   mul_reg32(state, EDX); // 2
   mov_reg64_preg64preg64pimm32(state, RAX, RAX, RBX, (int) offsetof(precomp_instr, ops)); // 8
   cmp_reg64_reg64(state, RAX, RDI); // 3
   je_rj(state, 4); // 2
   mov_preg64preg64_imm8(state, RCX, RSI, 1); // 4
#endif
}

void gensdc1(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[44]);
#endif
#ifdef INTERPRET_SDC1
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SDC1, 0);
#else
   gencheck_cop1_unusable(state);

   mov_xreg64_m64rel(state, RSI, (unsigned long long *)(&state->reg_cop1_double[state->dst->f.lf.ft]));
   mov_reg32_preg64(state, ECX, RSI);
   mov_reg32_preg64pimm32(state, EDX, RSI, 4);
   mov_xreg32_m32rel(state, EAX, (unsigned int *)(&state->reg[state->dst->f.lf.base]));
   add_eax_imm32(state, (int)state->dst->f.lf.offset);
   mov_reg32_reg32(state, EBX, EAX);
   mov_reg64_imm64(state, RSI, (unsigned long long) state->writememd);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, RDI, (unsigned long long) write_rdramd);
    shr_reg32_imm8(state, EAX, 16);
    mov_reg64_preg64x8preg64(state, RAX, RAX, RSI);
    cmp_reg64_reg64(state, RAX, RDI);
     }
   je_rj(state, 59);

   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1)); // 10
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), EBX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->cpu_dword), ECX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->cpu_dword)+1, EDX); // 7
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg64_preg64x8preg64(state, RBX, RBX, RSI);  // 4
   mov_reg64_reg64(state, RP0, 15); // 3
   call_reg64(state, RBX); // 2
   mov_xreg32_m32rel(state, EAX, (unsigned int *)(&state->address)); // 7
   jmp_imm_short(state, 28); // 2

   mov_reg64_imm64(state, RSI, (unsigned long long) state->g_rdram); // 10
   mov_reg32_reg32(state, EAX, EBX); // 2
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_preg64preg64pimm32_reg32(state, RBX, RSI, 4, ECX); // 7
   mov_preg64preg64_reg32(state, RBX, RSI, EDX); // 3

   mov_reg64_imm64(state, RSI, (unsigned long long) state->invalid_code);
   mov_reg32_reg32(state, EBX, EAX);
   shr_reg32_imm8(state, EBX, 12);
   cmp_preg64preg64_imm8(state, RBX, RSI, 0);
   jne_rj(state, 65);

   mov_reg64_imm64(state, RDI, (unsigned long long) state->blocks); // 10
   mov_reg32_reg32(state, ECX, EBX); // 2
   mov_reg64_preg64x8preg64(state, RBX, RBX, RDI);  // 4
   mov_reg64_preg64pimm32(state, RBX, RBX, (int) offsetof(precomp_block, block)); // 7
   mov_reg64_imm64(state, RDI, (unsigned long long) state->current_instruction_table.NOTCOMPILED); // 10
   and_eax_imm32(state, 0xFFF); // 5
   shr_reg32_imm8(state, EAX, 2); // 3
   mov_reg32_imm32(state, EDX, sizeof(precomp_instr)); // 5
   mul_reg32(state, EDX); // 2
   mov_reg64_preg64preg64pimm32(state, RAX, RAX, RBX, (int) offsetof(precomp_instr, ops)); // 8
   cmp_reg64_reg64(state, RAX, RDI); // 3
   je_rj(state, 4); // 2
   mov_preg64preg64_imm8(state, RCX, RSI, 1); // 4
#endif
}

void gensd(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[45]);
#endif
#ifdef INTERPRET_SD
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SD, 0);
#else
   free_registers_move_start(state);

   mov_xreg32_m32rel(state, ECX, (unsigned int *)state->dst->f.i.rt);
   mov_xreg32_m32rel(state, EDX, ((unsigned int *)state->dst->f.i.rt)+1);
   mov_xreg32_m32rel(state, EAX, (unsigned int *)state->dst->f.i.rs);
   add_eax_imm32(state, (int)state->dst->f.i.immediate);
   mov_reg32_reg32(state, EBX, EAX);
   mov_reg64_imm64(state, RSI, (unsigned long long) state->writememd);
   if(state->fast_memory)
     {
    and_eax_imm32(state, 0xDF800000);
    cmp_eax_imm32(state, 0x80000000);
     }
   else
     {
    mov_reg64_imm64(state, RDI, (unsigned long long) write_rdramd);
    shr_reg32_imm8(state, EAX, 16);
    mov_reg64_preg64x8preg64(state, RAX, RAX, RSI);
    cmp_reg64_reg64(state, RAX, RDI);
     }
   je_rj(state, 59);

   mov_reg64_imm64(state, RAX, (unsigned long long) (state->dst+1)); // 10
   mov_m64rel_xreg64(state, (unsigned long long *)(&state->PC), RAX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->address), EBX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->cpu_dword), ECX); // 7
   mov_m32rel_xreg32(state, (unsigned int *)(&state->cpu_dword)+1, EDX); // 7
   shr_reg32_imm8(state, EBX, 16); // 3
   mov_reg64_preg64x8preg64(state, RBX, RBX, RSI);  // 4
   mov_reg64_reg64(state, RP0, 15); // 3
   call_reg64(state, RBX); // 2
   mov_xreg32_m32rel(state, EAX, (unsigned int *)(&state->address)); // 7
   jmp_imm_short(state, 28); // 2

   mov_reg64_imm64(state, RSI, (unsigned long long) state->g_rdram); // 10
   mov_reg32_reg32(state, EAX, EBX); // 2
   and_reg32_imm32(state, EBX, 0x7FFFFF); // 6
   mov_preg64preg64pimm32_reg32(state, RBX, RSI, 4, ECX); // 7
   mov_preg64preg64_reg32(state, RBX, RSI, EDX); // 3

   mov_reg64_imm64(state, RSI, (unsigned long long) state->invalid_code);
   mov_reg32_reg32(state, EBX, EAX);
   shr_reg32_imm8(state, EBX, 12);
   cmp_preg64preg64_imm8(state, RBX, RSI, 0);
   jne_rj(state, 65);

   mov_reg64_imm64(state, RDI, (unsigned long long) state->blocks); // 10
   mov_reg32_reg32(state, ECX, EBX); // 2
   mov_reg64_preg64x8preg64(state, RBX, RBX, RDI);  // 4
   mov_reg64_preg64pimm32(state, RBX, RBX, (int) offsetof(precomp_block, block)); // 7
   mov_reg64_imm64(state, RDI, (unsigned long long) state->current_instruction_table.NOTCOMPILED); // 10
   and_eax_imm32(state, 0xFFF); // 5
   shr_reg32_imm8(state, EAX, 2); // 3
   mov_reg32_imm32(state, EDX, sizeof(precomp_instr)); // 5
   mul_reg32(state, EDX); // 2
   mov_reg64_preg64preg64pimm32(state, RAX, RAX, RBX, (int) offsetof(precomp_instr, ops)); // 8
   cmp_reg64_reg64(state, RAX, RDI); // 3
   je_rj(state, 4); // 2
   mov_preg64preg64_imm8(state, RCX, RSI, 1); // 4
#endif
}

void genll(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[42]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.LL, 0);
}

void gensc(usf_state_t * state)
{
#if defined(COUNT_INSTR)
   inc_m32rel(state, &state->instr_count[46]);
#endif
   gencallinterp(state, (unsigned long long)state->current_instruction_table.SC, 0);
}
#endif
