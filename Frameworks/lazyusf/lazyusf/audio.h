#ifndef _AUDIO_H_
#define _AUDIO_H_

#include "usf.h"
#include "cpu.h"
#include "memory.h"

uint32_t AiReadLength(usf_state_t *);
void AiLenChanged(usf_state_t *);
void AiDacrateChanged(usf_state_t *, uint32_t value);

#endif
