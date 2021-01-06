/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - cp0.c                                                   *
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

#include "r4300.h"
#include "cp0.h"
#include "exception.h"

#include "new_dynarec/new_dynarec.h"

#ifdef COMPARE_CORE
#include "api/debugger.h"
#endif

#ifdef DBG
#include "debugger/dbg_types.h"
#include "debugger/debugger.h"
#endif

/* global functions */
int osal_fastcall check_cop1_unusable(usf_state_t * state)
{
   if (!(state->g_cp0_regs[CP0_STATUS_REG] & 0x20000000))
     {
    state->g_cp0_regs[CP0_CAUSE_REG] = (11 << 2) | 0x10000000;
    exception_general(state);
    return 1;
     }
   return 0;
}

void update_count(usf_state_t * state)
{
#ifdef NEW_DYNAREC
    if (r4300emu != CORE_DYNAREC)
    {
#endif
        uint32_t count = ((state->PC->addr - state->last_addr) >> 2) * state->count_per_op;
        state->g_cp0_regs[CP0_COUNT_REG] += count;
        state->cycle_count += count;
        state->last_addr = state->PC->addr;
#ifdef NEW_DYNAREC
    }
#endif

/*#ifdef DBG
   if (g_DebuggerActive && !delay_slot) update_debugger(PC->addr);
#endif
*/
}

