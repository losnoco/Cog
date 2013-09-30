/////////////////////////////////////////////////////////////////////////////
//
// z80 - Emulates Z80 CPU
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __Q_Z80_H__
#define __Q_Z80_H__

#include "emuconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

sint32 EMU_CALL z80_init(void);
uint32 EMU_CALL z80_get_state_size(void);
void   EMU_CALL z80_clear_state(void *state);

typedef uint8 (EMU_CALL * z80_read_callback_t   )(void *hwstate, uint16 a         );
typedef void  (EMU_CALL * z80_write_callback_t  )(void *hwstate, uint16 a, uint8 d);
typedef void  (EMU_CALL * z80_advance_callback_t)(void *hwstate, uint32 cycles);

struct Z80_MEMORY_TYPE { uint16 mask, n; void *p; };
struct Z80_MEMORY_MAP { uint16 x, y; struct Z80_MEMORY_TYPE type; };
#define Z80_MAP_TYPE_POINTER        (0)
#define Z80_MAP_TYPE_CALLBACK       (1)

void EMU_CALL z80_set_memory_maps(
  void *state,
  struct Z80_MEMORY_MAP *map_op,
  struct Z80_MEMORY_MAP *map_read,
  struct Z80_MEMORY_MAP *map_write,
  struct Z80_MEMORY_MAP *map_in,
  struct Z80_MEMORY_MAP *map_out
);
void EMU_CALL z80_set_advance_callback(
  void *state,
  z80_advance_callback_t advance,
  void *hwstate
);

//uint32 EMU_CALL z80_getreg(void *state, sint32 regnum);
//void   EMU_CALL z80_setreg(void *state, sint32 regnum, uint32 value);

uint16 EMU_CALL z80_getpc(void *state);

void   EMU_CALL z80_setirq(void *state, uint8 irq, uint8 vector);
void   EMU_CALL z80_setnmi(void *state, uint8 nmi);
void   EMU_CALL z80_break(void *state);

//
// Returns 0 or positive on success
// Returns negative on error
// (value is otherwise meaningless for now)
// Performs all advance calls according to how many cycles it actually executed
//
sint32 EMU_CALL z80_execute(void *state, sint32 cycles);

#ifdef __cplusplus
}
#endif

#endif
