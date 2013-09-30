/////////////////////////////////////////////////////////////////////////////
//
// qsound - QSound emulation
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __Q_QSOUND_H__
#define __Q_QSOUND_H__

#include "emuconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

const char* EMU_CALL qsound_getversion(void);

sint32 EMU_CALL qsound_init(void);
uint32 EMU_CALL qsound_get_state_size(void);
void   EMU_CALL qsound_clear_state(void *state);

//
// Obtain substates
//
void*  EMU_CALL qsound_get_z80_state (void *state);
void*  EMU_CALL qsound_get_qmix_state(void *state);

uint16 EMU_CALL qsound_getpc(void *state);

//
// Set ROM pointers, etc.
//
void EMU_CALL qsound_set_sample_rom(void *state, void *rom, uint32 size);
void EMU_CALL qsound_set_z80_rom(void *state, void *rom, uint32 size);
void EMU_CALL qsound_set_kabuki_key(void *state,
  uint32 swap_key1, uint32 swap_key2, uint16 addr_key, uint8 xor_key
);

void EMU_CALL qsound_set_rates(
  void *state,
  uint32 z80,
  uint32 irq,
  uint32 sample
);

uint64 EMU_CALL qsound_get_odometer(void *state);

//
// Executes
// Returns:
// >= 0: success (the number of cycles actually executed)
//   -1: error
//
sint32 EMU_CALL qsound_execute(
  void   *state,
  sint32  cycles,
  sint16 *sound_buf,
  uint32 *sound_samples
);

#ifdef __cplusplus
}
#endif

#endif
