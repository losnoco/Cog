/////////////////////////////////////////////////////////////////////////////
//
// qsound - QSound emulation
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "qsound.h"

#include "z80.h"
#include "kabuki.h"
#include "qmix.h"

/////////////////////////////////////////////////////////////////////////////
//
// Static information
//
static uint8 safe_rom_area[4] = {0,0,0,0};

/////////////////////////////////////////////////////////////////////////////
//
// Static init for the whole library
//
static uint8 library_was_initialized = 0;

//
// Deliberately create a NULL dereference
// Useful for calling attention to show-stopper problems like forgetting to
// call qsound_init() or compiling with the wrong byte order
//
static void qsound_hang(const char *message) {
  for(;;) { *((volatile char*)0) = *message; }
}

//
// Endian check
//
static void qsound_endian_check(void) {
  uint32 num = 0x61626364;
  // Big
  if(!memcmp(&num, "abcd", 4)) {
#ifdef EMU_BIG_ENDIAN
    return;
#endif
    qsound_hang("endian check");
  }
  // Little
  if(!memcmp(&num, "dcba", 4)) {
#ifndef EMU_BIG_ENDIAN
    return;
#endif
    qsound_hang("endian check");
  }
  // Don't know what!
  qsound_hang("endian check");
}

//
// Data type size check
//
static void qsound_size_check(void) {
  if(sizeof(uint8 ) != 1) qsound_hang("size check");
  if(sizeof(uint16) != 2) qsound_hang("size check");
  if(sizeof(uint32) != 4) qsound_hang("size check");
  if(sizeof(uint64) != 8) qsound_hang("size check");
  if(sizeof(sint8 ) != 1) qsound_hang("size check");
  if(sizeof(sint16) != 2) qsound_hang("size check");
  if(sizeof(sint32) != 4) qsound_hang("size check");
  if(sizeof(sint64) != 8) qsound_hang("size check");
}

sint32 EMU_CALL qsound_init(void) {
  sint32 r;
  qsound_endian_check();
  qsound_size_check();
  r = z80_init(); if(r) return r;
  r = qmix_init(); if(r) return r;
  library_was_initialized = 1;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Version information
//
const char* EMU_CALL qsound_getversion(void) {
  static const char s[] = "QCore0001a (built " __DATE__ ")";
  return s;
}

/////////////////////////////////////////////////////////////////////////////
//
// State information
//
struct QSOUND_STATE {
  struct Z80_MEMORY_MAP *map_op;
  struct Z80_MEMORY_MAP *map_read;
  struct Z80_MEMORY_MAP *map_write;
  void *z80_state;
  void *qmix_state;
  sint16 *sound_buffer; // TEMPORARY pointer. safe.
  uint32  sound_buffer_samples_free;
  uint32  sound_cycles_pending;
  uint16 qsound_data_latch;
  uint8 fatalflag;
  uint8 dummy0;
  uint32 bank_ofs;
  uint32 cycles_until_irq;
  uint32 cycles_per_irq;
  uint32 cycles_per_sample;
  uint64 odometer;
  uint8 ramC[0x1000];
  uint8 ramF[0x1000];
  uint8 *z80_rom;
  uint32 z80_rom_size;
  uint32 kabuki_swap_key1;
  uint32 kabuki_swap_key2;
  uint16 kabuki_addr_key;
  uint8 kabuki_xor_key;
  uint8 kabuki_op  [0x8000];
  uint8 kabuki_data[0x8000];
};

#define QSOUNDSTATE   ((struct QSOUND_STATE*)(state))
#define Z80STATE      (QSOUNDSTATE->z80_state)
#define QMIXSTATE     (QSOUNDSTATE->qmix_state)

extern const uint32 qsound_map_op_entries;
extern const uint32 qsound_map_read_entries;
extern const uint32 qsound_map_write_entries;

/*
void EMU_CALL qsound_write_ram_C(void *state, uint16 addr, uint8 data) {
  QSOUNDSTATE->ramC[addr & 0xFFF] = data;
}
void EMU_CALL qsound_write_ram_F(void *state, uint16 addr, uint8 data) {
  QSOUNDSTATE->ramF[addr & 0xFFF] = data;
}
*/

uint32 EMU_CALL qsound_get_state_size(void) {
  uint32 size = 0;
  size += sizeof(struct QSOUND_STATE);
  size += sizeof(struct Z80_MEMORY_MAP) * qsound_map_op_entries;
  size += sizeof(struct Z80_MEMORY_MAP) * qsound_map_read_entries;
  size += sizeof(struct Z80_MEMORY_MAP) * qsound_map_write_entries;
  size += z80_get_state_size();
  size += qmix_get_state_size();
  return size;
}

static void EMU_CALL recompute_memory_maps(struct QSOUND_STATE *state);
static void EMU_CALL qsound_advance(void *state, uint32 elapse);

static struct Z80_MEMORY_MAP qsound_map_in [1];
static struct Z80_MEMORY_MAP qsound_map_out[1];

void EMU_CALL qsound_set_rates(
  void *state,
  uint32 z80,
  uint32 irq,
  uint32 sample
) {
  QSOUNDSTATE->cycles_per_irq    = (z80 + (irq   /2)) / irq   ;
  QSOUNDSTATE->cycles_per_sample = (z80 + (sample/2)) / sample;
  qmix_set_sample_rate(QMIXSTATE, sample);
}

void EMU_CALL qsound_clear_state(void *state) {
  uint32 offset;
  // Clear local struct
  memset(state, 0, sizeof(struct QSOUND_STATE));
  // Set up sub-pointers
  offset = sizeof(struct QSOUND_STATE);
  QSOUNDSTATE->map_op     = (void*)(((char*)state)+offset); offset += sizeof(struct Z80_MEMORY_MAP) * qsound_map_op_entries;
  QSOUNDSTATE->map_read   = (void*)(((char*)state)+offset); offset += sizeof(struct Z80_MEMORY_MAP) * qsound_map_read_entries;
  QSOUNDSTATE->map_write  = (void*)(((char*)state)+offset); offset += sizeof(struct Z80_MEMORY_MAP) * qsound_map_write_entries;
  QSOUNDSTATE->z80_state  = (void*)(((char*)state)+offset); offset += z80_get_state_size();
  QSOUNDSTATE->qmix_state = (void*)(((char*)state)+offset); offset += qmix_get_state_size();
  //
  // Set other variables
  //
  QSOUNDSTATE->bank_ofs = 0x8000;
  //
  // Take care of substructures: memory maps, Z80 state, mixer state
  //
  recompute_memory_maps(QSOUNDSTATE);

  z80_clear_state(Z80STATE);
  z80_set_advance_callback(Z80STATE, qsound_advance, QSOUNDSTATE);
  z80_set_memory_maps(
    Z80STATE,
    QSOUNDSTATE->map_op,
    QSOUNDSTATE->map_read,
    QSOUNDSTATE->map_write,
    qsound_map_in,
    qsound_map_out
  );

  qmix_clear_state(QMIXSTATE);

  // Set rates
  qsound_set_rates(QSOUNDSTATE, 8000000, 250, 44100);
  QSOUNDSTATE->cycles_until_irq = QSOUNDSTATE->cycles_per_irq;

  // Done
}

/////////////////////////////////////////////////////////////////////////////
//
// Obtain substates
//
void* EMU_CALL qsound_get_r3000_state(void *state) { return Z80STATE;  }
void* EMU_CALL qsound_get_qmix_state (void *state) { return QMIXSTATE; }

/////////////////////////////////////////////////////////////////////////////

uint16 EMU_CALL qsound_getpc(void *state) { return z80_getpc(Z80STATE); }

/////////////////////////////////////////////////////////////////////////////
//
// Signal an interrupt
//
static void EMU_CALL irq_signal(struct QSOUND_STATE *state) {
  z80_setirq(Z80STATE, 1, 0x00);
}

/////////////////////////////////////////////////////////////////////////////
//
// Bank switching
//
static void set_memory_map_rom_area(
  struct Z80_MEMORY_MAP *map,
  uint8 *rom,
  sint32 rom_size
) {
  if(rom_size < 1) { rom = safe_rom_area; rom_size = 4; }
  if(rom_size > ((map->type.mask)+1)) rom_size = ((map->type.mask)+1);
  map->y = map->x + (rom_size-1);
  map->type.p = rom;
}

/*
static void recompute_fixed_rom_areas(struct QSOUND_STATE *state) {
  set_memory_map_rom_area(
    state->map_op,
    state->op_rom,
    state->op_rom_size
  );
  set_memory_map_rom_area(
    state->map_read,
    state->data_rom,
    state->data_rom_size
  );
}
*/

static void recompute_banked_rom_areas(struct QSOUND_STATE *state) {
  set_memory_map_rom_area(
    state->map_op + 1,
    state->z80_rom      + state->bank_ofs,
    state->z80_rom_size - state->bank_ofs
  );
  set_memory_map_rom_area(
    state->map_read + 1,
    state->z80_rom      + state->bank_ofs,
    state->z80_rom_size - state->bank_ofs
  );
}

static void EMU_CALL qsound_banksw_w(void *state, uint16 a, uint8 d) {
  QSOUNDSTATE->bank_ofs = 0x8000 + 0x4000 * ((uint32)(d&0xF));
  recompute_banked_rom_areas(QSOUNDSTATE);
  // just in case we're executing from the banked area
  // (for CPS1/2 this is exquisitely unlikely)
  z80_break(Z80STATE);
}

/////////////////////////////////////////////////////////////////////////////
//
// QMIX: Forward read/write/advance calls to the QSound mixer
//

static void EMU_CALL flush_sound(struct QSOUND_STATE *state) {
  uint32 samples_needed = state->sound_cycles_pending / (state->cycles_per_sample);
  if(samples_needed > state->sound_buffer_samples_free) {
    samples_needed = state->sound_buffer_samples_free;
  }
  if(!samples_needed) return;

  qmix_render(QMIXSTATE, state->sound_buffer, samples_needed);

  if(state->sound_buffer) state->sound_buffer += 2 * samples_needed;
  state->sound_buffer_samples_free -= samples_needed;
  state->sound_cycles_pending -= (state->cycles_per_sample) * samples_needed;
}

//
// Register reads/writes
//
static uint8 EMU_CALL qsound_status_r(void *state, uint16 a) {
  return 0x80;
}

static void EMU_CALL qsound_data_h_w(void *state, uint16 a, uint8 d) {
  QSOUNDSTATE->qsound_data_latch =
    (QSOUNDSTATE->qsound_data_latch & 0x00FF) |
    (((uint32)d) << 8);
}

static void EMU_CALL qsound_data_l_w(void *state, uint16 a, uint8 d) {
  QSOUNDSTATE->qsound_data_latch =
    (QSOUNDSTATE->qsound_data_latch & 0xFF00) |
    ((uint32)d);
}

static void EMU_CALL qsound_cmd_w(void *state, uint16 a, uint8 d) {
  flush_sound(QSOUNDSTATE);
  qmix_command(QMIXSTATE, d, QSOUNDSTATE->qsound_data_latch);
}

/////////////////////////////////////////////////////////////////////////////
//
// Invalid-address catchers
//
static uint8 EMU_CALL catcher_read(void *state, uint16 a) { return 0; }
static void EMU_CALL catcher_write(void *state, uint16 a, uint8 d) { }

/////////////////////////////////////////////////////////////////////////////
//
// Advance hardware activity by the given cycle count
//
static void EMU_CALL qsound_advance(void *state, uint32 elapse) {
  if(!elapse) return;
  //
  // Check timers
  //
  if(QSOUNDSTATE->cycles_until_irq <= elapse) {
    QSOUNDSTATE->cycles_until_irq += QSOUNDSTATE->cycles_per_irq;
    irq_signal(QSOUNDSTATE);
  }
  QSOUNDSTATE->cycles_until_irq -= elapse;
  //
  // Update pending sound cycles
  //
  QSOUNDSTATE->sound_cycles_pending += elapse;
  //
  // Update odometer
  //
  QSOUNDSTATE->odometer += ((uint64)elapse);
}

/////////////////////////////////////////////////////////////////////////////
//
// Determine how many cycles until the next interrupt
//
// This is then used as an upper bound for how many cycles can be executed
// before checking for futher interrupts
//
static uint32 EMU_CALL cycles_until_next_interrupt(
  struct QSOUND_STATE *state
) {
  uint32 cyc, min = 0xFFFFFFFF;
  //
  // Timers
  //
  cyc = state->cycles_until_irq;
  if(cyc < min) min = cyc;

  if(min < 1) min = 1;
  return min;
}

/////////////////////////////////////////////////////////////////////////////
//
// Memory map structures
//
#define STATEOFS(thetype,thefield) ((void*)(&(((struct thetype*)0)->thefield)))

static struct Z80_MEMORY_MAP qsound_map_op[] = {
  { 0x0000, 0x7FFF, { 0x7FFF, Z80_MAP_TYPE_POINTER , STATEOFS(QSOUND_STATE,kabuki_op)   } },
  { 0x8000, 0xBFFF, { 0x3FFF, Z80_MAP_TYPE_POINTER , NULL                               } },
  { 0xC000, 0xCFFF, { 0x0FFF, Z80_MAP_TYPE_POINTER , STATEOFS(QSOUND_STATE,ramC)        } },
  { 0xF000, 0xFFFF, { 0x0FFF, Z80_MAP_TYPE_POINTER , STATEOFS(QSOUND_STATE,ramF)        } },
  { 0x0000, 0xFFFF, { 0xFFFF, Z80_MAP_TYPE_CALLBACK, catcher_read                       } }
};

static struct Z80_MEMORY_MAP qsound_map_read[] = {
  { 0x0000, 0x7FFF, { 0x7FFF, Z80_MAP_TYPE_POINTER , STATEOFS(QSOUND_STATE,kabuki_data) } },
  { 0x8000, 0xBFFF, { 0x3FFF, Z80_MAP_TYPE_POINTER , NULL                               } },
  { 0xC000, 0xCFFF, { 0x0FFF, Z80_MAP_TYPE_POINTER , STATEOFS(QSOUND_STATE,ramC)        } },
  { 0xF000, 0xFFFF, { 0x0FFF, Z80_MAP_TYPE_POINTER , STATEOFS(QSOUND_STATE,ramF)        } },
  { 0xD007, 0xD007, { 0x0000, Z80_MAP_TYPE_CALLBACK, qsound_status_r                    } },
  { 0x0000, 0xFFFF, { 0xFFFF, Z80_MAP_TYPE_CALLBACK, catcher_read                       } }
};

static struct Z80_MEMORY_MAP qsound_map_write[] = {
  { 0xC000, 0xCFFF, { 0x0FFF, Z80_MAP_TYPE_POINTER , STATEOFS(QSOUND_STATE,ramC)        } },
  { 0xF000, 0xFFFF, { 0x0FFF, Z80_MAP_TYPE_POINTER , STATEOFS(QSOUND_STATE,ramF)        } },
  { 0xD000, 0xD000, { 0x0000, Z80_MAP_TYPE_CALLBACK, qsound_data_h_w                    } },
  { 0xD001, 0xD001, { 0x0000, Z80_MAP_TYPE_CALLBACK, qsound_data_l_w                    } },
  { 0xD002, 0xD002, { 0x0000, Z80_MAP_TYPE_CALLBACK, qsound_cmd_w                       } },
  { 0xD003, 0xD003, { 0x0000, Z80_MAP_TYPE_CALLBACK, qsound_banksw_w                    } },
  { 0x0000, 0xFFFF, { 0xFFFF, Z80_MAP_TYPE_CALLBACK, catcher_write                      } }
};

static struct Z80_MEMORY_MAP qsound_map_in [1] = {
  { 0x0000, 0xFFFF, { 0xFFFF, Z80_MAP_TYPE_CALLBACK, catcher_read } }
};

static struct Z80_MEMORY_MAP qsound_map_out[1] = {
  { 0x0000, 0xFFFF, { 0xFFFF, Z80_MAP_TYPE_CALLBACK, catcher_write } }
};

#define QSOUND_ARRAY_ENTRIES(x) (sizeof(x)/sizeof((x)[0]))

const uint32 qsound_map_op_entries    = QSOUND_ARRAY_ENTRIES(qsound_map_op   );
const uint32 qsound_map_read_entries  = QSOUND_ARRAY_ENTRIES(qsound_map_read );
const uint32 qsound_map_write_entries = QSOUND_ARRAY_ENTRIES(qsound_map_write);

/////////////////////////////////////////////////////////////////////////////
//
// Memory map recomputation
//
static void perform_state_offset(
  struct Z80_MEMORY_MAP *map,
  struct QSOUND_STATE *state
) {
  //
  // It's safe to typecast this "pointer" to a uint32 since, due to
  // the STATEOFS hack, it'll never be bigger than 4GB
  //
  uint32 o = (uint32)(map->type.p);
  map->type.p = ((uint8*)state) + o;
}

static void perform_nonnull_state_offsets(
  struct QSOUND_STATE *state,
  struct Z80_MEMORY_MAP *map,
  uint32 entries
) {
  uint32 i;
  for(i = 0; i < entries; i++) {
    if(
      (map[i].type.n == Z80_MAP_TYPE_POINTER) &&
      (map[i].type.p)
    ) {
      perform_state_offset(map + i, state);
    }
  }
}

//
// Recompute the memory maps.
// PERFORMS NO REGISTRATION with the actual R3000 state.
//
static void recompute_memory_maps(struct QSOUND_STATE *state) {
  //
  // First, just copy from the static tables
  //
  memcpy(state->map_op   , qsound_map_op   , sizeof(qsound_map_op   ));
  memcpy(state->map_read , qsound_map_read , sizeof(qsound_map_read ));
  memcpy(state->map_write, qsound_map_write, sizeof(qsound_map_write));
  //
  // Now perform state offsets on all non-NULL pointers
  //
  perform_nonnull_state_offsets(state, state->map_op   , qsound_map_op_entries   );
  perform_nonnull_state_offsets(state, state->map_read , qsound_map_read_entries );
  perform_nonnull_state_offsets(state, state->map_write, qsound_map_write_entries);
  //
  // Now take care of ROM areas
  //
//  recompute_fixed_rom_areas (state);
  recompute_banked_rom_areas(state);
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
// <  0   Unrecoverable error
//
sint32 EMU_CALL qsound_execute(
  void   *state,
  sint32  cycles,
  sint16 *sound_buf,
  uint32 *sound_samples
) {
  sint32 r = 0;
  sint64 cyc_upperbound;
  uint64 old_odometer;
  uint64 target_odometer;

  old_odometer = QSOUNDSTATE->odometer;

  QSOUNDSTATE->sound_buffer              =  sound_buf;
  QSOUNDSTATE->sound_buffer_samples_free = *sound_samples;
  //
  // If the fatal flag was set, return -2
  //
  if(QSOUNDSTATE->fatalflag) { return -1; }
  //
  // If we have a bogus cycle count, return error
  //
  if(cycles < 0) { return -1; }

  /*
  ** Begin by flushing any pending sound data into the newly available
  ** buffer, if necessary
  */
  flush_sound(QSOUNDSTATE);
  /*
  ** Compute an upper bound for the number of cycles that can be generated
  ** while still fitting in the buffer
  */
  cyc_upperbound =
    ((sint64)(QSOUNDSTATE->cycles_per_sample)) *
    ((sint64)(QSOUNDSTATE->sound_buffer_samples_free));
  if(cyc_upperbound > (QSOUNDSTATE->sound_cycles_pending)) {
    cyc_upperbound -= QSOUNDSTATE->sound_cycles_pending;
  } else {
    cyc_upperbound = 0;
  }
  /*
  ** Bound cyc_upperbound by the number of cycles requested
  */
  if(cycles > 0x70000000) cycles = 0x70000000;
  if(cyc_upperbound > cycles) cyc_upperbound = cycles;
  /*
  ** Now cyc_upperbound is the number of cycles to execute.
  ** Compute the target odometer
  */
  target_odometer = (QSOUNDSTATE->odometer)+((uint64)(cyc_upperbound));
  /*
  ** Execution loop
  */
  for(;;) {
    uint32 diff, ci;
    if(QSOUNDSTATE->odometer >= target_odometer) break;

    diff = (uint32)((target_odometer) - (QSOUNDSTATE->odometer));
    ci = cycles_until_next_interrupt(QSOUNDSTATE);
    if(diff > ci) diff = ci;
    r = z80_execute(Z80STATE, diff);
    // Create an error if necessary
    if(r < 0 || QSOUNDSTATE->fatalflag) { r = -1; break; }
  }
  //
  // End with a final sound flush
  //
  flush_sound(QSOUNDSTATE);
  /*
  ** Determine how many sound samples we generated
  */
  (*sound_samples) -= (QSOUNDSTATE->sound_buffer_samples_free);
  //
  // If there was an error, return it
  //
  if(r < 0) {
//char s[100];
//sprintf(s,"blah blah %d", r);
//MessageBox(NULL,s,NULL,MB_OK);
    return r;
  }
  //
  // Otherwise return the number of cycles we executed
  //
  r = QSOUNDSTATE->odometer - old_odometer;
  return r;
}

/////////////////////////////////////////////////////////////////////////////

uint64 EMU_CALL qsound_get_odometer(void *state) {
  return QSOUNDSTATE->odometer;
}

/////////////////////////////////////////////////////////////////////////////

static void recompute_kabuki_rom(struct QSOUND_STATE *state) {
  uint32 len = state->z80_rom_size;
  if(len > 0x8000) len = 0x8000;
  kabuki_decode(
    state->z80_rom,
    state->kabuki_op,
    state->kabuki_data,
    len,
    state->kabuki_swap_key1,
    state->kabuki_swap_key2,
    state->kabuki_addr_key,
    state->kabuki_xor_key
  );
}

/////////////////////////////////////////////////////////////////////////////

void EMU_CALL qsound_set_z80_rom(void *state, void *rom, uint32 size) {
  QSOUNDSTATE->z80_rom      = rom;
  QSOUNDSTATE->z80_rom_size = size;
  recompute_kabuki_rom(QSOUNDSTATE);
  recompute_memory_maps(QSOUNDSTATE);
}

void EMU_CALL qsound_set_kabuki_key(void *state,
  uint32 swap_key1,
  uint32 swap_key2,
  uint16 addr_key,
  uint8 xor_key
) {
  QSOUNDSTATE->kabuki_swap_key1 = swap_key1;
  QSOUNDSTATE->kabuki_swap_key2 = swap_key2;
  QSOUNDSTATE->kabuki_addr_key  = addr_key;
  QSOUNDSTATE->kabuki_xor_key   = xor_key;
  recompute_kabuki_rom(QSOUNDSTATE);
}

void EMU_CALL qsound_set_sample_rom(void *state, void *rom, uint32 size) {
  qmix_set_sample_rom(QMIXSTATE, rom, size);
}

/////////////////////////////////////////////////////////////////////////////
