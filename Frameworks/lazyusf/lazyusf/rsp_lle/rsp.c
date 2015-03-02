/******************************************************************************\
* Authors:  Iconoclast                                                         *
* Release:  2013.12.12                                                         *
* License:  CC0 Public Domain Dedication                                       *
*                                                                              *
* To the extent possible under law, the author(s) have dedicated all copyright *
* and related and neighboring rights to this software to the public domain     *
* worldwide. This software is distributed without any warranty.                *
*                                                                              *
* You should have received a copy of the CC0 Public Domain Dedication along    *
* with this software.                                                          *
* If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.             *
\******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "usf/usf.h"

#include "usf/usf_internal.h"

#include "api/callbacks.h"

#undef JUMP

#include "config.h"

#include "rsp.h"

#include "../rsp_hle/hle.h"

void real_run_rsp(usf_state_t * state, uint32_t cycles)
{
    (void)cycles;

    if (state->g_sp.regs[SP_STATUS_REG] & 0x00000003)
    {
        message(state, "SP_STATUS_HALT", 3);
        return;
    }
    switch (*(unsigned int *)(state->DMEM + 0xFC0))
    { /* Simulation barrier to redirect processing externally. */
        case 0x00000002: /* OSTask.type == M_AUDTASK */
            if (state->enable_hle_audio == 0)
                break;
            hle_execute(&state->hle);
            return;
            
      /* XXX USF sets should not be processing DLists, but several of them
         require them at least once to boot properly. And for some reason,
         with this emulator core, Iconoclast's RSP core is not up to the
         task of running the DLists, so this HLE will do instead. */
        case 0x00000001:
            hle_execute(&state->hle);
            return;
    }
    run_task(state);
}

int32_t init_rsp_lle(usf_state_t * state)
{
    state->CR[0x0] = &state->g_sp.regs[SP_MEM_ADDR_REG];
    state->CR[0x1] = &state->g_sp.regs[SP_DRAM_ADDR_REG];
    state->CR[0x2] = &state->g_sp.regs[SP_RD_LEN_REG];
    state->CR[0x3] = &state->g_sp.regs[SP_WR_LEN_REG];
    state->CR[0x4] = &state->g_sp.regs[SP_STATUS_REG];
    state->CR[0x5] = &state->g_sp.regs[SP_DMA_FULL_REG];
    state->CR[0x6] = &state->g_sp.regs[SP_DMA_BUSY_REG];
    state->CR[0x7] = &state->g_sp.regs[SP_SEMAPHORE_REG];
    state->CR[0x8] = &state->g_dp.dpc_regs[DPC_START_REG];
    state->CR[0x9] = &state->g_dp.dpc_regs[DPC_END_REG];
    state->CR[0xA] = &state->g_dp.dpc_regs[DPC_CURRENT_REG];
    state->CR[0xB] = &state->g_dp.dpc_regs[DPC_STATUS_REG];
    state->CR[0xC] = &state->g_dp.dpc_regs[DPC_CLOCK_REG];
    state->CR[0xD] = &state->g_dp.dpc_regs[DPC_BUFBUSY_REG];
    state->CR[0xE] = &state->g_dp.dpc_regs[DPC_PIPEBUSY_REG];
    state->CR[0xF] = &state->g_dp.dpc_regs[DPC_TMEM_REG];

    state->DMEM = (unsigned char *)state->g_sp.mem;
    state->IMEM = (unsigned char *)state->g_sp.mem + 0x1000;
    
    hle_init(&state->hle,
        (unsigned char *)state->g_rdram,
        state->DMEM,
        state->IMEM,
        &state->g_r4300.mi.regs[MI_INTR_REG],
        &state->g_sp.regs[SP_MEM_ADDR_REG],
        &state->g_sp.regs[SP_DRAM_ADDR_REG],
        &state->g_sp.regs[SP_RD_LEN_REG],
        &state->g_sp.regs[SP_WR_LEN_REG],
        &state->g_sp.regs[SP_STATUS_REG],
        &state->g_sp.regs[SP_DMA_FULL_REG],
        &state->g_sp.regs[SP_DMA_BUSY_REG],
        &state->g_sp.regs[SP_PC_REG],
        &state->g_sp.regs[SP_SEMAPHORE_REG],
        &state->g_dp.dpc_regs[DPC_START_REG],
        &state->g_dp.dpc_regs[DPC_END_REG],
        &state->g_dp.dpc_regs[DPC_CURRENT_REG],
        &state->g_dp.dpc_regs[DPC_STATUS_REG],
        &state->g_dp.dpc_regs[DPC_CLOCK_REG],
        &state->g_dp.dpc_regs[DPC_BUFBUSY_REG],
        &state->g_dp.dpc_regs[DPC_PIPEBUSY_REG],
        &state->g_dp.dpc_regs[DPC_TMEM_REG],
        state);

    return 0;
}
