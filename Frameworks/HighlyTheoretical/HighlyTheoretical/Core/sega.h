/////////////////////////////////////////////////////////////////////////////
//
// sega - Top-level emulation for DC and Saturn
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SEGA_SEGA_H__
#define __SEGA_SEGA_H__

#include "emuconfig.h"

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Static init for the whole library
// Returns nonzero on error
//
// May generate a null dereference if the library was compiled wrong, but
// that's for me to worry about.
//
sint32 EMU_CALL sega_init(void);

/////////////////////////////////////////////////////////////////////////////
//
// Version information
//
const char* EMU_CALL sega_getversion(void);

/////////////////////////////////////////////////////////////////////////////
//
// State init
// Version = 1 for Saturn, 2 for Dreamcast
//
// Example of usage:
//
//   void *my_sega_state = malloc(sega_get_state_size(version));
//   sega_clear_state(my_sega_state, version);
//   ...
//   [subsequent sega calls]
//   ...
//
uint32 EMU_CALL sega_get_state_size(uint8 version);
void   EMU_CALL sega_clear_state(void *state, uint8 version);

/////////////////////////////////////////////////////////////////////////////
//
// Obtain substates
//
void*  EMU_CALL sega_get_satsound_state(void *state);
void*  EMU_CALL sega_get_dcsound_state(void *state);

/////////////////////////////////////////////////////////////////////////////
//
// Upload program - must INCLUDE the 4-byte header.
// Returns nonzero on error.
//
sint32 EMU_CALL sega_upload_program(void *state, void *program, uint32 size);

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
sint32 EMU_CALL sega_execute(
  void   *state,
  sint32  cycles,
  sint16 *sound_buf,
  uint32 *sound_samples
);

/////////////////////////////////////////////////////////////////////////////
//
// Get the current program counter
//
uint32 EMU_CALL sega_get_pc(void *state);

/////////////////////////////////////////////////////////////////////////////
//
// Enable or disable various things
//
void EMU_CALL sega_enable_dry(void *state, uint8 enable);
void EMU_CALL sega_enable_dsp(void *state, uint8 enable);
void EMU_CALL sega_enable_dsp_dynarec(void *state, uint8 enable);

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

/////////////////////////////////////////////////////////////////////////////

#endif
