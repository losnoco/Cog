#include "usf.h"
#include "memory.h"
#include "audio.h"
#include <stdlib.h>
#include <stdio.h>

#include "usf_internal.h"

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

void AiLenChanged(usf_state_t * state) {
	int32_t length = 0;
	uint32_t address = (AI_DRAM_ADDR_REG & 0x00FFFFF8);

	length = AI_LEN_REG & 0x3FFF8;

	AddBuffer(state, state->RDRAM+address, length);

	if(length && !(AI_STATUS_REG&0x80000000)) {
		const float VSyncTiming = 789000.0f;
		double BytesPerSecond = 48681812.0 / (AI_DACRATE_REG + 1) * 4;
		double CountsPerSecond = (double)((((double)VSyncTiming) * (double)60.0)) * 2.0;
		double CountsPerByte = (double)CountsPerSecond / (double)BytesPerSecond;
		unsigned int IntScheduled = (unsigned int)((double)AI_LEN_REG * CountsPerByte);

		ChangeTimer(state,AiTimer,IntScheduled);
	}

	if(state->enableFIFOfull) {
		if(AI_STATUS_REG&0x40000000)
			AI_STATUS_REG|=0x80000000;
	}

	AI_STATUS_REG|=0x40000000;
}

unsigned int AiReadLength(usf_state_t * state) {
	AI_LEN_REG = 0;
	return AI_LEN_REG;
}

void AiDacrateChanged(usf_state_t * state, unsigned  int value) {
	AI_DACRATE_REG = value;
	state->SampleRate = 48681812 / (AI_DACRATE_REG + 1);
}
