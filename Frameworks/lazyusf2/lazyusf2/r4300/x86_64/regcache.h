/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - regcache.h                                              *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2007 Richard Goedeken (Richard42)                       *
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

#ifndef M64P_R4300_REGCACHE_H
#define M64P_R4300_REGCACHE_H

#include "r4300/recomp.h"

void init_cache(usf_state_t *, precomp_instr* start);
void free_registers_move_start(usf_state_t *);
void free_all_registers(usf_state_t *);
void free_register(usf_state_t *, int reg);
int is64(usf_state_t *, unsigned int *addr);
int lru_register(usf_state_t *);
int lru_base_register(usf_state_t *);
void set_register_state(usf_state_t *, int reg, unsigned int *addr, int dirty, int is64bits);
int lock_register(usf_state_t *, int reg);
void unlock_register(usf_state_t *, int reg);
int allocate_register_32(usf_state_t *, unsigned int *addr);
int allocate_register_64(usf_state_t *, unsigned long long *addr);
int allocate_register_32_w(usf_state_t *, unsigned int *addr);
int allocate_register_64_w(usf_state_t *, unsigned long long *addr);
void allocate_register_32_manually(usf_state_t *, int reg, unsigned int *addr);
void allocate_register_32_manually_w(usf_state_t *, int reg, unsigned int *addr);
void build_wrappers(usf_state_t *, precomp_instr*, int, int, precomp_block*);

#endif /* M64P_R4300_REGCACHE_H */

