/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - alist_nead.c                                    *
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

/* remove windows define to 0x06 */
#ifdef DUPLICATE
#undef DUPLICATE
#endif



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

static void LOADADPCM(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t count   = w1;
    uint32_t address = (w2 & 0xffffff);

    dram_load_u16(state, (uint16_t*)state->l_alist_nead.table, address, count >> 1);
}

static void SETLOOP(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    state->l_alist_nead.loop = w2 & 0xffffff;
}

static void SETBUFF(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    state->l_alist_nead.in    = w1;
    state->l_alist_nead.out   = (w2 >> 16);
    state->l_alist_nead.count = w2;
}

static void ADPCM(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint32_t address = (w2 & 0xffffff);

    alist_adpcm(
            state,
            flags & 0x1,
            flags & 0x2,
            flags & 0x4,
            state->l_alist_nead.out,
            state->l_alist_nead.in,
            (state->l_alist_nead.count + 0x1f) & ~0x1f,
            state->l_alist_nead.table,
            state->l_alist_nead.loop,
            address);
}

static void CLEARBUFF(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t dmem  = w1;
    uint16_t count = w2;

    if (count == 0)
        return;

    alist_clear(state, dmem, count);
}

static void LOADBUFF(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t count   = (w1 >> 12) & 0xfff;
    uint16_t dmem    = (w1 & 0xfff);
    uint32_t address = (w2 & 0xffffff);

    alist_load(state, dmem, address, count);
}

static void SAVEBUFF(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t count   = (w1 >> 12) & 0xfff;
    uint16_t dmem    = (w1 & 0xfff);
    uint32_t address = (w2 & 0xffffff);

    alist_save(state, dmem, address, count);
}

static void MIXER(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t count = (w1 >> 12) & 0xff0;
    int16_t  gain  = w1;
    uint16_t dmemi = (w2 >> 16);
    uint16_t dmemo = w2;

    alist_mix(state, dmemo, dmemi, count, gain);
}


static void RESAMPLE(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint16_t pitch   = w1;
    uint32_t address = (w2 & 0xffffff);

    alist_resample(
            state,
            flags & 0x1,
            false,          /* TODO: check which ABI supports it */
            state->l_alist_nead.out,
            state->l_alist_nead.in,
            (state->l_alist_nead.count + 0xf) & ~0xf,
            pitch << 1,
            address);
}

static void RESAMPLE_ZOH(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t pitch      = w1;
    uint16_t pitch_accu = w2;

    alist_resample_zoh(
            state,
            state->l_alist_nead.out,
            state->l_alist_nead.in,
            state->l_alist_nead.count,
            pitch << 1,
            pitch_accu);
}

static void DMEMMOVE(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t dmemi = w1;
    uint16_t dmemo = (w2 >> 16);
    uint16_t count = w2;

    if (count == 0)
        return;

    alist_move(state, dmemo, dmemi, (count + 3) & ~3);
}

static void ENVSETUP1_MK(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    state->l_alist_nead.env_values[2] = (w1 >> 8) & 0xff00;
    state->l_alist_nead.env_steps[2]  = 0;
    state->l_alist_nead.env_steps[0]  = (w2 >> 16);
    state->l_alist_nead.env_steps[1]  = w2;
}

static void ENVSETUP1(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    state->l_alist_nead.env_values[2] = (w1 >> 8) & 0xff00;
    state->l_alist_nead.env_steps[2]  = w1;
    state->l_alist_nead.env_steps[0]  = (w2 >> 16);
    state->l_alist_nead.env_steps[1]  = w2;
}

static void ENVSETUP2(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    state->l_alist_nead.env_values[0] = (w2 >> 16);
    state->l_alist_nead.env_values[1] = w2;
}

static void ENVMIXER_MK(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    int16_t xors[4];

    uint16_t dmemi = (w1 >> 12) & 0xff0;
    uint8_t  count = (w1 >>  8) & 0xff;
    xors[2] = 0;    /* unsupported by this ucode */
    xors[3] = 0;    /* unsupported by this ucode */
    xors[0] = 0 - (int16_t)((w1 & 0x2) >> 1);
    xors[1] = 0 - (int16_t)((w1 & 0x1)     );
    uint16_t dmem_dl = (w2 >> 20) & 0xff0;
    uint16_t dmem_dr = (w2 >> 12) & 0xff0;
    uint16_t dmem_wl = (w2 >>  4) & 0xff0;
    uint16_t dmem_wr = (w2 <<  4) & 0xff0;

    alist_envmix_nead(
            state,
            false,  /* unsupported by this ucode */
            dmem_dl, dmem_dr,
            dmem_wl, dmem_wr,
            dmemi, count,
            state->l_alist_nead.env_values,
            state->l_alist_nead.env_steps,
            xors);
}

static void ENVMIXER(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    int16_t xors[4];

    uint16_t dmemi = (w1 >> 12) & 0xff0;
    uint8_t  count = (w1 >>  8) & 0xff;
    bool     swap_wet_LR = (w1 >> 4) & 0x1;
    xors[2] = 0 - (int16_t)((w1 & 0x8) >> 1);
    xors[3] = 0 - (int16_t)((w1 & 0x4) >> 1);
    xors[0] = 0 - (int16_t)((w1 & 0x2) >> 1);
    xors[1] = 0 - (int16_t)((w1 & 0x1)     );
    uint16_t dmem_dl = (w2 >> 20) & 0xff0;
    uint16_t dmem_dr = (w2 >> 12) & 0xff0;
    uint16_t dmem_wl = (w2 >>  4) & 0xff0;
    uint16_t dmem_wr = (w2 <<  4) & 0xff0;

    alist_envmix_nead(
            state,
            swap_wet_LR,
            dmem_dl, dmem_dr,
            dmem_wl, dmem_wr,
            dmemi, count,
            state->l_alist_nead.env_values,
            state->l_alist_nead.env_steps,
            xors);
}

static void DUPLICATE(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t  count = (w1 >> 16);
    uint16_t dmemi = w1;
    uint16_t dmemo = (w2 >> 16);

    alist_repeat64(state, dmemo, dmemi, count);
}

static void INTERL(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t count = w1;
    uint16_t dmemi = (w2 >> 16);
    uint16_t dmemo = w2;

    alist_copy_every_other_sample(state, dmemo, dmemi, count);
}

static void INTERLEAVE_MK(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t left = (w2 >> 16);
    uint16_t right = w2;

    if (state->l_alist_nead.count == 0)
        return;

    alist_interleave(state, state->l_alist_nead.out, left, right, state->l_alist_nead.count);
}

static void INTERLEAVE(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t count = ((w1 >> 12) & 0xff0);
    uint16_t dmemo = w1;
    uint16_t left = (w2 >> 16);
    uint16_t right = w2;

    alist_interleave(state, dmemo, left, right, count);
}

static void ADDMIXER(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint16_t count = (w1 >> 12) & 0xff0;
    uint16_t dmemi = (w2 >> 16);
    uint16_t dmemo = w2;

    alist_add(state, dmemo, dmemi, count);
}

static void HILOGAIN(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    int8_t   gain  = (w1 >> 16); /* Q4.4 signed */
    uint16_t count = w1;
    uint16_t dmem  = (w2 >> 16);

    alist_multQ44(state, dmem, count, gain);
}

static void FILTER(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint32_t address = (w2 & 0xffffff);

    if (flags > 1) {
        state->l_alist_nead.filter_count          = w1;
        state->l_alist_nead.filter_lut_address[0] = address; /* t6 */
    }
    else {
        uint16_t dmem = w1;

        state->l_alist_nead.filter_lut_address[1] = address + 0x10; /* t5 */
        alist_filter(state, dmem, state->l_alist_nead.filter_count, address, state->l_alist_nead.filter_lut_address);
    }
}

static void SEGMENT(usf_state_t* state, uint32_t w1, uint32_t w2)
{
}

static void NEAD_16(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t  count      = (w1 >> 16);
    uint16_t dmemi      = w1;
    uint16_t dmemo      = (w2 >> 16);
    uint16_t block_size = w2;

    alist_copy_blocks(state, dmemo, dmemi, block_size, count);
}

static void POLEF(usf_state_t* state, uint32_t w1, uint32_t w2)
{
    uint8_t  flags   = (w1 >> 16);
    uint16_t gain    = w1;
    uint32_t address = (w2 & 0xffffff);

    if (state->l_alist_nead.count == 0)
        return;

    alist_polef(
            state,
            flags & A_INIT,
            state->l_alist_nead.out,
            state->l_alist_nead.in,
            state->l_alist_nead.count,
            gain,
            state->l_alist_nead.table,
            address);
}


void alist_process_nead_mk(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x20] = {
        SPNOOP,         ADPCM,          CLEARBUFF,      SPNOOP,
        SPNOOP,         RESAMPLE,       SPNOOP,         SEGMENT,
        SETBUFF,        SPNOOP,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE_MK,  POLEF,          SETLOOP,
        NEAD_16,        INTERL,         ENVSETUP1_MK,   ENVMIXER_MK,
        LOADBUFF,       SAVEBUFF,       ENVSETUP2,      SPNOOP,
        SPNOOP,         SPNOOP,         SPNOOP,         SPNOOP,
        SPNOOP,         SPNOOP,         SPNOOP,         SPNOOP
    };

    alist_process(state, ABI, 0x20);
}

void alist_process_nead_sf(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x20] = {
        SPNOOP,         ADPCM,          CLEARBUFF,      SPNOOP,
        ADDMIXER,       RESAMPLE,       RESAMPLE_ZOH,   SPNOOP,
        SETBUFF,        SPNOOP,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE_MK,  POLEF,          SETLOOP,
        NEAD_16,        INTERL,         ENVSETUP1,      ENVMIXER,
        LOADBUFF,       SAVEBUFF,       ENVSETUP2,      SPNOOP,
        HILOGAIN,       UNKNOWN,        DUPLICATE,      SPNOOP,
        SPNOOP,         SPNOOP,         SPNOOP,         SPNOOP
    };

    alist_process(state, ABI, 0x20);
}

void alist_process_nead_sfj(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x20] = {
        SPNOOP,         ADPCM,          CLEARBUFF,      SPNOOP,
        ADDMIXER,       RESAMPLE,       RESAMPLE_ZOH,   SPNOOP,
        SETBUFF,        SPNOOP,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE_MK,  POLEF,          SETLOOP,
        NEAD_16,        INTERL,         ENVSETUP1,      ENVMIXER,
        LOADBUFF,       SAVEBUFF,       ENVSETUP2,      UNKNOWN,
        HILOGAIN,       UNKNOWN,        DUPLICATE,      SPNOOP,
        SPNOOP,         SPNOOP,         SPNOOP,         SPNOOP
    };

    alist_process(state, ABI, 0x20);
}

void alist_process_nead_fz(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x20] = {
        UNKNOWN,        ADPCM,          CLEARBUFF,      SPNOOP,
        ADDMIXER,       RESAMPLE,       SPNOOP,         SPNOOP,
        SETBUFF,        SPNOOP,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     SPNOOP,         SETLOOP,
        NEAD_16,        INTERL,         ENVSETUP1,      ENVMIXER,
        LOADBUFF,       SAVEBUFF,       ENVSETUP2,      UNKNOWN,
        SPNOOP,         UNKNOWN,        DUPLICATE,      SPNOOP,
        SPNOOP,         SPNOOP,         SPNOOP,         SPNOOP
    };

    alist_process(state, ABI, 0x20);
}

void alist_process_nead_wrjb(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x20] = {
        SPNOOP,         ADPCM,          CLEARBUFF,      UNKNOWN,
        ADDMIXER,       RESAMPLE,       RESAMPLE_ZOH,   SPNOOP,
        SETBUFF,        SPNOOP,         DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     SPNOOP,         SETLOOP,
        NEAD_16,        INTERL,         ENVSETUP1,      ENVMIXER,
        LOADBUFF,       SAVEBUFF,       ENVSETUP2,      UNKNOWN,
        HILOGAIN,       UNKNOWN,        DUPLICATE,      FILTER,
        SPNOOP,         SPNOOP,         SPNOOP,         SPNOOP
    };

    alist_process(state, ABI, 0x20);
}

void alist_process_nead_ys(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x18] = {
        UNKNOWN,        ADPCM,          CLEARBUFF,      UNKNOWN,
        ADDMIXER,       RESAMPLE,       RESAMPLE_ZOH,   FILTER,
        SETBUFF,        DUPLICATE,      DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     HILOGAIN,       SETLOOP,
        NEAD_16,        INTERL,         ENVSETUP1,      ENVMIXER,
        LOADBUFF,       SAVEBUFF,       ENVSETUP2,      UNKNOWN
    };

    alist_process(state, ABI, 0x18);
}

void alist_process_nead_1080(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x18] = {
        UNKNOWN,        ADPCM,          CLEARBUFF,      UNKNOWN,
        ADDMIXER,       RESAMPLE,       RESAMPLE_ZOH,   FILTER,
        SETBUFF,        DUPLICATE,      DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     HILOGAIN,       SETLOOP,
        NEAD_16,        INTERL,         ENVSETUP1,      ENVMIXER,
        LOADBUFF,       SAVEBUFF,       ENVSETUP2,      UNKNOWN
    };

    alist_process(state, ABI, 0x18);
}

void alist_process_nead_oot(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x18] = {
        UNKNOWN,        ADPCM,          CLEARBUFF,      UNKNOWN,
        ADDMIXER,       RESAMPLE,       RESAMPLE_ZOH,   FILTER,
        SETBUFF,        DUPLICATE,      DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     HILOGAIN,       SETLOOP,
        NEAD_16,        INTERL,         ENVSETUP1,      ENVMIXER,
        LOADBUFF,       SAVEBUFF,       ENVSETUP2,      UNKNOWN
    };

    alist_process(state, ABI, 0x18);
}

void alist_process_nead_mm(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x18] = {
        UNKNOWN,        ADPCM,          CLEARBUFF,      SPNOOP,
        ADDMIXER,       RESAMPLE,       RESAMPLE_ZOH,   FILTER,
        SETBUFF,        DUPLICATE,      DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     HILOGAIN,       SETLOOP,
        NEAD_16,        INTERL,         ENVSETUP1,      ENVMIXER,
        LOADBUFF,       SAVEBUFF,       ENVSETUP2,      UNKNOWN
    };

    alist_process(state, ABI, 0x18);
}

void alist_process_nead_mmb(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x18] = {
        SPNOOP,         ADPCM,          CLEARBUFF,      SPNOOP,
        ADDMIXER,       RESAMPLE,       RESAMPLE_ZOH,   FILTER,
        SETBUFF,        DUPLICATE,      DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     HILOGAIN,       SETLOOP,
        NEAD_16,        INTERL,         ENVSETUP1,      ENVMIXER,
        LOADBUFF,       SAVEBUFF,       ENVSETUP2,      UNKNOWN
    };

    alist_process(state, ABI, 0x18);
}

void alist_process_nead_ac(usf_state_t* state)
{
    static const acmd_callback_t ABI[0x18] = {
        UNKNOWN,        ADPCM,          CLEARBUFF,      SPNOOP,
        ADDMIXER,       RESAMPLE,       RESAMPLE_ZOH,   FILTER,
        SETBUFF,        DUPLICATE,      DMEMMOVE,       LOADADPCM,
        MIXER,          INTERLEAVE,     HILOGAIN,       SETLOOP,
        NEAD_16,        INTERL,         ENVSETUP1,      ENVMIXER,
        LOADBUFF,       SAVEBUFF,       ENVSETUP2,      UNKNOWN
    };

    alist_process(state, ABI, 0x18);
}
