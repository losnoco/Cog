/////////////////////////////////////////////////////////////////////////////
//
// ioptimer - IOP timers
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "ioptimer.h"

/////////////////////////////////////////////////////////////////////////////
//
// Static information
//
sint32 EMU_CALL ioptimer_init(void) { return 0; }

#define COUNTERS (6)

#define IOP_INT_VBLANK  (1<<0)
#define IOP_INT_RTC0    (1<<4)
#define IOP_INT_RTC1    (1<<5)
#define IOP_INT_RTC2    (1<<6)
#define IOP_INT_RTC3    (1<<14)
#define IOP_INT_RTC4    (1<<15)
#define IOP_INT_RTC5    (1<<16)

static const uint32 intrflag[COUNTERS] = {
  IOP_INT_RTC0, IOP_INT_RTC1, IOP_INT_RTC2,
  IOP_INT_RTC3, IOP_INT_RTC4, IOP_INT_RTC5
};

/////////////////////////////////////////////////////////////////////////////
//
// State information
//
#define IOPTIMERSTATE ((struct IOPTIMER_STATE*)(state))

struct IOPTIMER_COUNTER {
  //
  // quick values used in advance loop, etc.
  //
  uint64 counter;
  uint32 delta;
  uint64 target;
  uint8 target_is_overflow;
  //
  // other values
  //
  uint16 mode;
  uint16 status;
  uint64 compare;
};

struct IOPTIMER_STATE {
  struct IOPTIMER_COUNTER counter[COUNTERS];
  uint8 gate;
  uint64 field_counter;
  uint64 field_vblank;
  uint64 field_total;
  uint32 hz_sysclock;
  uint32 hz_hline;
  uint32 hz_pixel;
};

uint32 EMU_CALL ioptimer_get_state_size(void) {
  return sizeof(struct IOPTIMER_STATE);
}

void EMU_CALL ioptimer_clear_state(void *state) {
  memset(IOPTIMERSTATE, 0, sizeof(struct IOPTIMER_STATE));
}

/////////////////////////////////////////////////////////////////////////////

void EMU_CALL ioptimer_set_rates(void *state, uint32 sysclock, uint32 dots, uint32 lines, uint32 lines_visible, uint32 refresh_rate) {
  IOPTIMERSTATE->hz_sysclock = sysclock;
  IOPTIMERSTATE->hz_hline = lines * refresh_rate;
  IOPTIMERSTATE->hz_pixel = IOPTIMERSTATE->hz_hline * dots;

  IOPTIMERSTATE->field_counter = 0;
  IOPTIMERSTATE->field_vblank = ((uint64)(lines_visible)) * ((uint64)(sysclock));
  IOPTIMERSTATE->field_total  = ((uint64)(lines        )) * ((uint64)(sysclock));
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE uint32 EMU_CALL cycles_until_gate(struct IOPTIMER_STATE *state) {
  uint64 diff;
  if(!(IOPTIMERSTATE->hz_hline)) return 0xFFFFFFFF;
  if(state->field_counter < state->field_vblank) {
    diff = state->field_vblank - state->field_counter;
  } else {
    diff = state->field_total - state->field_counter;
  }
  diff += (IOPTIMERSTATE->hz_hline-1);
  diff /= ((uint64)(IOPTIMERSTATE->hz_hline));
  if(diff > 0xFFFFFFFF) diff = 0xFFFFFFFF;
  if(diff < 1) diff = 1;
  return (uint32)diff;
}

/////////////////////////////////////////////////////////////////////////////

uint32 EMU_CALL ioptimer_cycles_until_interrupt(void *state) {
  uint32 min = cycles_until_gate(IOPTIMERSTATE);
  uint32 c;
  //
  // counters
  //
  for(c = 0; c < COUNTERS; c++) {
    uint64 diff;
    if(!(IOPTIMERSTATE->counter[c].delta)) continue;
    if(IOPTIMERSTATE->counter[c].counter >= IOPTIMERSTATE->counter[c].target) {
      diff = 0;
    } else {
      diff = IOPTIMERSTATE->counter[c].target - IOPTIMERSTATE->counter[c].counter;
      diff += (IOPTIMERSTATE->counter[c].delta-1);
      diff /= ((uint64)(IOPTIMERSTATE->counter[c].delta));
    }
    if(diff < ((uint64)(min))) min = (uint32)diff;
  }
  if(min < 1) min = 1;
  return min;
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE uint32 EMU_CALL counters_advance(struct IOPTIMER_STATE *state, uint32 cycles) {
  uint32 intr = 0;
  uint32 c;
  for(c = 0; c < COUNTERS; c++) {
    struct IOPTIMER_COUNTER *ctr = IOPTIMERSTATE->counter + c;;
    if(!ctr->delta) continue;
    ctr->counter += ((uint64)(cycles)) * ((uint64)(ctr->delta));
    //
    // timer loop handling
    //
    for(;;) {
      //
      // if we're below the given target, then good - quit.
      //
      if(ctr->counter < ctr->target) break;
      //
      // otherwise, we have a transition to make.
      //
      if(ctr->target_is_overflow) {
        ctr->status |= 0x1000;
        if(ctr->mode & 0x20) intr |= intrflag[c];
        // counter always loops on overflow (duh!)
        ctr->counter -= ctr->target;
        // counter now becomes the compare target
        ctr->target = ((uint64)(state->hz_sysclock)) * ((uint64)(ctr->compare));
        ctr->target_is_overflow = 0;
      } else {
        ctr->status |= 0x800;
        if(ctr->mode & 0x10) intr |= intrflag[c];
        // counter only loops on target if the appropriate bit is set
        if(ctr->mode & 8) {
          // no change to target, just loop counter
          ctr->counter -= ctr->target;
        // no target loop - proceed to overflow
        } else {
          if(c < 3) {
            ctr->target = ((uint64)(state->hz_sysclock)) << 16;
          } else {
            ctr->target = ((uint64)(state->hz_sysclock)) << 32;
          }
          ctr->target_is_overflow = 1;
        }
      }
    }
  }
  return intr;
}

/////////////////////////////////////////////////////////////////////////////

static void EMU_CALL counter_start(struct IOPTIMER_STATE *state, uint32 c) {
  struct IOPTIMER_COUNTER *ctr = state->counter + c;
  uint32 delta = state->hz_sysclock;
  switch(c) {
  case 0: if(ctr->mode & 0x100) { delta = state->hz_pixel; } break;
  case 1: if(ctr->mode & 0x100) { delta = state->hz_hline; } break;
  case 2: if(ctr->mode & 0x200) { delta /= 8; } break;
  case 3: if(ctr->mode & 0x100) { delta = state->hz_hline; } break;
  case 4: case 5:
    switch((ctr->mode >> 13) & 3) {
    case 0: delta /= 1; break;
    case 1: delta /= 8; break;
    case 2: delta /= 16; break;
    case 3: delta /= 256; break;
    }
    break;
  }
  ctr->counter = 0;
  ctr->delta = delta;
  ctr->target = ((uint64)(ctr->compare)) * ((uint64)(state->hz_sysclock));
  ctr->target_is_overflow = 0;
}

/////////////////////////////////////////////////////////////////////////////

static void EMU_CALL counter_stop(struct IOPTIMER_STATE *state, uint32 c) {
  state->counter[c].delta = 0;
}

/////////////////////////////////////////////////////////////////////////////

static void EMU_CALL gate_transition(struct IOPTIMER_STATE *state) {
  uint32 c;
  for(c = 0; c < COUNTERS; c++) {
    // must be both enabled and gate-enabled
    if((state->counter[c].mode & 0x41) != 0x41) continue;
    switch(state->counter[c].mode & 0x6) {
    case 0x0: // TM_GATE_ON_Count
      if(state->gate) { counter_start(state, c); }
      else            { counter_stop(state, c); }
      break;
    case 0x2: // TM_GATE_ON_ClearStart
      if(state->gate) { counter_start(state, c); }
      break;
    case 0x4: // TM_GATE_ON_Clear_OFF_Start
      if(state->gate) { counter_stop(state, c); }
      else            { counter_start(state, c); }
      break;
    case 0x6: // TM_GATE_ON_Start
      if(state->gate) {
        // one-time start: disable gate bit
        state->counter[c].mode &= ~1;
        counter_start(state, c);
      }
      break;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE uint32 EMU_CALL gate_advance(struct IOPTIMER_STATE *state, uint32 cycles) {
  uint32 intr = 0;
  state->field_counter += ((uint64)(cycles)) * ((uint64)(state->hz_hline));
  //
  // gate overflow loop
  //
  for(;;) {
    //
    // if we're below the given target, then good - quit.
    //
    if(state->gate) {
      if(state->field_counter < state->field_vblank) break;
      //
      // gate transition 1->0
      //
      state->gate = 0;
      gate_transition(state);
      intr |= IOP_INT_VBLANK;
    } else {
      if(state->field_counter < state->field_total) break;
      //
      // gate transition 0->1
      //
      state->gate = 1;
      gate_transition(state);
      state->field_counter -= state->field_total;
    }
  }
  return intr;
}

/////////////////////////////////////////////////////////////////////////////

uint32 EMU_CALL ioptimer_advance(void *state, uint32 cycles) {
  uint32 intr = 0;
  uint32 cycles_left = cycles;
  while(cycles_left) {
    uint32 g = cycles_until_gate(IOPTIMERSTATE);
    if(g > cycles_left) g = cycles_left;
    intr |= counters_advance(IOPTIMERSTATE, g);
    intr |= gate_advance(IOPTIMERSTATE, g);
    cycles_left -= g;
  }
  return intr;
}

/////////////////////////////////////////////////////////////////////////////

static uint32 EMU_CALL which_counter(uint32 a) {
  switch(a & 0xFFF0) {
  case 0x1100: return 0;
  case 0x1110: return 1;
  case 0x1120: return 2;
  case 0x1480: return 3;
  case 0x1490: return 4;
  case 0x14A0: return 5;
  }
  return 0xFFFFFFFF;
}

/////////////////////////////////////////////////////////////////////////////

uint32 EMU_CALL ioptimer_lw(void *state, uint32 a, uint32 mask) {
  uint32 d = 0;
  uint32 c = which_counter(a);
  struct IOPTIMER_COUNTER *ctr;
  if(c >= COUNTERS) return 0;
  ctr = IOPTIMERSTATE->counter + c;
  switch(a & 0xC) {
  case 0x0:
    if(ctr->delta) { d = (uint32)((ctr->counter) / ((uint64)(ctr->delta))); }
    break;
  case 0x4:
    d = ctr->status;
    ctr->status = 0;
    break;
  case 0x8:
    d = (uint32)(ctr->compare);
    break;
  }
  if(c < 3) d &= 0xFFFF;
  return d & mask;
}

/////////////////////////////////////////////////////////////////////////////

void EMU_CALL ioptimer_sw(void *state, uint32 a, uint32 d, uint32 mask) {
  uint32 c = which_counter(a);
  struct IOPTIMER_COUNTER *ctr;
  if(c >= COUNTERS) return;
  ctr = IOPTIMERSTATE->counter + c;
  d &= mask;
  if(c < 3) d &= 0xFFFF;
  switch(a & 0xC) {
  case 0x4:
    ctr->delta = 0;
    ctr->mode = d;
    if(d & 0x40) {
      if((d & 7) != 7) {
        counter_start(state, c);
      }
    }
    break;
  case 0x8:
    ctr->compare = d;
    if(!ctr->compare) {
      if(c < 3) {
        ctr->compare = 0x10000;
      } else {
        ctr->compare = 0x100000000;
      }
    }
    //
    // if this timer was running, recompute the target
    //
    if(ctr->delta) {
      ctr->target = ctr->compare * ((uint64)(IOPTIMERSTATE->hz_sysclock));
      ctr->target_is_overflow = 0;
      if(ctr->counter >= ctr->target) {
        if(c < 3) {
          ctr->target = ((uint64)(IOPTIMERSTATE->hz_sysclock)) << 16;
        } else {
          ctr->target = ((uint64)(IOPTIMERSTATE->hz_sysclock)) << 32;
        }
        ctr->target_is_overflow = 1;
      }
    }
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////
