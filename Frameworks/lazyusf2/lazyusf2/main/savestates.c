/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *   Mupen64plus - savestates.c                                            *
 *   Mupen64Plus homepage: http://code.google.com/p/mupen64plus/           *
 *   Copyright (C) 2012 CasualJames                                        *
 *   Copyright (C) 2009 Olejl Tillin9                                      *
 *   Copyright (C) 2008 Richard42 Tillin9                                  *
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

#include <stdlib.h>
#include <string.h>

#include "usf/usf.h"

#include "usf/usf_internal.h"

#define M64P_CORE_PROTOTYPES 1
#include "api/m64p_types.h"
#include "api/callbacks.h"

#include "savestates.h"
#include "main.h"
#include "rom.h"
#include "util.h"

#include "ai/ai_controller.h"
#include "memory/memory.h"
#include "r4300/tlb.h"
#include "r4300/cp0.h"
#include "r4300/cp1.h"
#include "r4300/r4300.h"
#include "r4300/r4300_core.h"
#include "r4300/cached_interp.h"
#include "r4300/interupt.h"
#include "pi/pi_controller.h"
#ifdef NEW_DYNAREC
#include "r4300/new_dynarec/new_dynarec.h"
#endif
#include "rdp/rdp_core.h"
#include "ri/ri_controller.h"
#include "rsp/rsp_core.h"
#include "si/si_controller.h"
#include "vi/vi_controller.h"

static const char* savestate_magic = "M64+SAVE";
static const int savestate_latest_version = 0x00010000;  /* 1.0 */
static const unsigned char pj64_magic[4] = { 0xC8, 0xA6, 0xD8, 0x23 };

#define GETARRAY(buff, type, count) \
    (to_little_endian_buffer(buff, sizeof(type),count), \
     buff += count*sizeof(type), \
     (type *)(buff-count*sizeof(type)))
#define COPYARRAY(dst, buff, type, count) \
    memcpy(dst, GETARRAY(buff, type, count), sizeof(type)*count)
#define GETDATA(buff, type) *GETARRAY(buff, type, 1)

#define PUTARRAY(src, buff, type, count) \
    memcpy(buff, src, sizeof(type)*count); \
    to_little_endian_buffer(buff, sizeof(type), count); \
    buff += count*sizeof(type);

#define PUTDATA(buff, type, value) \
    do { type x = value; PUTARRAY(&x, buff, type, 1); } while(0)

#define read_bytes(ptr, size) \
{ \
    if ((size) > state_size) \
    { \
        if (savestateData) free(savestateData); \
        return 0; \
    } \
    memcpy((ptr), state_ptr, (size)); \
    state_ptr += (size); \
    state_size -= (size); \
}

static int savestates_load_m64p(usf_state_t * state, unsigned char * ptr, unsigned int size)
{
    unsigned char header[44];
    int version;
    int i;
    
    unsigned char * state_ptr = ptr;
    unsigned int state_size = size;

    size_t savestateSize;
    unsigned char *savestateData = 0, *curr;
    char queue[1024];

    /* Read and check Mupen64Plus magic number. */
    read_bytes(header, 44);
    curr = header;

    if(strncmp((char *)curr, savestate_magic, 8)!=0)
        return 0;
    curr += 8;

    version = *curr++;
    version = (version << 8) | *curr++;
    version = (version << 8) | *curr++;
    version = (version << 8) | *curr++;
    if(version != 0x00010000)
        return 0;

    /* skip MD5 */
    curr += 32;

    /* Read the rest of the savestate */
    savestateSize = 16788244;
    if (state_size < savestateSize + sizeof(queue))
        return 0;
    
    savestateData = curr = malloc(savestateSize);
    if (!savestateData)
        return 0;

    read_bytes(savestateData, savestateSize);
    read_bytes(queue, sizeof(queue));
    
    // Parse savestate
    state->g_ri.rdram.regs[RDRAM_CONFIG_REG]       = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_DEVICE_ID_REG]    = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_DELAY_REG]        = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_MODE_REG]         = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_REF_INTERVAL_REG] = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_REF_ROW_REG]      = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_RAS_INTERVAL_REG] = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_MIN_INTERVAL_REG] = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_ADDR_SELECT_REG]  = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_DEVICE_MANUF_REG] = GETDATA(curr, uint32_t);

    curr += 4; /* Padding from old implementation */
    state->g_r4300.mi.regs[MI_INIT_MODE_REG] = GETDATA(curr, uint32_t);
    curr += 4; // Duplicate MI init mode flags from old implementation
    state->g_r4300.mi.regs[MI_VERSION_REG]   = GETDATA(curr, uint32_t);
    state->g_r4300.mi.regs[MI_INTR_REG]      = GETDATA(curr, uint32_t);
    state->g_r4300.mi.regs[MI_INTR_MASK_REG] = GETDATA(curr, uint32_t);
    curr += 4; /* Padding from old implementation */
    curr += 8; // Duplicated MI intr flags and padding from old implementation

    state->g_pi.regs[PI_DRAM_ADDR_REG]    = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_CART_ADDR_REG]    = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_RD_LEN_REG]       = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_WR_LEN_REG]       = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_STATUS_REG]       = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM1_LAT_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM1_PWD_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM1_PGS_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM1_RLS_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM2_LAT_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM2_PWD_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM2_PGS_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM2_RLS_REG] = GETDATA(curr, uint32_t);

    state->g_sp.regs[SP_MEM_ADDR_REG]  = GETDATA(curr, uint32_t);
    state->g_sp.regs[SP_DRAM_ADDR_REG] = GETDATA(curr, uint32_t);
    state->g_sp.regs[SP_RD_LEN_REG]    = GETDATA(curr, uint32_t);
    state->g_sp.regs[SP_WR_LEN_REG]    = GETDATA(curr, uint32_t);
    curr += 4; /* Padding from old implementation */
    state->g_sp.regs[SP_STATUS_REG]    = GETDATA(curr, uint32_t);
    curr += 16; // Duplicated SP flags and padding from old implementation
    state->g_sp.regs[SP_DMA_FULL_REG]  = GETDATA(curr, uint32_t);
    state->g_sp.regs[SP_DMA_BUSY_REG]  = GETDATA(curr, uint32_t);
    state->g_sp.regs[SP_SEMAPHORE_REG] = GETDATA(curr, uint32_t);

    state->g_sp.regs2[SP_PC_REG]    = GETDATA(curr, uint32_t);
    state->g_sp.regs2[SP_IBIST_REG] = GETDATA(curr, uint32_t);

    state->g_si.regs[SI_DRAM_ADDR_REG]      = GETDATA(curr, uint32_t);
    state->g_si.regs[SI_PIF_ADDR_RD64B_REG] = GETDATA(curr, uint32_t);
    state->g_si.regs[SI_PIF_ADDR_WR64B_REG] = GETDATA(curr, uint32_t);
    state->g_si.regs[SI_STATUS_REG]         = GETDATA(curr, uint32_t);

    state->g_vi.regs[VI_STATUS_REG]  = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_ORIGIN_REG]  = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_WIDTH_REG]   = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_V_INTR_REG]  = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_CURRENT_REG] = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_BURST_REG]   = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_V_SYNC_REG]  = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_H_SYNC_REG]  = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_LEAP_REG]    = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_H_START_REG] = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_V_START_REG] = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_V_BURST_REG] = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_X_SCALE_REG] = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_Y_SCALE_REG] = GETDATA(curr, uint32_t);
    state->g_vi.delay = GETDATA(curr, unsigned int);

    state->g_ri.regs[RI_MODE_REG]         = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_CONFIG_REG]       = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_CURRENT_LOAD_REG] = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_SELECT_REG]       = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_REFRESH_REG]      = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_LATENCY_REG]      = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_ERROR_REG]        = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_WERROR_REG]       = GETDATA(curr, uint32_t);

    state->g_ai.regs[AI_DRAM_ADDR_REG] = GETDATA(curr, uint32_t);
    state->g_ai.regs[AI_LEN_REG]       = GETDATA(curr, uint32_t);
    state->g_ai.regs[AI_CONTROL_REG]   = GETDATA(curr, uint32_t);
    state->g_ai.regs[AI_STATUS_REG]    = GETDATA(curr, uint32_t);
    state->g_ai.regs[AI_DACRATE_REG]   = GETDATA(curr, uint32_t);
    state->g_ai.regs[AI_BITRATE_REG]   = GETDATA(curr, uint32_t);
    state->g_ai.fifo[1].duration  = GETDATA(curr, unsigned int);
    state->g_ai.fifo[1].length = GETDATA(curr, uint32_t);
    state->g_ai.fifo[0].duration  = GETDATA(curr, unsigned int);
    state->g_ai.fifo[0].length = GETDATA(curr, uint32_t);
    /* best effort initialization of fifo addresses...
     * You might get a small sound "pop" because address might be wrong.
     * Proper initialization requires changes to savestate format
     */
    state->g_ai.fifo[0].address = state->g_ai.regs[AI_DRAM_ADDR_REG];
    state->g_ai.fifo[1].address = state->g_ai.regs[AI_DRAM_ADDR_REG];
    state->g_ai.samples_format_changed = 1;

    state->g_dp.dpc_regs[DPC_START_REG]    = GETDATA(curr, uint32_t);
    state->g_dp.dpc_regs[DPC_END_REG]      = GETDATA(curr, uint32_t);
    state->g_dp.dpc_regs[DPC_CURRENT_REG]  = GETDATA(curr, uint32_t);
    curr += 4; // Padding from old implementation
    state->g_dp.dpc_regs[DPC_STATUS_REG]   = GETDATA(curr, uint32_t);
    curr += 12; // Duplicated DPC flags and padding from old implementation
    state->g_dp.dpc_regs[DPC_CLOCK_REG]    = GETDATA(curr, uint32_t);
    state->g_dp.dpc_regs[DPC_BUFBUSY_REG]  = GETDATA(curr, uint32_t);
    state->g_dp.dpc_regs[DPC_PIPEBUSY_REG] = GETDATA(curr, uint32_t);
    state->g_dp.dpc_regs[DPC_TMEM_REG]     = GETDATA(curr, uint32_t);

    state->g_dp.dps_regs[DPS_TBIST_REG]        = GETDATA(curr, uint32_t);
    state->g_dp.dps_regs[DPS_TEST_MODE_REG]    = GETDATA(curr, uint32_t);
    state->g_dp.dps_regs[DPS_BUFTEST_ADDR_REG] = GETDATA(curr, uint32_t);
    state->g_dp.dps_regs[DPS_BUFTEST_DATA_REG] = GETDATA(curr, uint32_t);

    COPYARRAY(state->g_rdram, curr, uint32_t, RDRAM_MAX_SIZE/4);
    COPYARRAY(state->g_sp.mem, curr, uint32_t, SP_MEM_SIZE/4);
    COPYARRAY(state->g_si.pif.ram, curr, uint8_t, PIF_RAM_SIZE);

    /*state->g_pi.use_flashram =*/ (void)GETDATA(curr, int);
    /*state->g_pi.flashram.mode =*/ (void)GETDATA(curr, int);
    /*state->g_pi.flashram.status =*/ (void)GETDATA(curr, unsigned long long);
    /*state->g_pi.flashram.erase_offset =*/ (void)GETDATA(curr, unsigned int);
    /*state->g_pi.flashram.write_pointer =*/ (void)GETDATA(curr, unsigned int);

    COPYARRAY(state->tlb_LUT_r, curr, unsigned int, 0x100000);
    COPYARRAY(state->tlb_LUT_w, curr, unsigned int, 0x100000);

    state->llbit = GETDATA(curr, unsigned int);
    COPYARRAY(state->reg, curr, long long int, 32);
    COPYARRAY(state->g_cp0_regs, curr, unsigned int, CP0_REGS_COUNT);
    set_fpr_pointers(state, state->g_cp0_regs[CP0_STATUS_REG]);
    state->lo = GETDATA(curr, long long int);
    state->hi = GETDATA(curr, long long int);
    COPYARRAY(state->reg_cop1_fgr_64, curr, long long int, 32);
    if ((state->g_cp0_regs[CP0_STATUS_REG] & 0x04000000) == 0)  // 32-bit FPR mode requires data shuffling because 64-bit layout is always stored in savestate file
        shuffle_fpr_data(state, 0x04000000, 0);
    state->FCR0 = GETDATA(curr, int);
    state->FCR31 = GETDATA(curr, int);

    for (i = 0; i < 32; i++)
    {
        state->tlb_e[i].mask = GETDATA(curr, short);
        curr += 2;
        state->tlb_e[i].vpn2 = GETDATA(curr, int);
        state->tlb_e[i].g = GETDATA(curr, char);
        state->tlb_e[i].asid = GETDATA(curr, unsigned char);
        curr += 2;
        state->tlb_e[i].pfn_even = GETDATA(curr, int);
        state->tlb_e[i].c_even = GETDATA(curr, char);
        state->tlb_e[i].d_even = GETDATA(curr, char);
        state->tlb_e[i].v_even = GETDATA(curr, char);
        curr++;
        state->tlb_e[i].pfn_odd = GETDATA(curr, int);
        state->tlb_e[i].c_odd = GETDATA(curr, char);
        state->tlb_e[i].d_odd = GETDATA(curr, char);
        state->tlb_e[i].v_odd = GETDATA(curr, char);
        state->tlb_e[i].r = GETDATA(curr, char);
   
        state->tlb_e[i].start_even = GETDATA(curr, unsigned int);
        state->tlb_e[i].end_even = GETDATA(curr, unsigned int);
        state->tlb_e[i].phys_even = GETDATA(curr, unsigned int);
        state->tlb_e[i].start_odd = GETDATA(curr, unsigned int);
        state->tlb_e[i].end_odd = GETDATA(curr, unsigned int);
        state->tlb_e[i].phys_odd = GETDATA(curr, unsigned int);
    }

#ifdef NEW_DYNAREC
    if (state->r4300emu == CORE_DYNAREC) {
        state->pcaddr = GETDATA(curr, unsigned int);
        state->pending_exception = 1;
        invalidate_all_pages(state);
    } else {
        if(state->r4300emu != CORE_PURE_INTERPRETER)
        {
            for (i = 0; i < 0x100000; i++)
                state->invalid_code[i] = 1;
        }
        generic_jump_to(state, GETDATA(curr, unsigned int)); // PC
    }
#else
    if(state->r4300emu != CORE_PURE_INTERPRETER)
    {
        for (i = 0; i < 0x100000; i++)
            state->invalid_code[i] = 1;
    }
    generic_jump_to(state, GETDATA(curr, unsigned int)); // PC
#endif

    state->next_interupt = GETDATA(curr, unsigned int);
    state->g_vi.next_vi = GETDATA(curr, unsigned int);
    state->g_vi.field = GETDATA(curr, unsigned int);
    
    free(savestateData);

    // assert(savestateData+savestateSize == curr)

    to_little_endian_buffer(queue, 4, 256);
    load_eventqueue_infos(state, queue);

#ifdef NEW_DYNAREC
    if (state->r4300emu == CORE_DYNAREC)
        state->last_addr = state->pcaddr;
    else
        state->last_addr = state->PC->addr;
#else
    state->last_addr = state->PC->addr;
#endif

    return 1;
}

static int savestates_load_pj64(usf_state_t * state, unsigned char * ptr, unsigned int size)
{
    char buffer[1024];
    unsigned int vi_timer, SaveRDRAMSize;
    int i;
#ifdef DYNAREC
    unsigned long long dummy;
#endif

    unsigned char header[8];

    unsigned char * state_ptr = ptr;
    unsigned int state_size = size;
    unsigned int count_per_scanline;
    
    size_t savestateSize;
    unsigned char *savestateData = 0, *curr;

    /* Read and check Project64 magic number. */
    read_bytes(header, 8);

    curr = header;
    if (memcmp(curr, pj64_magic, 4) != 0)
    {
        return 0;
    }
    curr += 4;

    SaveRDRAMSize = GETDATA(curr, unsigned int);

    /* Read the rest of the savestate into memory. */
    savestateSize = SaveRDRAMSize + 0x2754;
    savestateData = curr = malloc(savestateSize);
    if (!savestateData)
        return 0;
    
    if (state_size < savestateSize)
    {
        free(savestateData);
        return 0;
    }
    
    read_bytes(savestateData, savestateSize);

    // skip ROM header
    curr += 0x40;

    // vi_timer
    vi_timer = GETDATA(curr, unsigned int);

    // Program Counter
    state->last_addr = GETDATA(curr, unsigned int);

    // GPR
    COPYARRAY(state->reg, curr, long long int, 32);

    // FPR
    COPYARRAY(state->reg_cop1_fgr_64, curr, long long int, 32);

    // CP0
    COPYARRAY(state->g_cp0_regs, curr, unsigned int, CP0_REGS_COUNT);

    set_fpr_pointers(state, state->g_cp0_regs[CP0_STATUS_REG]);
    /*if ((state->g_cp0_regs[CP0_STATUS_REG] & 0x04000000) == 0) // pj64 always stores data depending on the current mode
        shuffle_fpr_data(state, 0x04000000, 0);*/

    // Initialze the interupts
    vi_timer += state->g_cp0_regs[CP0_COUNT_REG];
    state->next_interupt = (state->g_cp0_regs[CP0_COMPARE_REG] < vi_timer)
                  ? state->g_cp0_regs[CP0_COMPARE_REG]
                  : vi_timer;
    state->g_vi.next_vi = vi_timer;
    state->g_vi.field = 0;
    *((unsigned int*)&buffer[0]) = VI_INT;
    *((unsigned int*)&buffer[4]) = vi_timer;
    *((unsigned int*)&buffer[8]) = COMPARE_INT;
    *((unsigned int*)&buffer[12]) = state->g_cp0_regs[CP0_COMPARE_REG];
    *((unsigned int*)&buffer[16]) = 0xFFFFFFFF;

    load_eventqueue_infos(state, buffer);

    state->cycle_count = state->g_cp0_regs[CP0_COUNT_REG] - state->q.first->data.count;

    // FPCR
    state->FCR0 = GETDATA(curr, int);
    curr += 30 * 4; // FCR1...FCR30 not supported
    state->FCR31 = GETDATA(curr, int);

    // hi / lo
    state->hi = GETDATA(curr, long long int);
    state->lo = GETDATA(curr, long long int);

    // rdram register
    state->g_ri.rdram.regs[RDRAM_CONFIG_REG]       = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_DEVICE_ID_REG]    = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_DELAY_REG]        = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_MODE_REG]         = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_REF_INTERVAL_REG] = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_REF_ROW_REG]      = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_RAS_INTERVAL_REG] = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_MIN_INTERVAL_REG] = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_ADDR_SELECT_REG]  = GETDATA(curr, uint32_t);
    state->g_ri.rdram.regs[RDRAM_DEVICE_MANUF_REG] = GETDATA(curr, uint32_t);

    // sp_register
    state->g_sp.regs[SP_MEM_ADDR_REG]  = GETDATA(curr, uint32_t);
    state->g_sp.regs[SP_DRAM_ADDR_REG] = GETDATA(curr, uint32_t);
    state->g_sp.regs[SP_RD_LEN_REG]    = GETDATA(curr, uint32_t);
    state->g_sp.regs[SP_WR_LEN_REG]    = GETDATA(curr, uint32_t);
    state->g_sp.regs[SP_STATUS_REG]    = GETDATA(curr, uint32_t);
    state->g_sp.regs[SP_DMA_FULL_REG]  = GETDATA(curr, uint32_t);
    state->g_sp.regs[SP_DMA_BUSY_REG]  = GETDATA(curr, uint32_t);
    state->g_sp.regs[SP_SEMAPHORE_REG] = GETDATA(curr, uint32_t);
    state->g_sp.regs2[SP_PC_REG]    = GETDATA(curr, uint32_t);
    state->g_sp.regs2[SP_IBIST_REG] = GETDATA(curr, uint32_t);

    // dpc_register
    state->g_dp.dpc_regs[DPC_START_REG]    = GETDATA(curr, uint32_t);
    state->g_dp.dpc_regs[DPC_END_REG]      = GETDATA(curr, uint32_t);
    state->g_dp.dpc_regs[DPC_CURRENT_REG]  = GETDATA(curr, uint32_t);
    state->g_dp.dpc_regs[DPC_STATUS_REG]   = GETDATA(curr, uint32_t);
    state->g_dp.dpc_regs[DPC_CLOCK_REG]    = GETDATA(curr, uint32_t);
    state->g_dp.dpc_regs[DPC_BUFBUSY_REG]  = GETDATA(curr, uint32_t);
    state->g_dp.dpc_regs[DPC_PIPEBUSY_REG] = GETDATA(curr, uint32_t);
    state->g_dp.dpc_regs[DPC_TMEM_REG]     = GETDATA(curr, uint32_t);
    (void)GETDATA(curr, unsigned int); // Dummy read
    (void)GETDATA(curr, unsigned int); // Dummy read

    // mi_register
    state->g_r4300.mi.regs[MI_INIT_MODE_REG] = GETDATA(curr, uint32_t);
    state->g_r4300.mi.regs[MI_VERSION_REG]   = GETDATA(curr, uint32_t);
    state->g_r4300.mi.regs[MI_INTR_REG]      = GETDATA(curr, uint32_t);
    state->g_r4300.mi.regs[MI_INTR_MASK_REG] = GETDATA(curr, uint32_t);

    // vi_register
    state->g_vi.regs[VI_STATUS_REG]  = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_ORIGIN_REG]  = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_WIDTH_REG]   = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_V_INTR_REG]  = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_CURRENT_REG] = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_BURST_REG]   = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_V_SYNC_REG]  = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_H_SYNC_REG]  = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_LEAP_REG]    = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_H_START_REG] = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_V_START_REG] = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_V_BURST_REG] = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_X_SCALE_REG] = GETDATA(curr, uint32_t);
    state->g_vi.regs[VI_Y_SCALE_REG] = GETDATA(curr, uint32_t);

    // ai_register
    state->g_ai.regs[AI_DRAM_ADDR_REG] = GETDATA(curr, uint32_t);
    state->g_ai.regs[AI_LEN_REG]       = GETDATA(curr, uint32_t);
    state->g_ai.regs[AI_CONTROL_REG]   = GETDATA(curr, uint32_t);
    state->g_ai.regs[AI_STATUS_REG]    = GETDATA(curr, uint32_t);
    state->g_ai.regs[AI_DACRATE_REG]   = GETDATA(curr, uint32_t);
    state->g_ai.regs[AI_BITRATE_REG]   = GETDATA(curr, uint32_t);
    state->g_ai.samples_format_changed = 1;
    // XXX USF
    state->g_ai.regs[AI_STATUS_REG] = 0;

    // pi_register
    state->g_pi.regs[PI_DRAM_ADDR_REG]    = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_CART_ADDR_REG]    = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_RD_LEN_REG]       = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_WR_LEN_REG]       = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_STATUS_REG]       = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM1_LAT_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM1_PWD_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM1_PGS_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM1_RLS_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM2_LAT_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM2_PWD_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM2_PGS_REG] = GETDATA(curr, uint32_t);
    state->g_pi.regs[PI_BSD_DOM2_RLS_REG] = GETDATA(curr, uint32_t);

    // ri_register
    state->g_ri.regs[RI_MODE_REG]         = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_CONFIG_REG]       = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_CURRENT_LOAD_REG] = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_SELECT_REG]       = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_REFRESH_REG]      = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_LATENCY_REG]      = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_ERROR_REG]        = GETDATA(curr, uint32_t);
    state->g_ri.regs[RI_WERROR_REG]       = GETDATA(curr, uint32_t);

    // si_register
    state->g_si.regs[SI_DRAM_ADDR_REG]      = GETDATA(curr, uint32_t);
    state->g_si.regs[SI_PIF_ADDR_RD64B_REG] = GETDATA(curr, uint32_t);
    state->g_si.regs[SI_PIF_ADDR_WR64B_REG] = GETDATA(curr, uint32_t);
    state->g_si.regs[SI_STATUS_REG]         = GETDATA(curr, uint32_t);

    // tlb
    memset(state->tlb_LUT_r, 0, 0x400000);
    memset(state->tlb_LUT_w, 0, 0x400000);
    for (i=0; i < 32; i++)
    {
        unsigned int MyPageMask, MyEntryHi, MyEntryLo0, MyEntryLo1;

        (void)GETDATA(curr, unsigned int); // Dummy read - EntryDefined
        MyPageMask = GETDATA(curr, unsigned int);
        MyEntryHi = GETDATA(curr, unsigned int);
        MyEntryLo0 = GETDATA(curr, unsigned int);
        MyEntryLo1 = GETDATA(curr, unsigned int);

        // This is copied from TLBWI instruction
        state->tlb_e[i].g = (MyEntryLo0 & MyEntryLo1 & 1);
        state->tlb_e[i].pfn_even = (MyEntryLo0 & 0x3FFFFFC0) >> 6;
        state->tlb_e[i].pfn_odd = (MyEntryLo1 & 0x3FFFFFC0) >> 6;
        state->tlb_e[i].c_even = (MyEntryLo0 & 0x38) >> 3;
        state->tlb_e[i].c_odd = (MyEntryLo1 & 0x38) >> 3;
        state->tlb_e[i].d_even = (MyEntryLo0 & 0x4) >> 2;
        state->tlb_e[i].d_odd = (MyEntryLo1 & 0x4) >> 2;
        state->tlb_e[i].v_even = (MyEntryLo0 & 0x2) >> 1;
        state->tlb_e[i].v_odd = (MyEntryLo1 & 0x2) >> 1;
        state->tlb_e[i].asid = (MyEntryHi & 0xFF);
        state->tlb_e[i].vpn2 = (MyEntryHi & 0xFFFFE000) >> 13;
        //state->tlb_e[i].r = (MyEntryHi & 0xC000000000000000LL) >> 62;
        state->tlb_e[i].mask = (MyPageMask & 0x1FFE000) >> 13;
           
        state->tlb_e[i].start_even = state->tlb_e[i].vpn2 << 13;
        state->tlb_e[i].end_even = state->tlb_e[i].start_even+
          (state->tlb_e[i].mask << 12) + 0xFFF;
        state->tlb_e[i].phys_even = state->tlb_e[i].pfn_even << 12;
           
        state->tlb_e[i].start_odd = state->tlb_e[i].end_even+1;
        state->tlb_e[i].end_odd = state->tlb_e[i].start_odd+
          (state->tlb_e[i].mask << 12) + 0xFFF;
        state->tlb_e[i].phys_odd = state->tlb_e[i].pfn_odd << 12;

        tlb_map(state, &state->tlb_e[i]);
    }

    // pif ram
    COPYARRAY(state->g_si.pif.ram, curr, uint8_t, PIF_RAM_SIZE);

    // RDRAM
    memset(state->g_rdram, 0, RDRAM_MAX_SIZE);
    COPYARRAY(state->g_rdram, curr, uint32_t, SaveRDRAMSize/4);

    // DMEM + IMEM
    COPYARRAY(state->g_sp.mem, curr, uint32_t, SP_MEM_SIZE/4);

    // The following values should not matter because we don't have any AI interrupt
    // g_ai.fifo[1].delay = 0; g_ai.fifo[1].length = 0;
    // g_ai.fifo[0].delay = 0; g_ai.fifo[0].length = 0;

    // The following is not available in PJ64 savestate. Keep the values as is.
    // g_dp.dps_regs[DPS_TBIST_REG] = 0; g_dp.dps_regs[DPS_TEST_MODE_REG] = 0;
    // g_dp.dps_regs[DPS_BUFTEST_ADDR_REG] = 0; g_dp.dps_regs[DPS_BUFTEST_DATA_REG] = 0; llbit = 0;

    // No flashram info in pj64 savestate.
    //init_flashram(&state->g_pi.flashram);

    open_rom_header(state, savestateData, sizeof(m64p_rom_header));

    // Needs the rom header parsed first before the delay can be calculated
    count_per_scanline = (unsigned int)((float)state->ROM_PARAMS.aidacrate / (float)state->ROM_PARAMS.vilimit) / (state->g_vi.regs[VI_V_SYNC_REG] + 1);
    state->g_vi.delay = (state->g_vi.regs[VI_V_SYNC_REG] + 1) * count_per_scanline;

#ifdef NEW_DYNAREC
    if (state->r4300emu == CORE_DYNAREC) {
        state->pcaddr = state->last_addr;
        state->pending_exception = 1;
        invalidate_all_pages(state);
    } else {
        if(state->r4300emu != CORE_PURE_INTERPRETER)
        {
            for (i = 0; i < 0x100000; i++)
                state->invalid_code[i] = 1;
        }
        generic_jump_to(state, state->last_addr);
    }
#else
    if(state->r4300emu != CORE_PURE_INTERPRETER)
    {
        for (i = 0; i < 0x100000; i++)
            state->invalid_code[i] = 1;
    }
#ifdef DYNAREC
    *(void **)&state->return_address = (void *)&dummy;
#endif
    generic_jump_to(state, state->last_addr);
#ifdef DYNAREC
	*(void **)&state->return_address = (void *)0;
#endif
#endif

    // assert(savestateData+savestateSize == curr)

    free(savestateData);
    return 1;
}

int savestates_load(usf_state_t * state, unsigned char * ptr, unsigned int size, unsigned int is_m64p)
{
    if (is_m64p)
        return savestates_load_m64p(state, ptr, size);
    else
        return savestates_load_pj64(state, ptr, size);
}
