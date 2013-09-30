/////////////////////////////////////////////////////////////////////////////
//
// dcsound - Dreamcast sound system emulation
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SEGA_DCSOUND_H__
#define __SEGA_DCSOUND_H__

#include "emuconfig.h"

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

//
// Init / state
//
sint32 EMU_CALL dcsound_init(void);
uint32 EMU_CALL dcsound_get_state_size(void);
void   EMU_CALL dcsound_clear_state(void *state);

//
// Obtain substates
//
void*  EMU_CALL dcsound_get_arm_state(void *state);
void*  EMU_CALL dcsound_get_yam_state(void *state);

//
// Get / set memory words with no side effects
//
uint32 EMU_CALL dcsound_getword(void *state, uint32 a);
void   EMU_CALL dcsound_setword(void *state, uint32 a, uint32 d);

//
// Uploads a section of data; only affects RAM
//
void   EMU_CALL dcsound_upload_to_ram(void *state, uint32 address, void *src, uint32 len);

/////////////////////////////////////////////////////////////////////////////
//
// Executes the given number of cycles or the given number of samples
// (whichever is less)
//
// Sets *sound_samples to the number of samples actually generated,
// which may be ZERO or LESS than the number requested, but never more.
//
// Return value:
// >= 0   The number of cycles actually executed, which may be ZERO, MORE,
//        or LESS than the number requested
// <= -1  Unrecoverable error
//
sint32 EMU_CALL dcsound_execute(
  void   *state,
  sint32  cycles,
  sint16 *sound_buf,
  uint32 *sound_samples
);

/////////////////////////////////////////////////////////////////////////////
//
// Get the current program counter
//
uint32 EMU_CALL dcsound_get_pc(void *state);

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

/////////////////////////////////////////////////////////////////////////////

#endif
