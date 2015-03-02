/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - regcache.c                                              *
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

#include "regcache.h"

#include "r4300/recomp.h"
#include "r4300/r4300.h"
#include "r4300/recomph.h"

void init_cache(usf_state_t * state, precomp_instr* start)
{
   int i;
   for (i=0; i<8; i++)
     {
    state->last_access[i] = NULL;
    state->free_since[i] = start;
     }
     state->r0 = (unsigned int*)state->reg;
}

void free_all_registers(usf_state_t * state)
{
   int i;
   for (i=0; i<8; i++)
     {
    if (state->last_access[i]) free_register(state, i);
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

// this function frees a specific X86 GPR
void free_register(usf_state_t * state, int reg)
{
   precomp_instr *last;
   
   if (state->last_access[reg] != NULL &&
       state->r64[reg] != -1 && (int)state->reg_content[reg] != (int)state->reg_content[state->r64[reg]]-4)
     {
    free_register(state, state->r64[reg]);
    return;
     }
   
   if (state->last_access[reg] != NULL) last = state->last_access[reg]+1;
   else last = state->free_since[reg];
   
   while (last <= state->dst)
     {
    if (state->last_access[reg] != NULL && state->dirty[reg])
      last->reg_cache_infos.needed_registers[reg] = state->reg_content[reg];
    else
      last->reg_cache_infos.needed_registers[reg] = NULL;
    
    if (state->last_access[reg] != NULL && state->r64[reg] != -1)
      {
         if (state->dirty[state->r64[reg]])
           last->reg_cache_infos.needed_registers[state->r64[reg]] = state->reg_content[state->r64[reg]];
         else
           last->reg_cache_infos.needed_registers[state->r64[reg]] = NULL;
      }
    
    last++;
     }
   if (state->last_access[reg] == NULL)
     {
    state->free_since[reg] = state->dst+1;
    return;
     }
   
   if (state->dirty[reg])
     {
    mov_m32_reg32(state, state->reg_content[reg], reg);
    if (state->r64[reg] == -1)
      {
         sar_reg32_imm8(state, reg, 31);
         mov_m32_reg32(state, (unsigned int*)state->reg_content[reg]+1, reg);
      }
    else mov_m32_reg32(state, state->reg_content[state->r64[reg]], state->r64[reg]);
     }
   state->last_access[reg] = NULL;
   state->free_since[reg] = state->dst+1;
   if (state->r64[reg] != -1)
     {
    state->last_access[state->r64[reg]] = NULL;
    state->free_since[state->r64[reg]] = state->dst+1;
     }
}

int lru_register(usf_state_t * state)
{
   unsigned int oldest_access = 0xFFFFFFFF;
   int i, reg = 0;
   for (i=0; i<8; i++)
     {
    if (i != ESP && i != ESI && (unsigned int)state->last_access[i] < oldest_access)
      {
         oldest_access = (int)state->last_access[i];
         reg = i;
      }
     }
   if (oldest_access == 0xFFFFFFFF)
   {
	   int i = rand();
   }
   return reg;
}

int lru_register_exc1(usf_state_t * state, int exc1)
{
   unsigned int oldest_access = 0xFFFFFFFF;
   int i, reg = 0;
   for (i=0; i<8; i++)
     {
    if (i != ESP && i != ESI && i != exc1 && (unsigned int)state->last_access[i] < oldest_access)
      {
         oldest_access = (int)state->last_access[i];
         reg = i;
      }
     }
   if (oldest_access == 0xFFFFFFFF)
   {
	   int i = rand();
   }
   return reg;
}

// this function finds a register to put the data contained in addr,
// if there was another value before it's cleanly removed of the
// register cache. After that, the register number is returned.
// If data are already cached, the function only returns the register number
int allocate_register(usf_state_t * state, unsigned int *addr)
{
   unsigned int oldest_access = 0xFFFFFFFF;
   int reg = 0, i;
   
   // is it already cached ?
   if (addr != NULL)
     {
    for (i=0; i<8; i++)
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
          if (state->r64[i] != -1)
            {
               last = state->last_access[state->r64[i]]+1;
               
               while (last <= state->dst)
             {
                last->reg_cache_infos.needed_registers[state->r64[i]] = state->reg_content[state->r64[i]];
                last++;
             }
               state->last_access[state->r64[i]] = state->dst;
            }
          
          return i;
           }
      }
     }
   
   // if it's not cached, we take the least recently used register
   for (i=0; i<8; i++)
     {
    if (i != ESP && i != ESI && (unsigned int)state->last_access[i] < oldest_access)
      {
         oldest_access = (int)state->last_access[i];
         reg = i;
      }
     }
   
   if (oldest_access == 0xFFFFFFFF)
   {
	   int i = rand();
   }
   if (state->last_access[reg]) free_register(state, reg);
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
   state->r64[reg] = -1;
   
   if (addr != NULL)
     {
    if (addr == state->r0 || addr == state->r0+1)
      xor_reg32_reg32(state, reg, reg);
    else
      mov_reg32_m32(state, reg, addr);
     }
   
   return reg;
}

// this function is similar to allocate_register except it loads
// a 64 bits value, and return the register number of the LSB part
int allocate_64_register1(usf_state_t * state, unsigned int *addr)
{
   int reg1, reg2, i;
   
   // is it already cached as a 32 bits value ?
   for (i=0; i<8; i++)
     {
    if (state->last_access[i] != NULL && state->reg_content[i] == addr)
      {
         if (state->r64[i] == -1)
           {
          allocate_register(state, addr);
          reg2 = allocate_register(state, state->dirty[i] ? NULL : addr+1);
          state->r64[i] = reg2;
          state->r64[reg2] = i;
          
          if (state->dirty[i])
            {
               state->reg_content[reg2] = addr+1;
               state->dirty[reg2] = 1;
               mov_reg32_reg32(state, reg2, i);
               sar_reg32_imm8(state, reg2, 31);
            }
          
          return i;
           }
      }
     }
   
   reg1 = allocate_register(state, addr);
   reg2 = allocate_register(state, addr+1);
   state->r64[reg1] = reg2;
   state->r64[reg2] = reg1;
   
   return reg1;
}

// this function is similar to allocate_register except it loads
// a 64 bits value, and return the register number of the MSB part
int allocate_64_register2(usf_state_t * state, unsigned int *addr)
{
   int reg1, reg2, i;
   
   // is it already cached as a 32 bits value ?
   for (i=0; i<8; i++)
     {
    if (state->last_access[i] != NULL && state->reg_content[i] == addr)
      {
         if (state->r64[i] == -1)
           {
          allocate_register(state, addr);
          reg2 = allocate_register(state, state->dirty[i] ? NULL : addr+1);
          state->r64[i] = reg2;
          state->r64[reg2] = i;
          
          if (state->dirty[i])
            {
               state->reg_content[reg2] = addr+1;
               state->dirty[reg2] = 1;
               mov_reg32_reg32(state, reg2, i);
               sar_reg32_imm8(state, reg2, 31);
            }
          
          return reg2;
           }
      }
     }
   
   reg1 = allocate_register(state, addr);
   reg2 = allocate_register(state, addr+1);
   state->r64[reg1] = reg2;
   state->r64[reg2] = reg1;
   
   return reg2;
}

// this function checks if the data located at addr are cached in a register
// and then, it returns 1  if it's a 64 bit value
//                      0  if it's a 32 bit value
//                      -1 if it's not cached
int is64(usf_state_t * state, unsigned int *addr)
{
   int i;
   for (i=0; i<8; i++)
     {
    if (state->last_access[i] != NULL && state->reg_content[i] == addr)
      {
         if (state->r64[i] == -1) return 0;
         return 1;
      }
     }
   return -1;
}

int allocate_register_w(usf_state_t * state, unsigned int *addr)
{
   unsigned int oldest_access = 0xFFFFFFFF;
   int reg = 0, i;
   
   // is it already cached ?
   for (i=0; i<8; i++)
     {
    if (state->last_access[i] != NULL && state->reg_content[i] == addr)
      {
         precomp_instr *last = state->last_access[i]+1;
         
         while (last <= state->dst)
           {
          last->reg_cache_infos.needed_registers[i] = NULL;
          last++;
           }
         state->last_access[i] = state->dst;
         state->dirty[i] = 1;
         if (state->r64[i] != -1)
           {
          last = state->last_access[state->r64[i]]+1;
          while (last <= state->dst)
            {
               last->reg_cache_infos.needed_registers[state->r64[i]] = NULL;
               last++;
            }
          state->free_since[state->r64[i]] = state->dst+1;
          state->last_access[state->r64[i]] = NULL;
          state->r64[i] = -1;
           }
         
         return i;
      }
     }
   
   // if it's not cached, we take the least recently used register
   for (i=0; i<8; i++)
     {
    if (i != ESP && i != ESI && (unsigned int)state->last_access[i] < oldest_access)
      {
         oldest_access = (int)state->last_access[i];
         reg = i;
      }
     }
   
   if (oldest_access == 0xFFFFFFFF)
   {
	   int i = rand();
   }

   if (state->last_access[reg]) free_register(state, reg);
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
   state->r64[reg] = -1;
   
   return reg;
}

int allocate_64_register1_w(usf_state_t * state, unsigned int *addr)
{
   int reg1, reg2, i;
   
   // is it already cached as a 32 bits value ?
   for (i=0; i<8; i++)
     {
    if (state->last_access[i] != NULL && state->reg_content[i] == addr)
      {
         if (state->r64[i] == -1)
           {
          allocate_register_w(state, addr);
          reg2 = lru_register(state);
          if (state->last_access[reg2]) free_register(state, reg2);
          else
          {
            while (state->free_since[reg2] <= state->dst)
            {
              state->free_since[reg2]->reg_cache_infos.needed_registers[reg2] = NULL;
              state->free_since[reg2]++;
            }
          }
          state->r64[i] = reg2;
          state->r64[reg2] = i;
          state->last_access[reg2] = state->dst;
          
          state->reg_content[reg2] = addr+1;
          state->dirty[reg2] = 1;
          mov_reg32_reg32(state, reg2, i);
          sar_reg32_imm8(state, reg2, 31);
          
          return i;
           }
         else
           {
          state->last_access[i] = state->dst;
          state->last_access[state->r64[i]] = state->dst;
          state->dirty[i] = state->dirty[state->r64[i]] = 1;
          return i;
           }
      }
     }
   
   reg1 = allocate_register_w(state, addr);
   reg2 = lru_register(state);
   if (state->last_access[reg2]) free_register(state, reg2);
   else
     {
    while (state->free_since[reg2] <= state->dst)
      {
         state->free_since[reg2]->reg_cache_infos.needed_registers[reg2] = NULL;
         state->free_since[reg2]++;
      }
     }
   state->r64[reg1] = reg2;
   state->r64[reg2] = reg1;
   state->last_access[reg2] = state->dst;
   state->reg_content[reg2] = addr+1;
   state->dirty[reg2] = 1;
   
   return reg1;
}

int allocate_64_register2_w(usf_state_t * state, unsigned int *addr)
{
   int reg1, reg2, i;
   
   // is it already cached as a 32 bits value ?
   for (i=0; i<8; i++)
     {
    if (state->last_access[i] != NULL && state->reg_content[i] == addr)
      {
         if (state->r64[i] == -1)
           {
          allocate_register_w(state, addr);
          reg2 = lru_register(state);
          if (state->last_access[reg2]) free_register(state, reg2);
          else
          {
            while (state->free_since[reg2] <= state->dst)
            {
              state->free_since[reg2]->reg_cache_infos.needed_registers[reg2] = NULL;
              state->free_since[reg2]++;
            }
          }
          state->r64[i] = reg2;
          state->r64[reg2] = i;
          state->last_access[reg2] = state->dst;
          
          state->reg_content[reg2] = addr+1;
          state->dirty[reg2] = 1;
          mov_reg32_reg32(state, reg2, i);
          sar_reg32_imm8(state, reg2, 31);
          
          return reg2;
           }
         else
           {
          state->last_access[i] = state->dst;
          state->last_access[state->r64[i]] = state->dst;
          state->dirty[i] = state->dirty[state->r64[i]] = 1;
          return state->r64[i];
           }
      }
     }
   
   reg1 = allocate_register_w(state, addr);
   reg2 = lru_register(state);
   if (state->last_access[reg2]) free_register(state, reg2);
   else
     {
    while (state->free_since[reg2] <= state->dst)
      {
         state->free_since[reg2]->reg_cache_infos.needed_registers[reg2] = NULL;
         state->free_since[reg2]++;
      }
     }
   state->r64[reg1] = reg2;
   state->r64[reg2] = reg1;
   state->last_access[reg2] = state->dst;
   state->reg_content[reg2] = addr+1;
   state->dirty[reg2] = 1;
   
   return reg2;
}

void set_register_state(usf_state_t * state, int reg, unsigned int *addr, int d)
{
   state->last_access[reg] = state->dst;
   state->reg_content[reg] = addr;
   state->r64[reg] = -1;
   state->dirty[reg] = d;
}

void set_64_register_state(usf_state_t * state, int reg1, int reg2, unsigned int *addr, int d)
{
   state->last_access[reg1] = state->dst;
   state->last_access[reg2] = state->dst;
   state->reg_content[reg1] = addr;
   state->reg_content[reg2] = addr+1;
   state->r64[reg1] = reg2;
   state->r64[reg2] = reg1;
   state->dirty[reg1] = d;
   state->dirty[reg2] = d;
}

void force_32(usf_state_t * state, int reg)
{
   if (state->r64[reg] != -1)
     {
    precomp_instr *last = state->last_access[reg]+1;
    
    while (last <= state->dst)
      {
         if (state->dirty[reg])
           last->reg_cache_infos.needed_registers[reg] = state->reg_content[reg];
         else
           last->reg_cache_infos.needed_registers[reg] = NULL;
         
         if (state->dirty[state->r64[reg]])
           last->reg_cache_infos.needed_registers[state->r64[reg]] = state->reg_content[state->r64[reg]];
         else
           last->reg_cache_infos.needed_registers[state->r64[reg]] = NULL;
         
         last++;
      }
    
    if (state->dirty[reg])
      {
         mov_m32_reg32(state, state->reg_content[reg], reg);
         mov_m32_reg32(state, state->reg_content[state->r64[reg]], state->r64[reg]);
         state->dirty[reg] = 0;
      }
    state->last_access[state->r64[reg]] = NULL;
    state->free_since[state->r64[reg]] = state->dst+1;
    state->r64[reg] = -1;
     }
}

void allocate_register_manually(usf_state_t * state, int reg, unsigned int *addr)
{
   int i;
   
   if (state->last_access[reg] != NULL && state->reg_content[reg] == addr)
     {
    precomp_instr *last = state->last_access[reg]+1;
         
    while (last <= state->dst)
      {
         last->reg_cache_infos.needed_registers[reg] = state->reg_content[reg];
         last++;
      }
    state->last_access[reg] = state->dst;
    if (state->r64[reg] != -1)
      {
         last = state->last_access[state->r64[reg]]+1;
         
         while (last <= state->dst)
           {
          last->reg_cache_infos.needed_registers[state->r64[reg]] = state->reg_content[state->r64[reg]];
          last++;
           }
         state->last_access[state->r64[reg]] = state->dst;
      }
    return;
     }
   
   if (state->last_access[reg]) free_register(state, reg);
   else
     {
    while (state->free_since[reg] <= state->dst)
      {
         state->free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
         state->free_since[reg]++;
      }
     }
   
   // is it already cached ?
   for (i=0; i<8; i++)
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
         if (state->r64[i] != -1)
           {
          last = state->last_access[state->r64[i]]+1;
          
          while (last <= state->dst)
            {
               last->reg_cache_infos.needed_registers[state->r64[i]] = state->reg_content[state->r64[i]];
               last++;
            }
          state->last_access[state->r64[i]] = state->dst;
           }
         
         mov_reg32_reg32(state, reg, i);
         state->last_access[reg] = state->dst;
         state->r64[reg] = state->r64[i];
         if (state->r64[reg] != -1) state->r64[state->r64[reg]] = reg;
         state->dirty[reg] = state->dirty[i];
         state->reg_content[reg] = state->reg_content[i];
         state->free_since[i] = state->dst+1;
         state->last_access[i] = NULL;
         
         return;
      }
     }
   
   state->last_access[reg] = state->dst;
   state->reg_content[reg] = addr;
   state->dirty[reg] = 0;
   state->r64[reg] = -1;
   
   if (addr != NULL)
     {
    if (addr == state->r0 || addr == state->r0+1)
      xor_reg32_reg32(state, reg, reg);
    else
      mov_reg32_m32(state, reg, addr);
     }
}

void allocate_register_manually_w(usf_state_t * state, int reg, unsigned int *addr, int load)
{
   int i;
   
   if (state->last_access[reg] != NULL && state->reg_content[reg] == addr)
     {
    precomp_instr *last = state->last_access[reg]+1;
         
    while (last <= state->dst)
      {
         last->reg_cache_infos.needed_registers[reg] = state->reg_content[reg];
         last++;
      }
    state->last_access[reg] = state->dst;
    
    if (state->r64[reg] != -1)
      {
         last = state->last_access[state->r64[reg]]+1;
         
         while (last <= state->dst)
           {
          last->reg_cache_infos.needed_registers[state->r64[reg]] = state->reg_content[state->r64[reg]];
          last++;
           }
         state->last_access[state->r64[reg]] = NULL;
         state->free_since[state->r64[reg]] = state->dst+1;
         state->r64[reg] = -1;
      }
    state->dirty[reg] = 1;
    return;
     }
   
   if (state->last_access[reg]) free_register(state, reg);
   else
     {
    while (state->free_since[reg] <= state->dst)
      {
         state->free_since[reg]->reg_cache_infos.needed_registers[reg] = NULL;
         state->free_since[reg]++;
      }
     }
   
   // is it already cached ?
   for (i=0; i<8; i++)
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
         if (state->r64[i] != -1)
           {
          last = state->last_access[state->r64[i]]+1;
          while (last <= state->dst)
            {
               last->reg_cache_infos.needed_registers[state->r64[i]] = NULL;
               last++;
            }
          state->free_since[state->r64[i]] = state->dst+1;
          state->last_access[state->r64[i]] = NULL;
          state->r64[i] = -1;
           }
         
         if (load)
           mov_reg32_reg32(state, reg, i);
         state->last_access[reg] = state->dst;
         state->dirty[reg] = 1;
         state->r64[reg] = -1;
         state->reg_content[reg] = state->reg_content[i];
         state->free_since[i] = state->dst+1;
         state->last_access[i] = NULL;
         
         return;
      }
     }
   
   state->last_access[reg] = state->dst;
   state->reg_content[reg] = addr;
   state->dirty[reg] = 1;
   state->r64[reg] = -1;
   
   if (addr != NULL && load)
     {
    if (addr == state->r0 || addr == state->r0+1)
      xor_reg32_reg32(state, reg, reg);
    else
      mov_reg32_m32(state, reg, addr);
     }
}

// 0x81 0xEC 0x4 0x0 0x0 0x0  sub esp, 4
// 0xA1            0xXXXXXXXX mov eax, XXXXXXXX (&code start)
// 0x05            0xXXXXXXXX add eax, XXXXXXXX (local_addr)
// 0x89 0x04 0x24             mov [esp], eax
// 0x8B (reg<<3)|5 0xXXXXXXXX mov eax, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov ebx, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov ecx, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov edx, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov ebp, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov esi, [XXXXXXXX]
// 0x8B (reg<<3)|5 0xXXXXXXXX mov edi, [XXXXXXXX]
// 0xC3 ret
// total : 62 bytes
static void build_wrapper(usf_state_t * state, precomp_instr *instr, unsigned char* code, precomp_block* block)
{
   int i;
   int j=0;

   code[j++] = 0x81;
   code[j++] = 0xEC;
   code[j++] = 0x04;
   code[j++] = 0x00;
   code[j++] = 0x00;
   code[j++] = 0x00;
   
   code[j++] = 0xA1;
   *((unsigned int*)&code[j]) = (unsigned int)(&block->code);
   j+=4;
   
   code[j++] = 0x05;
   *((unsigned int*)&code[j]) = (unsigned int)instr->local_addr;
   j+=4;
   
   code[j++] = 0x89;
   code[j++] = 0x04;
   code[j++] = 0x24;
   
   for (i=0; i<8; i++)
     {
    if (instr->reg_cache_infos.needed_registers[i] != NULL)
      {
         code[j++] = 0x8B;
         code[j++] = (i << 3) | 5;
         *((unsigned int*)&code[j]) =
                 (unsigned int)instr->reg_cache_infos.needed_registers[i];
         j+=4;
      }
     }
   
   code[j++] = 0xC3;
}

void build_wrappers(usf_state_t * state, precomp_instr *instr, int start, int end, precomp_block* block)
{
   int i, reg;;
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

void simplify_access(usf_state_t * state)
{
   int i;
   state->dst->local_addr = state->code_length;
   for(i=0; i<8; i++) state->dst->reg_cache_infos.needed_registers[i] = NULL;
}

