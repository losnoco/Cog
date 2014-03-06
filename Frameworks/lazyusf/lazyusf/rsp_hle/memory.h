/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - memory.h                                        *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
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

#ifndef MEMORY_H
#define MEMORY_H

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "../usf.h"
#include "../usf_internal.h"

#include "plugin.h"

#ifdef M64P_BIG_ENDIAN
#define S 0
#define S16 0
#define S8 0
#else
#define S 1
#define S16 2
#define S8 3
#endif

enum {
    TASK_TYPE               = 0xfc0,
    TASK_FLAGS              = 0xfc4,
    TASK_UCODE_BOOT         = 0xfc8,
    TASK_UCODE_BOOT_SIZE    = 0xfcc,
    TASK_UCODE              = 0xfd0,
    TASK_UCODE_SIZE         = 0xfd4,
    TASK_UCODE_DATA         = 0xfd8,
    TASK_UCODE_DATA_SIZE    = 0xfdc,
    TASK_DRAM_STACK         = 0xfe0,
    TASK_DRAM_STACK_SIZE    = 0xfe4,
    TASK_OUTPUT_BUFF        = 0xfe8,
    TASK_OUTPUT_BUFF_SIZE   = 0xfec,
    TASK_DATA_PTR           = 0xff0,
    TASK_DATA_SIZE          = 0xff4,
    TASK_YIELD_DATA_PTR     = 0xff8,
    TASK_YIELD_DATA_SIZE    = 0xffc
};

static inline unsigned int align(unsigned int x, unsigned amount)
{
    --amount;
    return (x + amount) & ~amount;
}

static inline uint8_t* const dmem_u8(usf_state_t* state, uint16_t address)
{
    return (uint8_t*)(&state->DMEM[(address & 0xfff) ^ S8]);
}

static inline uint16_t* const dmem_u16(usf_state_t* state, uint16_t address)
{
    assert((address & 1) == 0);
    return (uint16_t*)(&state->DMEM[(address & 0xfff) ^ S16]);
}

static inline uint32_t* const dmem_u32(usf_state_t* state, uint16_t address)
{
    assert((address & 3) == 0);
    return (uint32_t*)(&state->DMEM[(address & 0xfff)]);
}

static inline uint8_t* const dram_u8(usf_state_t* state, uint32_t address)
{
    return (uint8_t*)&state->N64MEM[(address & 0xffffff) ^ S8];
}

static inline uint16_t* const dram_u16(usf_state_t* state, uint32_t address)
{
    assert((address & 1) == 0);
    return (uint16_t*)&state->N64MEM[(address & 0xffffff) ^ S16];
}

static inline uint32_t* const dram_u32(usf_state_t* state, uint32_t address)
{
    assert((address & 3) == 0);
    return (uint32_t*)&state->N64MEM[address & 0xffffff];
}

void dmem_load_u8 (usf_state_t* state, uint8_t*  dst, uint16_t address, size_t count);
void dmem_load_u16(usf_state_t* state, uint16_t* dst, uint16_t address, size_t count);
void dmem_load_u32(usf_state_t* state, uint32_t* dst, uint16_t address, size_t count);
void dmem_store_u8 (usf_state_t* state, const uint8_t*  src, uint16_t address, size_t count);
void dmem_store_u16(usf_state_t* state, const uint16_t* src, uint16_t address, size_t count);
void dmem_store_u32(usf_state_t* state, const uint32_t* src, uint16_t address, size_t count);

void dram_load_u8 (usf_state_t* state, uint8_t*  dst, uint32_t address, size_t count);
void dram_load_u16(usf_state_t* state, uint16_t* dst, uint32_t address, size_t count);
void dram_load_u32(usf_state_t* state, uint32_t* dst, uint32_t address, size_t count);
void dram_store_u8 (usf_state_t* state, const uint8_t*  src, uint32_t address, size_t count);
void dram_store_u16(usf_state_t* state, const uint16_t* src, uint32_t address, size_t count);
void dram_store_u32(usf_state_t* state, const uint32_t* src, uint32_t address, size_t count);

#endif

