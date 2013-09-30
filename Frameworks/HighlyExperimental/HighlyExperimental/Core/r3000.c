/////////////////////////////////////////////////////////////////////////////
//
// r3000 - Emulates R3000 CPU
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "r3000.h"

/////////////////////////////////////////////////////////////////////////////
/*
** integer overflow exceptions are not taken!
** divide by zero exceptions are not taken!
*/
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/*
** Divider is now hardcoded
*/
#define DIVIDER (1)

/////////////////////////////////////////////////////////////////////////////
/*
** Static information
*/

/* (none) */

sint32 EMU_CALL r3000_init(void) {
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
/*
** State information
*/

#define STATE ((struct R3000_STATE*)(state))

#define C0_status (12)
#define C0_cause  (13)
#define C0_epc    (14)
#define C0_prid   (15)

struct R3000_STATE {
  uint32 regs[32];

  uint32 c0_status;
  uint32 c0_cause;
  uint32 c0_epc;
  uint32 c0_prid;

  uint32 pc;
  uint32 hi,lo;

  uint32 intpending;

  sint32 slot;
  uint32 slot_target;

  sint32 cycles_remaining;
  sint32 cycles_remaining_last_checkpoint;
  sint32 cycles_deferred_from_break;

  /* usage statistics */
  sint32 usage_total_cycles;
  sint32 usage_idle_cycles;
  sint32 usage_cycles_max;
  uint32 usage_last_fraction;

  uint32 cache_ctrl;

  uint32 cache_isolate;

  //
  // These are REGISTERED EXTERNAL POINTERS.
  //
  r3000_advance_callback_t advance;
  void *hwstate;
  struct R3000_MEMORY_MAP *map_load;
  struct R3000_MEMORY_MAP *map_store;

  //
  // The following are TEMPORARY.
  // There are no location invariance issues.
  //
  uint32 maxpc;
  void *fetchbase;
  uint32 fetchbox;
};

uint32 EMU_CALL r3000_get_state_size(void) {
  return sizeof(struct R3000_STATE);
}

void EMU_CALL r3000_clear_state(void *state) {
  memset(state, 0, sizeof(struct R3000_STATE));
  STATE->c0_prid = 0x02;
  STATE->pc = 0xBFC00000;
  /* update statistics every 20 million cycles (may overshoot a little) */
  STATE->usage_cycles_max = 20000000;
}

/////////////////////////////////////////////////////////////////////////////
/*
** Registration of external pointers within the state
*/
void EMU_CALL r3000_set_memory_maps(
  void *state,
  struct R3000_MEMORY_MAP *map_load,
  struct R3000_MEMORY_MAP *map_store
) {
  STATE->map_load  = map_load;
  STATE->map_store = map_store;
}

void EMU_CALL r3000_set_advance_callback(
  void *state,
  r3000_advance_callback_t advance,
  void *hwstate
) {
  STATE->advance = advance;
  STATE->hwstate = hwstate;
}

/////////////////////////////////////////////////////////////////////////////
/*
** Set the processor revision ID
*/
void EMU_CALL r3000_set_prid(void *state, uint32 prid) {
  STATE->c0_prid = prid;
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE uint32 EMU_CALL getc0(struct R3000_STATE *state, uint32 regnum) {
  switch(regnum) {
  case 12: return state->c0_status;
  case 13: return state->c0_cause;
  case 14: return state->c0_epc;
  case 15: return state->c0_prid;
  }
  return 0;
}

static EMU_INLINE void EMU_CALL setc0(struct R3000_STATE *state, uint32 regnum, uint32 value) {
  switch(regnum) {
  case 12: state->c0_status = value; break;
  case 13: state->c0_cause = value; break;
  case 14: state->c0_epc = value; break;
  case 15: break;
  }
}

/////////////////////////////////////////////////////////////////////////////

uint32 EMU_CALL r3000_getreg(void *state, sint32 regnum) {
  if(regnum >= R3000_REG_GEN && regnum < (R3000_REG_GEN+32)) { return STATE->regs[regnum - R3000_REG_GEN]; }
  if(regnum >= R3000_REG_C0 && regnum < (R3000_REG_C0+32)) { return getc0(STATE, regnum - R3000_REG_C0); }
  switch(regnum) {
  case R3000_REG_PC: return STATE->pc;
  case R3000_REG_HI: return STATE->hi;
  case R3000_REG_LO: return STATE->lo;

  case R3000_REG_CI: return STATE->cache_isolate;

  }
  return 0;
}

void EMU_CALL r3000_setreg(void *state, sint32 regnum, uint32 value) {
  if(regnum >= R3000_REG_GEN && regnum < (R3000_REG_GEN+32)) { STATE->regs[regnum - R3000_REG_GEN] = value; return; }
  if(regnum >= R3000_REG_C0 && regnum < (R3000_REG_C0+32)) { setc0(STATE, regnum - R3000_REG_C0, value); return; }
  switch(regnum) {
  case R3000_REG_PC: STATE->pc = value; return;
  case R3000_REG_HI: STATE->hi = value; return;
  case R3000_REG_LO: STATE->lo = value; return;

  case R3000_REG_CI: STATE->cache_isolate = value; return;

  }
}

/////////////////////////////////////////////////////////////////////////////

void EMU_CALL r3000_break(void *state) {
  if(STATE->cycles_remaining <= 0) return;
  STATE->cycles_deferred_from_break += STATE->cycles_remaining;
  STATE->cycles_remaining_last_checkpoint -= STATE->cycles_remaining;
  STATE->cycles_remaining                  = 0;
}

void EMU_CALL r3000_setinterrupt(void *state, uint32 intpending) {
  STATE->intpending = intpending;
  r3000_break(state);
}

/////////////////////////////////////////////////////////////////////////////
/*
** Memory map walker
*/
static EMU_INLINE struct R3000_MEMORY_TYPE* EMU_CALL mmwalk(struct R3000_MEMORY_MAP *map, uint32 a) {
  a &= 0x1FFFFFFF;
  for(;; map++) {
    uint32 x = map->x;
    uint32 y = map->y;
    if(a < x || a > y) continue;
    return &(map->type);
  }
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE void EMU_CALL exception(struct R3000_STATE *state, uint32 cause, uint32 IP) {
  /* Step out of the delay slot, if we were in one */
  if(state->slot) {
//    state->pc = state->slot_target & (~3);
    state->pc -= 4;
    state->slot = 0;
  }

  /* Shift mode/interrupt bits */
  state->c0_status =
    (state->c0_status & 0xFFFFFFC0) |
    ((state->c0_status & 0x0F) << 2);
  state->c0_epc = state->pc;
  state->c0_cause = ((cause & 0xF) << 2) | ((IP & 0x3F) << 10);
  /* ignore BEV for now */
  state->pc = 0x80000080;

  /* Force a recheck of the memory map */
  state->maxpc = 0;

//printf("exception %08X %08X\n",cause,IP);
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE void EMU_CALL update_usage(void *state) {
  /*
  ** Update usage statistics
  */
  if(STATE->usage_total_cycles >= STATE->usage_cycles_max) {
    sint64 fraction;
    /*
    ** Bounds checking
    */
    if(STATE->usage_total_cycles <= 0) { STATE->usage_total_cycles = 1; }

    if(STATE->usage_idle_cycles > STATE->usage_total_cycles) {
      STATE->usage_idle_cycles = STATE->usage_total_cycles;
    } else if(STATE->usage_idle_cycles < 0) {
      STATE->usage_idle_cycles = 0;
    }

    fraction = STATE->usage_total_cycles - STATE->usage_idle_cycles;
    fraction *= ((sint64)(0x20000));
    fraction /= ((sint64)(STATE->usage_total_cycles));
    fraction++;
    fraction /= 2;
    if(fraction < 0x00000) fraction = 0x00000;
    if(fraction > 0x10000) fraction = 0x10000;
    STATE->usage_last_fraction = (uint32)fraction;
    STATE->usage_total_cycles = 0;
    STATE->usage_idle_cycles = 0;
  }
}

/////////////////////////////////////////////////////////////////////////////

#define INS_S ((uint32)((instruction>>21)&0x1F))
#define INS_T ((uint32)((instruction>>16)&0x1F))
#define INS_D ((uint32)((instruction>>11)&0x1F))
#define INS_H ((uint32)((instruction>>6)&0x1F))
#define INS_I ((uint32)(instruction&0xFFFF))

#define   SIGNED16(x) ((sint32)(((sint16)(x))))
#define UNSIGNED16(x) ((uint32)(((uint16)(x))))

#define REL_I (PC+4+(((sint32)(SIGNED16(INS_I)))<<2))
#define ABS_I ((PC&0xF0000000)|((instruction<<2)&0x0FFFFFFC))

#define J_REL_I {STATE->slot=2;STATE->slot_target=REL_I;}
#define J_ABS_I {STATE->slot=2;STATE->slot_target=ABS_I;}

#define REGS (STATE->regs)
#define HI   (STATE->hi)
#define LO   (STATE->lo)
#define PC   (STATE->pc)

static EMU_INLINE void EMU_CALL cold_interrupt_check(struct R3000_STATE *state) {
  uint32 hotint = ((STATE->intpending) & (STATE->c0_status >> 8)) & 0x3F;
  if(!(STATE->c0_status & 1)) hotint = 0;
  if(hotint) { exception(STATE, 0, hotint); state->cycles_remaining -= DIVIDER; }
}

static EMU_INLINE void EMU_CALL hw_sync(struct R3000_STATE *state) {
  sint32 d = state->cycles_remaining_last_checkpoint - state->cycles_remaining;
  if(d > 0) {
    state->advance(state->hwstate, d);
    state->cycles_remaining_last_checkpoint = state->cycles_remaining;
  }
}

static EMU_INLINE void EMU_CALL renew_fetch_region(struct R3000_STATE *state) {
  struct R3000_MEMORY_TYPE *t = mmwalk(state->map_load, state->pc);
  if(t->n == R3000_MAP_TYPE_POINTER) {
    uint32 astart = (state->pc) & (~(t->mask));
    state->maxpc = astart + ((t->mask) + 1);
    state->fetchbase = ((uint8*)(t->p)) - astart;
  } else {
    state->maxpc = state->pc + 4;
    state->fetchbase = ((uint8*)(&(state->fetchbox)))-(state->pc);
    state->fetchbox = ((r3000_load_callback_t)(t->p))(state->hwstate, state->pc, 0xFFFFFFFF);
  }
}

static EMU_INLINE uint32 EMU_CALL lb(struct R3000_STATE *state, uint32 a) {
  struct R3000_MEMORY_TYPE *t = mmwalk(state->map_load, a);
  a &= t->mask;
  if(t->n == R3000_MAP_TYPE_POINTER) {
    a ^= EMU_ENDIAN_XOR(3);
    return *((uint8*)(((uint8*)(t->p))+a));
  } else {
    uint32 sh = (a & 3) * 8;
    hw_sync(state);
    a = ((r3000_load_callback_t)(t->p))(state->hwstate, a & (~3), 0xFF << sh);
    a >>= sh;
    return a & 0xFF;
  }
}

static EMU_INLINE uint32 EMU_CALL lh(struct R3000_STATE *state, uint32 a) {
  struct R3000_MEMORY_TYPE *t = mmwalk(state->map_load, a);
  a &= t->mask;
  if(t->n == R3000_MAP_TYPE_POINTER) {
    a ^= EMU_ENDIAN_XOR(2);
    a &= (~1);
    return *((uint16*)(((uint8*)(t->p))+a));
  } else {
    uint32 sh = (a & 2) * 8;
    hw_sync(state);
    a = ((r3000_load_callback_t)(t->p))(state->hwstate, a & (~3), 0xFFFF << sh);
    a >>= sh;
    return a & 0xFFFF;
  }
}

static EMU_INLINE uint32 EMU_CALL lw(struct R3000_STATE *state, uint32 a) {
  struct R3000_MEMORY_TYPE *t;
  /*
  ** HACK HACK HACK TODO fix later
  */
  if(a == 0xFFFE0130) {
    return state->cache_ctrl;
  }
  t = mmwalk(state->map_load, a);
  a &= t->mask;
  if(t->n == R3000_MAP_TYPE_POINTER) {
    a &= (~3);
    return *((uint32*)(((uint8*)(t->p))+a));
  } else {
    hw_sync(state);
    a = ((r3000_load_callback_t)(t->p))(state->hwstate, a & (~3), 0xFFFFFFFF);
    return a;
  }
}

//
// Performs no callbacks; suitable for prediction
//
static EMU_INLINE uint32 EMU_CALL lw_noeffects(struct R3000_STATE *state, uint32 a) {
  struct R3000_MEMORY_TYPE *t;
  t = mmwalk(state->map_load, a);
  a &= t->mask;
  if(t->n == R3000_MAP_TYPE_POINTER) {
    a &= (~3);
    return *((uint32*)(((uint8*)(t->p))+a));
  } else {
    return 0;
  }
}

static EMU_INLINE void EMU_CALL sb(struct R3000_STATE *state, uint32 a, uint32 d) {
  struct R3000_MEMORY_TYPE *t = mmwalk(state->map_store, a);
  a &= t->mask;
  if(t->n == R3000_MAP_TYPE_POINTER) {
    a ^= EMU_ENDIAN_XOR(3);
    *((uint8*)(((uint8*)(t->p))+a)) = d;
  } else {
    uint32 sh = (a & 3) * 8;
    hw_sync(state);
    d &= 0xFF;
    ((r3000_store_callback_t)(t->p))(state->hwstate, a & (~3), d << sh, 0xFF << sh);
  }
}

static EMU_INLINE void EMU_CALL sh(struct R3000_STATE *state, uint32 a, uint32 d) {
  struct R3000_MEMORY_TYPE *t = mmwalk(state->map_store, a);
  a &= t->mask;
  if(t->n == R3000_MAP_TYPE_POINTER) {
    a ^= EMU_ENDIAN_XOR(2);
    a &= (~1);
    *((uint16*)(((uint8*)(t->p))+a)) = d;
  } else {
    uint32 sh = (a & 2) * 8;
    hw_sync(state);
    d &= 0xFFFF;
    ((r3000_store_callback_t)(t->p))(state->hwstate, a & (~3), d << sh, 0xFFFF << sh);
  }
}

static EMU_INLINE void EMU_CALL sw(struct R3000_STATE *state, uint32 a, uint32 d) {
  struct R3000_MEMORY_TYPE *t;
  /*
  ** HACK HACK HACK TODO fix later
  */
  if(a == 0xFFFE0130) {
    state->cache_ctrl = d;
    if(d >= 0x10000) {
      state->cache_isolate = 0;
    } else {
      state->cache_isolate = 1;
    }
    return;
  }
  if(state->cache_isolate) return;

  t = mmwalk(state->map_store, a);
  a &= t->mask;
  a &= (~3);
  if(t->n == R3000_MAP_TYPE_POINTER) {
    *((uint32*)(((uint8*)(t->p))+a)) = d;
  } else {
    hw_sync(state);
    ((r3000_store_callback_t)(t->p))(state->hwstate, a & (~3), d, 0xFFFFFFFF);
  }
}

static EMU_INLINE uint32 EMU_CALL fetch(struct R3000_STATE *state) {
  if(state->pc >= state->maxpc) renew_fetch_region(state);
  return *((uint32*)(((uint8*)(state->fetchbase))+(state->pc)));
}

#define caseMINOR(a) case(a):
#define caseMAJOR(a) case(a):

//
// Returns 0 or positive on success
// Returns negative on error
// (value is otherwise meaningless for now)
//
sint32 EMU_CALL r3000_execute(void *state, sint32 cycles) {
  uint32 a, d;
  uint32 instruction;

  STATE->cycles_remaining_last_checkpoint = cycles;
  STATE->cycles_remaining = cycles;
  STATE->cycles_deferred_from_break = 0;

  STATE->usage_total_cycles += cycles;

  PC &= (~3);

  cold_interrupt_check(STATE);

  //
  // This guarantees location invariance on fetchbox/fetchbase
  //
  STATE->maxpc = 0;

  while(STATE->slot || STATE->cycles_remaining > 0) {
    instruction = fetch(state);
    if(instruction < 0x04000000) {
      switch(instruction & 0x3F) {
      /* MINOR */
      caseMINOR(0x00) /* sll   */ if(INS_D) { REGS[INS_D] = ((uint32)(REGS[INS_T])) << INS_H;
        } else {
          /* idle detect */
          /* if this is a nop in a branch delay slot, and the target is the
          ** previous instruction, then we are officially idle */
          if((STATE->slot) && (STATE->slot_target == (PC - 4))) {
            /* steal off cycles under cycidle */
            sint32 cycidle = STATE->cycles_remaining;
            if(cycidle < 0) { cycidle = 0; }
            STATE->cycles_remaining -= cycidle;
            STATE->usage_idle_cycles += cycidle;

            STATE->slot = 0;
            PC -= 4;
            STATE->maxpc = 0;
            goto finishing_sync;
          }
        }
        break;
      caseMINOR(0x02) /* srl   */ if(INS_D) { REGS[INS_D] = ((uint32)(REGS[INS_T])) >> INS_H; } break;
      caseMINOR(0x03) /* sra   */ if(INS_D) { REGS[INS_D] = ((sint32)(REGS[INS_T])) >> INS_H; } break;
      caseMINOR(0x04) /* sllv  */ if(INS_D) { uint32 sc=REGS[INS_S]; if(sc>=32){REGS[INS_D]=0;}else{REGS[INS_D]=REGS[INS_T]<<sc;} } break;
      caseMINOR(0x06) /* srlv  */ if(INS_D) { uint32 sc=REGS[INS_S]; if(sc>=32){REGS[INS_D]=0;}else{REGS[INS_D]=REGS[INS_T]>>sc;} } break;
      caseMINOR(0x07) /* srav  */ if(INS_D) { uint32 sc=REGS[INS_S]; if(sc>=32){sc=31;}            {REGS[INS_D]=(((sint32)(REGS[INS_T]))>>sc);} } break;
      caseMINOR(0x08) /* jr    */ if(!STATE->slot) {                      {STATE->slot=2;STATE->slot_target=REGS[INS_S];} } break;
      caseMINOR(0x09) /* jalr  */ if(!STATE->slot) { REGS[INS_D] = PC + 8;{STATE->slot=2;STATE->slot_target=REGS[INS_S];} } break;
      caseMINOR(0x0C) /*syscall*/ if(!STATE->slot) { exception(STATE, 8, 0); PC -= 4; } break;
      caseMINOR(0x10) /* mfhi  */ if(INS_D) { REGS[INS_D] = HI; } break;
      caseMINOR(0x11) /* mthi  */           { HI = REGS[INS_S]; } break;
      caseMINOR(0x12) /* mflo  */ if(INS_D) { REGS[INS_D] = LO; } break;
      caseMINOR(0x13) /* mtlo  */           { LO = REGS[INS_S]; } break;
      caseMINOR(0x18) /* mult  */ { sint64 t = ((sint64)(((sint32)(REGS[INS_S])))) * ((sint64)(((sint32)(REGS[INS_T])))); LO = (uint32)(t); HI = (uint32)(t >> 32); } break;
      caseMINOR(0x19) /* multu */ { uint64 t = ((uint64)(((uint32)(REGS[INS_S])))) * ((uint64)(((uint32)(REGS[INS_T])))); LO = (uint32)(t); HI = (uint32)(t >> 32); } break;
      caseMINOR(0x1A) /* div   */ if(REGS[INS_T]) { LO = ((sint32)(REGS[INS_S])) / ((sint32)(REGS[INS_T])); HI = ((sint32)(REGS[INS_S])) % ((sint32)(REGS[INS_T])); } break;
      caseMINOR(0x1B) /* divu  */ if(REGS[INS_T]) { LO = ((uint32)(REGS[INS_S])) / ((uint32)(REGS[INS_T])); HI = ((uint32)(REGS[INS_S])) % ((uint32)(REGS[INS_T])); } break;
      caseMINOR(0x20) /* add   */ if(INS_D) { REGS[INS_D] =  (((uint32)(REGS[INS_S])) + ((uint32)(REGS[INS_T]))); } break;
      caseMINOR(0x21) /* addu  */ if(INS_D) { REGS[INS_D] =  (((uint32)(REGS[INS_S])) + ((uint32)(REGS[INS_T]))); } break;
      caseMINOR(0x22) /* sub   */ if(INS_D) { REGS[INS_D] =  (((uint32)(REGS[INS_S])) - ((uint32)(REGS[INS_T]))); } break;
      caseMINOR(0x23) /* subu  */ if(INS_D) { REGS[INS_D] =  (((uint32)(REGS[INS_S])) - ((uint32)(REGS[INS_T]))); } break;
      caseMINOR(0x24) /* and   */ if(INS_D) { REGS[INS_D] =  (((uint32)(REGS[INS_S])) & ((uint32)(REGS[INS_T]))); } break;
      caseMINOR(0x25) /* or    */ if(INS_D) { REGS[INS_D] =  (((uint32)(REGS[INS_S])) | ((uint32)(REGS[INS_T]))); } break;
      caseMINOR(0x26) /* xor   */ if(INS_D) { REGS[INS_D] =  (((uint32)(REGS[INS_S])) ^ ((uint32)(REGS[INS_T]))); } break;
      caseMINOR(0x27) /* nor   */ if(INS_D) { REGS[INS_D] = ~(((uint32)(REGS[INS_S])) | ((uint32)(REGS[INS_T]))); } break;
      caseMINOR(0x2A) /* slt   */ if(INS_D) { REGS[INS_D] =  (((sint32)(REGS[INS_S])) < ((sint32)(REGS[INS_T]))); } break;
      caseMINOR(0x2B) /* sltu  */ if(INS_D) { REGS[INS_D] =  (((uint32)(REGS[INS_S])) < ((uint32)(REGS[INS_T]))); } break;
      default: goto badins;
      }
    } else {
      switch(instruction >> 26) {
      /* MAJOR */
      caseMAJOR(0x01)
        switch(INS_T) {
        case 0x00: /* bltz  */ if(!STATE->slot) { if( ((sint32)(REGS[INS_S])) <  0) {                J_REL_I; } } break;
        case 0x01: /* bgez  */ if(!STATE->slot) { if( ((sint32)(REGS[INS_S])) >= 0) {                J_REL_I; } } break;
        case 0x10: /* bltzal*/ if(!STATE->slot) { if( ((sint32)(REGS[INS_S])) <  0) { REGS[31]=PC+8; J_REL_I; } } break;
        case 0x11: /* bgezal*/ if(!STATE->slot) { if( ((sint32)(REGS[INS_S])) >= 0) { REGS[31]=PC+8; J_REL_I; } } break;
        default: goto badins;
        }
        break;
      caseMAJOR(0x02) /* j     */ if(!STATE->slot) {                J_ABS_I; } break;
      caseMAJOR(0x03) /* jal   */ if(!STATE->slot) { REGS[31]=PC+8; J_ABS_I; } break;
      caseMAJOR(0x04) /* beq   */ if(!STATE->slot) { if(REGS[INS_S] == REGS[INS_T]) { J_REL_I; } } break;
      caseMAJOR(0x05) /* bne   */ if(!STATE->slot) { if(REGS[INS_S] != REGS[INS_T]) { J_REL_I; } } break;
      caseMAJOR(0x06) /* blez  */ if(!STATE->slot) { if( ((sint32)(REGS[INS_S])) <= 0) { J_REL_I; } } break;
      caseMAJOR(0x07) /* bgtz  */ if(!STATE->slot) { if( ((sint32)(REGS[INS_S])) >  0) { J_REL_I; } } break;
      caseMAJOR(0x08) /* addi  */ if(INS_T) { REGS[INS_T] = REGS[INS_S] +   SIGNED16(INS_I); } break;
      caseMAJOR(0x09) /* addiu */ if(INS_T) { REGS[INS_T] = REGS[INS_S] +   SIGNED16(INS_I); } break;
      caseMAJOR(0x0A) /* slti  */ if(INS_T) { REGS[INS_T] = ( ((sint32)(REGS[INS_S])) < ((sint32)( ((sint32)( SIGNED16(INS_I) )) )) ); } break;
      caseMAJOR(0x0B) /* sltiu */ if(INS_T) { REGS[INS_T] = ( ((uint32)(REGS[INS_S])) < ((uint32)( ((sint32)( SIGNED16(INS_I) )) )) ); } break;
      caseMAJOR(0x0C) /* andi  */ if(INS_T) { REGS[INS_T] = REGS[INS_S] & UNSIGNED16(INS_I); } break;
      caseMAJOR(0x0D) /* ori   */ if(INS_T) { REGS[INS_T] = REGS[INS_S] | UNSIGNED16(INS_I); } break;
      caseMAJOR(0x0E) /* xori  */ if(INS_T) { REGS[INS_T] = REGS[INS_S] ^ UNSIGNED16(INS_I); } break;
      caseMAJOR(0x0F) /* lui   */ if(INS_T) { REGS[INS_T] = INS_I << 16; } break;
      caseMAJOR(0x10) /* cop0  */
        switch(INS_S) {
        case 0x00: /* mfc0  */ if(INS_T) { REGS[INS_T] = getc0(STATE, INS_D); } break;
        case 0x04: /* mtc0  */           { setc0(STATE, INS_D, REGS[INS_T]); r3000_break(state); } break;
        case 0x10: /* rfe   */ {
          STATE->c0_status =
            (STATE->c0_status & 0xFFFFFFC0) |
            ((STATE->c0_status & 0x3C) >> 2) |
            ((STATE->c0_status & 0x03) << 4);
          r3000_break(state); } break;
        default: goto badins;
        }
        break;
      caseMAJOR(0x20) /* lb    */ { a = REGS[INS_S] + SIGNED16(INS_I); d = lb(state, a); if(INS_T) { REGS[INS_T] = ((sint32)((sint8)(d)));  } } break;
      caseMAJOR(0x21) /* lh    */ { a = REGS[INS_S] + SIGNED16(INS_I); d = lh(state, a); if(INS_T) { REGS[INS_T] = ((sint32)((sint16)(d))); } } break;
      caseMAJOR(0x23) /* lw    */ { a = REGS[INS_S] + SIGNED16(INS_I); d = lw(state, a); if(INS_T) { REGS[INS_T] = d;                       } } break;
      caseMAJOR(0x24) /* lbu   */ { a = REGS[INS_S] + SIGNED16(INS_I); d = lb(state, a); if(INS_T) { REGS[INS_T] = d & 0x000000FF;          } } break;
      caseMAJOR(0x25) /* lhu   */ { a = REGS[INS_S] + SIGNED16(INS_I); d = lh(state, a); if(INS_T) { REGS[INS_T] = d & 0x0000FFFF;          } } break;
      caseMAJOR(0x28) /* sb    */ { a = REGS[INS_S] + SIGNED16(INS_I);     sb(state, a, REGS[INS_T] & 0x000000FF); } break;
      caseMAJOR(0x29) /* sh    */ { a = REGS[INS_S] + SIGNED16(INS_I);     sh(state, a, REGS[INS_T] & 0x0000FFFF); } break;
      caseMAJOR(0x2B) /* sw    */ { a = REGS[INS_S] + SIGNED16(INS_I);     sw(state, a, REGS[INS_T]             ); } break;

      caseMAJOR(0x22) /* lwl   */
        a = REGS[INS_S] + SIGNED16(INS_I);
        { int bitshift;
          for(bitshift = 24;; bitshift -= 8) {
            REGS[INS_T] &= ~(0xFF << bitshift);
            REGS[INS_T] |= (((uint32)(lb(state, a))) & 0xFF) << bitshift;
            if(!(a&3))break;
            a--;
          }
        }
        REGS[0] = 0;
        break;
      caseMAJOR(0x26) /* lwr   */
        a = REGS[INS_S] + SIGNED16(INS_I);
        { int bitshift;
          for(bitshift = 0;; bitshift += 8) {
            REGS[INS_T] &= ~(0xFF << bitshift);
            REGS[INS_T] |= (((uint32)(lb(state, a))) & 0xFF) << bitshift;
            if((a&3) == 3)break;
            a++;
          }
        }
        REGS[0] = 0;
        break;
      caseMAJOR(0x2A) /* swl   */
        a = REGS[INS_S] + SIGNED16(INS_I);
        { int bitshift;
          for(bitshift = 24;; bitshift -= 8) {
            sb(state, a, REGS[INS_T]>>bitshift);
            if(!(a&3))break;
            a--;
          }
        }
        break;
      caseMAJOR(0x2E) /* swr   */
        a = REGS[INS_S] + SIGNED16(INS_I);
        { int bitshift;
          for(bitshift = 0;; bitshift += 8) {
            sb(state, a, REGS[INS_T]>>bitshift);
            if((a&3) == 3)break;
            a++;
          }
        }
        break;

      default: goto badins;
      }
    }

    PC += 4;

    STATE->cycles_remaining -= DIVIDER;
    if(STATE->slot) {
      STATE->slot--;
      if(!(STATE->slot)) {
        PC = STATE->slot_target & (~3);
        STATE->maxpc = 0;
      }
    }
  }

finishing_sync:

  STATE->cycles_remaining_last_checkpoint += STATE->cycles_deferred_from_break;
  STATE->cycles_remaining                 += STATE->cycles_deferred_from_break;
  STATE->cycles_deferred_from_break = 0;
  STATE->usage_total_cycles -= STATE->cycles_remaining;
  update_usage(state);

  hw_sync(state);
  return 0;

badins:

  STATE->cycles_remaining_last_checkpoint += STATE->cycles_deferred_from_break;
  STATE->cycles_remaining                 += STATE->cycles_deferred_from_break;
  STATE->cycles_deferred_from_break = 0;
  STATE->usage_total_cycles -= STATE->cycles_remaining;
  update_usage(state);

  hw_sync(state);
  return -1;

}

/////////////////////////////////////////////////////////////////////////////
//
// Get usage statistics
//
uint32 EMU_CALL r3000_get_usage_fraction(void *state) {
  return STATE->usage_last_fraction;
}

/////////////////////////////////////////////////////////////////////////////
//
// Perform prediction of what next instruction will do
//
// first word: address of next instruction
// following words describe possible activity
// 0xxxxxxx yyyyyyyy means "read x bytes from address y"
// 1xxxxxxx yyyyyyyy means "write x bytes from address y"
// 20000000 yyyyyyyy means "jump to y"
// 30000000 yyyyyyyy means "call to y"
// 00000000 terminates the list
// max 256 bytes in prediction profile
//
#define PROFILE_READ  0x00000000
#define PROFILE_WRITE 0x10000000
#define PROFILE_JUMP  0x20000000
#define PROFILE_CALL  0x30000000

void EMU_CALL r3000_predict(void *state, uint32 *profile) {
  uint32 instruction = lw_noeffects(STATE, PC);
  *profile++ = PC + 4;
  if(instruction < 0x04000000) {
    switch(instruction & 0x3F) {
    caseMINOR(0x08) /* jr    */ *profile++ = PROFILE_JUMP; *profile++ = REGS[INS_S]; break;
    caseMINOR(0x09) /* jalr  */ *profile++ = PROFILE_CALL; *profile++ = REGS[INS_S]; break;
    }
  } else {
    switch(instruction >> 26) {
    caseMAJOR(0x01)
      switch(INS_T) {
      case 0x00: /* bltz  */ *profile++ = PROFILE_JUMP; *profile++ = REL_I; break;
      case 0x01: /* bgez  */ *profile++ = PROFILE_JUMP; *profile++ = REL_I; break;
      case 0x10: /* bltzal*/ *profile++ = PROFILE_CALL; *profile++ = REL_I; break;
      case 0x11: /* bgezal*/ *profile++ = PROFILE_CALL; *profile++ = REL_I; break;
      }
      break;
    caseMAJOR(0x02) /* j     */ *profile++ = PROFILE_JUMP; *profile++ = ABS_I; break;
    caseMAJOR(0x03) /* jal   */ *profile++ = PROFILE_CALL; *profile++ = ABS_I; break;
    caseMAJOR(0x04) /* beq   */ *profile++ = PROFILE_JUMP; *profile++ = REL_I; break;
    caseMAJOR(0x05) /* bne   */ *profile++ = PROFILE_JUMP; *profile++ = REL_I; break;
    caseMAJOR(0x06) /* blez  */ *profile++ = PROFILE_JUMP; *profile++ = REL_I; break;
    caseMAJOR(0x07) /* bgtz  */ *profile++ = PROFILE_JUMP; *profile++ = REL_I; break;

    caseMAJOR(0x20) /* lb    */ *profile++ = PROFILE_READ +1; *profile++ = REGS[INS_S] + SIGNED16(INS_I); break;
    caseMAJOR(0x21) /* lh    */ *profile++ = PROFILE_READ +2; *profile++ = REGS[INS_S] + SIGNED16(INS_I); break;
    caseMAJOR(0x23) /* lw    */ *profile++ = PROFILE_READ +4; *profile++ = REGS[INS_S] + SIGNED16(INS_I); break;
    caseMAJOR(0x24) /* lbu   */ *profile++ = PROFILE_READ +1; *profile++ = REGS[INS_S] + SIGNED16(INS_I); break;
    caseMAJOR(0x25) /* lhu   */ *profile++ = PROFILE_READ +2; *profile++ = REGS[INS_S] + SIGNED16(INS_I); break;
    caseMAJOR(0x28) /* sb    */ *profile++ = PROFILE_WRITE+1; *profile++ = REGS[INS_S] + SIGNED16(INS_I); break;
    caseMAJOR(0x29) /* sh    */ *profile++ = PROFILE_WRITE+2; *profile++ = REGS[INS_S] + SIGNED16(INS_I); break;
    caseMAJOR(0x2B) /* sw    */ *profile++ = PROFILE_WRITE+4; *profile++ = REGS[INS_S] + SIGNED16(INS_I); break;

    // these could be done better, but this is good enough for now
    caseMAJOR(0x22) /* lwl   */ *profile++ = PROFILE_READ +4; *profile++ = REGS[INS_S] + SIGNED16(INS_I); break;
    caseMAJOR(0x26) /* lwr   */ *profile++ = PROFILE_READ +4; *profile++ = REGS[INS_S] + SIGNED16(INS_I); break;
    caseMAJOR(0x2A) /* swl   */ *profile++ = PROFILE_WRITE+4; *profile++ = REGS[INS_S] + SIGNED16(INS_I); break;
    caseMAJOR(0x2E) /* swr   */ *profile++ = PROFILE_WRITE+4; *profile++ = REGS[INS_S] + SIGNED16(INS_I); break;
    }
  }
  *profile = 0;
}

/////////////////////////////////////////////////////////////////////////////
