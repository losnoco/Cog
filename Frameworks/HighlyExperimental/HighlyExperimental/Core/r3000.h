/////////////////////////////////////////////////////////////////////////////
//
// r3000 - Emulates R3000 CPU
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __PSX_R3000_H__
#define __PSX_R3000_H__

#include "emuconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

sint32 EMU_CALL r3000_init(void);
uint32 EMU_CALL r3000_get_state_size(void);
void   EMU_CALL r3000_clear_state(void *state);

typedef uint32 (EMU_CALL * r3000_load_callback_t   )(void *hwstate, uint32 a,           uint32 dmask);
typedef void   (EMU_CALL * r3000_store_callback_t  )(void *hwstate, uint32 a, uint32 d, uint32 dmask);
typedef void   (EMU_CALL * r3000_advance_callback_t)(void *hwstate, uint32 cycles);

struct R3000_MEMORY_TYPE { uint32 mask, n; void *p; };
struct R3000_MEMORY_MAP { uint32 x, y; struct R3000_MEMORY_TYPE type; };
#define R3000_MAP_TYPE_POINTER        (0)
#define R3000_MAP_TYPE_CALLBACK       (1)

void EMU_CALL r3000_set_memory_maps(
  void *state,
  struct R3000_MEMORY_MAP *map_load,
  struct R3000_MEMORY_MAP *map_store
);
void EMU_CALL r3000_set_advance_callback(
  void *state,
  r3000_advance_callback_t advance,
  void *hwstate
);

void EMU_CALL r3000_set_prid(void *state, uint32 prid);

#define R3000_REG_GEN (0)
#define R3000_REG_C0  (32)
#define R3000_REG_PC  (64)
#define R3000_REG_HI  (65)
#define R3000_REG_LO  (66)
#define R3000_REG_CI  (67)
#define R3000_REG_MAX (68)

uint32 EMU_CALL r3000_getreg(void *state, sint32 regnum);
void   EMU_CALL r3000_setreg(void *state, sint32 regnum, uint32 value);

void   EMU_CALL r3000_setinterrupt(void *state, uint32 intpending);
void   EMU_CALL r3000_break(void *state);

//
// Returns 0 or positive on success
// Returns negative on error
// (value is otherwise meaningless for now)
// Performs all iop_advance calls according to how many cycles it actually executed
//
sint32 EMU_CALL r3000_execute(void *state, sint32 cycles);

void EMU_CALL r3000_predict(void *state, uint32 *profile);

/* usage statistics - 0x10000 is 100% */
uint32 EMU_CALL r3000_get_usage_fraction(void *state);

#ifdef __cplusplus
}
#endif

#endif
