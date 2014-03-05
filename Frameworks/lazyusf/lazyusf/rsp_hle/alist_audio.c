/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - alist_audio.c                                   *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
 *   Copyright (C) 2009 Richard Goedeken                                   *
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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "../usf.h"

#include "alist_internal.h"
#include "memory_hle.h"

#include "../usf_internal.h"

/* state moved to usf_internal.h */

/* helper functions */
static uint32_t get_address(usf_state_t* state, uint32_t so)
{
    return alist_get_address(state, so, state->l_alist_audio.segments, N_SEGMENTS);
}

static void set_address(usf_state_t* state, uint32_t so)
{
    alist_set_address(state, so, state->l_alist_audio.segments, N_SEGMENTS);
}

static void clear_segments(usf_state_t* state)
{
    memset(state->l_alist_audio.segments, 0, N_SEGMENTS*sizeof(state->l_alist_audio.segments[0]));
}

/* audio commands definition */
static void SPNOOP(usf_state_t* state, uint32_t w1, uint32_t w2)
{
}

static void CLEARBUFF(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t dmem  = w1 + DMEM_BASE;
    uint16_t count = w2;

    if (count == 0)
        return;

    alist_clear(state, dmem, align(count, 16));
}

static void ENVMIXER(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint32_t address = get_address(state, w2);

    alist_envmix_exp(
            state,
            flags & A_INIT,
            flags & A_AUX,
            state->l_alist_audio.out, state->l_alist_audio.dry_right,
            state->l_alist_audio.wet_left, state->l_alist_audio.wet_right,
            state->l_alist_audio.in, state->l_alist_audio.count,
            state->l_alist_audio.dry, state->l_alist_audio.wet,
            state->l_alist_audio.vol,
            state->l_alist_audio.target,
            state->l_alist_audio.rate,
            address);
}

static void RESAMPLE(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint16_t pitch   = w1;
    uint32_t address = get_address(state, w2);

    alist_resample(
            state,
            flags & 0x1,
            flags & 0x2,
            state->l_alist_audio.out,
            state->l_alist_audio.in,
            align(state->l_alist_audio.count, 16),
            pitch << 1,
            address);
}

static void SETVOL(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16);

    if (flags & A_AUX) {
        state->l_alist_audio.dry = w1;
        state->l_alist_audio.wet = w2;
    }
    else {
        unsigned lr = (flags & A_LEFT) ? 0 : 1;

        if (flags & A_VOL)
            state->l_alist_audio.vol[lr] = w1;
        else {
            state->l_alist_audio.target[lr] = w1;
            state->l_alist_audio.rate[lr]   = w2;
        }
    }
}

static void SETLOOP(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    state->l_alist_audio.loop = get_address(state, w2);
}

static void ADPCM(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint32_t address = get_address(state, w2);

    alist_adpcm(
            state,
            flags & 0x1,
            flags & 0x2,
            false,          /* unsupported in this ucode */
            state->l_alist_audio.out,
            state->l_alist_audio.in,
            align(state->l_alist_audio.count, 32),
            state->l_alist_audio.table,
            state->l_alist_audio.loop,
            address);
}

static void LOADBUFF(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint32_t address = get_address(state, w2);

    if (state->l_alist_audio.count == 0)
        return;

    alist_load(state, state->l_alist_audio.in, address, state->l_alist_audio.count);
}

static void SAVEBUFF(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint32_t address = get_address(state, w2);

    if (state->l_alist_audio.count == 0)
        return;

    alist_save(state, state->l_alist_audio.out, address, state->l_alist_audio.count);
}

static void SETBUFF(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16);

    if (flags & A_AUX) {
        state->l_alist_audio.dry_right = w1 + DMEM_BASE;
        state->l_alist_audio.wet_left  = (w2 >> 16) + DMEM_BASE;
        state->l_alist_audio.wet_right = w2 + DMEM_BASE;
    } else {
        state->l_alist_audio.in    = w1 + DMEM_BASE;
        state->l_alist_audio.out   = (w2 >> 16) + DMEM_BASE;
        state->l_alist_audio.count = w2;
    }
}

static void DMEMMOVE(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t dmemi = w1 + DMEM_BASE;
    uint16_t dmemo = (w2 >> 16) + DMEM_BASE;
    uint16_t count = w2;

    if (count == 0)
        return;

    alist_move(state, dmemo, dmemi, align(count, 16));
}

static void LOADADPCM(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t count   = w1;
    uint32_t address = get_address(state, w2);

    dram_load_u16(state, (uint16_t*)state->l_alist_audio.table, address, align(count, 8) >> 1);
}

static void INTERLEAVE(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t left  = (w2 >> 16) + DMEM_BASE;
    uint16_t right = w2 + DMEM_BASE;

    if (state->l_alist_audio.count == 0)
        return;

    alist_interleave(state, state->l_alist_audio.out, left, right, align(state->l_alist_audio.count, 16));
}

static void MIXER(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    int16_t  gain  = w1;
    uint16_t dmemi = (w2 >> 16) + DMEM_BASE;
    uint16_t dmemo = w2 + DMEM_BASE;

    if (state->l_alist_audio.count == 0)
        return;

    alist_mix(state, dmemo, dmemi, align(state->l_alist_audio.count, 32), gain);
}

static void SEGMENT(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    set_address(state, w2);
}

static void POLEF(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint16_t gain    = w1;
    uint32_t address = get_address(state, w2);

    if (state->l_alist_audio.count == 0)
        return;

    alist_polef(
            state,
            flags & A_INIT,
            state->l_alist_audio.out,
            state->l_alist_audio.in,
            align(state->l_alist_audio.count, 16),
            gain,
            state->l_alist_audio.table,
            address);
}

/* global functions */
void alist_process_audio(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x10] = {
        SPNOOP,         ADPCM ,         CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       SEGMENT,
        SETBUFF,        SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     POLEF,          SETLOOP
    };

    clear_segments(state);
    alist_process(state, ABI, 0x10);
}

void alist_process_audio_ge(usf_state_t* state)
{
    /* TODO: see what differs from alist_process_audio */
    static const acmd_callback_t ABI[0x10] = {
        SPNOOP,         ADPCM ,         CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       SEGMENT,
        SETBUFF,        SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     POLEF,          SETLOOP
    };

    clear_segments(state);
    alist_process(state, ABI, 0x10);
}

void alist_process_audio_bc(usf_state_t* state)
{
    /* TODO: see what differs from alist_process_audio */
    static const acmd_callback_t ABI[0x10] = {
        SPNOOP,         ADPCM ,         CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       SEGMENT,
        SETBUFF,        SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     POLEF,          SETLOOP
    };

    clear_segments(state);
    alist_process(state, ABI, 0x10);
}
