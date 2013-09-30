/////////////////////////////////////////////////////////////////////////////
//
// iop - PS2 IOP emulation; can also do PS1 via compatibility mode
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __PSX_IOP_H__
#define __PSX_IOP_H__

#include "emuconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
** Types of events.
**
** Each event has a 64-bit cycle time, a format string and up to 4 dword args
*/
#define IOP_EVENT_REG_STORE    (0)
#define IOP_EVENT_REG_LOAD     (1)
#define IOP_EVENT_INTR_SIGNAL  (2)
#define IOP_EVENT_DMA_TRANSFER (3)
#define IOP_EVENT_VIRTUAL_IO   (4)
#define IOP_EVENT_MAX          (5)

/*
** Types of audit markers.
*/
#define IOP_AUDIT_UNUSED (0)
#define IOP_AUDIT_READ   (1)
#define IOP_AUDIT_WRITE  (2)

/*
** Static init
*/
sint32 EMU_CALL iop_init(void);

/*
** State init
** version = 1 for PS1, 2 for PS2
*/
uint32 EMU_CALL iop_get_state_size(int version);
void   EMU_CALL iop_clear_state(void *state, int version);

/*
** Obtain substates
*/
void*  EMU_CALL iop_get_r3000_state(void *state);
void*  EMU_CALL iop_get_spu_state(void *state);

/*
** For debugger use
** These functions have no side effects
*/
uint32 EMU_CALL iop_getword(void *state, uint32 a);
void   EMU_CALL iop_setword(void *state, uint32 a, uint32 d);

/*
** Uploads a section of data; only affects RAM
*/
void   EMU_CALL iop_upload_to_ram(void *state, uint32 address, const void *src, uint32 len);

/*
** IOP timing may be nonuniform
*/
uint64 EMU_CALL iop_get_odometer(void *state);

/*
** Event handling
*/
void   EMU_CALL iop_clear_events(void *state);
uint32 EMU_CALL iop_get_event_count(void *state);
void   EMU_CALL iop_get_event(void *state, uint64 *time, uint32 *type, char **fmt, uint32 *arg);
void   EMU_CALL iop_dismiss_event(void *state);
void   EMU_CALL iop_set_event_mask(void *state, uint64 mask);

/*
** Register a map for auditing purposes
** (must contain 1 byte for every RAM byte)
**
** Pass NULL to disable auditing.
*/
void EMU_CALL iop_register_map_for_auditing(void *state, uint8 *map);
uint32 EMU_CALL iop_get_bytes_used_in_audit(void *state);

//
// Set the screen refresh rate in Hz (50 or 60 for PAL or NTSC)
// Only 50 or 60 are valid; other values will be ignored
//
void EMU_CALL iop_set_refresh(void *state, uint32 refresh);

//
// DO NOT CALL THIS DIRECTLY.
// Call psx_execute instead.
//
sint32 EMU_CALL iop_execute(
  void   *state,
  void   *psx_state,
  sint32  cycles,
  sint16 *sound_buf,
  uint32 *sound_samples,
  uint32  event_mask
);

/*
** Compatibility level
*/
#define IOP_COMPAT_DEFAULT  (0)
#define IOP_COMPAT_HARSH    (1)
#define IOP_COMPAT_FRIENDLY (2)

void EMU_CALL iop_set_compat(void *state, uint8 compat);

/*
** Useful for playing with the tempo.
** 768 is normal.
** Higher is faster; lower is slower.
*/
void EMU_CALL iop_set_cycles_per_sample(void *state, uint32 cycles_per_sample);

#ifdef __cplusplus
}
#endif

#endif
