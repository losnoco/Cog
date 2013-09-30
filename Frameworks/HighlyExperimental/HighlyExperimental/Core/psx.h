/////////////////////////////////////////////////////////////////////////////
//
// psx - Top-level emulation
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __PSX_PSX_H__
#define __PSX_PSX_H__

/////////////////////////////////////////////////////////////////////////////

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
sint32 EMU_CALL psx_init(void);

/////////////////////////////////////////////////////////////////////////////
//
// Version information
//
const char* EMU_CALL psx_getversion(void);

/////////////////////////////////////////////////////////////////////////////
//
// State init
// Version = 1 for PS1, 2 for PS2
//
// Example of usage:
//
//   void *my_psx_state = malloc(psx_get_state_size(version));
//   psx_clear_state(my_psx_state, version);
//   ...
//   [subsequent emu calls]
//   ...
//
uint32 EMU_CALL psx_get_state_size(uint8 version);
void   EMU_CALL psx_clear_state(void *state, uint8 version);

// Obtain substates
void*  EMU_CALL psx_get_iop_state(void *state);

/////////////////////////////////////////////////////////////////////////////
//
// Upload PS-X EXE - must INCLUDE the header.
// If the header includes the strings "North America", "Japan", or "Europe",
// the appropriate refresh rate is set.
// Returns nonzero on error.
// Will return error for PS2.
//
sint32 EMU_CALL psx_upload_psxexe(void *state, void *program, uint32 size);

/////////////////////////////////////////////////////////////////////////////
//
// Set the screen refresh rate in Hz (50 or 60 for PAL or NTSC)
// Only 50 or 60 are valid; other values will be ignored
//
void EMU_CALL psx_set_refresh(void *state, uint32 refresh);

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
// -1     Halted successfully (only applicable to PS2 environment)
// <= -2  Unrecoverable error
//
sint32 EMU_CALL psx_execute(
  void   *state,
  sint32  cycles,
  sint16 *sound_buf,
  uint32 *sound_samples,
  uint32  event_mask
);

/////////////////////////////////////////////////////////////////////////////
//
// hefile emucall handler
// Do not call this.
//
sint32 EMU_CALL psx_emucall(
  void   *state,
  uint8  *ram_native,
  uint32  ram_size,
  sint32  type,
  sint32  emufd,
  sint32  ofs,
  sint32  arg1,
  sint32  arg2
);

/////////////////////////////////////////////////////////////////////////////
//
// Read file callback
//
typedef sint32 (EMU_CALL * psx_readfile_t)(
  void *context,
  const char *path,
  sint32 offset,
  char *buffer,
  sint32 length
);

//
// Register a callback and context
// Pass NULL for both to disable
//
void EMU_CALL psx_set_readfile(
  void *state,
  psx_readfile_t callback,
  void *context
);

/////////////////////////////////////////////////////////////////////////////
//
// Console
//
typedef void (EMU_CALL * psx_console_out_t)(
  void *context,
  char c
);

void EMU_CALL psx_set_console_out(
  void *state,
  psx_console_out_t callback,
  void *context
);

void EMU_CALL psx_console_in(void *state, char c);

/////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

/////////////////////////////////////////////////////////////////////////////

#endif
