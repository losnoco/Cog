/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - memory.c                                                *
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

#include "usf/barray.h"

#include "memory.h"

#include "api/m64p_types.h"
#include "api/callbacks.h"

#include "main/main.h"
#include "main/rom.h"

#include "r4300/r4300.h"
#include "r4300/r4300_core.h"
#include "r4300/cached_interp.h"
#include "r4300/new_dynarec/new_dynarec.h"
#include "r4300/recomph.h"
#include "r4300/ops.h"
#include "r4300/tlb.h"

#include "rdp/rdp_core.h"
#include "rsp/rsp_core.h"

#include "ai/ai_controller.h"
#include "pi/pi_controller.h"
#include "ri/ri_controller.h"
#include "si/si_controller.h"
#include "vi/vi_controller.h"

#ifdef DBG
#include "debugger/dbg_types.h"
#include "debugger/dbg_memory.h"
#include "debugger/dbg_breakpoints.h"

#include <string.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "osal/preproc.h"

typedef int (*readfn)(void*, uint32_t, uint32_t*);
typedef int (*writefn)(void*, uint32_t, uint32_t, uint32_t);

static osal_inline unsigned int bshift(uint32_t address)
{
    return ((address & 3) ^ 3) << 3;
}

static osal_inline unsigned int hshift(uint32_t address)
{
    return ((address & 2) ^ 2) << 3;
}

static int readb(readfn read_word, void* opaque, uint32_t address, unsigned long long int* value)
{
    uint32_t w;
    unsigned shift = bshift(address);
    int result = read_word(opaque, address, &w);
    *value = (w >> shift) & 0xff;

    return result;
}

static int readh(readfn read_word, void* opaque, uint32_t address, unsigned long long int* value)
{
    uint32_t w;
    unsigned shift = hshift(address);
    int result = read_word(opaque, address, &w);
    *value = (w >> shift) & 0xffff;

    return result;
}

static int readw(readfn read_word, void* opaque, uint32_t address, unsigned long long int* value)
{
    uint32_t w;
    int result = read_word(opaque, address, &w);
    *value = w;

    return result;
}

static int readd(readfn read_word, void* opaque, uint32_t address, unsigned long long int* value)
{
    uint32_t w[2];
    int result =
    read_word(opaque, address    , &w[0]);
    read_word(opaque, address + 4, &w[1]);
    *value = ((uint64_t)w[0] << 32) | w[1];

    return result;
}

static int writeb(writefn write_word, void* opaque, uint32_t address, uint8_t value)
{
    unsigned int shift = bshift(address);
    uint32_t w = (uint32_t)value << shift;
    uint32_t mask = (uint32_t)0xff << shift;

    return write_word(opaque, address, w, mask);
}

static int writeh(writefn write_word, void* opaque, uint32_t address, uint16_t value)
{
    unsigned int shift = hshift(address);
    uint32_t w = (uint32_t)value << shift;
    uint32_t mask = (uint32_t)0xffff << shift;

    return write_word(opaque, address, w, mask);
}

static int writew(writefn write_word, void* opaque, uint32_t address, uint32_t value)
{
    return write_word(opaque, address, value, ~0U);
}

static int writed(writefn write_word, void* opaque, uint32_t address, uint64_t value)
{
    int result =
    write_word(opaque, address    , value >> 32, ~0U);
    write_word(opaque, address + 4, value      , ~0U);

    return result;
}


static void osal_fastcall invalidate_code(usf_state_t * state, uint32_t address)
{
    if (state->r4300emu != CORE_PURE_INTERPRETER && !state->invalid_code[address>>12])
        if (state->blocks[address>>12]->block[(address&0xFFF)/4].ops !=
            state->current_instruction_table.NOTCOMPILED)
            state->invalid_code[address>>12] = 1;
}


static void osal_fastcall read_nothing(usf_state_t * state)
{
    *state->rdword = 0;
}

static void osal_fastcall read_nothingb(usf_state_t * state)
{
    *state->rdword = 0;
}

static void osal_fastcall read_nothingh(usf_state_t * state)
{
    *state->rdword = 0;
}

static void osal_fastcall read_nothingd(usf_state_t * state)
{
    *state->rdword = 0;
}

static void osal_fastcall write_nothing(usf_state_t * state)
{
}

static void osal_fastcall write_nothingb(usf_state_t * state)
{
}

static void osal_fastcall write_nothingh(usf_state_t * state)
{
}

static void osal_fastcall write_nothingd(usf_state_t * state)
{
}

static void osal_fastcall read_nomem(usf_state_t * state)
{
    state->address = virtual_to_physical_address(state,state->address,0);
    if (state->address == 0x00000000) return;
    read_word_in_memory();
}

static void osal_fastcall read_nomemb(usf_state_t * state)
{
    state->address = virtual_to_physical_address(state,state->address,0);
    if (state->address == 0x00000000) return;
    read_byte_in_memory();
}

static void osal_fastcall read_nomemh(usf_state_t * state)
{
    state->address = virtual_to_physical_address(state,state->address,0);
    if (state->address == 0x00000000) return;
    read_hword_in_memory();
}

static void osal_fastcall read_nomemd(usf_state_t * state)
{
    state->address = virtual_to_physical_address(state,state->address,0);
    if (state->address == 0x00000000) return;
    read_dword_in_memory();
}

static void osal_fastcall write_nomem(usf_state_t * state)
{
    invalidate_code(state,state->address);
    state->address = virtual_to_physical_address(state,state->address,1);
    if (state->address == 0x00000000) return;
    write_word_in_memory();
}

static void osal_fastcall write_nomemb(usf_state_t * state)
{
    invalidate_code(state,state->address);
    state->address = virtual_to_physical_address(state,state->address,1);
    if (state->address == 0x00000000) return;
    write_byte_in_memory();
}

static void osal_fastcall write_nomemh(usf_state_t * state)
{
    invalidate_code(state,state->address);
    state->address = virtual_to_physical_address(state,state->address,1);
    if (state->address == 0x00000000) return;
    write_hword_in_memory();
}

static void osal_fastcall write_nomemd(usf_state_t * state)
{
    invalidate_code(state,state->address);
    state->address = virtual_to_physical_address(state,state->address,1);
    if (state->address == 0x00000000) return;
    write_dword_in_memory();
}


void osal_fastcall read_rdram(usf_state_t * state)
{
    readw(read_rdram_dram, &state->g_ri, state->address, state->rdword);
}

void osal_fastcall read_rdramb(usf_state_t * state)
{
    readb(read_rdram_dram, &state->g_ri, state->address, state->rdword);
}

void osal_fastcall read_rdramh(usf_state_t * state)
{
    readh(read_rdram_dram, &state->g_ri, state->address, state->rdword);
}

void osal_fastcall read_rdramd(usf_state_t * state)
{
    readd(read_rdram_dram, &state->g_ri, state->address, state->rdword);
}

void osal_fastcall write_rdram(usf_state_t * state)
{
    writew(write_rdram_dram, &state->g_ri, state->address, state->cpu_word);
}

void osal_fastcall write_rdramb(usf_state_t * state)
{
    writeb(write_rdram_dram, &state->g_ri, state->address, state->cpu_byte);
}

void osal_fastcall write_rdramh(usf_state_t * state)
{
    writeh(write_rdram_dram, &state->g_ri, state->address, state->cpu_hword);
}

void osal_fastcall write_rdramd(usf_state_t * state)
{
    writed(write_rdram_dram, &state->g_ri, state->address, state->cpu_dword);
}


void osal_fastcall read_rdram_tracked(usf_state_t * state)
{
    readw(read_rdram_dram_tracked, state, state->address, state->rdword);
}

void osal_fastcall read_rdram_trackedb(usf_state_t * state)
{
    readb(read_rdram_dram_tracked, state, state->address, state->rdword);
}

void osal_fastcall read_rdram_trackedh(usf_state_t * state)
{
    readh(read_rdram_dram_tracked, state, state->address, state->rdword);
}

void osal_fastcall read_rdram_trackedd(usf_state_t * state)
{
    readd(read_rdram_dram_tracked, state, state->address, state->rdword);
}

void osal_fastcall write_rdram_tracked(usf_state_t * state)
{
    writew(write_rdram_dram_tracked, state, state->address, state->cpu_word);
}

void osal_fastcall write_rdram_trackedb(usf_state_t * state)
{
    writeb(write_rdram_dram_tracked, state, state->address, state->cpu_byte);
}

void osal_fastcall write_rdram_trackedh(usf_state_t * state)
{
    writeh(write_rdram_dram_tracked, state, state->address, state->cpu_hword);
}

void osal_fastcall write_rdram_trackedd(usf_state_t * state)
{
    writed(write_rdram_dram_tracked, state, state->address, state->cpu_dword);
}


static void osal_fastcall read_rdramreg(usf_state_t * state)
{
    readw(read_rdram_regs, &state->g_ri, state->address, state->rdword);
}

static void osal_fastcall read_rdramregb(usf_state_t * state)
{
    readb(read_rdram_regs, &state->g_ri, state->address, state->rdword);
}

static void osal_fastcall read_rdramregh(usf_state_t * state)
{
    readh(read_rdram_regs, &state->g_ri, state->address, state->rdword);
}

static void osal_fastcall read_rdramregd(usf_state_t * state)
{
    readd(read_rdram_regs, &state->g_ri, state->address, state->rdword);
}

static void osal_fastcall write_rdramreg(usf_state_t * state)
{
    writew(write_rdram_regs, &state->g_ri, state->address, state->cpu_word);
}

static void osal_fastcall write_rdramregb(usf_state_t * state)
{
    writeb(write_rdram_regs, &state->g_ri, state->address, state->cpu_byte);
}

static void osal_fastcall write_rdramregh(usf_state_t * state)
{
    writeh(write_rdram_regs, &state->g_ri, state->address, state->cpu_hword);
}

static void osal_fastcall write_rdramregd(usf_state_t * state)
{
    writed(write_rdram_regs, &state->g_ri, state->address, state->cpu_dword);
}


static void osal_fastcall read_rspmem(usf_state_t * state)
{
    readw(read_rsp_mem, &state->g_sp, state->address, state->rdword);
}

static void osal_fastcall read_rspmemb(usf_state_t * state)
{
    readb(read_rsp_mem, &state->g_sp, state->address, state->rdword);
}

static void osal_fastcall read_rspmemh(usf_state_t * state)
{
    readh(read_rsp_mem, &state->g_sp, state->address, state->rdword);
}

static void osal_fastcall read_rspmemd(usf_state_t * state)
{
    readd(read_rsp_mem, &state->g_sp, state->address, state->rdword);
}

static void osal_fastcall write_rspmem(usf_state_t * state)
{
    writew(write_rsp_mem, &state->g_sp, state->address, state->cpu_word);
}

static void osal_fastcall write_rspmemb(usf_state_t * state)
{
    writeb(write_rsp_mem, &state->g_sp, state->address, state->cpu_byte);
}

static void osal_fastcall write_rspmemh(usf_state_t * state)
{
    writeh(write_rsp_mem, &state->g_sp, state->address, state->cpu_hword);
}

static void osal_fastcall write_rspmemd(usf_state_t * state)
{
    writed(write_rsp_mem, &state->g_sp, state->address, state->cpu_dword);
}


static void osal_fastcall read_rspreg(usf_state_t * state)
{
    readw(read_rsp_regs, &state->g_sp, state->address, state->rdword);
}

static void osal_fastcall read_rspregb(usf_state_t * state)
{
    readb(read_rsp_regs, &state->g_sp, state->address, state->rdword);
}

static void osal_fastcall read_rspregh(usf_state_t * state)
{
    readh(read_rsp_regs, &state->g_sp, state->address, state->rdword);
}

static void osal_fastcall read_rspregd(usf_state_t * state)
{
    readd(read_rsp_regs, &state->g_sp, state->address, state->rdword);
}

static void osal_fastcall write_rspreg(usf_state_t * state)
{
    writew(write_rsp_regs, &state->g_sp, state->address, state->cpu_word);
}

static void osal_fastcall write_rspregb(usf_state_t * state)
{
    writeb(write_rsp_regs, &state->g_sp, state->address, state->cpu_byte);
}

static void osal_fastcall write_rspregh(usf_state_t * state)
{
    writeh(write_rsp_regs, &state->g_sp, state->address, state->cpu_hword);
}

static void osal_fastcall write_rspregd(usf_state_t * state)
{
    writed(write_rsp_regs, &state->g_sp, state->address, state->cpu_dword);
}


static void osal_fastcall read_rspreg2(usf_state_t * state)
{
    readw(read_rsp_regs2, &state->g_sp, state->address, state->rdword);
}

static void osal_fastcall read_rspreg2b(usf_state_t * state)
{
    readb(read_rsp_regs2, &state->g_sp, state->address, state->rdword);
}

static void osal_fastcall read_rspreg2h(usf_state_t * state)
{
    readh(read_rsp_regs2, &state->g_sp, state->address, state->rdword);
}

static void osal_fastcall read_rspreg2d(usf_state_t * state)
{
    readd(read_rsp_regs2, &state->g_sp, state->address, state->rdword);
}

static void osal_fastcall write_rspreg2(usf_state_t * state)
{
    writew(write_rsp_regs2, &state->g_sp, state->address, state->cpu_word);
}

static void osal_fastcall write_rspreg2b(usf_state_t * state)
{
    writeb(write_rsp_regs2, &state->g_sp, state->address, state->cpu_byte);
}

static void osal_fastcall write_rspreg2h(usf_state_t * state)
{
    writeh(write_rsp_regs2, &state->g_sp, state->address, state->cpu_hword);
}

static void osal_fastcall write_rspreg2d(usf_state_t * state)
{
    writed(write_rsp_regs2, &state->g_sp, state->address, state->cpu_dword);
}


static void osal_fastcall read_dp(usf_state_t * state)
{
    readw(read_dpc_regs, &state->g_dp, state->address, state->rdword);
}

static void osal_fastcall read_dpb(usf_state_t * state)
{
    readb(read_dpc_regs, &state->g_dp, state->address, state->rdword);
}

static void osal_fastcall read_dph(usf_state_t * state)
{
    readh(read_dpc_regs, &state->g_dp, state->address, state->rdword);
}

static void osal_fastcall read_dpd(usf_state_t * state)
{
    readd(read_dpc_regs, &state->g_dp, state->address, state->rdword);
}

static void osal_fastcall write_dp(usf_state_t * state)
{
    writew(write_dpc_regs, &state->g_dp, state->address, state->cpu_word);
}

static void osal_fastcall write_dpb(usf_state_t * state)
{
    writeb(write_dpc_regs, &state->g_dp, state->address, state->cpu_byte);
}

static void osal_fastcall write_dph(usf_state_t * state)
{
    writeh(write_dpc_regs, &state->g_dp, state->address, state->cpu_hword);
}

static void osal_fastcall write_dpd(usf_state_t * state)
{
    writed(write_dpc_regs, &state->g_dp, state->address, state->cpu_dword);
}


static void osal_fastcall read_dps(usf_state_t * state)
{
    readw(read_dps_regs, &state->g_dp, state->address, state->rdword);
}

static void osal_fastcall read_dpsb(usf_state_t * state)
{
    readb(read_dps_regs, &state->g_dp, state->address, state->rdword);
}

static void osal_fastcall read_dpsh(usf_state_t * state)
{
    readh(read_dps_regs, &state->g_dp, state->address, state->rdword);
}

static void osal_fastcall read_dpsd(usf_state_t * state)
{
    readd(read_dps_regs, &state->g_dp, state->address, state->rdword);
}

static void osal_fastcall write_dps(usf_state_t * state)
{
    writew(write_dps_regs, &state->g_dp, state->address, state->cpu_word);
}

static void osal_fastcall write_dpsb(usf_state_t * state)
{
    writeb(write_dps_regs, &state->g_dp, state->address, state->cpu_byte);
}

static void osal_fastcall write_dpsh(usf_state_t * state)
{
    writeh(write_dps_regs, &state->g_dp, state->address, state->cpu_hword);
}

static void osal_fastcall write_dpsd(usf_state_t * state)
{
    writed(write_dps_regs, &state->g_dp, state->address, state->cpu_dword);
}


static void osal_fastcall read_mi(usf_state_t * state)
{
    readw(read_mi_regs, &state->g_r4300, state->address, state->rdword);
}

static void osal_fastcall read_mib(usf_state_t * state)
{
    readb(read_mi_regs, &state->g_r4300, state->address, state->rdword);
}

static void osal_fastcall read_mih(usf_state_t * state)
{
    readh(read_mi_regs, &state->g_r4300, state->address, state->rdword);
}

static void osal_fastcall read_mid(usf_state_t * state)
{
    readd(read_mi_regs, &state->g_r4300, state->address, state->rdword);
}

static void osal_fastcall write_mi(usf_state_t * state)
{
    writew(write_mi_regs, &state->g_r4300, state->address, state->cpu_word);
}

static void osal_fastcall write_mib(usf_state_t * state)
{
    writeb(write_mi_regs, &state->g_r4300, state->address, state->cpu_byte);
}

static void osal_fastcall write_mih(usf_state_t * state)
{
    writeh(write_mi_regs, &state->g_r4300, state->address, state->cpu_hword);
}

static void osal_fastcall write_mid(usf_state_t * state)
{
    writed(write_mi_regs, &state->g_r4300, state->address, state->cpu_dword);
}


static void osal_fastcall read_vi(usf_state_t * state)
{
    readw(read_vi_regs, &state->g_vi, state->address, state->rdword);
}

static void osal_fastcall read_vib(usf_state_t * state)
{
    readb(read_vi_regs, &state->g_vi, state->address, state->rdword);
}

static void osal_fastcall read_vih(usf_state_t * state)
{
    readh(read_vi_regs, &state->g_vi, state->address, state->rdword);
}

static void osal_fastcall read_vid(usf_state_t * state)
{
    readd(read_vi_regs, &state->g_vi, state->address, state->rdword);
}

static void osal_fastcall write_vi(usf_state_t * state)
{
    writew(write_vi_regs, &state->g_vi, state->address, state->cpu_word);
}

static void osal_fastcall write_vib(usf_state_t * state)
{
    writeb(write_vi_regs, &state->g_vi, state->address, state->cpu_byte);
}

static void osal_fastcall write_vih(usf_state_t * state)
{
    writeh(write_vi_regs, &state->g_vi, state->address, state->cpu_hword);
}

static void osal_fastcall write_vid(usf_state_t * state)
{
    writed(write_vi_regs, &state->g_vi, state->address, state->cpu_dword);
}


static void osal_fastcall read_ai(usf_state_t * state)
{
    readw(read_ai_regs, &state->g_ai, state->address, state->rdword);
}

static void osal_fastcall read_aib(usf_state_t * state)
{
    readb(read_ai_regs, &state->g_ai, state->address, state->rdword);
}

static void osal_fastcall read_aih(usf_state_t * state)
{
    readh(read_ai_regs, &state->g_ai, state->address, state->rdword);
}

static void osal_fastcall read_aid(usf_state_t * state)
{
    readd(read_ai_regs, &state->g_ai, state->address, state->rdword);
}

static void osal_fastcall write_ai(usf_state_t * state)
{
    writew(write_ai_regs, &state->g_ai, state->address, state->cpu_word);
}

static void osal_fastcall write_aib(usf_state_t * state)
{
    writeb(write_ai_regs, &state->g_ai, state->address, state->cpu_byte);
}

static void osal_fastcall write_aih(usf_state_t * state)
{
    writeh(write_ai_regs, &state->g_ai, state->address, state->cpu_hword);
}

static void osal_fastcall write_aid(usf_state_t * state)
{
    writed(write_ai_regs, &state->g_ai, state->address, state->cpu_dword);
}


static void osal_fastcall read_pi(usf_state_t * state)
{
    readw(read_pi_regs, &state->g_pi, state->address, state->rdword);
}

static void osal_fastcall read_pib(usf_state_t * state)
{
    readb(read_pi_regs, &state->g_pi, state->address, state->rdword);
}

static void osal_fastcall read_pih(usf_state_t * state)
{
    readh(read_pi_regs, &state->g_pi, state->address, state->rdword);
}

static void osal_fastcall read_pid(usf_state_t * state)
{
    readd(read_pi_regs, &state->g_pi, state->address, state->rdword);
}

static void osal_fastcall write_pi(usf_state_t * state)
{
    writew(write_pi_regs, &state->g_pi, state->address, state->cpu_word);
}

static void osal_fastcall write_pib(usf_state_t * state)
{
    writeb(write_pi_regs, &state->g_pi, state->address, state->cpu_byte);
}

static void osal_fastcall write_pih(usf_state_t * state)
{
    writeh(write_pi_regs, &state->g_pi, state->address, state->cpu_hword);
}

static void osal_fastcall write_pid(usf_state_t * state)
{
    writed(write_pi_regs, &state->g_pi, state->address, state->cpu_dword);
}


static void osal_fastcall read_ri(usf_state_t * state)
{
    readw(read_ri_regs, &state->g_ri, state->address, state->rdword);
}

static void osal_fastcall read_rib(usf_state_t * state)
{
    readb(read_ri_regs, &state->g_ri, state->address, state->rdword);
}

static void osal_fastcall read_rih(usf_state_t * state)
{
    readh(read_ri_regs, &state->g_ri, state->address, state->rdword);
}

static void osal_fastcall read_rid(usf_state_t * state)
{
    readd(read_ri_regs, &state->g_ri, state->address, state->rdword);
}

static void osal_fastcall write_ri(usf_state_t * state)
{
    writew(write_ri_regs, &state->g_ri, state->address, state->cpu_word);
}

static void osal_fastcall write_rib(usf_state_t * state)
{
    writeb(write_ri_regs, &state->g_ri, state->address, state->cpu_byte);
}

static void osal_fastcall write_rih(usf_state_t * state)
{
    writeh(write_ri_regs, &state->g_ri, state->address, state->cpu_hword);
}

static void osal_fastcall write_rid(usf_state_t * state)
{
    writed(write_ri_regs, &state->g_ri, state->address, state->cpu_dword);
}


static void osal_fastcall read_si(usf_state_t * state)
{
    readw(read_si_regs, &state->g_si, state->address, state->rdword);
}

static void osal_fastcall read_sib(usf_state_t * state)
{
    readb(read_si_regs, &state->g_si, state->address, state->rdword);
}

static void osal_fastcall read_sih(usf_state_t * state)
{
    readh(read_si_regs, &state->g_si, state->address, state->rdword);
}

static void osal_fastcall read_sid(usf_state_t * state)
{
    readd(read_si_regs, &state->g_si, state->address, state->rdword);
}

static void osal_fastcall write_si(usf_state_t * state)
{
    writew(write_si_regs, &state->g_si, state->address, state->cpu_word);
}

static void osal_fastcall write_sib(usf_state_t * state)
{
    writeb(write_si_regs, &state->g_si, state->address, state->cpu_byte);
}

static void osal_fastcall write_sih(usf_state_t * state)
{
    writeh(write_si_regs, &state->g_si, state->address, state->cpu_hword);
}

static void osal_fastcall write_sid(usf_state_t * state)
{
    writed(write_si_regs, &state->g_si, state->address, state->cpu_dword);
}


static void osal_fastcall read_rom(usf_state_t * state)
{
    readw(read_cart_rom, &state->g_pi, state->address, state->rdword);
}

static void osal_fastcall read_romb(usf_state_t * state)
{
    readb(read_cart_rom, &state->g_pi, state->address, state->rdword);
}

static void osal_fastcall read_romh(usf_state_t * state)
{
    readh(read_cart_rom, &state->g_pi, state->address, state->rdword);
}

static void osal_fastcall read_romd(usf_state_t * state)
{
    readd(read_cart_rom, &state->g_pi, state->address, state->rdword);
}

static void osal_fastcall write_rom(usf_state_t * state)
{
    writew(write_cart_rom, &state->g_pi, state->address, state->cpu_word);
}


static void osal_fastcall read_rom_tracked(usf_state_t * state)
{
    readw(read_cart_rom_tracked, state, state->address, state->rdword);
}

static void osal_fastcall read_rom_trackedb(usf_state_t * state)
{
    readb(read_cart_rom_tracked, state, state->address, state->rdword);
}

static void osal_fastcall read_rom_trackedh(usf_state_t * state)
{
    readh(read_cart_rom_tracked, state, state->address, state->rdword);
}

static void osal_fastcall read_rom_trackedd(usf_state_t * state)
{
    readd(read_cart_rom_tracked, state, state->address, state->rdword);
}


static void osal_fastcall read_pif(usf_state_t * state)
{
    readw(read_pif_ram, &state->g_si, state->address, state->rdword);
}

static void osal_fastcall read_pifb(usf_state_t * state)
{
    readb(read_pif_ram, &state->g_si, state->address, state->rdword);
}

static void osal_fastcall read_pifh(usf_state_t * state)
{
    readh(read_pif_ram, &state->g_si, state->address, state->rdword);
}

static void osal_fastcall read_pifd(usf_state_t * state)
{
    readd(read_pif_ram, &state->g_si, state->address, state->rdword);
}

static void osal_fastcall write_pif(usf_state_t * state)
{
    writew(write_pif_ram, &state->g_si, state->address, state->cpu_word);
}

static void osal_fastcall write_pifb(usf_state_t * state)
{
    writeb(write_pif_ram, &state->g_si, state->address, state->cpu_byte);
}

static void osal_fastcall write_pifh(usf_state_t * state)
{
    writeh(write_pif_ram, &state->g_si, state->address, state->cpu_hword);
}

static void osal_fastcall write_pifd(usf_state_t * state)
{
    writed(write_pif_ram, &state->g_si, state->address, state->cpu_dword);
}

/* HACK: just to get F-Zero to boot
 * TODO: implement a real DD module
 */
static int read_dd_regs(void* opaque, uint32_t address, uint32_t* value)
{
    *value = (address == 0xa5000508)
           ? 0xffffffff
           : 0x00000000;

    return 0;
}

static int write_dd_regs(void* opaque, uint32_t address, uint32_t value, uint32_t mask)
{
    return 0;
}

static void osal_fastcall read_dd(usf_state_t * state)
{
    readw(read_dd_regs, NULL, state->address, state->rdword);
}

static void osal_fastcall read_ddb(usf_state_t * state)
{
    readb(read_dd_regs, NULL, state->address, state->rdword);
}

static void osal_fastcall read_ddh(usf_state_t * state)
{
    readh(read_dd_regs, NULL, state->address, state->rdword);
}

static void osal_fastcall read_ddd(usf_state_t * state)
{
    readd(read_dd_regs, NULL, state->address, state->rdword);
}

static void osal_fastcall write_dd(usf_state_t * state)
{
    writew(write_dd_regs, NULL, state->address, state->cpu_word);
}

static void osal_fastcall write_ddb(usf_state_t * state)
{
    writeb(write_dd_regs, NULL, state->address, state->cpu_byte);
}

static void osal_fastcall write_ddh(usf_state_t * state)
{
    writeh(write_dd_regs, NULL, state->address, state->cpu_hword);
}

static void osal_fastcall write_ddd(usf_state_t * state)
{
    writed(write_dd_regs, NULL, state->address, state->cpu_dword);
}

#define R(x) read_ ## x ## b, read_ ## x ## h, read_ ## x, read_ ## x ## d
#define W(x) write_ ## x ## b, write_ ## x ## h, write_ ## x, write_ ## x ## d
#define RW(x) R(x), W(x)

int init_memory(usf_state_t * state, uint32_t rdram_size)
{
    int i;

    /* clear mappings */
    for(i = 0; i < 0x10000; ++i)
    {
        map_region(state, i, M64P_MEM_NOMEM, RW(nomem));
    }

    /* map RDRAM */
    if (state->enable_trimming_mode)
    {
        for(i = 0; i < /*0x40*/(rdram_size >> 16); ++i)
        {
            map_region(state, 0x8000+i, M64P_MEM_RDRAM, RW(rdram_tracked));
            map_region(state, 0xa000+i, M64P_MEM_RDRAM, RW(rdram_tracked));
        }
    }
    else
    {
        for(i = 0; i < /*0x40*/(rdram_size >> 16); ++i)
        {
            map_region(state, 0x8000+i, M64P_MEM_RDRAM, RW(rdram));
            map_region(state, 0xa000+i, M64P_MEM_RDRAM, RW(rdram));
        }
    }
    for(i = /*0x40*/(rdram_size >> 16); i < 0x3f0; ++i)
    {
        map_region(state, 0x8000+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa000+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map RDRAM registers */
    map_region(state, 0x83f0, M64P_MEM_RDRAMREG, RW(rdramreg));
    map_region(state, 0xa3f0, M64P_MEM_RDRAMREG, RW(rdramreg));
    for(i = 1; i < 0x10; ++i)
    {
        map_region(state, 0x83f0+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa3f0+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map RSP memory */
    map_region(state, 0x8400, M64P_MEM_RSPMEM, RW(rspmem));
    map_region(state, 0xa400, M64P_MEM_RSPMEM, RW(rspmem));
    for(i = 1; i < 0x4; ++i)
    {
        map_region(state, 0x8400+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa400+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map RSP registers (1) */
    map_region(state, 0x8404, M64P_MEM_RSPREG, RW(rspreg));
    map_region(state, 0xa404, M64P_MEM_RSPREG, RW(rspreg));
    for(i = 0x5; i < 0x8; ++i)
    {
        map_region(state, 0x8400+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa400+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map RSP registers (2) */
    map_region(state, 0x8408, M64P_MEM_RSP, RW(rspreg2));
    map_region(state, 0xa408, M64P_MEM_RSP, RW(rspreg2));
    for(i = 0x9; i < 0x10; ++i)
    {
        map_region(state, 0x8400+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa400+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map DPC registers */
    map_region(state, 0x8410, M64P_MEM_DP, RW(dp));
    map_region(state, 0xa410, M64P_MEM_DP, RW(dp));
    for(i = 1; i < 0x10; ++i)
    {
        map_region(state, 0x8410+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa410+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map DPS registers */
    map_region(state, 0x8420, M64P_MEM_DPS, RW(dps));
    map_region(state, 0xa420, M64P_MEM_DPS, RW(dps));
    for(i = 1; i < 0x10; ++i)
    {
        map_region(state, 0x8420+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa420+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map MI registers */
    map_region(state, 0x8430, M64P_MEM_MI, RW(mi));
    map_region(state, 0xa430, M64P_MEM_MI, RW(mi));
    for(i = 1; i < 0x10; ++i)
    {
        map_region(state, 0x8430+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa430+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map VI registers */
    map_region(state, 0x8440, M64P_MEM_VI, RW(vi));
    map_region(state, 0xa440, M64P_MEM_VI, RW(vi));
    for(i = 1; i < 0x10; ++i)
    {
        map_region(state, 0x8440+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa440+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map AI registers */
    map_region(state, 0x8450, M64P_MEM_AI, RW(ai));
    map_region(state, 0xa450, M64P_MEM_AI, RW(ai));
    for(i = 1; i < 0x10; ++i)
    {
        map_region(state, 0x8450+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa450+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map PI registers */
    map_region(state, 0x8460, M64P_MEM_PI, RW(pi));
    map_region(state, 0xa460, M64P_MEM_PI, RW(pi));
    for(i = 1; i < 0x10; ++i)
    {
        map_region(state, 0x8460+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa460+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map RI registers */
    map_region(state, 0x8470, M64P_MEM_RI, RW(ri));
    map_region(state, 0xa470, M64P_MEM_RI, RW(ri));
    for(i = 1; i < 0x10; ++i)
    {
        map_region(state, 0x8470+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa470+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map SI registers */
    map_region(state, 0x8480, M64P_MEM_SI, RW(si));
    map_region(state, 0xa480, M64P_MEM_SI, RW(si));
    for(i = 0x481; i < 0x500; ++i)
    {
        map_region(state, 0x8000+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa000+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map DD regsiters */
    map_region(state, 0x8500, M64P_MEM_NOTHING, RW(dd));
    map_region(state, 0xa500, M64P_MEM_NOTHING, RW(dd));
    for(i = 0x501; i < 0x800; ++i)
    {
        map_region(state, 0x8000+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa000+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map flashram/sram */
    for(i = 0x800; i < 0x1000; ++i)
    {
        map_region(state, 0x8000+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xa000+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map cart ROM */
    if (state->enable_trimming_mode)
    {
        for(i = 0; i < (state->g_rom_size >> 16); ++i)
        {
            map_region(state, 0x9000+i, M64P_MEM_ROM, R(rom_tracked), W(nothing));
            map_region(state, 0xb000+i, M64P_MEM_ROM, R(rom_tracked),
                       write_nothingb, write_nothingh, write_rom, write_nothingd);
        }
    }
    else
    {
        for(i = 0; i < (state->g_rom_size >> 16); ++i)
        {
            map_region(state, 0x9000+i, M64P_MEM_ROM, R(rom), W(nothing));
            map_region(state, 0xb000+i, M64P_MEM_ROM, R(rom),
                       write_nothingb, write_nothingh, write_rom, write_nothingd);
        }
    }
    for(i = (state->g_rom_size >> 16); i < 0xfc0; ++i)
    {
        map_region(state, 0x9000+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xb000+i, M64P_MEM_NOTHING, RW(nothing));
    }

    /* map PIF RAM */
    map_region(state, 0x9fc0, M64P_MEM_PIF, RW(pif));
    map_region(state, 0xbfc0, M64P_MEM_PIF, RW(pif));
    for(i = 0xfc1; i < 0x1000; ++i)
    {
        map_region(state, 0x9000+i, M64P_MEM_NOTHING, RW(nothing));
        map_region(state, 0xb000+i, M64P_MEM_NOTHING, RW(nothing));
    }

    state->fast_memory = state->enable_trimming_mode ? 0 : 1;

    if (state->g_rom && state->g_rom_size >= 0xfc0)
        init_cic_using_ipl3(state, &state->g_si.pif.cic, state->g_rom + 0x40);

    init_r4300(&state->g_r4300);
    init_rdp(&state->g_dp);
    init_rsp(&state->g_sp);
    init_ai(&state->g_ai);
    init_pi(&state->g_pi);
    init_ri(&state->g_ri);
    init_si(&state->g_si);
    init_vi(&state->g_vi);

    DebugMessage(state, M64MSG_VERBOSE, "Memory initialized");
    return 0;
}

static void map_region_r(usf_state_t * state,
        uint16_t region,
		void (osal_fastcall *read8)(usf_state_t *),
		void (osal_fastcall *read16)(usf_state_t *),
		void (osal_fastcall *read32)(usf_state_t *),
		void (osal_fastcall *read64)(usf_state_t *))
{
    {
        state->readmemb[region] = read8;
        state->readmemh[region] = read16;
        state->readmem [region] = read32;
        state->readmemd[region] = read64;
    }
}

static void map_region_w(usf_state_t * state,
        uint16_t region,
		void (osal_fastcall *write8)(usf_state_t *),
		void (osal_fastcall *write16)(usf_state_t *),
		void (osal_fastcall *write32)(usf_state_t *),
		void (osal_fastcall *write64)(usf_state_t *))
{
    {
        state->writememb[region] = write8;
        state->writememh[region] = write16;
        state->writemem [region] = write32;
        state->writememd[region] = write64;
    }
}

void map_region(usf_state_t * state,
                uint16_t region,
                int type,
				void (osal_fastcall *read8)(usf_state_t *),
				void (osal_fastcall *read16)(usf_state_t *),
				void (osal_fastcall *read32)(usf_state_t *),
				void (osal_fastcall *read64)(usf_state_t *),
				void (osal_fastcall *write8)(usf_state_t *),
				void (osal_fastcall *write16)(usf_state_t *),
				void (osal_fastcall *write32)(usf_state_t *),
				void (osal_fastcall *write64)(usf_state_t *))
{
    (void)type;
    map_region_r(state, region, read8, read16, read32, read64);
    map_region_w(state, region, write8, write16, write32, write64);
}

unsigned int * osal_fastcall fast_mem_access(usf_state_t * state, unsigned int address)
{
    /* This code is performance critical, specially on pure interpreter mode.
     * Removing error checking saves some time, but the emulator may crash. */

    if ((address & 0xc0000000) != 0x80000000)
        address = virtual_to_physical_address(state, address, 2);

    address &= 0x1ffffffc;

    /* XXX this method is only valid for single 32 bit word fetches,
     * as used by the pure interpreter CPU. The cached interpreter
     * and the recompiler, on the other hand, fetch the start of a
     * block and use the pointer for the entire block. */

    if (state->enable_trimming_mode)
    {
        if (address < RDRAM_MAX_SIZE)
        {
            if (!bit_array_test(state->barray_ram_written_first, address / 4))
                bit_array_set(state->barray_ram_read, address / 4);
        }
        else if ((address - 0x10000000) < state->g_rom_size)
        {
            bit_array_set(state->barray_rom, address / 4);
        }
    }

    if (address < RDRAM_MAX_SIZE)
        return (unsigned int*)((unsigned char*)state->g_rdram + address);
    else if (address >= 0x10000000)
    {
        if ((address - 0x10000000) < state->g_rom_size)
            return (unsigned int*)((unsigned char*)state->g_rom + address - 0x10000000);
        else
            return (unsigned int*)((unsigned char*)state->EmptySpace + (address & 0xFFFF));
    }
    else if ((address & 0xffffe000) == 0x04000000)
        return (unsigned int*)((unsigned char*)state->g_sp.mem + (address & 0x1ffc));
    else
        return NULL;
}
