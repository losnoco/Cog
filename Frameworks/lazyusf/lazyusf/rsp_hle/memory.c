/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - memory.c                                        *
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

#include <string.h>

#include "../usf.h"

#include "memory.h"

#include "../usf_internal.h"

/* Global functions */
void dmem_load_u8 (usf_state_t* state, uint8_t*  dst, uint16_t address, size_t count)
{
    while (count != 0) {
        *(dst++) = *dmem_u8(state, address);
        address += 1;
        --count;
    }
}

void dmem_load_u16(usf_state_t* state, uint16_t* dst, uint16_t address, size_t count)
{
    while (count != 0) {
        *(dst++) = *dmem_u16(state, address);
        address += 2;
        --count;
    }
}

void dmem_load_u32(usf_state_t* state, uint32_t* dst, uint16_t address, size_t count)
{
    /* Optimization for uint32_t */
    memcpy(dst, dmem_u32(state, address), count * sizeof(uint32_t));
}

void dmem_store_u8 (usf_state_t* state, const uint8_t*  src, uint16_t address, size_t count)
{
    while (count != 0) {
        *dmem_u8(state, address) = *(src++);
        address += 1;
        --count;
    }
}

void dmem_store_u16(usf_state_t* state, const uint16_t* src, uint16_t address, size_t count)
{
    while (count != 0) {
        *dmem_u16(state, address) = *(src++);
        address += 2;
        --count;
    }
}

void dmem_store_u32(usf_state_t* state, const uint32_t* src, uint16_t address, size_t count)
{
    /* Optimization for uint32_t */
    memcpy(dmem_u32(state, address), src, count * sizeof(uint32_t));
}


void dram_load_u8 (usf_state_t* state, uint8_t*  dst, uint32_t address, size_t count)
{
    while (count != 0) {
        *(dst++) = *dram_u8(state, address);
        address += 1;
        --count;
    }
}

void dram_load_u16(usf_state_t* state, uint16_t* dst, uint32_t address, size_t count)
{
    while (count != 0) {
        *(dst++) = *dram_u16(state, address);
        address += 2;
        --count;
    }
}

void dram_load_u32(usf_state_t* state, uint32_t* dst, uint32_t address, size_t count)
{
    /* Optimization for uint32_t */
    memcpy(dst, dram_u32(state, address), count * sizeof(uint32_t));
}

void dram_store_u8 (usf_state_t* state, const uint8_t*  src, uint32_t address, size_t count)
{
    while (count != 0) {
        *dram_u8(state, address) = *(src++);
        address += 1;
        --count;
    }
}

void dram_store_u16(usf_state_t* state, const uint16_t* src, uint32_t address, size_t count)
{
    while (count != 0) {
        *dram_u16(state, address) = *(src++);
        address += 2;
        --count;
    }
}

void dram_store_u32(usf_state_t* state, const uint32_t* src, uint32_t address, size_t count)
{
    /* Optimization for uint32_t */
    memcpy(dram_u32(state, address), src, count * sizeof(uint32_t));
}

