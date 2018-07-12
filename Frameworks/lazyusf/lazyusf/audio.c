#include "usf.h"
#include "memory.h"
#include "audio.h"
#include <stdlib.h>
#include <stdio.h>

#include "usf_internal.h"

static const uint32_t AI_STATUS_BUSY = 0x40000000;
static const uint32_t AI_STATUS_FULL = 0x80000000;

static uint32_t get_remaining_dma_length(usf_state_t *state)
{
    unsigned int next_ai_event;
    unsigned int remaining_dma_duration;
    
    if (state->fifo[0].duration == 0)
        return 0;
    
    next_ai_event = state->Timers->NextTimer[AiTimer] + state->Timers->Timer;
    if (!state->Timers->Active[AiTimer])
        return 0;
    
    remaining_dma_duration = next_ai_event;
    
    if (remaining_dma_duration >= 0x80000000)
        return 0;
    
    return (uint32_t)((uint64_t)remaining_dma_duration * state->fifo[0].length / state->fifo[0].duration);
}

void AddBuffer(usf_state_t *state, unsigned char *buf, unsigned int length) {
	unsigned int i, do_max;
    int16_t * sample_buffer = state->sample_buffer;
	
	if(!state->cpu_running)
		return;
    
    do_max = length >> 2;
    if ( do_max > state->sample_buffer_count )
        do_max = (unsigned int) state->sample_buffer_count;

    if ( sample_buffer )
        for (i = 0; i < do_max; ++i)
        {
            *sample_buffer++ = ((int16_t*)buf)[1];
            *sample_buffer++ = ((int16_t*)buf)[0];
            buf += 4;
        }
    else
        buf += 4 * do_max;

    state->sample_buffer_count -= do_max;
    state->sample_buffer = sample_buffer;

    length -= do_max << 2;
    
    if ( length )
    {
        sample_buffer = state->samplebuf;
        do_max = length >> 2;
        for (i = 0; i < do_max; ++i)
        {
            *sample_buffer++ = ((int16_t*)buf)[1];
            *sample_buffer++ = ((int16_t*)buf)[0];
            buf += 4;
        }

        state->samples_in_buffer = do_max;
        state->cpu_running = 0;
    }
}

static unsigned int get_dma_duration(usf_state_t *state)
{
    unsigned int samples_per_sec = state->ROM_PARAMS.aidacrate / (1 + AI_DACRATE_REG);
    
    return (uint32_t)(((uint64_t)(AI_LEN_REG)*state->VI_INTR_TIME*state->ROM_PARAMS.vilimit)
                      / (4 * samples_per_sec));
}

void do_dma(usf_state_t * state, const struct ai_dma * dma) {
	AddBuffer(state, state->RDRAM + (dma->address & (state->RdramSize - 1) & ~3), dma->length);

	if(!(AI_STATUS_REG&AI_STATUS_FULL)) {
        if (state->enableFIFOfull) {
            ChangeTimer(state,AiTimer,dma->duration + state->Timers->Timer);
        }
        else {
            state->AudioIntrReg|=4;
        }
	}
}

void AiQueueInt(usf_state_t *state) {
    ChangeTimer(state,AiTimer,state->enableFIFOfull ? get_dma_duration(state) + state->Timers->Timer : 0);
}

void AiLenChanged(usf_state_t *state) {
    unsigned int duration = get_dma_duration(state);
    
    if (AI_STATUS_REG & AI_STATUS_BUSY) {
        state->fifo[1].address = AI_DRAM_ADDR_REG;
        state->fifo[1].length = AI_LEN_REG;
        state->fifo[1].duration = duration;
        
        if (state->enableFIFOfull)
            AI_STATUS_REG |= AI_STATUS_FULL;
        else
            do_dma(state, &state->fifo[1]);
    }
    else {
        state->fifo[0].address = AI_DRAM_ADDR_REG;
        state->fifo[0].length = AI_LEN_REG;
        state->fifo[0].duration = duration;
        AI_STATUS_REG |= AI_STATUS_BUSY;
        
        do_dma(state, &state->fifo[0]);
    }
}

void AiTimerDone(usf_state_t *state) {
    if (AI_STATUS_REG & AI_STATUS_FULL) {
        state->fifo[0].address  = state->fifo[1].address;
        state->fifo[0].length   = state->fifo[1].length;
        state->fifo[0].duration = state->fifo[1].duration;
        AI_STATUS_REG &= ~AI_STATUS_FULL;
        
        do_dma(state, &state->fifo[0]);
    }
    else {
        AI_STATUS_REG &= ~AI_STATUS_BUSY;
    }
}

unsigned int AiReadLength(usf_state_t * state) {
    return get_remaining_dma_length(state);
}

void AiDacrateChanged(usf_state_t * state, unsigned  int value) {
	AI_DACRATE_REG = value;
	state->SampleRate = (state->ROM_PARAMS.aidacrate) / (AI_DACRATE_REG + 1);
}
