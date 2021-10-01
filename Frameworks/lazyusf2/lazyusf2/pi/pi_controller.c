/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - pi_controller.c                                         *
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

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "usf/barray.h"

#include "pi_controller.h"

#define M64P_CORE_PROTOTYPES 1
#include "api/m64p_types.h"
#include "api/callbacks.h"
#include "main/rom.h"
#include "memory/memory.h"
#include "r4300/cached_interp.h"
#include "r4300/cp0.h"
#include "r4300/interupt.h"
#include "r4300/new_dynarec/new_dynarec.h"
#include "r4300/ops.h"
#include "r4300/r4300.h"
#include "r4300/r4300_core.h"
#include "ri/rdram_detection_hack.h"
#include "ri/ri_controller.h"

#include <string.h>

static void dma_pi_read(usf_state_t * state, struct pi_controller* pi)
{
    if (pi->regs[PI_CART_ADDR_REG] >= 0x08000000 && pi->regs[PI_CART_ADDR_REG] < 0x08010000)
    {
    }
    else
    {
        DebugMessage(state, M64MSG_WARNING, "Unknown dma read in dma_pi_read()");
    }

    pi->regs[PI_STATUS_REG] |= 1;
    update_count(state);
    add_interupt_event(state, PI_INT, state->g_delay_pi ? 0x1000/*pi->regs[PI_RD_LEN_REG]*/ : 0);
}

static void dma_pi_write(usf_state_t * state, struct pi_controller* pi)
{
    unsigned int longueur;
    int i;
    
#ifdef DEBUG_INFO
    if (state->debug_log)
      fprintf(state->debug_log, "PI DMA WRITE: %08x to %08x for %08x bytes\n", pi->regs[PI_CART_ADDR_REG], pi->regs[PI_DRAM_ADDR_REG], pi->regs[PI_WR_LEN_REG] + 1);
#endif

    if (pi->regs[PI_CART_ADDR_REG] < 0x10000000)
    {
        if (pi->regs[PI_CART_ADDR_REG] >= 0x08000000 && pi->regs[PI_CART_ADDR_REG] < 0x08010000)
        {
        }
        else if (pi->regs[PI_CART_ADDR_REG] >= 0x06000000 && pi->regs[PI_CART_ADDR_REG] < 0x08000000)
        {
        }
        else
        {
            DebugMessage(state, M64MSG_WARNING, "Unknown dma write 0x%x in dma_pi_write()", (int)pi->regs[PI_CART_ADDR_REG]);
        }

        pi->regs[PI_STATUS_REG] |= 1;
        update_count(state);
        add_interupt_event(state, PI_INT, state->g_delay_pi ? /*pi->regs[PI_WR_LEN_REG]*/0x1000 : 0);

        return;
    }

    if (pi->regs[PI_CART_ADDR_REG] >= 0x1fc00000) // for paper mario
    {
        pi->regs[PI_STATUS_REG] |= 1;
        update_count(state);
        add_interupt_event(state, PI_INT, state->g_delay_pi ? 0x1000 : 0);

        return;
    }

    longueur = (pi->regs[PI_WR_LEN_REG] & 0xFFFFFF)+1;
    i = (pi->regs[PI_CART_ADDR_REG]-0x10000000)&0x3FFFFFF;
    longueur = (i + (int) longueur) > pi->cart_rom.rom_size ?
               (unsigned int)(pi->cart_rom.rom_size - i) : longueur;
    longueur = (pi->regs[PI_DRAM_ADDR_REG] + longueur) > 0x7FFFFF ?
               (0x7FFFFF - pi->regs[PI_DRAM_ADDR_REG]) : longueur;

    if (i > pi->cart_rom.rom_size || pi->regs[PI_DRAM_ADDR_REG] > 0x7FFFFF)
    {
        pi->regs[PI_STATUS_REG] |= 3;
        update_count(state);
        add_interupt_event(state, PI_INT, state->g_delay_pi ? longueur/8 : 0);

        return;
    }

    if (state->r4300emu != CORE_PURE_INTERPRETER)
    {
        for (i=0; i<(int)longueur; i++)
        {
            unsigned long rdram_address1 = pi->regs[PI_DRAM_ADDR_REG]+i+0x80000000;
            unsigned long rdram_address2 = pi->regs[PI_DRAM_ADDR_REG]+i+0xa0000000;
            unsigned long rom_address = (((pi->regs[PI_CART_ADDR_REG]-0x10000000)&0x3FFFFFF)+i);
            unsigned long ram_address = (pi->regs[PI_DRAM_ADDR_REG]+i);

            if (state->enable_trimming_mode)
            {
                bit_array_set(state->barray_rom, rom_address / 4);
                if (!(ram_address & 3) && (longueur - i) >= 4)
                {
                    if (!bit_array_test(state->barray_ram_read, ram_address / 4))
                        bit_array_set(state->barray_ram_written_first, ram_address / 4);
                }
            }
            
            ((unsigned char*)pi->ri->rdram.dram)[ram_address^S8]=
                pi->cart_rom.rom[rom_address^S8];

            if (!state->invalid_code[rdram_address1>>12])
            {
                if (!state->blocks[rdram_address1>>12] ||
                    state->blocks[rdram_address1>>12]->block[(rdram_address1&0xFFF)/4].ops !=
                    state->current_instruction_table.NOTCOMPILED)
                {
                    state->invalid_code[rdram_address1>>12] = 1;
                }
#ifdef NEW_DYNAREC
                invalidate_block(state, rdram_address1>>12);
#endif
            }
            if (!state->invalid_code[rdram_address2>>12])
            {
                if (!state->blocks[rdram_address1>>12] ||
                    state->blocks[rdram_address2>>12]->block[(rdram_address2&0xFFF)/4].ops !=
                    state->current_instruction_table.NOTCOMPILED)
                {
                    state->invalid_code[rdram_address2>>12] = 1;
                }
            }
        }
    }
    else
    {
        for (i=0; i<(int)longueur; i++)
        {
            unsigned long rom_address = (((pi->regs[PI_CART_ADDR_REG]-0x10000000)&0x3FFFFFF)+i);
            unsigned long ram_address = (pi->regs[PI_DRAM_ADDR_REG]+i);

            if (state->enable_trimming_mode)
            {
                bit_array_set(state->barray_rom, rom_address / 4);
                if (!(ram_address & 3) && (longueur - i) >= 4)
                {
                    if (!bit_array_test(state->barray_ram_read, ram_address / 4))
                        bit_array_set(state->barray_ram_written_first, ram_address / 4);
                }
            }

            ((unsigned char*)pi->ri->rdram.dram)[ram_address^S8]=
                pi->cart_rom.rom[rom_address^S8];
        }
    }

    /* HACK: monitor PI DMA to trigger RDRAM size detection
     * hack just before initial cart ROM loading. */
    if (pi->regs[PI_CART_ADDR_REG] == 0x10001000)
    {
        force_detected_rdram_size_hack(state);
    }

    pi->regs[PI_STATUS_REG] |= 3;
    update_count(state);
    add_interupt_event(state, PI_INT, state->g_delay_pi ? longueur/8 : 0);

    return;
}

void connect_pi(struct pi_controller* pi,
                struct r4300_core* r4300,
                struct ri_controller* ri,
                uint8_t* rom, size_t rom_size)
{
    connect_cart_rom(&pi->cart_rom, rom, rom_size);

    pi->r4300 = r4300;
    pi->ri = ri;
}

void init_pi(struct pi_controller* pi)
{
    memset(pi->regs, 0, PI_REGS_COUNT*sizeof(uint32_t));
}


int read_pi_regs(void* opaque, uint32_t address, uint32_t* value)
{
    struct pi_controller* pi = (struct pi_controller*)opaque;
    uint32_t reg = pi_reg(address);

    *value = pi->regs[reg];

    return 0;
}

int write_pi_regs(void* opaque, uint32_t address, uint32_t value, uint32_t mask)
{
    struct pi_controller* pi = (struct pi_controller*)opaque;
    uint32_t reg = pi_reg(address);

    switch (reg)
    {
    case PI_RD_LEN_REG:
        masked_write(&pi->regs[PI_RD_LEN_REG], value, mask);
        dma_pi_read(pi->r4300->state, pi);
        return 0;

    case PI_WR_LEN_REG:
        masked_write(&pi->regs[PI_WR_LEN_REG], value, mask);
        dma_pi_write(pi->r4300->state, pi);
        return 0;

    case PI_STATUS_REG:
        if (value & mask & 2)
            clear_rcp_interrupt(pi->r4300, MI_INTR_PI);
        return 0;

    case PI_BSD_DOM1_LAT_REG:
    case PI_BSD_DOM1_PWD_REG:
    case PI_BSD_DOM1_PGS_REG:
    case PI_BSD_DOM1_RLS_REG:
    case PI_BSD_DOM2_LAT_REG:
    case PI_BSD_DOM2_PWD_REG:
    case PI_BSD_DOM2_PGS_REG:
    case PI_BSD_DOM2_RLS_REG:
        masked_write(&pi->regs[reg], value & 0xff, mask);
        return 0;
    }

    masked_write(&pi->regs[reg], value, mask);

    return 0;
}

void pi_end_of_dma_event(struct pi_controller* pi)
{
    pi->regs[PI_STATUS_REG] &= ~3;
    raise_rcp_interrupt(pi->r4300, MI_INTR_PI);
}
