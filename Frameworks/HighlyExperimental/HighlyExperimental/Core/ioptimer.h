/////////////////////////////////////////////////////////////////////////////
//
// ioptimer - IOP timers
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __PSX_IOPTIMER_H__
#define __PSX_IOPTIMER_H__

#include "emuconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

sint32 EMU_CALL ioptimer_init(void);
uint32 EMU_CALL ioptimer_get_state_size(void);
void   EMU_CALL ioptimer_clear_state(void *state);

void   EMU_CALL ioptimer_set_rates(void *state, uint32 sysclock, uint32 dots, uint32 lines, uint32 lines_visible, uint32 refresh_rate);

uint32 EMU_CALL ioptimer_cycles_until_interrupt(void *state);
uint32 EMU_CALL ioptimer_advance(void *state, uint32 cycles);

uint32 EMU_CALL ioptimer_lw(void *state, uint32 a, uint32 mask);
void   EMU_CALL ioptimer_sw(void *state, uint32 a, uint32 d, uint32 mask);

#ifdef __cplusplus
}
#endif

#endif
