/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - assemble.h                                              *
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

#ifndef M64P_R4300_ASSEMBLE_H
#define M64P_R4300_ASSEMBLE_H

#include "r4300/recomph.h"
#include "api/callbacks.h"
#include "osal/preproc.h"

#include <stdlib.h>

#define RAX 0
#define RCX 1
#define RDX 2
#define RBX 3
#define RSP 4
#define RBP 5
#define RSI 6
#define RDI 7

#define EAX 0
#define ECX 1
#define EDX 2
#define EBX 3
#define ESP 4
#define EBP 5
#define ESI 6
#define EDI 7

#define AX 0
#define CX 1
#define DX 2
#define BX 3
#define SP 4
#define BP 5
#define SI 6
#define DI 7

#define AL 0
#define CL 1
#define DL 2
#define BL 3
#define AH 4
#define CH 5
#define DH 6
#define BH 7

#ifdef _WIN32
#define RP0 RCX
#define RP1 RDX
#define RP2 8
#define RP3 9
#else
#define RP0 RDI
#define RP1 RSI
#define RP2 RDX
#define RP3 RCX
#endif

void jump_start_rel8(usf_state_t *);
void jump_end_rel8(usf_state_t *);
void jump_start_rel32(usf_state_t *);
void jump_end_rel32(usf_state_t *);
void add_jump(usf_state_t *, unsigned int pc_addr, unsigned int mi_addr, unsigned int absolute64);

static inline void put8(usf_state_t * state, unsigned char octet)
{
  (*state->inst_pointer)[state->code_length] = octet;
  state->code_length++;
  if (state->code_length == state->max_code_length)
  {
    *state->inst_pointer = realloc_exec(state, *state->inst_pointer, state->max_code_length, state->max_code_length+8192);
    state->max_code_length += 8192;
  }
}

static inline void put32(usf_state_t * state, unsigned int dword)
{
  if ((state->code_length + 4) >= state->max_code_length)
  {
    *state->inst_pointer = realloc_exec(state, *state->inst_pointer, state->max_code_length, state->max_code_length+8192);
    state->max_code_length += 8192;
  }
  *((unsigned int *) (*state->inst_pointer + state->code_length)) = dword;
  state->code_length += 4;
}

static inline void put64(usf_state_t * state, unsigned long long qword)
{
  if ((state->code_length + 8) >= state->max_code_length)
  {
    *state->inst_pointer = realloc_exec(state, *state->inst_pointer, state->max_code_length, state->max_code_length+8192);
    state->max_code_length += 8192;
  }
  *((unsigned long long *) (*state->inst_pointer + state->code_length)) = qword;
  state->code_length += 8;
}

static inline int rel_r15_offset(usf_state_t * state, void *dest, const char *op_name)
{
    /* calculate the destination pointer's offset from the base of the r4300 registers */
    long long rel_offset = (long long) ((unsigned char *) dest - (unsigned char *) state);

    if (llabs(rel_offset) > 0x7fffffff)
    {
        DebugMessage(state, M64MSG_ERROR, "Error: destination %p more than 2GB away from r15 base %p in %s()", dest, state, op_name);
        OSAL_BREAKPOINT_INTERRUPT;
    }

    return (int) rel_offset;
}

static inline void mov_memoffs32_eax(usf_state_t * state, unsigned int *memoffs32)
{
   put8(state, 0xA3);
   put64(state, (unsigned long long) memoffs32);
}

static inline void mov_rax_memoffs64(usf_state_t * state, unsigned long long *memoffs64)
{
   put8(state, 0x48);
   put8(state, 0xA1);
   put64(state, (unsigned long long) memoffs64);
}

static inline void mov_memoffs64_rax(usf_state_t * state, unsigned long long *memoffs64)
{
   put8(state, 0x48);
   put8(state, 0xA3);
   put64(state, (unsigned long long) memoffs64);
}

static inline void mov_m8rel_xreg8(usf_state_t * state, unsigned char *m8, int xreg8)
{
   int offset = rel_r15_offset(state, m8, "mov_m8rel_xreg8");

   put8(state, 0x41 | ((xreg8 & 8) >> 1));
   put8(state, 0x88);
   put8(state, 0x87 | ((xreg8 & 7) << 3));
   put32(state, offset);
}

static inline void mov_xreg16_m16rel(usf_state_t * state, int xreg16, unsigned short *m16)
{
   int offset = rel_r15_offset(state, m16, "mov_xreg16_m16rel");

   put8(state, 0x66);
   put8(state, 0x41 | ((xreg16 & 8) >> 1));
   put8(state, 0x8B);
   put8(state, 0x87 | ((xreg16 & 7) << 3));
   put32(state, offset);
}

static inline void mov_m16rel_xreg16(usf_state_t * state, unsigned short *m16, int xreg16)
{
   int offset = rel_r15_offset(state, m16, "mov_m16rel_xreg16");

   put8(state, 0x66);
   put8(state, 0x41 | ((xreg16 & 8) >> 1));
   put8(state, 0x89);
   put8(state, 0x87 | ((xreg16 & 7) << 3));
   put32(state, offset);
}

static inline void cmp_xreg32_m32rel(usf_state_t * state, int xreg32, unsigned int *m32)
{
   int offset = rel_r15_offset(state, m32, "cmp_xreg32_m32rel");

   put8(state, 0x41 | ((xreg32 & 8) >> 1));
   put8(state, 0x3B);
   put8(state, 0x87 | ((xreg32 & 7) << 3));
   put32(state, offset);
}

static inline void cmp_xreg64_m64rel(usf_state_t * state, int xreg64, unsigned long long *m64)
{
   int offset = rel_r15_offset(state, m64, "cmp_xreg64_m64rel");

   put8(state, 0x49 | ((xreg64 & 8) >> 1));
   put8(state, 0x3B);
   put8(state, 0x87 | ((xreg64 & 7) << 3));
   put32(state, offset);
}

static inline void cmp_reg32_reg32(usf_state_t * state, int reg1, int reg2)
{
   put8(state, 0x39);
   put8(state, (reg2 << 3) | reg1 | 0xC0);
}

static inline void cmp_reg64_reg64(usf_state_t * state, int reg1, int reg2)
{
   put8(state, 0x48);
   put8(state, 0x39);
   put8(state, (reg2 << 3) | reg1 | 0xC0);
}

static inline void cmp_reg32_imm8(usf_state_t * state, int reg32, unsigned char imm8)
{
   put8(state, 0x83);
   put8(state, 0xF8 + reg32);
   put8(state, imm8);
}

static inline void cmp_reg64_imm8(usf_state_t * state, int reg64, unsigned char imm8)
{
   put8(state, 0x48);
   put8(state, 0x83);
   put8(state, 0xF8 + reg64);
   put8(state, imm8);
}

static inline void cmp_reg32_imm32(usf_state_t * state, int reg32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0xF8 + reg32);
   put32(state, imm32);
}

static inline void cmp_reg64_imm32(usf_state_t * state, int reg64, unsigned int imm32)
{
   put8(state, 0x48);
   put8(state, 0x81);
   put8(state, 0xF8 + reg64);
   put32(state, imm32);
}

static inline void cmp_preg64preg64_imm8(usf_state_t * state, int reg1, int reg2, unsigned char imm8)
{
   put8(state, 0x80);
   put8(state, 0x3C);
   put8(state, (reg1 << 3) | reg2);
   put8(state, imm8);
}

static inline void sete_m8rel(usf_state_t * state, unsigned char *m8)
{
   int offset = rel_r15_offset(state, m8, "sete_m8rel");

   put8(state, 0x41);
   put8(state, 0x0F);
   put8(state, 0x94);
   put8(state, 0x87);
   put32(state, offset);
}

static inline void setne_m8rel(usf_state_t * state, unsigned char *m8)
{
   int offset = rel_r15_offset(state, m8, "setne_m8rel");

   put8(state, 0x41);
   put8(state, 0x0F);
   put8(state, 0x95);
   put8(state, 0x87);
   put32(state, offset);
}

static inline void setl_m8rel(usf_state_t * state, unsigned char *m8)
{
   int offset = rel_r15_offset(state, m8, "setl_m8rel");

   put8(state, 0x41);
   put8(state, 0x0F);
   put8(state, 0x9C);
   put8(state, 0x87);
   put32(state, offset);
}

static inline void setle_m8rel(usf_state_t * state, unsigned char *m8)
{
   int offset = rel_r15_offset(state, m8, "setle_m8rel");

   put8(state, 0x41);
   put8(state, 0x0F);
   put8(state, 0x9E);
   put8(state, 0x87);
   put32(state, offset);
}

static inline void setg_m8rel(usf_state_t * state, unsigned char *m8)
{
   int offset = rel_r15_offset(state, m8, "setg_m8rel");

   put8(state, 0x41);
   put8(state, 0x0F);
   put8(state, 0x9F);
   put8(state, 0x87);
   put32(state, offset);
}

static inline void setge_m8rel(usf_state_t * state, unsigned char *m8)
{
   int offset = rel_r15_offset(state, m8, "setge_m8rel");

   put8(state, 0x41);
   put8(state, 0x0F);
   put8(state, 0x9D);
   put8(state, 0x87);
   put32(state, offset);
}

static inline void setl_reg8(usf_state_t * state, unsigned int reg8)
{
   put8(state, 0x40);  /* we need an REX prefix to use the uniform byte registers */
   put8(state, 0x0F);
   put8(state, 0x9C);
   put8(state, 0xC0 | reg8);
}

static inline void setb_reg8(usf_state_t * state, unsigned int reg8)
{
   put8(state, 0x40);  /* we need an REX prefix to use the uniform byte registers */
   put8(state, 0x0F);
   put8(state, 0x92);
   put8(state, 0xC0 | reg8);
}

static inline void test_m32rel_imm32(usf_state_t * state, unsigned int *m32, unsigned int imm32)
{
   int offset = rel_r15_offset(state, m32, "test_m32rel_imm32");

   put8(state, 0x41);
   put8(state, 0xF7);
   put8(state, 0x87);
   put32(state, offset);
   put32(state, imm32);
}

static inline void add_m32rel_xreg32(usf_state_t * state, unsigned int *m32, int xreg32)
{
   int offset = rel_r15_offset(state, m32, "add_m32rel_xreg32");

   put8(state, 0x41 | ((xreg32 & 8) >> 1));
   put8(state, 0x01);
   put8(state, 0x87 | ((xreg32 & 7) << 3));
   put32(state, offset);
}

static inline void sub_xreg32_m32rel(usf_state_t * state, int xreg32, unsigned int *m32)
{
   int offset = rel_r15_offset(state, m32, "sub_xreg32_m32rel");

   put8(state, 0x41 | ((xreg32 & 8) >> 1));
   put8(state, 0x2B);
   put8(state, 0x87 | ((xreg32 & 7) << 3));
   put32(state, offset);
}

static inline void sub_reg32_reg32(usf_state_t * state, int reg1, int reg2)
{
   put8(state, 0x29);
   put8(state, (reg2 << 3) | reg1 | 0xC0);
}

static inline void sub_reg64_reg64(usf_state_t * state, int reg1, int reg2)
{
   put8(state, 0x48);
   put8(state, 0x29);
   put8(state, (reg2 << 3) | reg1 | 0xC0);
}

static inline void sub_reg64_imm32(usf_state_t * state, int reg64, unsigned int imm32)
{
   put8(state, 0x48);
   put8(state, 0x81);
   put8(state, 0xE8 + reg64);
   put32(state, imm32);
}

static inline void sub_eax_imm32(usf_state_t * state, unsigned int imm32)
{
   put8(state, 0x2D);
   put32(state, imm32);
}

static inline void jne_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x75);
   put8(state, saut);
}

static inline void je_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x74);
   put8(state, saut);
}

static inline void jbe_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x76);
   put8(state, saut);
}

static inline void ja_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x77);
   put8(state, saut);
}

static inline void jb_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x72);
   put8(state, saut);
}

static inline void jae_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x73);
   put8(state, saut);
}

static inline void jp_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x7A);
   put8(state, saut);
}

static inline void je_near_rj(usf_state_t * state, unsigned int saut)
{
   put8(state, 0x0F);
   put8(state, 0x84);
   put32(state, saut);
}

static inline void mov_reg32_imm32(usf_state_t * state, int reg32, unsigned int imm32)
{
   put8(state, 0xB8+reg32);
   put32(state, imm32);
}

static inline void mov_reg64_imm64(usf_state_t * state, int reg64, unsigned long long imm64)
{
   put8(state, 0x48);
   put8(state, 0xB8+reg64);
   put64(state, imm64);
}

static inline void jmp_imm_short(usf_state_t * state, char saut)
{
   put8(state, 0xEB);
   put8(state, saut);
}

static inline void or_m32rel_imm32(usf_state_t * state, unsigned int *m32, unsigned int imm32)
{
   int offset = rel_r15_offset(state, m32, "or_m32rel_imm32");

   put8(state, 0x41);
   put8(state, 0x81);
   put8(state, 0x8F);
   put32(state, offset);
   put32(state, imm32);
}

static inline void or_reg64_reg64(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x48);
   put8(state, 0x09);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static inline void and_reg64_reg64(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x48);
   put8(state, 0x21);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static inline void and_m32rel_imm32(usf_state_t * state, unsigned int *m32, unsigned int imm32)
{
   int offset = rel_r15_offset(state, m32, "and_m32rel_imm32");

   put8(state, 0x41);
   put8(state, 0x81);
   put8(state, 0xA7);
   put32(state, offset);
   put32(state, imm32);
}

static inline void xor_reg32_reg32(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x31);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static inline void xor_reg64_reg64(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x48);
   put8(state, 0x31);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static inline void add_reg64_imm32(usf_state_t * state, unsigned int reg64, unsigned int imm32)
{
   put8(state, 0x48);
   put8(state, 0x81);
   put8(state, 0xC0+reg64);
   put32(state, imm32);
}

static inline void add_reg32_imm32(usf_state_t * state, unsigned int reg32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0xC0+reg32);
   put32(state, imm32);
}

static inline void inc_m32rel(usf_state_t * state, unsigned int *m32)
{
   int offset = rel_r15_offset(state, m32, "inc_m32rel");

   put8(state, 0x41);
   put8(state, 0xFF);
   put8(state, 0x87);
   put32(state, offset);
}

static inline void cmp_m32rel_imm32(usf_state_t * state, unsigned int *m32, unsigned int imm32)
{
   int offset = rel_r15_offset(state, m32, "cmp_m32rel_imm32");

   put8(state, 0x41);
   put8(state, 0x81);
   put8(state, 0xBF);
   put32(state, offset);
   put32(state, imm32);
}

static inline void cmp_eax_imm32(usf_state_t * state, unsigned int imm32)
{
   put8(state, 0x3D);
   put32(state, imm32);
}

static inline void mov_m32rel_imm32(usf_state_t * state, unsigned int *m32, unsigned int imm32)
{
   int offset = rel_r15_offset(state, m32, "mov_m32rel_imm32");

   put8(state, 0x41);
   put8(state, 0xC7);
   put8(state, 0x87);
   put32(state, offset);
   put32(state, imm32);
}

static inline void jmp(usf_state_t * state, unsigned int mi_addr)
{
   put8(state, 0xFF);
   put8(state, 0x25);
   put32(state, 0);
   put64(state, 0);
   add_jump(state, state->code_length-8, mi_addr, 1);
}

static inline void cdq(usf_state_t * state)
{
   put8(state, 0x99);
}

static inline void call_reg64(usf_state_t * state, unsigned int reg64)
{
   put8(state, 0xFF);
   put8(state, 0xD0+reg64);
}

static inline void shr_reg64_imm8(usf_state_t * state, unsigned int reg64, unsigned char imm8)
{
   put8(state, 0x48);
   put8(state, 0xC1);
   put8(state, 0xE8+reg64);
   put8(state, imm8);
}

static inline void shr_reg32_imm8(usf_state_t * state, unsigned int reg32, unsigned char imm8)
{
   put8(state, 0xC1);
   put8(state, 0xE8+reg32);
   put8(state, imm8);
}

static inline void shr_reg32_cl(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xD3);
   put8(state, 0xE8+reg32);
}

static inline void shr_reg64_cl(usf_state_t * state, unsigned int reg64)
{
   put8(state, 0x48);
   put8(state, 0xD3);
   put8(state, 0xE8+reg64);
}

static inline void sar_reg32_cl(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xD3);
   put8(state, 0xF8+reg32);
}

static inline void sar_reg64_cl(usf_state_t * state, unsigned int reg64)
{
   put8(state, 0x48);
   put8(state, 0xD3);
   put8(state, 0xF8+reg64);
}

static inline void shl_reg32_cl(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xD3);
   put8(state, 0xE0+reg32);
}

static inline void shl_reg64_cl(usf_state_t * state, unsigned int reg64)
{
   put8(state, 0x48);
   put8(state, 0xD3);
   put8(state, 0xE0+reg64);
}

static inline void sar_reg32_imm8(usf_state_t * state, unsigned int reg32, unsigned char imm8)
{
   put8(state, 0xC1);
   put8(state, 0xF8+reg32);
   put8(state, imm8);
}

static inline void sar_reg64_imm8(usf_state_t * state, unsigned int reg64, unsigned char imm8)
{
   put8(state, 0x48);
   put8(state, 0xC1);
   put8(state, 0xF8+reg64);
   put8(state, imm8);
}

static inline void mul_m32rel(usf_state_t * state, unsigned int *m32)
{
   int offset = rel_r15_offset(state, m32, "mul_m32rel");

   put8(state, 0x41);
   put8(state, 0xF7);
   put8(state, 0xA7);
   put32(state, offset);
}

static inline void imul_reg32(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xF7);
   put8(state, 0xE8+reg32);
}

static inline void mul_reg64(usf_state_t * state, unsigned int reg64)
{
   put8(state, 0x48);
   put8(state, 0xF7);
   put8(state, 0xE0+reg64);
}

static inline void mul_reg32(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xF7);
   put8(state, 0xE0+reg32);
}

static inline void idiv_reg32(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xF7);
   put8(state, 0xF8+reg32);
}

static inline void div_reg32(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xF7);
   put8(state, 0xF0+reg32);
}

static inline void add_reg32_reg32(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x01);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static inline void add_reg64_reg64(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x48);
   put8(state, 0x01);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static inline void jmp_reg64(usf_state_t * state, unsigned int reg64)
{
   put8(state, 0xFF);
   put8(state, 0xE0 + reg64);
}

static inline void mov_reg32_preg64(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x8B);
   put8(state, (reg1 << 3) | reg2);
}

static inline void mov_preg64_reg32(usf_state_t * state, int reg1, int reg2)
{
   put8(state, 0x89);
   put8(state, (reg2 << 3) | reg1);
}

static inline void mov_reg64_preg64(usf_state_t * state, int reg1, int reg2)
{
   put8(state, 0x48);
   put8(state, 0x8B);
   put8(state, (reg1 << 3) | reg2);
}

static inline void mov_reg32_preg64preg64pimm32(usf_state_t * state, int reg1, int reg2, int reg3, unsigned int imm32)
{
   put8(state, 0x8B);
   put8(state, (reg1 << 3) | 0x84);
   put8(state, reg2 | (reg3 << 3));
   put32(state, imm32);
}

static inline void mov_preg64preg64pimm32_reg32(usf_state_t * state, int reg1, int reg2, unsigned int imm32, int reg3)
{
   put8(state, 0x89);
   put8(state, (reg3 << 3) | 0x84);
   put8(state, reg1 | (reg2 << 3));
   put32(state, imm32);
}

static inline void mov_reg64_preg64preg64pimm32(usf_state_t * state, int reg1, int reg2, int reg3, unsigned int imm32)
{
   put8(state, 0x48);
   put8(state, 0x8B);
   put8(state, (reg1 << 3) | 0x84);
   put8(state, reg2 | (reg3 << 3));
   put32(state, imm32);
}

static inline void mov_reg32_preg64preg64(usf_state_t * state, int reg1, int reg2, int reg3)
{
   put8(state, 0x8B);
   put8(state, (reg1 << 3) | 0x04);
   put8(state, (reg2 << 3) | reg3);
}

static inline void mov_reg64_preg64preg64(usf_state_t * state, int reg1, int reg2, int reg3)
{
   put8(state, 0x48);
   put8(state, 0x8B);
   put8(state, (reg1 << 3) | 0x04);
   put8(state, reg2 | (reg3 << 3));
}

static inline void mov_reg32_preg64pimm32(usf_state_t * state, int reg1, int reg2, unsigned int imm32)
{
   put8(state, 0x8B);
   put8(state, 0x80 | (reg1 << 3) | reg2);
   put32(state, imm32);
}

static inline void mov_reg64_preg64pimm32(usf_state_t * state, int reg1, int reg2, unsigned int imm32)
{
   put8(state, 0x48);
   put8(state, 0x8B);
   put8(state, 0x80 | (reg1 << 3) | reg2);
   put32(state, imm32);
}

static inline void mov_reg64_preg64pimm8(usf_state_t * state, int reg1, int reg2, unsigned int imm8)
{
   put8(state, 0x48);
   put8(state, 0x8B);
   put8(state, 0x40 | (reg1 << 3) | reg2);
   put8(state, imm8);
}

static inline void mov_reg64_preg64x8preg64(usf_state_t * state, int reg1, int reg2, int reg3)
{
   put8(state, 0x48);
   put8(state, 0x8B);
   put8(state, (reg1 << 3) | 4);
   put8(state, 0xC0 | (reg2 << 3) | reg3);
}

static inline void mov_preg64preg64_reg8(usf_state_t * state, int reg1, int reg2, int reg8)
{
   put8(state, 0x88);
   put8(state, 0x04 | (reg8 << 3));
   put8(state, (reg1 << 3) | reg2);
}

static inline void mov_preg64preg64_imm8(usf_state_t * state, int reg1, int reg2, unsigned char imm8)
{
   put8(state, 0xC6);
   put8(state, 0x04);
   put8(state, (reg1 << 3) | reg2);
   put8(state, imm8);
}

static inline void mov_preg64preg64_reg16(usf_state_t * state, int reg1, int reg2, int reg16)
{
   put8(state, 0x66);
   put8(state, 0x89);
   put8(state, 0x04 | (reg16 << 3));
   put8(state, (reg1 << 3) | reg2);
}

static inline void mov_preg64preg64_reg32(usf_state_t * state, int reg1, int reg2, int reg32)
{
   put8(state, 0x89);
   put8(state, 0x04 | (reg32 << 3));
   put8(state, (reg1 << 3) | reg2);
}

static inline void mov_preg64pimm32_reg32(usf_state_t * state, int reg1, unsigned int imm32, int reg2)
{
   put8(state, 0x89);
   put8(state, 0x80 | reg1 | (reg2 << 3));
   put32(state, imm32);
}

static inline void mov_preg64pimm8_reg64(usf_state_t * state, int reg1, unsigned int imm8, int reg2)
{
   put8(state, 0x48);
   put8(state, 0x89);
   put8(state, 0x40 | (reg2 << 3) | reg1);
   put8(state, imm8);
}

static inline void add_eax_imm32(usf_state_t * state, unsigned int imm32)
{
   put8(state, 0x05);
   put32(state, imm32);
}

static inline void shl_reg32_imm8(usf_state_t * state, unsigned int reg32, unsigned char imm8)
{
   put8(state, 0xC1);
   put8(state, 0xE0 + reg32);
   put8(state, imm8);
}

static inline void shl_reg64_imm8(usf_state_t * state, unsigned int reg64, unsigned char imm8)
{
   put8(state, 0x48);
   put8(state, 0xC1);
   put8(state, 0xE0 + reg64);
   put8(state, imm8);
}

static inline void mov_reg32_reg32(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   if (reg1 == reg2) return;
   put8(state, 0x89);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static inline void mov_reg64_reg64(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   if (reg1 == reg2) return;
   put8(state, 0x48 | ((reg1 & 8) >> 3) | ((reg2 & 8) >> 1));
   put8(state, 0x89);
   put8(state, 0xC0 | ((reg2 & 7) << 3) | (reg1 & 7));
}

static inline void mov_xreg32_m32rel(usf_state_t * state, unsigned int xreg32, unsigned int *m32)
{
   int offset = rel_r15_offset(state, m32, "mov_xreg32_m32rel");

   put8(state, 0x41 | ((xreg32 & 8) >> 1));
   put8(state, 0x8B);
   put8(state, 0x87 | ((xreg32 & 7) << 3));
   put32(state, offset);
}

static inline void mov_m32rel_xreg32(usf_state_t * state, unsigned int *m32, unsigned int xreg32)
{
   int offset = rel_r15_offset(state, m32, "mov_m32rel_xreg32");

   put8(state, 0x41 | ((xreg32 & 8) >> 1));
   put8(state, 0x89);
   put8(state, 0x87 | ((xreg32 & 7) << 3));
   put32(state, offset);
}

static inline void mov_xreg64_m64rel(usf_state_t * state, unsigned int xreg64, unsigned long long* m64)
{
   int offset = rel_r15_offset(state, m64, "mov_xreg64_m64rel");

   put8(state, 0x49 | ((xreg64 & 8) >> 1));
   put8(state, 0x8B);
   put8(state, 0x87 | ((xreg64 & 7) << 3));
   put32(state, offset);
}

static inline void mov_m64rel_xreg64(usf_state_t * state, unsigned long long *m64, unsigned int xreg64)
{
   int offset = rel_r15_offset(state, m64, "mov_m64rel_xreg64");

   put8(state, 0x49 | ((xreg64 & 8) >> 1));
   put8(state, 0x89);
   put8(state, 0x87 | ((xreg64 & 7) << 3));
   put32(state, offset);
}

static inline void mov_xreg8_m8rel(usf_state_t * state, int xreg8, unsigned char *m8)
{
   int offset = rel_r15_offset(state, m8, "mov_xreg8_m8rel");

   put8(state, 0x41 | ((xreg8 & 8) >> 1));
   put8(state, 0x8A);
   put8(state, 0x87 | ((xreg8 & 7) << 3));
   put32(state, offset);
}

static inline void and_eax_imm32(usf_state_t * state, unsigned int imm32)
{
   put8(state, 0x25);
   put32(state, imm32);
}

static inline void or_reg64_imm32(usf_state_t * state, int reg64, unsigned int imm32)
{
   put8(state, 0x48);
   put8(state, 0x81);
   put8(state, 0xC8 + reg64);
   put32(state, imm32);
}

static inline void and_reg32_imm32(usf_state_t * state, int reg32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0xE0 + reg32);
   put32(state, imm32);
}

static inline void and_reg64_imm32(usf_state_t * state, int reg64, unsigned int imm32)
{
   put8(state, 0x48);
   put8(state, 0x81);
   put8(state, 0xE0 + reg64);
   put32(state, imm32);
}

static inline void and_reg64_imm8(usf_state_t * state, int reg64, unsigned char imm8)
{
   put8(state, 0x48);
   put8(state, 0x83);
   put8(state, 0xE0 + reg64);
   put8(state, imm8);
}

static inline void xor_reg64_imm32(usf_state_t * state, int reg64, unsigned int imm32)
{
   put8(state, 0x48);
   put8(state, 0x81);
   put8(state, 0xF0 + reg64);
   put32(state, imm32);
}

static inline void xor_reg8_imm8(usf_state_t * state, int reg8, unsigned char imm8)
{
   put8(state, 0x40);  /* we need an REX prefix to use the uniform byte registers */
   put8(state, 0x80);
   put8(state, 0xF0 + reg8);
   put8(state, imm8);
}

static inline void not_reg64(usf_state_t * state, unsigned int reg64)
{
   put8(state, 0x48);
   put8(state, 0xF7);
   put8(state, 0xD0 + reg64);
}

static inline void neg_reg32(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xF7);
   put8(state, 0xD8 + reg32);
}

static inline void neg_reg64(usf_state_t * state, unsigned int reg64)
{
   put8(state, 0x48);
   put8(state, 0xF7);
   put8(state, 0xD8 + reg64);
}

static inline void movsx_xreg32_m8rel(usf_state_t * state, int xreg32, unsigned char *m8)
{
   int offset = rel_r15_offset(state, m8, "movsx_xreg32_m8rel");

   put8(state, 0x41 | ((xreg32 & 8) >> 1));
   put8(state, 0x0F);
   put8(state, 0xBE);
   put8(state, 0x87 | ((xreg32 & 7) << 3));
   put32(state, offset);
}

static inline void movsx_reg32_8preg64preg64(usf_state_t * state, int reg1, int reg2, int reg3)
{
   put8(state, 0x0F);
   put8(state, 0xBE);
   put8(state, (reg1 << 3) | 0x04);
   put8(state, (reg2 << 3) | reg3);
}

static inline void movsx_reg32_16preg64preg64(usf_state_t * state, int reg1, int reg2, int reg3)
{
   put8(state, 0x0F);
   put8(state, 0xBF);
   put8(state, (reg1 << 3) | 0x04);
   put8(state, (reg2 << 3) | reg3);
}

static inline void movsx_xreg32_m16rel(usf_state_t * state, int xreg32, unsigned short *m16)
{
   int offset = rel_r15_offset(state, m16, "movsx_xreg32_m16rel");

   put8(state, 0x41 | ((xreg32 & 8) >> 1));
   put8(state, 0x0F);
   put8(state, 0xBF);
   put8(state, 0x87 | ((xreg32 & 7) << 3));
   put32(state, offset);
}

static inline void movsxd_reg64_reg32(usf_state_t * state, int reg64, int reg32)
{
   put8(state, 0x48);
   put8(state, 0x63);
   put8(state, (reg64 << 3) | reg32 | 0xC0);
}

static inline void fldcw_m16rel(usf_state_t * state, unsigned short *m16)
{
   int offset = rel_r15_offset(state, m16, "fldcw_m16rel");

   put8(state, 0x41);
   put8(state, 0xD9);
   put8(state, 0xAF);
   put32(state, offset);
}

static inline void fld_preg64_dword(usf_state_t * state, int reg64)
{
   put8(state, 0xD9);
   put8(state, reg64);
}

static inline void fdiv_preg64_dword(usf_state_t * state, int reg64)
{
   put8(state, 0xD8);
   put8(state, 0x30 + reg64);
}

static inline void fstp_preg64_dword(usf_state_t * state, int reg64)
{
   put8(state, 0xD9);
   put8(state, 0x18 + reg64);
}

static inline void fchs(usf_state_t * state)
{
   put8(state, 0xD9);
   put8(state, 0xE0);
}

static inline void fstp_preg64_qword(usf_state_t * state, int reg64)
{
   put8(state, 0xDD);
   put8(state, 0x18 + reg64);
}

static inline void fadd_preg64_dword(usf_state_t * state, int reg64)
{
   put8(state, 0xD8);
   put8(state, reg64);
}

static inline void fsub_preg64_dword(usf_state_t * state, int reg64)
{
   put8(state, 0xD8);
   put8(state, 0x20 + reg64);
}

static inline void fmul_preg64_dword(usf_state_t * state, int reg64)
{
   put8(state, 0xD8);
   put8(state, 0x08 + reg64);
}

static inline void fistp_preg64_dword(usf_state_t * state, int reg64)
{
   put8(state, 0xDB);
   put8(state, 0x18 + reg64);
}

static inline void fistp_preg64_qword(usf_state_t * state, int reg64)
{
   put8(state, 0xDF);
   put8(state, 0x38 + reg64);
}

static inline void fld_preg64_qword(usf_state_t * state, int reg64)
{
   put8(state, 0xDD);
   put8(state, reg64);
}

static inline void fild_preg64_qword(usf_state_t * state, int reg64)
{
   put8(state, 0xDF);
   put8(state, 0x28+reg64);
}

static inline void fild_preg64_dword(usf_state_t * state, int reg64)
{
   put8(state, 0xDB);
   put8(state, reg64);
}

static inline void fadd_preg64_qword(usf_state_t * state, int reg64)
{
   put8(state, 0xDC);
   put8(state, reg64);
}

static inline void fdiv_preg64_qword(usf_state_t * state, int reg64)
{
   put8(state, 0xDC);
   put8(state, 0x30 + reg64);
}

static inline void fsub_preg64_qword(usf_state_t * state, int reg64)
{
   put8(state, 0xDC);
   put8(state, 0x20 + reg64);
}

static inline void fmul_preg64_qword(usf_state_t * state, int reg64)
{
   put8(state, 0xDC);
   put8(state, 0x08 + reg64);
}

static inline void fsqrt(usf_state_t * state)
{
   put8(state, 0xD9);
   put8(state, 0xFA);
}

static inline void fabs_(usf_state_t * state)
{
   put8(state, 0xD9);
   put8(state, 0xE1);
}

static inline void fcomip_fpreg(usf_state_t * state, int fpreg)
{
   put8(state, 0xDF);
   put8(state, 0xF0 + fpreg);
}

static inline void fucomip_fpreg(usf_state_t * state, int fpreg)
{
   put8(state, 0xDF);
   put8(state, 0xE8 + fpreg);
}

static inline void ffree_fpreg(usf_state_t * state, int fpreg)
{
   put8(state, 0xDD);
   put8(state, 0xC0 + fpreg);
}

#endif /* M64P_R4300_ASSEMBLE_H */

