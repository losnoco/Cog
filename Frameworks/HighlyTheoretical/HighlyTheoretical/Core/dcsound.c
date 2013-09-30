/////////////////////////////////////////////////////////////////////////////
//
// dcsound - Dreamcast sound system emulation
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "dcsound.h"

#include "arm.h"
#include "yam.h"

/////////////////////////////////////////////////////////////////////////////
//
// Static information
//
sint32 EMU_CALL dcsound_init(void) { return 0; }

#define CYCLES_PER_SAMPLE (128)

/////////////////////////////////////////////////////////////////////////////
//
// State information
//
struct DCSOUND_STATE {
  struct DCSOUND_STATE *myself; // Pointer used to check location invariance

  uint32 offset_to_map_load;
  uint32 offset_to_map_store;
  uint32 offset_to_arm;
  uint32 offset_to_yam;
  uint32 offset_to_ram;

  uint32 sound_samples_remaining;
  uint32 cycles_ahead_of_sound;
  sint32 cycles_executed;

//  uint64 timetotal[3];
//  uint64 timelast[3];
//  sint32 timecur;
};

#define DCSOUNDSTATE ((struct DCSOUND_STATE*)(state))
#define MAPLOAD     ((void*)(((char*)(DCSOUNDSTATE))+(DCSOUNDSTATE->offset_to_map_load)))
#define MAPSTORE    ((void*)(((char*)(DCSOUNDSTATE))+(DCSOUNDSTATE->offset_to_map_store)))
#define ARMSTATE    ((void*)(((char*)(DCSOUNDSTATE))+(DCSOUNDSTATE->offset_to_arm)))
#define YAMSTATE    ((void*)(((char*)(DCSOUNDSTATE))+(DCSOUNDSTATE->offset_to_yam)))
#define RAMBYTEPTR ((uint8*)(((char*)(DCSOUNDSTATE))+(DCSOUNDSTATE->offset_to_ram)))

extern const uint32 dcsound_map_load_entries;
extern const uint32 dcsound_map_store_entries;

uint32 EMU_CALL dcsound_get_state_size(void) {
  uint32 offset = 0;
  offset += sizeof(struct DCSOUND_STATE);
  offset += sizeof(struct ARM_MEMORY_MAP) * dcsound_map_load_entries;
  offset += sizeof(struct ARM_MEMORY_MAP) * dcsound_map_store_entries;
  offset += arm_get_state_size();
  offset += yam_get_state_size(2);
  offset += 0x800000;
  return offset;
}

static void recompute_memory_maps(struct DCSOUND_STATE *state);
static void EMU_CALL dcsound_advance(void *state, uint32 elapse);

void EMU_CALL dcsound_clear_state(void *state) {
  uint32 offset;

  // Clear local struct
  memset(state, 0, sizeof(struct DCSOUND_STATE));

  // Set up offsets
  offset = sizeof(struct DCSOUND_STATE);
  DCSOUNDSTATE->offset_to_map_load  = offset; offset += sizeof(struct ARM_MEMORY_MAP) * dcsound_map_load_entries;
  DCSOUNDSTATE->offset_to_map_store = offset; offset += sizeof(struct ARM_MEMORY_MAP) * dcsound_map_store_entries;
  DCSOUNDSTATE->offset_to_arm       = offset; offset += arm_get_state_size();
  DCSOUNDSTATE->offset_to_yam       = offset; offset += yam_get_state_size(2);
  DCSOUNDSTATE->offset_to_ram       = offset; offset += 0x800000;

  //
  // Take care of substructures
  //
  memset(RAMBYTEPTR, 0, 0x800000);

  recompute_memory_maps(DCSOUNDSTATE);

  arm_clear_state(ARMSTATE);
  arm_set_advance_callback(ARMSTATE, dcsound_advance, DCSOUNDSTATE);
  arm_set_memory_maps(ARMSTATE, MAPLOAD, MAPSTORE);

  yam_clear_state(YAMSTATE, 2);
  yam_setram(YAMSTATE, (uint32*)(RAMBYTEPTR), 0x800000, EMU_ENDIAN_XOR(3), EMU_ENDIAN_XOR(2));
  //
  // We want to initialize the Yamaha interrupt system here, because some
  // homebrew DC stuff expects it'll start at the BIOS-initialized values
  //
  yam_aica_store_reg(YAMSTATE, 0x289C, 0x0040, 0xFFFF, NULL);
  yam_aica_store_reg(YAMSTATE, 0x28A8, 0x0018, 0xFFFF, NULL);
  yam_aica_store_reg(YAMSTATE, 0x28AC, 0x0050, 0xFFFF, NULL);
  yam_aica_store_reg(YAMSTATE, 0x28B0, 0x0008, 0xFFFF, NULL);

  DCSOUNDSTATE->myself = DCSOUNDSTATE;
  // Done
}

/////////////////////////////////////////////////////////////////////////////
//
// Profiling
//
#define TIMEDCSOUND (0)
#define TIMEARM     (1)
#define TIMEYAM     (2)

#define timeenter(x,y)
#define timeleave(x)
#define timeswitch(x,y)

/*
static EMU_INLINE uint64 timerdtsc(void) {
  uint64 r;
  __asm {
    rdtsc
    mov dword ptr [r],eax
    mov dword ptr [r+4],edx
  }
  return r;
}

static EMU_INLINE void timeenter(struct DCSOUND_STATE *state, int n) {
  state->timecur = n; state->timelast[n] = timerdtsc();
}

static EMU_INLINE void timeleave(struct DCSOUND_STATE *state) {
  state->timetotal[state->timecur] += timerdtsc() - state->timelast[state->timecur];
}

static EMU_INLINE void timeswitch(struct DCSOUND_STATE *state, int n) {
  timeleave(state);
  timeenter(state, n);
}
*/

/////////////////////////////////////////////////////////////////////////////
//
// Check to see if this structure has moved, and if so, recompute
//
// This is currently ONLY done on dcsound_execute
//
static void location_check(struct DCSOUND_STATE *state) {
  if(state->myself != state) {
    recompute_memory_maps(state);
    arm_set_advance_callback(ARMSTATE, dcsound_advance, DCSOUNDSTATE);
    arm_set_memory_maps(ARMSTATE, MAPLOAD, MAPSTORE);
    yam_setram(YAMSTATE, (uint32*)(RAMBYTEPTR), 0x800000, EMU_ENDIAN_XOR(3), EMU_ENDIAN_XOR(2));
    state->myself = state;
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Obtain substates
//
void* EMU_CALL dcsound_get_arm_state(void *state) { return ARMSTATE; }
void* EMU_CALL dcsound_get_yam_state(void *state) { return YAMSTATE; }

/////////////////////////////////////////////////////////////////////////////
//
// Register loads/stores
// (CALLBACK)
//
static uint32 EMU_CALL dcsound_yam_lw(void *state, uint32 a, uint32 mask) {
  uint16 d;
  timeswitch(DCSOUNDSTATE, TIMEYAM);
  d = yam_aica_load_reg(YAMSTATE, a, mask) & mask;
  timeswitch(DCSOUNDSTATE, TIMEARM);
  return d;
}

static void EMU_CALL dcsound_yam_sw(void *state, uint32 a, uint32 d, uint32 mask) {
  uint8 b = 0;
  timeswitch(DCSOUNDSTATE, TIMEYAM);
  yam_aica_store_reg(YAMSTATE, a, d, mask, &b);
  timeswitch(DCSOUNDSTATE, TIMEARM);
  if(b) arm_break(ARMSTATE);
}

/////////////////////////////////////////////////////////////////////////////
//
// Sync Yamaha emulation with dcsound
//
static void sync_sound(struct DCSOUND_STATE *state) {
  if(state->cycles_ahead_of_sound >= CYCLES_PER_SAMPLE) {
    uint32 samples = (state->cycles_ahead_of_sound) / CYCLES_PER_SAMPLE;
    //
    // Avoid overflowing the number of samples remaining
    //
    if(samples > state->sound_samples_remaining) {
      samples = state->sound_samples_remaining;
    }
    if(samples > 0) {
      timeswitch(state, TIMEYAM);
      yam_advance(YAMSTATE, samples);
      timeswitch(state, TIMEDCSOUND);
      state->cycles_ahead_of_sound -= CYCLES_PER_SAMPLE * samples;
      state->sound_samples_remaining -= samples;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Advance hardware activity by the given cycle count
// (CALLBACK)
//
static void EMU_CALL dcsound_advance(void *state, uint32 elapse) {
  timeswitch(DCSOUNDSTATE, TIMEDCSOUND);
  //
  // Update cycles executed
  //
  DCSOUNDSTATE->cycles_executed += elapse;
  DCSOUNDSTATE->cycles_ahead_of_sound += elapse;
  //
  // Synchronize the sound part
  //
  sync_sound(DCSOUNDSTATE);

  timeswitch(DCSOUNDSTATE, TIMEARM);
}

/////////////////////////////////////////////////////////////////////////////
//
// Determine how many cycles until the next interrupt
//
// This is then used as an upper bound for how many cycles can be executed
// before checking for futher interrupts
//
static uint32 cycles_until_next_interrupt(
  struct DCSOUND_STATE *state
) {
  uint32 yamsamples;
  uint32 yamcycles;
  timeswitch(state, TIMEYAM);
  yamsamples = yam_get_min_samples_until_interrupt(YAMSTATE);
  timeswitch(state, TIMEDCSOUND);
  if(yamsamples > 0x10000) { yamsamples = 0x10000; }
  yamcycles = yamsamples * CYCLES_PER_SAMPLE;
  if(yamcycles <= state->cycles_ahead_of_sound) return 1;
  return yamcycles - state->cycles_ahead_of_sound;
}

/////////////////////////////////////////////////////////////////////////////
//
// Invalid-address catchers
// (CALLBACK)
//
static uint32 EMU_CALL catcher_lw(void *state, uint32 a, uint32 mask) { return 0; }
static void EMU_CALL catcher_sw(void *state, uint32 a, uint32 d, uint32 mask) { }

/////////////////////////////////////////////////////////////////////////////
//
// Static memory map structures
//

static const struct ARM_MEMORY_MAP dcsound_map_load[] = {
  { 0x00000000, 0x007FFFFF, { 0x007FFFFF, ARM_MAP_TYPE_POINTER , NULL } },
  { 0x00800000, 0x0080FFFF, { 0x0000FFFF, ARM_MAP_TYPE_CALLBACK, dcsound_yam_lw               } },
  { 0x00000000, 0xFFFFFFFF, { 0xFFFFFFFF, ARM_MAP_TYPE_CALLBACK, catcher_lw                   } }
};

static const struct ARM_MEMORY_MAP dcsound_map_store[] = {
  { 0x00000000, 0x007FFFFF, { 0x007FFFFF, ARM_MAP_TYPE_POINTER , NULL } },
  { 0x00800000, 0x0080FFFF, { 0x0000FFFF, ARM_MAP_TYPE_CALLBACK, dcsound_yam_sw               } },
  { 0x00000000, 0xFFFFFFFF, { 0xFFFFFFFF, ARM_MAP_TYPE_CALLBACK, catcher_sw                   } }
};

#define DCSOUND_ARRAY_ENTRIES(x) (sizeof(x)/sizeof((x)[0]))

const uint32 dcsound_map_load_entries  = DCSOUND_ARRAY_ENTRIES(dcsound_map_load );
const uint32 dcsound_map_store_entries = DCSOUND_ARRAY_ENTRIES(dcsound_map_store);

////////////////////////////////////////////////////////////////////////////////
//
// Memory map recomputation
//
// Necessary on structure location change or audit change
//

//
// Recompute the memory maps.
// PERFORMS NO REGISTRATION with the actual ARM state.
//
static void recompute_memory_maps(struct DCSOUND_STATE *state) {
  struct ARM_MEMORY_MAP *mapload  = MAPLOAD;
  struct ARM_MEMORY_MAP *mapstore = MAPSTORE;
  //
  // First, just copy from the static tables
  //
  memcpy(mapload , dcsound_map_load , sizeof(dcsound_map_load ));
  memcpy(mapstore, dcsound_map_store, sizeof(dcsound_map_store));
  //
  // Now perform state offsets on first entry in each map
  //
  mapload [0].type.p = RAMBYTEPTR;
  mapstore[0].type.p = RAMBYTEPTR;
}

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
) {
  sint32 error = 0;
  uint8 *yamintptr;
  //
  // If we have a bogus cycle count, return error
  //
  if(cycles < 0) { return -1; }
  //
  // Begin profiling
  //
  timeenter(DCSOUNDSTATE, TIMEDCSOUND);
  //
  // Ensure location invariance
  //
  location_check(DCSOUNDSTATE);
  //
  // Cap to sane values to avoid overflow problems
  //
  if(cycles > 0x1000000) { cycles = 0x1000000; }
  if((*sound_samples) > 0x10000) { (*sound_samples) = 0x10000; }
  //
  // Set up the buffer
  //
  timeswitch(DCSOUNDSTATE, TIMEYAM);
  yam_beginbuffer(YAMSTATE, sound_buf);
  timeswitch(DCSOUNDSTATE, TIMEDCSOUND);
  DCSOUNDSTATE->sound_samples_remaining = *sound_samples;
  //
  // Get the interrupt pending pointer
  //
  timeswitch(DCSOUNDSTATE, TIMEYAM);
  yamintptr = yam_get_interrupt_pending_ptr(YAMSTATE);
  timeswitch(DCSOUNDSTATE, TIMEDCSOUND);
  //
  // Zero out these counters
  //
  DCSOUNDSTATE->cycles_executed = 0;
  //
  // Sync any pending samples from last time
  //
  sync_sound(DCSOUNDSTATE);
  //
  // Cap cycles depending on how many samples we have left to generate
  //
  { sint32 cap = CYCLES_PER_SAMPLE * DCSOUNDSTATE->sound_samples_remaining;
    cap -= DCSOUNDSTATE->cycles_ahead_of_sound;
    if(cap < 0) cap = 0;
    if(cycles > cap) cycles = cap;
  }
  //
  // Execution loop
  //
  while(DCSOUNDSTATE->cycles_executed < cycles) {
    sint32 r;
    uint32 remain = cycles - DCSOUNDSTATE->cycles_executed;
    uint32 ci = cycles_until_next_interrupt(DCSOUNDSTATE);
    if(remain > ci) { remain = ci; }
    if(remain > 0x1000000) { remain = 0x1000000; }
    timeswitch(DCSOUNDSTATE, TIMEARM);
    r = arm_execute(ARMSTATE, remain, (*yamintptr) != 0);
    timeswitch(DCSOUNDSTATE, TIMEDCSOUND);
    if(r < 0) { error = -1; break; }
  }
  //
  // Flush out actual sound rendering
  //
  timeswitch(DCSOUNDSTATE, TIMEYAM);
  yam_flush(YAMSTATE);
  timeswitch(DCSOUNDSTATE, TIMEDCSOUND);
  //
  // Adjust outgoing sample count
  //
  (*sound_samples) -= DCSOUNDSTATE->sound_samples_remaining;
  //
  // End profiling
  //
  timeleave(DCSOUNDSTATE);
  //
  // Done
  //
  if(error) return error;
  return DCSOUNDSTATE->cycles_executed;
}

/////////////////////////////////////////////////////////////////////////////
//
// Get / set memory words with no side effects
//
uint32 EMU_CALL dcsound_getword(void *state, uint32 a) {
  return *((uint32*)(RAMBYTEPTR+(a&0x7FFFFC)));
}

void EMU_CALL dcsound_setword(void *state, uint32 a, uint32 d) {
  *((uint32*)(RAMBYTEPTR+(a&0x7FFFFC))) = d;
}

/////////////////////////////////////////////////////////////////////////////
//
// Upload data to RAM, no side effects
//
void EMU_CALL dcsound_upload_to_ram(
  void *state,
  uint32 address,
  void *src,
  uint32 len
) {
  uint32 i;
  for(i = 0; i < len; i++) {
    (RAMBYTEPTR)[((address+i)^(EMU_ENDIAN_XOR(3)))&0x7FFFFF] =
      ((uint8*)src)[i];
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Get the current program counter
//
uint32 EMU_CALL dcsound_get_pc(void *state) {
  return arm_getreg(ARMSTATE, ARM_REG_GEN+15);
}

/////////////////////////////////////////////////////////////////////////////
