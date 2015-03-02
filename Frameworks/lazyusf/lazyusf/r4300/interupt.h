/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - interupt.h                                              *
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

#ifndef M64P_R4300_INTERUPT_H
#define M64P_R4300_INTERUPT_H

#include <stdint.h>

#include "usf/usf.h"

void init_interupt(usf_state_t *);

// set to avoid savestates/reset if state may be inconsistent
// (e.g. in the middle of an instruction)
void raise_maskable_interrupt(usf_state_t *, uint32_t cause);

void osal_fastcall gen_interupt(usf_state_t *);
void check_interupt(usf_state_t *);

void translate_event_queue(usf_state_t *, unsigned int base);
void remove_event(usf_state_t *, int type);
void add_interupt_event_count(usf_state_t *, int type, unsigned int count);
void add_interupt_event(usf_state_t *, int type, unsigned int delay);
unsigned int get_event(usf_state_t *, int type);
int get_next_event_type(usf_state_t *);

int save_eventqueue_infos(usf_state_t *, char *buf);
void load_eventqueue_infos(usf_state_t *, char *buf);

#define VI_INT      0x001
#define COMPARE_INT 0x002
#define CHECK_INT   0x004
#define SI_INT      0x008
#define PI_INT      0x010
#define SPECIAL_INT 0x020
#define AI_INT      0x040
#define SP_INT      0x080
#define DP_INT      0x100
#define HW2_INT     0x200
#define NMI_INT     0x400

#endif /* M64P_R4300_INTERUPT_H */
