/////////////////////////////////////////////////////////////////////////////
//
// sega - Top-level emulation for Saturn and DC
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "sega.h"

#include "satsound.h"
#include "dcsound.h"
#include "arm.h"
#include "yam.h"
#ifdef USE_STARSCREAM
#include "Starscream/starcpu.h"
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Static init for the whole library
//
static uint8 library_was_initialized = 0;

//
// Deliberately create a NULL dereference
// Useful for calling attention to show-stopper problems like forgetting to
// call sega_init() or compiling with the wrong byte order
//
static void sega_hang(const char *message) {
  for(;;) { *((volatile char*)0) = *message; }
}

//
// Endian check
//
static void sega_endian_check(void) {
  uint32 num = 0x61626364;
  // Big
  if(!memcmp(&num, "abcd", 4)) {
#ifdef SEGA_BIG_ENDIAN
    return;
#endif
    sega_hang("endian check");
  }
  // Little
  if(!memcmp(&num, "dcba", 4)) {
#ifndef SEGA_BIG_ENDIAN
    return;
#endif
    sega_hang("endian check");
  }
  // Don't know what!
  sega_hang("endian check");
}

//
// Data type size check
//
static void sega_size_check(void) {
  if(sizeof(uint8 ) != 1) sega_hang("size check");
  if(sizeof(uint16) != 2) sega_hang("size check");
  if(sizeof(uint32) != 4) sega_hang("size check");
  if(sizeof(uint64) != 8) sega_hang("size check");
  if(sizeof(sint8 ) != 1) sega_hang("size check");
  if(sizeof(sint16) != 2) sega_hang("size check");
  if(sizeof(sint32) != 4) sega_hang("size check");
  if(sizeof(sint64) != 8) sega_hang("size check");
}

sint32 EMU_CALL sega_init(void) {
  sint32 r;
  sega_endian_check();
  sega_size_check();

  if(library_was_initialized) return 0;

#ifndef DISABLE_SSF
  r = satsound_init(); if(r) return r;
#endif
  r = dcsound_init(); if(r) return r;
  r = arm_init(); if(r) return r;
  r = yam_init(); if(r) return r;
#ifndef DISABLE_SSF
#ifdef USE_STARSCREAM
  r = s68000_init(); if(r) return r;
#endif
#endif

  library_was_initialized = 1;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Version information
//
const char* EMU_CALL sega_getversion(void) {
  static const char s[] = "SegaCore0001 (built " __DATE__ ")";
  static char rv[500];
  int sl = (int)strlen(s);
  memcpy(rv, s, sl);
#ifndef DISABLE_SSF
  rv[sl] = '\n';
#ifdef USE_STARSCREAM
  strcpy(rv+sl+1, s68000_get_version());
#elif defined(USE_M68K)
  strcpy(rv+sl+1, "M68K");
#else
  strcpy(rv+sl+1, "C68K");
#endif
#endif

  return rv;
}

/////////////////////////////////////////////////////////////////////////////
//
// State information
//

struct SEGA_STATE {
  uint32 offset_to_dcsound;
  uint32 offset_to_satsound;
};

#define SEGASTATE     ((struct SEGA_STATE*)(state))
#define DCSOUNDSTATE  ((void*)(((char*)(state))+(SEGASTATE->offset_to_dcsound)))
#define SATSOUNDSTATE ((void*)(((char*)(state))+(SEGASTATE->offset_to_satsound)))

#define HAVE_DCSOUND  (SEGASTATE->offset_to_dcsound!=0)
#define HAVE_SATSOUND (SEGASTATE->offset_to_satsound!=0)

uint32 EMU_CALL sega_get_state_size(uint8 version) {
  uint32 size = 0;
  if(version != 2) version = 1;
  size += sizeof(struct SEGA_STATE);
#ifndef DISABLE_SSF
  if(version == 1) size += satsound_get_state_size();
#endif
  if(version == 2) size += dcsound_get_state_size();
  return size;
}

void EMU_CALL sega_clear_state(void *state, uint8 version) {
  uint32 offset;

  if(version != 2) version = 1;
  //
  // If we haven't initialized, we really SHOULD have.
  //
  if(!library_was_initialized) sega_hang("library not initialized");

  // Clear local struct
  memset(state, 0, sizeof(struct SEGA_STATE));
  // Set up offsets
  offset = sizeof(struct SEGA_STATE);
#ifndef DISABLE_SSF
  if(version == 1) { SEGASTATE->offset_to_satsound = offset; offset += satsound_get_state_size(); }
#endif
  if(version == 2) { SEGASTATE->offset_to_dcsound  = offset; offset += dcsound_get_state_size(); }
  //
  // Take care of substructures
  //
#ifndef DISABLE_SSF
  if(HAVE_SATSOUND) satsound_clear_state(SATSOUNDSTATE);
#endif
  if(HAVE_DCSOUND) dcsound_clear_state(DCSOUNDSTATE);
  // Done
}

/////////////////////////////////////////////////////////////////////////////
//
// Obtain substates
//
void* EMU_CALL sega_get_satsound_state(void *state) {
#ifdef DISABLE_SSF
  return NULL;
#else
  if(!(HAVE_SATSOUND)) return NULL;
  return SATSOUNDSTATE;
#endif
}

void* EMU_CALL sega_get_dcsound_state(void *state) {
  if(!(HAVE_DCSOUND)) return NULL;
  return DCSOUNDSTATE;
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
sint32 EMU_CALL sega_execute(
  void   *state,
  sint32  cycles,
  sint16 *sound_buf,
  uint32 *sound_samples
) {
#ifndef DISABLE_SSF
  if(HAVE_SATSOUND) {
    return satsound_execute(SATSOUNDSTATE, cycles, sound_buf, sound_samples);
  } else
#endif
  if(HAVE_DCSOUND) {
    return dcsound_execute(DCSOUNDSTATE, cycles, sound_buf, sound_samples);
  } else {
    return -1;
  }
}

/////////////////////////////////////////////////////////////////////////////

static uint32 get32lsb(uint8 *src) {
  return
    ((((uint32)(src[0])) & 0xFF) <<  0) |
    ((((uint32)(src[1])) & 0xFF) <<  8) |
    ((((uint32)(src[2])) & 0xFF) << 16) |
    ((((uint32)(src[3])) & 0xFF) << 24);
}

#if 0
static uint32 get32msb(uint8 *src) {
  return
    ((((uint32)(src[3])) & 0xFF) <<  0) |
    ((((uint32)(src[2])) & 0xFF) <<  8) |
    ((((uint32)(src[1])) & 0xFF) << 16) |
    ((((uint32)(src[0])) & 0xFF) << 24);
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Upload program - must INCLUDE the 4-byte header.
// Returns nonzero on error.
//
sint32 EMU_CALL sega_upload_program(void *state, void *program, uint32 size) {
  uint32 start;
  if(size < 5) return -1;
  start = get32lsb((uint8*)program);
#ifndef DISABLE_SSF
  if(HAVE_SATSOUND) {
    satsound_upload_to_ram(SATSOUNDSTATE, start, ((uint8*)program) + 4, size - 4);
  } else
#endif
  if(HAVE_DCSOUND) {
    dcsound_upload_to_ram(DCSOUNDSTATE, start, ((uint8*)program) + 4, size - 4);
  } else {
    return -1;
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Get the current program counter
//
uint32 EMU_CALL sega_get_pc(void *state) {
#ifndef DISABLE_SSF
  if(HAVE_SATSOUND) return satsound_get_pc(SATSOUNDSTATE);
#endif
  if(HAVE_DCSOUND) return dcsound_get_pc(DCSOUNDSTATE);
  return 0;
}

/////////////////////////////////////////////////////////////////////////////

static void *getyamstate(struct SEGA_STATE *state) {
  void *yamstate = NULL;
#ifndef DISABLE_SSF
  if(HAVE_SATSOUND) { yamstate = satsound_get_yam_state(SATSOUNDSTATE); }
#endif
  if(HAVE_DCSOUND) { yamstate = dcsound_get_yam_state(DCSOUNDSTATE); }
  return yamstate;
}

/////////////////////////////////////////////////////////////////////////////
//
// Enable or disable various things
//
void EMU_CALL sega_enable_dry(void *state, uint8 enable) {
  void *yamstate = getyamstate(SEGASTATE);
  if(yamstate) yam_enable_dry(yamstate, enable);
}

void EMU_CALL sega_enable_dsp(void *state, uint8 enable) {
  void *yamstate = getyamstate(SEGASTATE);
  if(yamstate) yam_enable_dsp(yamstate, enable);
}

void EMU_CALL sega_enable_dsp_dynarec(void *state, uint8 enable) {
  void *yamstate = getyamstate(SEGASTATE);
  if(yamstate) yam_enable_dsp_dynarec(yamstate, enable);
}

/////////////////////////////////////////////////////////////////////////////
