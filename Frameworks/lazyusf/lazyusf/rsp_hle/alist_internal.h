/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus-rsp-hle - alist_internal.h                                *
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

#ifndef ALIST_INTERNAL_H
#define ALIST_INTERNAL_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef void (*acmd_callback_t)(usf_state_t* state, uint32_t w1, uint32_t w2);

void alist_process(usf_state_t* state, const acmd_callback_t abi[], unsigned int abi_size);
uint32_t alist_get_address(usf_state_t* state, uint32_t so, const uint32_t *segments, size_t n);
void alist_set_address(usf_state_t* state, uint32_t so, uint32_t *segments, size_t n);
void alist_clear(usf_state_t* state, uint16_t dmem, uint16_t count);
void alist_load(usf_state_t* state, uint16_t dmem, uint32_t address, uint16_t count);
void alist_save(usf_state_t* state, uint16_t dmem, uint32_t address, uint16_t count);
void alist_move(usf_state_t* state, uint16_t dmemo, uint16_t dmemi, uint16_t count);
void alist_copy_every_other_sample(usf_state_t* state, uint16_t dmemo, uint16_t dmemi, uint16_t count);
void alist_repeat64(usf_state_t* state, uint16_t dmemo, uint16_t dmemi, uint8_t count);
void alist_copy_blocks(usf_state_t* state, uint16_t dmemo, uint16_t dmemi, uint16_t block_size, uint8_t count);
void alist_interleave(usf_state_t* state, uint16_t dmemo, uint16_t left, uint16_t right, uint16_t count);

void alist_envmix_exp(
        usf_state_t* state,
        bool init,
        bool aux,
        uint16_t dmem_dl, uint16_t dmem_dr,
        uint16_t dmem_wl, uint16_t dmem_wr,
        uint16_t dmemi, uint16_t count,
        int16_t dry, int16_t wet,
        const int16_t *vol,
        const int16_t *target,
        const int32_t *rate,
        uint32_t address);

void alist_envmix_lin(
        usf_state_t* state,
        bool init,
        uint16_t dmem_dl, uint16_t dmem_dr,
        uint16_t dmem_wl, uint16_t dmem_wr,
        uint16_t dmemi, uint16_t count,
        int16_t dry, int16_t wet,
        const int16_t *vol,
        const int16_t *target,
        const int32_t *rate,
        uint32_t address);

void alist_envmix_nead(
        usf_state_t* state,
        bool swap_wet_LR,
        uint16_t dmem_dl,
        uint16_t dmem_dr,
        uint16_t dmem_wl,
        uint16_t dmem_wr,
        uint16_t dmemi,
        unsigned count,
        uint16_t *env_values,
        uint16_t *env_steps,
        const int16_t *xors);

void alist_mix(usf_state_t* state, uint16_t dmemo, uint16_t dmemi, uint16_t count, int16_t gain);
void alist_multQ44(usf_state_t* state, uint16_t dmem, uint16_t count, int8_t gain);
void alist_add(usf_state_t* state, uint16_t dmemo, uint16_t dmemi, uint16_t count);

void alist_adpcm(
        usf_state_t* state,
        bool init,
        bool loop,
        bool two_bit_per_sample,
        uint16_t dmemo,
        uint16_t dmemi,
        uint16_t count,
        const int16_t* codebook,
        uint32_t loop_address,
        uint32_t last_frame_address);

void alist_resample(
        usf_state_t* state,
        bool init,
        bool flag2,
        uint16_t dmemo, uint16_t dmemi, uint16_t count,
        uint32_t pitch, uint32_t address);

void alist_resample_zoh(
        usf_state_t* state,
        uint16_t dmemo,
        uint16_t dmemi,
        uint16_t count,
        uint32_t pitch,
        uint32_t pitch_accu);

void alist_filter(
        usf_state_t* state,
        uint16_t dmem,
        uint16_t count,
        uint32_t address,
        const uint32_t* lut_address);

void alist_polef(
        usf_state_t* state,
        bool init,
        uint16_t dmemo,
        uint16_t dmemi,
        uint16_t count,
        uint16_t gain,
        int16_t* table,
        uint32_t address);
/*
 * Audio flags
 */

#define A_INIT          0x01
#define A_CONTINUE      0x00
#define A_LOOP          0x02
#define A_OUT           0x02
#define A_LEFT          0x02
#define A_RIGHT         0x00
#define A_VOL           0x04
#define A_RATE          0x00
#define A_AUX           0x08
#define A_NOAUX         0x00
#define A_MAIN          0x00
#define A_MIX           0x10

#endif
