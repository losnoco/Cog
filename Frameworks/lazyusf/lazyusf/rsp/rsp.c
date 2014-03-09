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

#include "../usf.h"

#include "../dma.h"
#include "../exception.h"
#include "../main.h"
#include "../memory.h"
#include "../registers.h"

#include "../usf_internal.h"

#undef JUMP

#include "config.h"

#include "rsp.h"

#include "../rsp_hle/hle.h"

void real_run_rsp(usf_state_t * state, uint32_t cycles)
{
    (void)cycles;

    if (SP_STATUS_REG & 0x00000003)
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
            SP_STATUS_REG |= 0x00000203;
            if (SP_STATUS_REG & 0x00000040) /* SP_STATUS_INTR_BREAK */
            {
                MI_INTR_REG |= 0x00000001; /* VR4300 SP interrupt */
                CheckInterrupts(state);
            }
            return;
    }
    run_task(state);
    return;
}

int32_t init_rsp(usf_state_t * state)
{
    state->CR[0x0] = &SP_MEM_ADDR_REG;
    state->CR[0x1] = &SP_DRAM_ADDR_REG;
    state->CR[0x2] = &SP_RD_LEN_REG;
    state->CR[0x3] = &SP_WR_LEN_REG;
    state->CR[0x4] = &SP_STATUS_REG;
    state->CR[0x5] = &SP_DMA_FULL_REG;
    state->CR[0x6] = &SP_DMA_BUSY_REG;
    state->CR[0x7] = &SP_SEMAPHORE_REG;
    state->CR[0x8] = &DPC_START_REG;
    state->CR[0x9] = &DPC_END_REG;
    state->CR[0xA] = &DPC_CURRENT_REG;
    state->CR[0xB] = &DPC_STATUS_REG;
    state->CR[0xC] = &DPC_CLOCK_REG;
    state->CR[0xD] = &DPC_BUFBUSY_REG;
    state->CR[0xE] = &DPC_PIPEBUSY_REG;
    state->CR[0xF] = &DPC_TMEM_REG;

    hle_init(&state->hle,
        state->N64MEM,
        state->DMEM,
        state->IMEM,
        &MI_INTR_REG,
        &SP_MEM_ADDR_REG,
        &SP_DRAM_ADDR_REG,
        &SP_RD_LEN_REG,
        &SP_WR_LEN_REG,
        &SP_STATUS_REG,
        &SP_DMA_FULL_REG,
        &SP_DMA_BUSY_REG,
        &SP_PC_REG,
        &SP_SEMAPHORE_REG,
        &DPC_START_REG,
        &DPC_END_REG,
        &DPC_CURRENT_REG,
        &DPC_STATUS_REG,
        &DPC_CLOCK_REG,
        &DPC_BUFBUSY_REG,
        &DPC_PIPEBUSY_REG,
        &DPC_TMEM_REG,
        state);

    return 0;
}
