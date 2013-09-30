/////////////////////////////////////////////////////////////////////////////
//
// arm - Emulates ARM7DI CPU
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __SEGA_ARM_H__
#define __SEGA_ARM_H__

#include "emuconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

sint32 EMU_CALL arm_init(void);
uint32 EMU_CALL arm_get_state_size(void);
void   EMU_CALL arm_clear_state(void *state);

typedef uint32 (EMU_CALL * arm_load_callback_t   )(void *hwstate, uint32 a,           uint32 dmask);
typedef void   (EMU_CALL * arm_store_callback_t  )(void *hwstate, uint32 a, uint32 d, uint32 dmask);
typedef void   (EMU_CALL * arm_advance_callback_t)(void *hwstate, uint32 cycles);

struct ARM_MEMORY_TYPE { uint32 mask, n; void *p; };
struct ARM_MEMORY_MAP { uint32 x, y; struct ARM_MEMORY_TYPE type; };
#define ARM_MAP_TYPE_POINTER        (0)
#define ARM_MAP_TYPE_CALLBACK       (1)

void EMU_CALL arm_set_memory_maps(
  void *state,
  struct ARM_MEMORY_MAP *map_load,
  struct ARM_MEMORY_MAP *map_store
);
void EMU_CALL arm_set_advance_callback(
  void *state,
  arm_advance_callback_t advance,
  void *hwstate
);

#define ARM_REG_GEN      ( 0)
#define ARM_REG_CPSR     (16)
#define ARM_REG_SPSR     (17)
#define ARM_REG_MAX      (18)

uint32 EMU_CALL arm_getreg(void *state, sint32 regnum);
void   EMU_CALL arm_setreg(void *state, sint32 regnum, uint32 value);

void   EMU_CALL arm_break(void *state);

//
// Returns 0 or positive on success
// Returns negative on error
// (value is otherwise meaningless for now)
// Performs all advance calls according to how many cycles it actually executed
//
sint32 EMU_CALL arm_execute(void *state, sint32 cycles, uint8 fiq);

#ifdef __cplusplus
}
#endif

#endif
