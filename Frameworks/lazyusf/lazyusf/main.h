#include <stdint.h>

#include "usf.h"

#define CPU_Default					-1
#define CPU_Interpreter				0
#define CPU_Recompiler				1

void DisplayError (char * Message, ...);
void StopEmulation(usf_state_t *);
