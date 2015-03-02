/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - rom.c                                                   *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2008 Tillin9                                            *
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stddef.h>

#include "usf/usf.h"

#include "usf/usf_internal.h"

#define M64P_CORE_PROTOTYPES 1
#include "api/m64p_types.h"
#include "api/callbacks.h"

#include "rom.h"
#include "main.h"
#include "util.h"

#include "memory/memory.h"
#include "r4300/r4300.h"

#define DEFAULT 16

#define CHUNKSIZE 1024*128 /* Read files 128KB at a time. */

static m64p_system_type rom_country_code_to_system_type(unsigned short country_code);
static int rom_system_type_to_ai_dac_rate(m64p_system_type system_type);
static int rom_system_type_to_vi_limit(m64p_system_type system_type);

m64p_error open_rom(usf_state_t * state)
{
    if (state->g_rom_size >= sizeof(m64p_rom_header))
        memcpy(&state->ROM_HEADER, state->g_rom, sizeof(m64p_rom_header));

    /* add some useful properties to ROM_PARAMS */
    state->ROM_PARAMS.systemtype = SYSTEM_NTSC /*rom_country_code_to_system_type(ROM_HEADER.Country_code)*/;
    state->ROM_PARAMS.vilimit = rom_system_type_to_vi_limit(state->ROM_PARAMS.systemtype);
    state->ROM_PARAMS.aidacrate = rom_system_type_to_ai_dac_rate(state->ROM_PARAMS.systemtype);
    state->ROM_PARAMS.countperop = COUNT_PER_OP_DEFAULT;

    return M64ERR_SUCCESS;
}

m64p_error close_rom(usf_state_t * state)
{
    free(state->g_rom);
    state->g_rom = NULL;

    DebugMessage(state, M64MSG_STATUS, "Rom closed.");

    return M64ERR_SUCCESS;
}

/********************************************************************************************/
/* ROM utility functions */

// Get the system type associated to a ROM country code.
static m64p_system_type rom_country_code_to_system_type(unsigned short country_code)
{
    switch (country_code & 0xFF)
    {
        // PAL codes
        case 0x44:
        case 0x46:
        case 0x49:
        case 0x50:
        case 0x53:
        case 0x55:
        case 0x58:
        case 0x59:
            return SYSTEM_PAL;

        // NTSC codes
        case 0x37:
        case 0x41:
        case 0x45:
        case 0x4a:
        default: // Fallback for unknown codes
            return SYSTEM_NTSC;
    }
}

// Get the VI (vertical interrupt) limit associated to a ROM system type.
static int rom_system_type_to_vi_limit(m64p_system_type system_type)
{
    switch (system_type)
    {
        case SYSTEM_PAL:
        case SYSTEM_MPAL:
            return 50;

        case SYSTEM_NTSC:
        default:
            return 60;
    }
}

static int rom_system_type_to_ai_dac_rate(m64p_system_type system_type)
{
    switch (system_type)
    {
        case SYSTEM_PAL:
            return 49656530;
        case SYSTEM_MPAL:
            return 48628316;
        case SYSTEM_NTSC:
        default:
            return 48681812;
    }
}

