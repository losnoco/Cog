/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - cached_interp.c                                         *
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

#include "cached_interp.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "main/main.h"
#include "memory/memory.h"

#include "r4300.h"
#include "cp0.h"
#include "cp1.h"
#include "ops.h"
#include "exception.h"
#include "interupt.h"
#include "macros.h"
#include "recomp.h"
#include "tlb.h"

// -----------------------------------------------------------
// Cached interpreter functions (and fallback for dynarec).
// -----------------------------------------------------------
#define UPDATE_DEBUGGER() do { } while(0)

#define PCADDR state->PC->addr
#define ADD_TO_PC(x) state->PC += x;
#define DECLARE_INSTRUCTION(name) static void osal_fastcall name(usf_state_t * state)

#define DECLARE_JUMP(name, destination, condition, link, likely, cop1) \
   static void osal_fastcall name(usf_state_t * state) \
   { \
      const int take_jump = (condition); \
      const unsigned int jump_target = (destination); \
      long long int *link_register = (link); \
      if (cop1 && check_cop1_unusable(state)) return; \
      if (link_register != &state->reg[0]) \
      { \
         *link_register=state->PC->addr + 8; \
         sign_extended(*link_register); \
      } \
      if (!likely || take_jump) \
      { \
         state->PC++; \
         state->delay_slot=1; \
         UPDATE_DEBUGGER(); \
         state->PC->ops(state); \
         update_count(state); \
         state->delay_slot=0; \
         if (take_jump && !state->skip_jump) \
         { \
            state->PC=state->actual->block+((jump_target-state->actual->start)>>2); \
         } \
      } \
      else \
      { \
         state->PC += 2; \
         update_count(state); \
      } \
      state->last_addr = state->PC->addr; \
      if (state->cycle_count >=0) gen_interupt(state); \
   } \
   static void osal_fastcall name##_OUT(usf_state_t * state) \
   { \
      const int take_jump = (condition); \
      const unsigned int jump_target = (destination); \
      long long int *link_register = (link); \
      if (cop1 && check_cop1_unusable(state)) return; \
      if (link_register != &state->reg[0]) \
      { \
         *link_register=state->PC->addr + 8; \
         sign_extended(*link_register); \
      } \
      if (!likely || take_jump) \
      { \
         state->PC++; \
         state->delay_slot=1; \
         UPDATE_DEBUGGER(); \
         state->PC->ops(state); \
         update_count(state); \
         state->delay_slot=0; \
         if (take_jump && !state->skip_jump) \
         { \
            jump_to(jump_target); \
         } \
      } \
      else \
      { \
         state->PC += 2; \
         update_count(state); \
      } \
      state->last_addr = state->PC->addr; \
      if (state->cycle_count >=0) gen_interupt(state); \
   } \
   static void osal_fastcall name##_IDLE(usf_state_t * state) \
   { \
      const int take_jump = (condition); \
      if (cop1 && check_cop1_unusable(state)) return; \
      if (take_jump) \
      { \
         if (state->cycle_count < 0) \
         { \
            state->g_cp0_regs[CP0_COUNT_REG] -= state->cycle_count; \
            state->cycle_count = 0; \
         } \
      } \
      name(state); \
   }

#define CHECK_MEMORY() \
   if (!state->invalid_code[state->address>>12]) \
      if (state->blocks[state->address>>12]->block[(state->address&0xFFF)/4].ops != \
          state->current_instruction_table.NOTCOMPILED) \
         state->invalid_code[state->address>>12] = 1;

// two functions are defined from the macros above but never used
// these prototype declarations will prevent a warning
#if defined(__GNUC__)
  static void osal_fastcall JR_IDLE(usf_state_t *) __attribute__((used));
  static void osal_fastcall JALR_IDLE(usf_state_t *) __attribute__((used));
#endif

#include "interpreter.def"

// -----------------------------------------------------------
// Flow control 'fake' instructions
// -----------------------------------------------------------
static void osal_fastcall FIN_BLOCK(usf_state_t * state)
{
   if (!state->delay_slot)
     {
    jump_to((state->PC-1)->addr+4);
/*#ifdef DBG
            if (g_DebuggerActive) update_debugger(PC->addr);
#endif
Used by dynarec only, check should be unnecessary
*/
    state->PC->ops(state);
#ifdef DYNAREC
    if (state->r4300emu == CORE_DYNAREC) dyna_jump(state);
#endif
     }
   else
     {
    precomp_block *blk = state->actual;
    precomp_instr *inst = state->PC;
    jump_to((state->PC-1)->addr+4);
    
/*#ifdef DBG
            if (g_DebuggerActive) update_debugger(PC->addr);
#endif
Used by dynarec only, check should be unnecessary
*/
    if (!state->skip_jump)
      {
         state->PC->ops(state);
         state->actual = blk;
         state->PC = inst+1;
      }
    else
      state->PC->ops(state);
    
#ifdef DYNAREC
    if (state->r4300emu == CORE_DYNAREC) dyna_jump(state);
#endif
     }
}

static void osal_fastcall NOTCOMPILED(usf_state_t * state)
{
   unsigned int *mem = fast_mem_access(state, state->blocks[state->PC->addr>>12]->start);

   if (mem != NULL)
      recompile_block(state, (int *)mem, state->blocks[state->PC->addr >> 12], state->PC->addr);
   else
      DebugMessage(state, M64MSG_ERROR, "not compiled exception");

/*#ifdef DBG
            if (g_DebuggerActive) update_debugger(PC->addr);
#endif
The preceeding update_debugger SHOULD be unnecessary since it should have been
called before NOTCOMPILED would have been executed
*/
   state->PC->ops(state);
#ifdef DYNAREC
   if (state->r4300emu == CORE_DYNAREC)
     dyna_jump(state);
#endif
}

static void osal_fastcall NOTCOMPILED2(usf_state_t * state)
{
   NOTCOMPILED(state);
}

// -----------------------------------------------------------
// Cached interpreter instruction table
// -----------------------------------------------------------
const cpu_instruction_table cached_interpreter_table = {
   LB,
   LBU,
   LH,
   LHU,
   LW,
   LWL,
   LWR,
   SB,
   SH,
   SW,
   SWL,
   SWR,

   LD,
   LDL,
   LDR,
   LL,
   LWU,
   SC,
   SD,
   SDL,
   SDR,
   SYNC,

   ADDI,
   ADDIU,
   SLTI,
   SLTIU,
   ANDI,
   ORI,
   XORI,
   LUI,

   DADDI,
   DADDIU,

   ADD,
   ADDU,
   SUB,
   SUBU,
   SLT,
   SLTU,
   AND,
   OR,
   XOR,
   NOR,

   DADD,
   DADDU,
   DSUB,
   DSUBU,

   MULT,
   MULTU,
   DIV,
   DIVU,
   MFHI,
   MTHI,
   MFLO,
   MTLO,

   DMULT,
   DMULTU,
   DDIV,
   DDIVU,

   J,
   J_OUT,
   J_IDLE,
   JAL,
   JAL_OUT,
   JAL_IDLE,
   // Use the _OUT versions of JR and JALR, since we don't know
   // until runtime if they're going to jump inside or outside the block
   JR_OUT,
   JALR_OUT,
   BEQ,
   BEQ_OUT,
   BEQ_IDLE,
   BNE,
   BNE_OUT,
   BNE_IDLE,
   BLEZ,
   BLEZ_OUT,
   BLEZ_IDLE,
   BGTZ,
   BGTZ_OUT,
   BGTZ_IDLE,
   BLTZ,
   BLTZ_OUT,
   BLTZ_IDLE,
   BGEZ,
   BGEZ_OUT,
   BGEZ_IDLE,
   BLTZAL,
   BLTZAL_OUT,
   BLTZAL_IDLE,
   BGEZAL,
   BGEZAL_OUT,
   BGEZAL_IDLE,

   BEQL,
   BEQL_OUT,
   BEQL_IDLE,
   BNEL,
   BNEL_OUT,
   BNEL_IDLE,
   BLEZL,
   BLEZL_OUT,
   BLEZL_IDLE,
   BGTZL,
   BGTZL_OUT,
   BGTZL_IDLE,
   BLTZL,
   BLTZL_OUT,
   BLTZL_IDLE,
   BGEZL,
   BGEZL_OUT,
   BGEZL_IDLE,
   BLTZALL,
   BLTZALL_OUT,
   BLTZALL_IDLE,
   BGEZALL,
   BGEZALL_OUT,
   BGEZALL_IDLE,
   BC1TL,
   BC1TL_OUT,
   BC1TL_IDLE,
   BC1FL,
   BC1FL_OUT,
   BC1FL_IDLE,

   SLL,
   SRL,
   SRA,
   SLLV,
   SRLV,
   SRAV,

   DSLL,
   DSRL,
   DSRA,
   DSLLV,
   DSRLV,
   DSRAV,
   DSLL32,
   DSRL32,
   DSRA32,

   MTC0,
   MFC0,

   TLBR,
   TLBWI,
   TLBWR,
   TLBP,
   CACHE,
   ERET,

   LWC1,
   SWC1,
   MTC1,
   MFC1,
   CTC1,
   CFC1,
   BC1T,
   BC1T_OUT,
   BC1T_IDLE,
   BC1F,
   BC1F_OUT,
   BC1F_IDLE,

   DMFC1,
   DMTC1,
   LDC1,
   SDC1,

   CVT_S_D,
   CVT_S_W,
   CVT_S_L,
   CVT_D_S,
   CVT_D_W,
   CVT_D_L,
   CVT_W_S,
   CVT_W_D,
   CVT_L_S,
   CVT_L_D,

   ROUND_W_S,
   ROUND_W_D,
   ROUND_L_S,
   ROUND_L_D,

   TRUNC_W_S,
   TRUNC_W_D,
   TRUNC_L_S,
   TRUNC_L_D,

   CEIL_W_S,
   CEIL_W_D,
   CEIL_L_S,
   CEIL_L_D,

   FLOOR_W_S,
   FLOOR_W_D,
   FLOOR_L_S,
   FLOOR_L_D,

   ADD_S,
   ADD_D,

   SUB_S,
   SUB_D,

   MUL_S,
   MUL_D,

   DIV_S,
   DIV_D,
   
   ABS_S,
   ABS_D,

   MOV_S,
   MOV_D,

   NEG_S,
   NEG_D,

   SQRT_S,
   SQRT_D,

   C_F_S,
   C_F_D,
   C_UN_S,
   C_UN_D,
   C_EQ_S,
   C_EQ_D,
   C_UEQ_S,
   C_UEQ_D,
   C_OLT_S,
   C_OLT_D,
   C_ULT_S,
   C_ULT_D,
   C_OLE_S,
   C_OLE_D,
   C_ULE_S,
   C_ULE_D,
   C_SF_S,
   C_SF_D,
   C_NGLE_S,
   C_NGLE_D,
   C_SEQ_S,
   C_SEQ_D,
   C_NGL_S,
   C_NGL_D,
   C_LT_S,
   C_LT_D,
   C_NGE_S,
   C_NGE_D,
   C_LE_S,
   C_LE_D,
   C_NGT_S,
   C_NGT_D,

   SYSCALL,
   BREAK,

   TEQ,

   NOP,
   RESERVED,
   NI,

   FIN_BLOCK,
   NOTCOMPILED,
   NOTCOMPILED2
};

static unsigned int osal_fastcall update_invalid_addr(usf_state_t * state, unsigned int addr)
{
   if (addr >= 0x80000000 && addr < 0xc0000000)
     {
    if (state->invalid_code[addr>>12]) state->invalid_code[(addr^0x20000000)>>12] = 1;
    if (state->invalid_code[(addr^0x20000000)>>12]) state->invalid_code[addr>>12] = 1;
    return addr;
     }
   else
     {
    unsigned int paddr = virtual_to_physical_address(state, addr, 2);
    if (paddr)
      {
         unsigned int beg_paddr = paddr - (addr - (addr&~0xFFF));
         update_invalid_addr(state, paddr);
         if (state->invalid_code[(beg_paddr+0x000)>>12]) state->invalid_code[addr>>12] = 1;
         if (state->invalid_code[(beg_paddr+0xFFC)>>12]) state->invalid_code[addr>>12] = 1;
         if (state->invalid_code[addr>>12]) state->invalid_code[(beg_paddr+0x000)>>12] = 1;
         if (state->invalid_code[addr>>12]) state->invalid_code[(beg_paddr+0xFFC)>>12] = 1;
      }
    return paddr;
     }
}

#define addr state->jump_to_address
void osal_fastcall jump_to_func(usf_state_t * state)
{
   unsigned int paddr;
   if (state->skip_jump) return;
   paddr = update_invalid_addr(state, addr);
   if (!paddr) return;
   state->actual = state->blocks[addr>>12];
   if (state->invalid_code[addr>>12])
     {
    if (!state->blocks[addr>>12])
      {
         state->blocks[addr>>12] = (precomp_block *) malloc(sizeof(precomp_block));
         state->actual = state->blocks[addr>>12];
         state->blocks[addr>>12]->code = NULL;
         state->blocks[addr>>12]->block = NULL;
         state->blocks[addr>>12]->jumps_table = NULL;
         state->blocks[addr>>12]->riprel_table = NULL;
      }
    state->blocks[addr>>12]->start = addr & ~0xFFF;
    state->blocks[addr>>12]->end = (addr & ~0xFFF) + 0x1000;
    init_block(state, state->blocks[addr>>12]);
     }
   state->PC=state->actual->block+((addr-state->actual->start)>>2);
   
#ifdef DYNAREC
   if (state->r4300emu == CORE_DYNAREC) dyna_jump(state);
#endif
}
#undef addr

void osal_fastcall init_blocks(usf_state_t * state)
{
   int i;
   for (i=0; i<0x100000; i++)
   {
      state->invalid_code[i] = 1;
      state->blocks[i] = NULL;
   }
}

void osal_fastcall free_blocks(usf_state_t * state)
{
   int i;
   for (i=0; i<0x100000; i++)
   {
        if (state->blocks[i])
        {
            free_block(state, state->blocks[i]);
            free(state->blocks[i]);
            state->blocks[i] = NULL;
        }
    }
}

