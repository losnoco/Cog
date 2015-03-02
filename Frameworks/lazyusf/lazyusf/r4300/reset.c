/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - reset.c                                                 *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2011 CasualJames                                        *
 *   Copyright (C) 2008-2009 Richard Goedeken                              *
 *   Copyright (C) 2008 Ebenblues Nmn Okaygo Tillin9                       *
 *   Hard reset based on code by hacktarux.                                *
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

#include "r4300/reset.h"
#include "r4300/r4300.h"
#include "r4300/interupt.h"
#include "memory/memory.h"
#include "r4300/cached_interp.h"

void reset_hard(usf_state_t * state)
{
    init_memory(state, 0x800000);
    r4300_reset_hard(state);
    r4300_reset_soft(state);
    state->last_addr = 0xa4000040;
    state->next_interupt = 624999;
    init_interupt(state);
    if(state->r4300emu != CORE_PURE_INTERPRETER)
    {
        free_blocks(state);
        init_blocks(state);
    }
    generic_jump_to(state, state->last_addr);
}

void reset_soft(usf_state_t * state)
{
    add_interupt_event(state, HW2_INT, 0);  /* Hardware 2 Interrupt immediately */
    add_interupt_event(state, NMI_INT, 50000000);  /* Non maskable Interrupt after 1/2 second */
}
