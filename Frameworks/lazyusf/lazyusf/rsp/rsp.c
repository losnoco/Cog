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
#include "../memory.h"
#include "../registers.h"

#include "../usf_internal.h"

#undef JUMP

#include "config.h"

#include "rsp.h"

void real_run_rsp(usf_state_t * state, uint32_t cycles)
{
    (void)cycles;

    if (SP_STATUS_REG & 0x00000003)
    {
        message("SP_STATUS_HALT", 3);
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
    return 0;
}
