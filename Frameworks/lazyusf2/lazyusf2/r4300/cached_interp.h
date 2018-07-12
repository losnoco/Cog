/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - cached_interp.h                                         *
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

#ifndef M64P_R4300_CACHED_INTERP_H
#define M64P_R4300_CACHED_INTERP_H

#include "usf/usf.h"

#include "osal/preproc.h"

#include "ops.h"
/* FIXME: use forward declaration for precomp_block */
#include "recomp.h"

#include "osal/preproc.h"

extern const cpu_instruction_table cached_interpreter_table;

void osal_fastcall init_blocks(usf_state_t *);
void osal_fastcall free_blocks(usf_state_t *);
void osal_fastcall jump_to_func(usf_state_t *);

/* Jumps to the given address. This is for the cached interpreter / dynarec. */
#define jump_to(a) { state->jump_to_address = a; jump_to_func(state); }

#endif /* M64P_R4300_CACHED_INTERP_H */
