/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - tlb.c                                                   *
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

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "tlb.h"
#include "exception.h"

#include "main/rom.h"

void tlb_unmap(usf_state_t * state, tlb *entry)
{
    unsigned int i;

    if (entry->v_even)
    {
        for (i=entry->start_even; i<entry->end_even; i += 0x1000)
            state->tlb_LUT_r[i>>12] = 0;
        if (entry->d_even)
            for (i=entry->start_even; i<entry->end_even; i += 0x1000)
                state->tlb_LUT_w[i>>12] = 0;
    }

    if (entry->v_odd)
    {
        for (i=entry->start_odd; i<entry->end_odd; i += 0x1000)
            state->tlb_LUT_r[i>>12] = 0;
        if (entry->d_odd)
            for (i=entry->start_odd; i<entry->end_odd; i += 0x1000)
                state->tlb_LUT_w[i>>12] = 0;
    }
}

void tlb_map(usf_state_t * state, tlb *entry)
{
    unsigned int i;

    if (entry->v_even)
    {
        if (entry->start_even < entry->end_even &&
            !(entry->start_even >= 0x80000000 && entry->end_even < 0xC0000000) &&
            entry->phys_even < 0x20000000)
        {
            for (i=entry->start_even;i<entry->end_even;i+=0x1000)
                state->tlb_LUT_r[i>>12] = 0x80000000 | (entry->phys_even + (i - entry->start_even) + 0xFFF);
            if (entry->d_even)
                for (i=entry->start_even;i<entry->end_even;i+=0x1000)
                    state->tlb_LUT_w[i>>12] = 0x80000000 | (entry->phys_even + (i - entry->start_even) + 0xFFF);
        }
    }

    if (entry->v_odd)
    {
        if (entry->start_odd < entry->end_odd &&
            !(entry->start_odd >= 0x80000000 && entry->end_odd < 0xC0000000) &&
            entry->phys_odd < 0x20000000)
        {
            for (i=entry->start_odd;i<entry->end_odd;i+=0x1000)
                state->tlb_LUT_r[i>>12] = 0x80000000 | (entry->phys_odd + (i - entry->start_odd) + 0xFFF);
            if (entry->d_odd)
                for (i=entry->start_odd;i<entry->end_odd;i+=0x1000)
                    state->tlb_LUT_w[i>>12] = 0x80000000 | (entry->phys_odd + (i - entry->start_odd) + 0xFFF);
        }
    }
}

unsigned int virtual_to_physical_address(usf_state_t * state, unsigned int addresse, int w)
{
    if (w == 1)
    {
        if (state->tlb_LUT_w[addresse>>12])
            return (state->tlb_LUT_w[addresse>>12]&0xFFFFF000)|(addresse&0xFFF);
        else if (state->g_disable_tlb_write_exception)
            return 0;
    }
    else
    {
        if (state->tlb_LUT_r[addresse>>12])
            return (state->tlb_LUT_r[addresse>>12]&0xFFFFF000)|(addresse&0xFFF);
    }
    //printf("tlb exception !!! @ %x, %x, add:%x\n", addresse, w, PC->addr);
    //getchar();
#ifdef DEBUG_INFO
    if (state->debug_log)
      fprintf(state->debug_log, "TLB exception @ %x, %x, add:%x\n", addresse, w, state->PC->addr);
#endif
    TLB_refill_exception(state,addresse,w);
    //return 0x80000000;
    return 0x00000000;
}
