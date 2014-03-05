/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - alist_naudio.c                                  *
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

#include "../usf.h"

#include "alist_internal.h"
#include "memory_hle.h"
#include "plugin_hle.h"

#include "../usf_internal.h"

void MP3(usf_state_t* state, uint32_t w1, uint32_t w2);

/* audio commands definition */
static void UNKNOWN(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t acmd = (w1 >> 24);

    DebugMessage(state, M64MSG_WARNING,
            "Unknown audio comand %d: %08x %08x",
            acmd, w1, w2);
}


static void SPNOOP(usf_state_t* state, uint32_t w1, uint32_t w2)
{
}

static void NAUDIO_0000(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    /* ??? */
    UNKNOWN(state, w1, w2);
}

static void NAUDIO_02B0(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    /* ??? */
    /* UNKNOWN(state, w1, w2); commented to avoid constant spamming during gameplay */
}

static void NAUDIO_14(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    if (state->l_alist_naudio.table[0] == 0 && state->l_alist_naudio.table[1] == 0) {

        uint8_t  flags       = (w1 >> 16);
        uint16_t gain        = w1;
        uint8_t  select_main = (w2 >> 24);
        uint32_t address     = (w2 & 0xffffff);

        uint16_t dmem = (select_main == 0) ? NAUDIO_MAIN : NAUDIO_MAIN2;

        alist_polef(
                state,
                flags & A_INIT,
                dmem,
                dmem,
                NAUDIO_COUNT,
                gain,
                state->l_alist_naudio.table,
                address);
    }
    else
        DebugMessage(state, M64MSG_VERBOSE, "NAUDIO_14: non null codebook[0-3] case not implemented.");
}

static void SETVOL(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t flags = (w1 >> 16);

    if (flags & 0x4) {
        if (flags & 0x2) {
            state->l_alist_naudio.vol[0] = w1;
            state->l_alist_naudio.dry    = (w2 >> 16);
            state->l_alist_naudio.wet    = w2;
        }
        else {
            state->l_alist_naudio.target[1] = w1;
            state->l_alist_naudio.rate[1]   = w2;
        }
    }
    else {
        state->l_alist_naudio.target[0] = w1;
        state->l_alist_naudio.rate[0]   = w2;
    }
}

static void ENVMIXER(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint32_t address = (w2 & 0xffffff);

    state->l_alist_naudio.vol[1] = w1;

    alist_envmix_lin(
            state,
            flags & 0x1,
            NAUDIO_DRY_LEFT,
            NAUDIO_DRY_RIGHT,
            NAUDIO_WET_LEFT,
            NAUDIO_WET_RIGHT,
            NAUDIO_MAIN,
            NAUDIO_COUNT,
            state->l_alist_naudio.dry,
            state->l_alist_naudio.wet,
            state->l_alist_naudio.vol,
            state->l_alist_naudio.target,
            state->l_alist_naudio.rate,
            address);
}

static void CLEARBUFF(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t dmem  = w1 + NAUDIO_MAIN;
    uint16_t count = w2;

    alist_clear(state, dmem, count);
}

static void MIXER(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    int16_t  gain  = w1;
    uint16_t dmemi = (w2 >> 16) + NAUDIO_MAIN;
    uint16_t dmemo = w2 + NAUDIO_MAIN;

    alist_mix(state, dmemo, dmemi, NAUDIO_COUNT, gain);
}

static void LOADBUFF(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t count   = (w1 >> 12) & 0xfff;
    uint16_t dmem    = (w1 & 0xfff) + NAUDIO_MAIN;
    uint32_t address = (w2 & 0xffffff);

    alist_load(state, dmem, address, count);
}

static void SAVEBUFF(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t count   = (w1 >> 12) & 0xfff;
    uint16_t dmem    = (w1 & 0xfff) + NAUDIO_MAIN;
    uint32_t address = (w2 & 0xffffff);

    alist_save(state, dmem, address, count);
}

static void LOADADPCM(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t count   = w1;
    uint32_t address = (w2 & 0xffffff);

    dram_load_u16(state, (uint16_t*)state->l_alist_naudio.table, address, count >> 1);
}

static void DMEMMOVE(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t dmemi = w1 + NAUDIO_MAIN;
    uint16_t dmemo = (w2 >> 16) + NAUDIO_MAIN;
    uint16_t count = w2;

    alist_move(state, dmemo, dmemi, (count + 3) & ~3);
}

static void SETLOOP(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    state->l_alist_naudio.loop = (w2 & 0xffffff);
}

static void ADPCM(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint32_t address = (w1 & 0xffffff);
    uint8_t  flags   = (w2 >> 28);
    uint16_t count   = (w2 >> 16) & 0xfff;
    uint16_t dmemi   = ((w2 >> 12) & 0xf) + NAUDIO_MAIN;
    uint16_t dmemo   = (w2 & 0xfff) + NAUDIO_MAIN;

    alist_adpcm(
            state,
            flags & 0x1,
            flags & 0x2,
            false,          /* unsuported by this ucode */
            dmemo,
            dmemi,
            (count + 0x1f) & ~0x1f,
            state->l_alist_naudio.table,
            state->l_alist_naudio.loop,
            address);
}

static void RESAMPLE(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint32_t address = (w1 & 0xffffff);
    uint8_t  flags   = (w2 >> 30);
    uint16_t pitch   = (w2 >> 14);
    uint16_t dmemi   = ((w2 >> 2) & 0xfff) + NAUDIO_MAIN;
    uint16_t dmemo   = (w2 & 0x3) ? NAUDIO_MAIN2 : NAUDIO_MAIN;

    alist_resample(
            state,
            flags & 0x1,
            false,          /* TODO: check which ABI supports it */
            dmemo,
            dmemi,
            NAUDIO_COUNT,
            pitch << 1,
            address);
}

static void INTERLEAVE(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    alist_interleave(state, NAUDIO_MAIN, NAUDIO_DRY_LEFT, NAUDIO_DRY_RIGHT, NAUDIO_COUNT);
}

static void MP3ADDY(usf_state_t* state, uint32_t w1, uint32_t w2)
{
}

/* global functions */
void alist_process_naudio(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x10] = {
        SPNOOP,         ADPCM,          CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       NAUDIO_0000,
        NAUDIO_0000,    SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     NAUDIO_02B0,    SETLOOP
    };

    alist_process(state, ABI, 0x10);
}

void alist_process_naudio_bk(usf_state_t* state)
{
    /* TODO: see what differs from alist_process_naudio */
    static const acmd_callback_t ABI[0x10] = {
        SPNOOP,         ADPCM,          CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       NAUDIO_0000,
        NAUDIO_0000,    SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     NAUDIO_02B0,    SETLOOP
    };

    alist_process(state, ABI, 0x10);
}

void alist_process_naudio_dk(usf_state_t* state)
{
    /* TODO: see what differs from alist_process_naudio */
    static const acmd_callback_t ABI[0x10] = {
        SPNOOP,         ADPCM,          CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       MIXER,
        MIXER,          SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     NAUDIO_02B0,    SETLOOP
    };

    alist_process(state, ABI, 0x10);
}

void alist_process_naudio_mp3(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x10] = {
        UNKNOWN,        ADPCM,          CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       MP3,
        MP3ADDY,        SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     NAUDIO_14,      SETLOOP
    };

    alist_process(state, ABI, 0x10);
}

void alist_process_naudio_cbfd(usf_state_t* state)
{
    /* TODO: see what differs from alist_process_naudio_mp3 */
    static const acmd_callback_t ABI[0x10] = {
        UNKNOWN,        ADPCM,          CLEARBUFF,      ENVMIXER,
        LOADBUFF,       RESAMPLE,       SAVEBUFF,       MP3,
        MP3ADDY,        SETVOL,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     NAUDIO_14,      SETLOOP
    };

    alist_process(state, ABI, 0x10);
}
