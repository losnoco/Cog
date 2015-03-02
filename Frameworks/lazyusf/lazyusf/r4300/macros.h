/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - macros.h                                                *
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

#ifndef M64P_R4300_MACROS_H
#define M64P_R4300_MACROS_H

#define sign_extended(a) a = (long long)((int)a)
#define sign_extendedb(a) a = (long long)((signed char)a)
#define sign_extendedh(a) a = (long long)((short)a)

#define rrt *state->PC->f.r.rt
#define rrd *state->PC->f.r.rd
#define rfs state->PC->f.r.nrd
#define rrs *state->PC->f.r.rs
#define rsa state->PC->f.r.sa
#define irt *state->PC->f.i.rt
#define ioffset state->PC->f.i.immediate
#define iimmediate state->PC->f.i.immediate
#define irs *state->PC->f.i.rs
#define ibase *state->PC->f.i.rs
#define jinst_index state->PC->f.j.inst_index
#define lfbase state->PC->f.lf.base
#define lfft state->PC->f.lf.ft
#define lfoffset state->PC->f.lf.offset
#define cfft state->PC->f.cf.ft
#define cffs state->PC->f.cf.fs
#define cffd state->PC->f.cf.fd

// 32 bits macros
#ifndef M64P_BIG_ENDIAN
#define rrt32 *((int*)state->PC->f.r.rt)
#define rrd32 *((int*)state->PC->f.r.rd)
#define rrs32 *((int*)state->PC->f.r.rs)
#define irs32 *((int*)state->PC->f.i.rs)
#define irt32 *((int*)state->PC->f.i.rt)
#else
#define rrt32 *((int*)state->PC->f.r.rt+1)
#define rrd32 *((int*)state->PC->f.r.rd+1)
#define rrs32 *((int*)state->PC->f.r.rs+1)
#define irs32 *((int*)state->PC->f.i.rs+1)
#define irt32 *((int*)state->PC->f.i.rt+1)
#endif

#endif /* M64P_R4300_MACROS_H */

