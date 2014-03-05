/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - alist.h                                         *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2014 Bobby Smiles                                       *
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

#ifndef ALIST_H
#define ALIST_H

void alist_process_audio(usf_state_t* state);
void alist_process_audio_ge(usf_state_t* state);
void alist_process_audio_bc(usf_state_t* state);
void alist_process_nead_mk(usf_state_t* state);
void alist_process_nead_sfj(usf_state_t* state);
void alist_process_nead_sf(usf_state_t* state);
void alist_process_nead_fz(usf_state_t* state);
void alist_process_nead_wrjb(usf_state_t* state);
void alist_process_nead_ys(usf_state_t* state);
void alist_process_nead_1080(usf_state_t* state);
void alist_process_nead_oot(usf_state_t* state);
void alist_process_nead_mm(usf_state_t* state);
void alist_process_nead_mmb(usf_state_t* state);
void alist_process_nead_ac(usf_state_t* state);
void alist_process_naudio(usf_state_t* state);
void alist_process_naudio_bk(usf_state_t* state);
void alist_process_naudio_dk(usf_state_t* state);
void alist_process_naudio_mp3(usf_state_t* state);
void alist_process_naudio_cbfd(usf_state_t* state);

#endif

