/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - assemble.h                                              *
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

#ifndef M64P_R4300_ASSEMBLE_H
#define M64P_R4300_ASSEMBLE_H

#include "r4300/recomph.h"
#include "api/callbacks.h"
#include "osal/preproc.h"

#include <stdlib.h>

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

// osal_fastcall parameter
#ifdef _MSC_VER
#define RP0 ECX // fastcall
#else
#define RP0 EAX // regparm(1)
#endif

void jump_start_rel8(usf_state_t *);
void jump_end_rel8(usf_state_t *);
void jump_start_rel32(usf_state_t *);
void jump_end_rel32(usf_state_t *);
void add_jump(usf_state_t *, unsigned int pc_addr, unsigned int mi_addr);

static osal_inline void put8(usf_state_t * state, unsigned char octet)
{
   (*state->inst_pointer)[state->code_length] = octet;
   state->code_length++;
   if (state->code_length == state->max_code_length)
   {
     *state->inst_pointer = (unsigned char *) realloc_exec(state, *state->inst_pointer, state->max_code_length, state->max_code_length+8192);
     state->max_code_length += 8192;
   }
}

static osal_inline void put32(usf_state_t * state, unsigned int dword)
{
   if ((state->code_length+4) >= state->max_code_length)
   {
     *state->inst_pointer = (unsigned char *) realloc_exec(state, *state->inst_pointer, state->max_code_length, state->max_code_length+8192);
     state->max_code_length += 8192;
   }
   *((unsigned int *)(&(*state->inst_pointer)[state->code_length])) = dword;
   state->code_length+=4;
}

static osal_inline void push_reg32(usf_state_t * state, int reg32)
{
    put8(state, 0x50 + reg32);
}

static osal_inline void pop_reg32(usf_state_t * state, int reg32)
{
    put8(state, 0x58 + reg32);
}

static osal_inline void mov_eax_memoffs32(usf_state_t * state, unsigned int *memoffs32)
{
   put8(state, 0xA1);
   put32(state, (unsigned int)(memoffs32));
}

static osal_inline void mov_memoffs32_eax(usf_state_t * state, unsigned int *memoffs32)
{
   put8(state, 0xA3);
   put32(state, (unsigned int)(memoffs32));
}

static osal_inline void mov_m8_reg8(usf_state_t * state, unsigned char *m8, int reg8)
{
   put8(state, 0x88);
   put8(state, (reg8 << 3) | 5);
   put32(state, (unsigned int)(m8));
}

static osal_inline void mov_reg16_m16(usf_state_t * state, int reg16, unsigned short *m16)
{
   put8(state, 0x66);
   put8(state, 0x8B);
   put8(state, (reg16 << 3) | 5);
   put32(state, (unsigned int)(m16));
}

static osal_inline void mov_m16_reg16(usf_state_t * state, unsigned short *m16, int reg16)
{
   put8(state, 0x66);
   put8(state, 0x89);
   put8(state, (reg16 << 3) | 5);
   put32(state, (unsigned int)(m16));
}

static osal_inline void cmp_reg32_m32(usf_state_t * state, int reg32, unsigned int *m32)
{
   put8(state, 0x3B);
   put8(state, (reg32 << 3) | 5);
   put32(state, (unsigned int)(m32));
}

static osal_inline void cmp_reg32_reg32(usf_state_t * state, int reg1, int reg2)
{
   put8(state, 0x39);
   put8(state, (reg2 << 3) | reg1 | 0xC0);
}

static osal_inline void cmp_reg32_imm8(usf_state_t * state, int reg32, unsigned char imm8)
{
   put8(state, 0x83);
   put8(state, 0xF8 + reg32);
   put8(state, imm8);
}

static osal_inline void cmp_preg32pimm32_imm8(usf_state_t * state, int reg32, unsigned int imm32, unsigned char imm8)
{
   put8(state, 0x80);
   put8(state, 0xB8 + reg32);
   put32(state, imm32);
   put8(state, imm8);
}

static osal_inline void cmp_reg32_imm32(usf_state_t * state, int reg32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0xF8 + reg32);
   put32(state, imm32);
}

static osal_inline void test_reg32_imm32(usf_state_t * state, int reg32, unsigned int imm32)
{
   put8(state, 0xF7);
   put8(state, 0xC0 + reg32);
   put32(state, imm32);
}

static osal_inline void test_m32_imm32(usf_state_t * state, unsigned int *m32, unsigned int imm32)
{
   put8(state, 0xF7);
   put8(state, 0x05);
   put32(state, (unsigned int)m32);
   put32(state, imm32);
}

static osal_inline void add_m32_reg32(usf_state_t * state, unsigned int *m32, int reg32)
{
   put8(state, 0x01);
   put8(state, (reg32 << 3) | 5);
   put32(state, (unsigned int)(m32));
}

static osal_inline void sub_reg32_m32(usf_state_t * state, int reg32, unsigned int *m32)
{
   put8(state, 0x2B);
   put8(state, (reg32 << 3) | 5);
   put32(state, (unsigned int)(m32));
}

static osal_inline void sub_reg32_reg32(usf_state_t * state, int reg1, int reg2)
{
   put8(state, 0x29);
   put8(state, (reg2 << 3) | reg1 | 0xC0);
}

static osal_inline void sbb_reg32_reg32(usf_state_t * state, int reg1, int reg2)
{
   put8(state, 0x19);
   put8(state, (reg2 << 3) | reg1 | 0xC0);
}

static osal_inline void sub_reg32_imm32(usf_state_t * state, int reg32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0xE8 + reg32);
   put32(state, imm32);
}

static osal_inline void sub_eax_imm32(usf_state_t * state, unsigned int imm32)
{
   put8(state, 0x2D);
   put32(state, imm32);
}

static osal_inline void jne_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x75);
   put8(state, saut);
}

static osal_inline void je_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x74);
   put8(state, saut);
}

static osal_inline void jb_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x72);
   put8(state, saut);
}

static osal_inline void jbe_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x76);
   put8(state, saut);
}

static osal_inline void ja_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x77);
   put8(state, saut);
}

static osal_inline void jae_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x73);
   put8(state, saut);
}

static osal_inline void jle_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x7E);
   put8(state, saut);
}

static osal_inline void jge_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x7D);
   put8(state, saut);
}

static osal_inline void jg_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x7F);
   put8(state, saut);
}

static osal_inline void jl_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x7C);
   put8(state, saut);
}

static osal_inline void jp_rj(usf_state_t * state, unsigned char saut)
{
   put8(state, 0x7A);
   put8(state, saut);
}

static osal_inline void je_near_rj(usf_state_t * state, unsigned int saut)
{
   put8(state, 0x0F);
   put8(state, 0x84);
   put32(state, saut);
}

static osal_inline void mov_reg32_imm32(usf_state_t * state, int reg32, unsigned int imm32)
{
   put8(state, 0xB8+reg32);
   put32(state, imm32);
}

static osal_inline void jmp_imm_short(usf_state_t * state, char saut)
{
   put8(state, 0xEB);
   put8(state, saut);
}

static osal_inline void or_m32_imm32(usf_state_t * state, unsigned int *m32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0x0D);
   put32(state, (unsigned int)(m32));
   put32(state, imm32);
}

static osal_inline void or_reg32_reg32(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x09);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static osal_inline void and_reg32_reg32(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x21);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static osal_inline void and_m32_imm32(usf_state_t * state, unsigned int *m32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0x25);
   put32(state, (unsigned int)(m32));
   put32(state, imm32);
}

static osal_inline void xor_reg32_reg32(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x31);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static osal_inline void sub_m32_imm32(usf_state_t * state, unsigned int *m32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0x2D);
   put32(state, (unsigned int)(m32));
   put32(state, imm32);
}

static osal_inline void add_reg32_imm8(usf_state_t * state, unsigned int reg32, unsigned char imm8)
{
    put8(state, 0x83);
    put8(state, 0xC0+reg32);
    put8(state, imm8);
}

static osal_inline void add_reg32_imm32(usf_state_t * state, unsigned int reg32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0xC0+reg32);
   put32(state, imm32);
}

static osal_inline void inc_m32(usf_state_t * state, unsigned int *m32)
{
   put8(state, 0xFF);
   put8(state, 0x05);
   put32(state, (unsigned int)(m32));
}

static osal_inline void cmp_m32_imm32(usf_state_t * state, unsigned int *m32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0x3D);
   put32(state, (unsigned int)(m32));
   put32(state, imm32);
}

static osal_inline void cmp_eax_imm32(usf_state_t * state, unsigned int imm32)
{
   put8(state, 0x3D);
   put32(state, imm32);
}

static osal_inline void mov_m32_imm32(usf_state_t * state, unsigned int *m32, unsigned int imm32)
{
   put8(state, 0xC7);
   put8(state, 0x05);
   put32(state, (unsigned int)(m32));
   put32(state, imm32);
}

static osal_inline void jmp(usf_state_t * state, unsigned int mi_addr)
{
   put8(state, 0xE9);
   put32(state, 0);
   add_jump(state, state->code_length-4, mi_addr);
}

static osal_inline void cdq(usf_state_t * state)
{
   put8(state, 0x99);
}

static osal_inline void mov_m32_reg32(usf_state_t * state, unsigned int *m32, unsigned int reg32)
{
   put8(state, 0x89);
   put8(state, (reg32 << 3) | 5);
   put32(state, (unsigned int)(m32));
}

static osal_inline void call_reg32(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xFF);
   put8(state, 0xD0+reg32);
}

static osal_inline void shr_reg32_imm8(usf_state_t * state, unsigned int reg32, unsigned char imm8)
{
   put8(state, 0xC1);
   put8(state, 0xE8+reg32);
   put8(state, imm8);
}

static osal_inline void shr_reg32_cl(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xD3);
   put8(state, 0xE8+reg32);
}

static osal_inline void sar_reg32_cl(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xD3);
   put8(state, 0xF8+reg32);
}

static osal_inline void shl_reg32_cl(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xD3);
   put8(state, 0xE0+reg32);
}

static osal_inline void shld_reg32_reg32_cl(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x0F);
   put8(state, 0xA5);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static osal_inline void shld_reg32_reg32_imm8(usf_state_t * state, unsigned int reg1, unsigned int reg2, unsigned char imm8)
{
   put8(state, 0x0F);
   put8(state, 0xA4);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
   put8(state, imm8);
}

static osal_inline void shrd_reg32_reg32_cl(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x0F);
   put8(state, 0xAD);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static osal_inline void sar_reg32_imm8(usf_state_t * state, unsigned int reg32, unsigned char imm8)
{
   put8(state, 0xC1);
   put8(state, 0xF8+reg32);
   put8(state, imm8);
}

static osal_inline void shrd_reg32_reg32_imm8(usf_state_t * state, unsigned int reg1, unsigned int reg2, unsigned char imm8)
{
   put8(state, 0x0F);
   put8(state, 0xAC);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
   put8(state, imm8);
}

static osal_inline void mul_m32(usf_state_t * state, unsigned int *m32)
{
   put8(state, 0xF7);
   put8(state, 0x25);
   put32(state, (unsigned int)(m32));
}

static osal_inline void imul_reg32(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xF7);
   put8(state, 0xE8+reg32);
}

static osal_inline void mul_reg32(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xF7);
   put8(state, 0xE0+reg32);
}

static osal_inline void idiv_reg32(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xF7);
   put8(state, 0xF8+reg32);
}

static osal_inline void div_reg32(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xF7);
   put8(state, 0xF0+reg32);
}

static osal_inline void add_reg32_reg32(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x01);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static osal_inline void adc_reg32_reg32(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x11);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static osal_inline void add_reg32_m32(usf_state_t * state, unsigned int reg32, unsigned int *m32)
{
   put8(state, 0x03);
   put8(state, (reg32 << 3) | 5);
   put32(state, (unsigned int)(m32));
}

static osal_inline void adc_reg32_imm32(usf_state_t * state, unsigned int reg32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0xD0 + reg32);
   put32(state, imm32);
}

static osal_inline void jmp_reg32(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xFF);
   put8(state, 0xE0 + reg32);
}

static osal_inline void mov_reg32_preg32(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   put8(state, 0x8B);
   put8(state, (reg1 << 3) | reg2);
}

static osal_inline void mov_preg32_reg32(usf_state_t * state, int reg1, int reg2)
{
   put8(state, 0x89);
   put8(state, (reg2 << 3) | reg1);
}

static osal_inline void mov_reg32_preg32preg32pimm32(usf_state_t * state, int reg1, int reg2, int reg3, unsigned int imm32)
{
   put8(state, 0x8B);
   put8(state, (reg1 << 3) | 0x84);
   put8(state, reg2 | (reg3 << 3));
   put32(state, imm32);
}

static osal_inline void mov_reg32_preg32pimm32(usf_state_t * state, int reg1, int reg2, unsigned int imm32)
{
   put8(state, 0x8B);
   put8(state, 0x80 | (reg1 << 3) | reg2);
   put32(state, imm32);
}

static osal_inline void mov_reg32_preg32x4pimm32(usf_state_t * state, int reg1, int reg2, unsigned int imm32)
{
   put8(state, 0x8B);
   put8(state, (reg1 << 3) | 4);
   put8(state, 0x80 | (reg2 << 3) | 5);
   put32(state, imm32);
}

static osal_inline void mov_preg32pimm32_reg8(usf_state_t * state, int reg32, unsigned int imm32, int reg8)
{
   put8(state, 0x88);
   put8(state, 0x80 | reg32 | (reg8 << 3));
   put32(state, imm32);
}

static osal_inline void mov_preg32pimm32_imm8(usf_state_t * state, int reg32, unsigned int imm32, unsigned char imm8)
{
   put8(state, 0xC6);
   put8(state, 0x80 + reg32);
   put32(state, imm32);
   put8(state, imm8);
}

static osal_inline void mov_preg32pimm32_reg16(usf_state_t * state, int reg32, unsigned int imm32, int reg16)
{
   put8(state, 0x66);
   put8(state, 0x89);
   put8(state, 0x80 | reg32 | (reg16 << 3));
   put32(state, imm32);
}

static osal_inline void mov_preg32pimm32_reg32(usf_state_t * state, int reg1, unsigned int imm32, int reg2)
{
   put8(state, 0x89);
   put8(state, 0x80 | reg1 | (reg2 << 3));
   put32(state, imm32);
}

static osal_inline void add_eax_imm32(usf_state_t * state, unsigned int imm32)
{
   put8(state, 0x05);
   put32(state, imm32);
}

static osal_inline void shl_reg32_imm8(usf_state_t * state, unsigned int reg32, unsigned char imm8)
{
   put8(state, 0xC1);
   put8(state, 0xE0 + reg32);
   put8(state, imm8);
}

static osal_inline void mov_reg32_m32(usf_state_t * state, unsigned int reg32, unsigned int* m32)
{
   put8(state, 0x8B);
   put8(state, (reg32 << 3) | 5);
   put32(state, (unsigned int)(m32));
}

static osal_inline void mov_reg8_m8(usf_state_t * state, int reg8, unsigned char *m8)
{
   put8(state, 0x8A);
   put8(state, (reg8 << 3) | 5);
   put32(state, (unsigned int)(m8));
}

static osal_inline void and_eax_imm32(usf_state_t * state, unsigned int imm32)
{
   put8(state, 0x25);
   put32(state, imm32);
}

static osal_inline void and_reg32_imm32(usf_state_t * state, int reg32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0xE0 + reg32);
   put32(state, imm32);
}

static osal_inline void or_reg32_imm32(usf_state_t * state, int reg32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0xC8 + reg32);
   put32(state, imm32);
}

static osal_inline void xor_reg32_imm32(usf_state_t * state, int reg32, unsigned int imm32)
{
   put8(state, 0x81);
   put8(state, 0xF0 + reg32);
   put32(state, imm32);
}

static osal_inline void xor_reg8_imm8(usf_state_t * state, int reg8, unsigned char imm8)
{
   put8(state, 0x80);
   put8(state, 0xF0 + reg8);
   put8(state, imm8);
}

static osal_inline void mov_reg32_reg32(usf_state_t * state, unsigned int reg1, unsigned int reg2)
{
   if (reg1 == reg2) return;
   put8(state, 0x89);
   put8(state, 0xC0 | (reg2 << 3) | reg1);
}

static osal_inline void not_reg32(usf_state_t * state, unsigned int reg32)
{
   put8(state, 0xF7);
   put8(state, 0xD0 + reg32);
}

static osal_inline void movsx_reg32_m8(usf_state_t * state, int reg32, unsigned char *m8)
{
   put8(state, 0x0F);
   put8(state, 0xBE);
   put8(state, (reg32 << 3) | 5);
   put32(state, (unsigned int)(m8));
}

static osal_inline void movsx_reg32_8preg32pimm32(usf_state_t * state, int reg1, int reg2, unsigned int imm32)
{
   put8(state, 0x0F);
   put8(state, 0xBE);
   put8(state, (reg1 << 3) | reg2 | 0x80);
   put32(state, imm32);
}

static osal_inline void movsx_reg32_16preg32pimm32(usf_state_t * state, int reg1, int reg2, unsigned int imm32)
{
   put8(state, 0x0F);
   put8(state, 0xBF);
   put8(state, (reg1 << 3) | reg2 | 0x80);
   put32(state, imm32);
}

static osal_inline void movsx_reg32_m16(usf_state_t * state, int reg32, unsigned short *m16)
{
   put8(state, 0x0F);
   put8(state, 0xBF);
   put8(state, (reg32 << 3) | 5);
   put32(state, (unsigned int)(m16));
}

static osal_inline void fldcw_m16(usf_state_t * state, unsigned short *m16)
{
   put8(state, 0xD9);
   put8(state, 0x2D);
   put32(state, (unsigned int)(m16));
}

static osal_inline void fld_preg32_dword(usf_state_t * state, int reg32)
{
   put8(state, 0xD9);
   put8(state, reg32);
}

static osal_inline void fdiv_preg32_dword(usf_state_t * state, int reg32)
{
   put8(state, 0xD8);
   put8(state, 0x30 + reg32);
}

static osal_inline void fstp_preg32_dword(usf_state_t * state, int reg32)
{
   put8(state, 0xD9);
   put8(state, 0x18 + reg32);
}

static osal_inline void fchs(usf_state_t * state)
{
   put8(state, 0xD9);
   put8(state, 0xE0);
}

static osal_inline void fstp_preg32_qword(usf_state_t * state, int reg32)
{
   put8(state, 0xDD);
   put8(state, 0x18 + reg32);
}

static osal_inline void fadd_preg32_dword(usf_state_t * state, int reg32)
{
   put8(state, 0xD8);
   put8(state, reg32);
}

static osal_inline void fsub_preg32_dword(usf_state_t * state, int reg32)
{
   put8(state, 0xD8);
   put8(state, 0x20 + reg32);
}

static osal_inline void fmul_preg32_dword(usf_state_t * state, int reg32)
{
   put8(state, 0xD8);
   put8(state, 0x08 + reg32);
}

static osal_inline void fistp_preg32_dword(usf_state_t * state, int reg32)
{
   put8(state, 0xDB);
   put8(state, 0x18 + reg32);
}

static osal_inline void fistp_preg32_qword(usf_state_t * state, int reg32)
{
   put8(state, 0xDF);
   put8(state, 0x38 + reg32);
}

static osal_inline void fld_preg32_qword(usf_state_t * state, int reg32)
{
   put8(state, 0xDD);
   put8(state, reg32);
}

static osal_inline void fild_preg32_qword(usf_state_t * state, int reg32)
{
   put8(state, 0xDF);
   put8(state, 0x28+reg32);
}

static osal_inline void fild_preg32_dword(usf_state_t * state, int reg32)
{
   put8(state, 0xDB);
   put8(state, reg32);
}

static osal_inline void fadd_preg32_qword(usf_state_t * state, int reg32)
{
   put8(state, 0xDC);
   put8(state, reg32);
}

static osal_inline void fdiv_preg32_qword(usf_state_t * state, int reg32)
{
   put8(state, 0xDC);
   put8(state, 0x30 + reg32);
}

static osal_inline void fsub_preg32_qword(usf_state_t * state, int reg32)
{
   put8(state, 0xDC);
   put8(state, 0x20 + reg32);
}

static osal_inline void fmul_preg32_qword(usf_state_t * state, int reg32)
{
   put8(state, 0xDC);
   put8(state, 0x08 + reg32);
}

static osal_inline void fsqrt(usf_state_t * state)
{
   put8(state, 0xD9);
   put8(state, 0xFA);
}

static osal_inline void fabs_(usf_state_t * state)
{
   put8(state, 0xD9);
   put8(state, 0xE1);
}

static osal_inline void fcomip_fpreg(usf_state_t * state, int fpreg)
{
   put8(state, 0xDF);
   put8(state, 0xF0 + fpreg);
}

static osal_inline void fucomip_fpreg(usf_state_t * state, int fpreg)
{
   put8(state, 0xDF);
   put8(state, 0xE8 + fpreg);
}

static osal_inline void ffree_fpreg(usf_state_t * state, int fpreg)
{
   put8(state, 0xDD);
   put8(state, 0xC0 + fpreg);
}

#endif /* M64P_R4300_ASSEMBLE_H */

