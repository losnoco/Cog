/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - regcache.c                                              *
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

#include "regcache.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "r4300/recomp.h"
#include "r4300/r4300.h"
#include "r4300/recomph.h"

void init_cache(usf_state_t * state, precomp_instr* start)
{
  int i;
  for (i=0; i<8; i++)
  {
    state->reg_content[i] = NULL;
    state->last_access[i] = NULL;
    state->free_since[i] = start;
    state->dirty[i] = 0;
    state->is64bits[i] = 0;
  }
  state->r0 = (unsigned long long *) state->reg;
}

void free_all_registers(usf_state_t * state)
{
  int i;
  for (i=0; i<8; i++)
  {
    if (state->last_access[i])
    {
      free_register(state, i);
    }
    else
    {
      while (state->free_since[i] <= state->dst)
      {
        state->free_since[i]->reg_cache_infos.needed_registers[i] = NULL;
        state->free_since[i]++;
      }
    }
  }
}

static void simplify_access(usf_state_t * state)
{
   int i;
   state->dst->local_addr = state->code_length;
   for(i=0; i<8; i++) state->dst->reg_cache_infos.needed_registers[i] = NULL;
}

void free_registers_move_start(usf_state_t * state)
{
  /* flush all dirty registers and clear needed_registers table */
  free_all_registers(state);

  /* now move the start of the new instruction down past the flushing instructions */
  simplify_access(state);
}

// this function frees a specific X86 GPR
void free_register(usf_state_t * state, int reg)
{
  precomp_instr *last;
  
  if (state->last_access[reg] == (precomp_instr *) 0xFFFFFFFFFFFFFFFFULL)
    unlock_register(state, reg);

  if (state->last_access[reg] != NULL)
    last = state->last_access[reg]+1;
  else
    last = state->free_since[reg];
   
  while (last <= state->dst)
  {
    if (state->last_access[reg] != NULL && state->dirty[reg])
      last->reg_cache_infos.needed_registers[reg] = state->reg_content[reg];
    else
      last->reg_cache_infos.needed_registers[reg] = NULL;
    last++;
  }
  if (state->last_access[reg] == NULL)
  {
    state->free_since[reg] = state->dst+1;
    return;
  }

  if (state->dirty[reg])
  {
    if (state->is64bits[reg])
    {
      mov_m64rel_xreg64(state, (unsigned long long *) state->reg_content[reg], reg);
    }
    else
    {
      movsxd_reg64_reg32(state, reg, reg);
      mov_m64rel_xreg64(state, (unsigned long long *) state->reg_content[reg], reg);
    }
  }

  state->last_access[reg] = NULL;
  state->free_since[reg] = state->dst+1;
}

int lru_register(usf_state_t * state)
{
   unsigned long long oldest_access = 0xFFFFFFFFFFFFFFFFULL;
   int i, reg = 0;
   for (i=0; i<8; i++)
     {
    if (i != ESP && (unsigned long long) state->last_access[i] < oldest_access)
      {
         oldest_access = (unsigned long long) state->last_access[i];
         reg = i;
      }
     }
   return reg;
}

int lru_base_register(usf_state_t * state) /* EBP cannot be used as a base register for SIB addressing byte */
{
   unsigned long long oldest_access = 0xFFFFFFFFFFFFFFFFULL;
   int i, reg = 0;
   for (i=0; i<8; i++)
     {
    if (i != ESP && i != EBP && (unsigned long long) state->last_access[i] < oldest_access)
      {
         oldest_access = (unsigned long long) state->last_access[i];
         reg = i;
      }
     }
   return reg;
}

void set_register_state(usf_state_t * state, int reg, unsigned int *addr, int _dirty, int _is64bits)
{
  if (addr == NULL)
    state->last_access[reg] = NULL;
  else
    state->last_access[reg] = state->dst;
  state->reg_content[reg] = (unsigned long long *) addr;
  state->is64bits[reg] = _is64bits;
  state->dirty[reg] = _dirty;
}

int lock_register(usf_state_t * state, int reg)
{
   free_register(state, reg);
   state->last_access[reg] = (precomp_instr *) 0xFFFFFFFFFFFFFFFFULL;
   state->reg_content[reg] = NULL;
   return reg;
}

void unlock_register(usf_state_t * state, int reg)
{
   state->last_access[reg] = NULL;
}

// this function finds a register to put the data contained in addr,
// if there was another value before it's cleanly removed of the
// register cache. After that, the register number is returned.
// If data are already cached, the function only returns the register number
int allocate_register_32(usf_state_t * state, unsigned int *addr)
{
  int reg = 0, i;
   
  // is it already cached ?
  if (addr != NULL)
  {
    for (i = 0; i < 8; i++)
    {
      if (state->last_access[i] != NULL && (unsigned int *) state->reg_content[i] == addr)
      {
        precomp_instr *last = state->last_access[i]+1;

        while (last <= state->dst)
        {
          last->reg_cache_infos.needed_registers[i] = state->reg_content[i];
          last++;
        }
        state->last_access[i] = state->dst;
        state->is64bits[i] = 0;
        return i;
      }
    }
  }

  // it's not cached, so take the least recently used register
  reg = lru_register(state);
   
  if (state->last_access[reg])
    free_register(state, reg);
  else
  {
    while (state->free_since[reg] <= state->dst)
    {
      state->free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
      state->free_since[reg]++;
    }
  }
   
  state->last_access[reg] = state->dst;
  state->reg_content[reg] = (unsigned long long *) addr;
  state->dirty[reg] = 0;
  state->is64bits[reg] = 0;
   
  if (addr != NULL)
  {
    if (addr == (unsigned int *) state->r0)
      xor_reg32_reg32(state, reg, reg);
    else
      mov_xreg32_m32rel(state, reg, addr);
  }

  return reg;
}

// this function is similar to allocate_register except it loads
// a 64 bits value, and return the register number of the LSB part
int allocate_register_64(usf_state_t * state, unsigned long long *addr)
{
  int reg, i;

  // is it already cached?
  if (addr != NULL)
  {
    for (i = 0; i < 8; i++)
    {
      if (state->last_access[i] != NULL && state->reg_content[i] == addr)
      {
        precomp_instr *last = state->last_access[i]+1;

        while (last <= state->dst)
        {
          last->reg_cache_infos.needed_registers[i] = state->reg_content[i];
          last++;
        }
        state->last_access[i] = state->dst;
        if (state->is64bits[i] == 0)
        {
          movsxd_reg64_reg32(state, i, i);
          state->is64bits[i] = 1;
        }
        return i;
      }
    }
  }

  // it's not cached, so take the least recently used register
  reg = lru_register(state);
   
  if (state->last_access[reg])
    free_register(state, reg);
  else
  {
    while (state->free_since[reg] <= state->dst)
    {
      state->free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
      state->free_since[reg]++;
    }
  }
   
  state->last_access[reg] = state->dst;
  state->reg_content[reg] = addr;
  state->dirty[reg] = 0;
  state->is64bits[reg] = 1;
   
  if (addr != NULL)
  {
    if (addr == state->r0)
      xor_reg64_reg64(state, reg, reg);
    else
      mov_xreg64_m64rel(state, reg, addr);
  }

  return reg;
}

// this function checks if the data located at addr are cached in a register
// and then, it returns 1  if it's a 64 bit value
//                      0  if it's a 32 bit value
//                      -1 if it's not cached
int is64(usf_state_t * state, unsigned int *addr)
{
  int i;
  for (i = 0; i < 8; i++)
  {
    if (state->last_access[i] != NULL && state->reg_content[i] == (unsigned long long *) addr)
    {
      return state->is64bits[i];
    }
  }
  return -1;
}

int allocate_register_32_w(usf_state_t * state, unsigned int *addr)
{
  int reg = 0, i;
   
  // is it already cached ?
  for (i = 0; i < 8; i++)
  {
    if (state->last_access[i] != NULL && state->reg_content[i] == (unsigned long long *) addr)
    {
      precomp_instr *last = state->last_access[i] + 1;
         
      while (last <= state->dst)
      {
        last->reg_cache_infos.needed_registers[i] = NULL;
        last++;
      }
      state->last_access[i] = state->dst;
      state->dirty[i] = 1;
      state->is64bits[i] = 0;
      return i;
    }
  }

  // it's not cached, so take the least recently used register
  reg = lru_register(state);
   
  if (state->last_access[reg])
    free_register(state, reg);
  else
  {
    while (state->free_since[reg] <= state->dst)
    {
      state->free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
      state->free_since[reg]++;
    }
  }
   
  state->last_access[reg] = state->dst;
  state->reg_content[reg] = (unsigned long long *) addr;
  state->dirty[reg] = 1;
  state->is64bits[reg] = 0;

  return reg;
}

int allocate_register_64_w(usf_state_t *state, unsigned long long *addr)
{
  int reg, i;

  // is it already cached?
  for (i = 0; i < 8; i++)
  {
    if (state->last_access[i] != NULL && state->reg_content[i] == addr)
    {
      precomp_instr *last = state->last_access[i] + 1;

      while (last <= state->dst)
      {
        last->reg_cache_infos.needed_registers[i] = NULL;
        last++;
      }
      state->last_access[i] = state->dst;
      state->is64bits[i] = 1;
      state->dirty[i] = 1;
      return i;
    }
  }

  // it's not cached, so take the least recently used register
  reg = lru_register(state);
   
  if (state->last_access[reg])
    free_register(state, reg);
  else
  {
    while (state->free_since[reg] <= state->dst)
    {
      state->free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
      state->free_since[reg]++;
    }
  }
   
  state->last_access[reg] = state->dst;
  state->reg_content[reg] = addr;
  state->dirty[reg] = 1;
  state->is64bits[reg] = 1;

  return reg;
}

void allocate_register_32_manually(usf_state_t * state, int reg, unsigned int *addr)
{
  int i;

  /* check if we just happen to already have this r4300 reg cached in the requested x86 reg */
  if (state->last_access[reg] != NULL && state->reg_content[reg] == (unsigned long long *) addr)
  {
    precomp_instr *last = state->last_access[reg] + 1;
    while (last <= state->dst)
    {
      last->reg_cache_infos.needed_registers[reg] = state->reg_content[reg];
      last++;
    }
    state->last_access[reg] = state->dst;
    /* we won't touch is64bits or dirty; the register returned is "read-only" */
    return;
  }

  /* otherwise free up the requested x86 register */
  if (state->last_access[reg])
    free_register(state, reg);
  else
  {
    while (state->free_since[reg] <= state->dst)
    {
      state->free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
      state->free_since[reg]++;
    }
  }

  /* if the r4300 register is already cached in a different x86 register, then copy it to the requested x86 register */
  for (i=0; i<8; i++)
  {
    if (state->last_access[i] != NULL && state->reg_content[i] == (unsigned long long *) addr)
    {
      precomp_instr *last = state->last_access[i]+1;
      while (last <= state->dst)
      {
        last->reg_cache_infos.needed_registers[i] = state->reg_content[i];
        last++;
      }
      state->last_access[i] = state->dst;
      if (state->is64bits[i])
        mov_reg64_reg64(state, reg, i);
      else
        mov_reg32_reg32(state, reg, i);
      state->last_access[reg] = state->dst;
      state->is64bits[reg] = state->is64bits[i];
      state->dirty[reg] = state->dirty[i];
      state->reg_content[reg] = state->reg_content[i];
      /* free the previous x86 register used to cache this r4300 register */
      state->free_since[i] = state->dst + 1;
      state->last_access[i] = NULL;
      return;
    }
  }

  /* otherwise just load the 32-bit value into the requested register */
  state->last_access[reg] = state->dst;
  state->reg_content[reg] = (unsigned long long *) addr;
  state->dirty[reg] = 0;
  state->is64bits[reg] = 0;
   
  if ((unsigned long long *) addr == state->r0)
    xor_reg32_reg32(state, reg, reg);
  else
    mov_xreg32_m32rel(state, reg, addr);
}

void allocate_register_32_manually_w(usf_state_t * state, int reg, unsigned int *addr)
{
  int i;

  /* check if we just happen to already have this r4300 reg cached in the requested x86 reg */
  if (state->last_access[reg] != NULL && state->reg_content[reg] == (unsigned long long *) addr)
  {
    precomp_instr *last = state->last_access[reg]+1;
    while (last <= state->dst)
    {
      last->reg_cache_infos.needed_registers[reg] = NULL;
      last++;
    }
    state->last_access[reg] = state->dst;
    state->is64bits[reg] = 0;
    state->dirty[reg] = 1;
    return;
  }
   
  /* otherwise free up the requested x86 register */
  if (state->last_access[reg])
    free_register(state, reg);
  else
  {
    while (state->free_since[reg] <= state->dst)
    {
      state->free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
      state->free_since[reg]++;
    }
  }

  /* if the r4300 register is already cached in a different x86 register, then free it and bind to the requested x86 register */
  for (i = 0; i < 8; i++)
  {
    if (state->last_access[i] != NULL && state->reg_content[i] == (unsigned long long *) addr)
    {
      precomp_instr *last = state->last_access[i] + 1;
      while (last <= state->dst)
      {
        last->reg_cache_infos.needed_registers[i] = NULL;
        last++;
      }
      state->last_access[reg] = state->dst;
      state->reg_content[reg] = state->reg_content[i];
      state->dirty[reg] = 1;
      state->is64bits[reg] = 0;
      /* free the previous x86 register used to cache this r4300 register */
      state->free_since[i] = state->dst+1;
      state->last_access[i] = NULL;
      return;
    }
  }
   
  /* otherwise just set up the requested register as 32-bit */
  state->last_access[reg] = state->dst;
  state->reg_content[reg] = (unsigned long long *) addr;
  state->dirty[reg] = 1;
  state->is64bits[reg] = 0;
}


// 0x48 0x83 0xEC 0x8                     sub rsp, byte 8
// 0x48 0xA1           0xXXXXXXXXXXXXXXXX mov rax, qword (&code start)
// 0x48 0x05                   0xXXXXXXXX add rax, dword (local_addr)
// 0x48 0x89 0x04 0x24                    mov [rsp], rax
// 0x48 0xB8           0xXXXXXXXXXXXXXXXX mov rax, &reg[0]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rdi, [rax + XXXXXXXX]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rsi, [rax + XXXXXXXX]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rbp, [rax + XXXXXXXX]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rdx, [rax + XXXXXXXX]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rcx, [rax + XXXXXXXX]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rbx, [rax + XXXXXXXX]
// 0x48 0x8B (reg<<3)|0x80     0xXXXXXXXX mov rax, [rax + XXXXXXXX]
// 0xC3 ret
// total : 84 bytes

static void build_wrapper(usf_state_t * state, precomp_instr *instr, unsigned char* pCode, precomp_block* block)
{
   int i;

   *pCode++ = 0x48;
   *pCode++ = 0x83;
   *pCode++ = 0xEC;
   *pCode++ = 0x08;
   
   *pCode++ = 0x48;
   *pCode++ = 0xA1;
   *((unsigned long long *) pCode) = (unsigned long long) (&block->code);
   pCode += 8;
   
   *pCode++ = 0x48;
   *pCode++ = 0x05;
   *((unsigned int *) pCode) = (unsigned int) instr->local_addr;
   pCode += 4;
   
   *pCode++ = 0x48;
   *pCode++ = 0x89;
   *pCode++ = 0x04;
   *pCode++ = 0x24;

   *pCode++ = 0x48;
   *pCode++ = 0xB8;
   *((unsigned long long *) pCode) = (unsigned long long) &state->reg[0];
   pCode += 8;

   for (i=7; i>=0; i--)
   {
     long long riprel;
     if (instr->reg_cache_infos.needed_registers[i] != NULL)
     {
       *pCode++ = 0x48;
       *pCode++ = 0x8B;
       *pCode++ = 0x80 | (i << 3);
       riprel = (long long) ((unsigned char *) instr->reg_cache_infos.needed_registers[i] - (unsigned char *) &state->reg[0]);
       *((int *) pCode) = (int) riprel;
       pCode += 4;
       if (riprel >= 0x7fffffffLL || riprel < -0x80000000LL)
       {
         DebugMessage(state, M64MSG_ERROR, "build_wrapper error: reg[%i] offset too big for relative address from %p to %p",
                i, (&state->reg[0]), instr->reg_cache_infos.needed_registers[i]);
         OSAL_BREAKPOINT_INTERRUPT
       }
     }
   }
   *pCode++ = 0xC3;
}

void build_wrappers(usf_state_t * state, precomp_instr *instr, int start, int end, precomp_block* block)
{
   int i, reg;
   for (i=start; i<end; i++)
     {
    instr[i].reg_cache_infos.need_map = 0;
    for (reg=0; reg<8; reg++)
      {
         if (instr[i].reg_cache_infos.needed_registers[reg] != NULL)
           {
          instr[i].reg_cache_infos.need_map = 1;
          build_wrapper(state, &instr[i], instr[i].reg_cache_infos.jump_wrapper, block);
          break;
           }
      }
     }
}

