#ifndef _AUDIO_H_
#define _AUDIO_H_

#include "usf.h"
#include "cpu.h"
#include "memory.h"

struct ai_dma
{
    uint32_t address;
    uint32_t length;
    unsigned int duration;
};

uint32_t AiReadLength(usf_state_t *);
void AiLenChanged(usf_state_t *);
void AiDacrateChanged(usf_state_t *, uint32_t value);
void AiTimerDone(usf_state_t *);
void AiQueueInt(usf_state_t *);

#endif
