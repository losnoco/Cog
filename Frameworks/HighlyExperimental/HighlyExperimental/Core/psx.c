/////////////////////////////////////////////////////////////////////////////
//
// psx - Top-level emulation
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "psx.h"

#include "r3000.h"
#include "iop.h"
#include "spu.h"
#include "bios.h"
#include "ioptimer.h"
#include "spucore.h"
#include "vfs.h"

/////////////////////////////////////////////////////////////////////////////
//
// Static init for the whole library
//
static uint8 library_was_initialized = 0;

static uint32 ps1preboot;
static uint32 ps2preboot;

//
// Deliberately create a NULL dereference
// Useful for calling attention to show-stopper problems like forgetting to
// call psx_init() or compiling with the wrong byte order
//
static void psx_hang(const char *message) {
  for(;;) { *((volatile char*)0) = *message; }
}

//
// Endian check
//
static void psx_endian_check(void) {
  uint32 num = 0x61626364;
  // Big
  if(!memcmp(&num, "abcd", 4)) {
#ifdef EMU_BIG_ENDIAN
    return;
#endif
    psx_hang("endian check");
  }
  // Little
  if(!memcmp(&num, "dcba", 4)) {
#ifndef EMU_BIG_ENDIAN
    return;
#endif
    psx_hang("endian check");
  }
  // Don't know what!
  psx_hang("endian check");
}

//
// Data type size check
//
static void psx_size_check(void) {
  if(sizeof(uint8 ) != 1) psx_hang("size check");
  if(sizeof(uint16) != 2) psx_hang("size check");
  if(sizeof(uint32) != 4) psx_hang("size check");
  if(sizeof(uint64) != 8) psx_hang("size check");
  if(sizeof(sint8 ) != 1) psx_hang("size check");
  if(sizeof(sint16) != 2) psx_hang("size check");
  if(sizeof(sint32) != 4) psx_hang("size check");
  if(sizeof(sint64) != 8) psx_hang("size check");
}

static uint32 EMU_CALL getenvhex(const char *name) {
  uint32 value;
  char s[100];
  char *t = s;
  if(bios_getenv(name, s, sizeof(s))) psx_hang("getenv failed");
  s[99] = 0;
  value = 0;
  for(;;) {
    int c = *t++;
    if(!c) break;
         if(c >= 'a' && c <= 'f') { c -= 'a'; c += 10; }
    else if(c >= 'A' && c <= 'F') { c -= 'A'; c += 10; }
    else if(c >= '0' && c <= '9') { c -= '0'; c +=  0; }
    else psx_hang("invalid hex string");
    value <<= 4;
    value += c;
  }
  return value;
}

sint32 EMU_CALL psx_init(void) {
  sint32 r;
  psx_endian_check();
  psx_size_check();

  // BIOS must be loaded first
  if ( !bios_get_image_native() || !bios_get_imagesize() ) return 0;
  //
  // BIOS must be a power of 2, or all hell breaks loose
  //
  { uint32 s = bios_get_imagesize();
    if(s & (s - 1)) psx_hang("imagesize error");
  }
  //
  // Environment inits
  //
  ps1preboot = getenvhex("ps1preboot");
  ps2preboot = getenvhex("ps2preboot");

  r = iop_init(); if(r) return r;
  r = ioptimer_init(); if(r) return r;
  r = r3000_init(); if(r) return r;
  r = spu_init(); if(r) return r;
  r = spucore_init(); if(r) return r;
  r = vfs_init(); if(r) return r;
  library_was_initialized = 1;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Version information
//
const char* EMU_CALL psx_getversion(void) {
  static const char s[] = "PSXCore0008 (built " __DATE__ ")";
  return s;
}

/////////////////////////////////////////////////////////////////////////////
//
// State information
//
struct PSX_STATE {
  uint8 version;
  uint32 offset_to_vfs;
  uint32 offset_to_iop;
  psx_console_out_t console_callback;
  void *console_context;
  uint8 console_enable;
};

#define PSXSTATE ((struct PSX_STATE*)(state))
#define VFSSTATE ((void*)(((char*)(state))+(PSXSTATE->offset_to_vfs)))
#define IOPSTATE ((void*)(((char*)(state))+(PSXSTATE->offset_to_iop)))

#define HAVE_VFS (PSXSTATE->offset_to_vfs!=0)
#define HAVE_IOP (PSXSTATE->offset_to_iop!=0)

uint32 EMU_CALL psx_get_state_size(uint8 version) {
  uint32 size = 0;
  if(version != 2) version = 1;
  size += sizeof(struct PSX_STATE);
  size += iop_get_state_size(version);
  if(version == 2) size += vfs_get_state_size();
  return size;
}

static EMU_INLINE void EMU_CALL preboot(struct PSX_STATE *state, uint32 target_address) {
  sint32 r;
  for(;;) {
    uint32 s = 10000;
    r = iop_execute(IOPSTATE, state, 10000, NULL, &s, 0);
    if(r < 0) break;
    if(r3000_getreg(iop_get_r3000_state(IOPSTATE), R3000_REG_PC) == target_address) break;
  }
}

void EMU_CALL psx_clear_state(void *state, uint8 version) {
  uint32 offset;
  //
  // If we haven't initialized, we really SHOULD have.
  //
  if(!library_was_initialized) psx_hang("library not initialized");

  if(version != 2) version = 1;
  // Clear local struct
  memset(state, 0, sizeof(struct PSX_STATE));
  // Set local version
  PSXSTATE->version = version;
  // Set up offsets
  offset = sizeof(struct PSX_STATE);
  if(version == 2) {
    PSXSTATE->offset_to_vfs = offset; offset += vfs_get_state_size();
  }
  PSXSTATE->offset_to_iop = offset; offset += iop_get_state_size(version);
  // Take care of VFS state
  if(HAVE_VFS) vfs_clear_state(VFSSTATE);
  // Take care of IOP state
  if(HAVE_IOP) iop_clear_state(IOPSTATE, version);
  //
  // Do some final inits
  //
  switch(version) {
  case 1:
    //
    // preboot for PS1
    //
    preboot(PSXSTATE, 0xBFC00000 | (ps1preboot & 0x003FFFFF));
    r3000_setreg(iop_get_r3000_state(IOPSTATE), R3000_REG_PC    , 0x80010000);
    r3000_setreg(iop_get_r3000_state(IOPSTATE), R3000_REG_GEN+29, 0x801FFFF0);
    break;
  case 2:
    //
    // preboot for PS2
    //
    preboot(PSXSTATE, 0xBFC00000 | (ps2preboot & 0x003FFFFF));
    //
    // simulate jr $v0 to transfer execution to loadcore
    //
    r3000_setreg(iop_get_r3000_state(IOPSTATE), R3000_REG_PC,
      r3000_getreg(iop_get_r3000_state(IOPSTATE), R3000_REG_GEN+2)
    );
    break;
  }
  // Done
}

//
// Obtain substates
//
void* EMU_CALL psx_get_iop_state(void *state) { return IOPSTATE; }

/////////////////////////////////////////////////////////////////////////////
//
// Set readfile
//
void EMU_CALL psx_set_readfile(
  void *state,
  psx_readfile_t callback,
  void *context
) {
  if(HAVE_VFS) vfs_set_readfile(VFSSTATE, callback, context);
}

/////////////////////////////////////////////////////////////////////////////
//
// Console
//
void EMU_CALL psx_set_console_out(
  void *state,
  psx_console_out_t callback,
  void *context
) {
  PSXSTATE->console_callback = callback;
  PSXSTATE->console_context = context;
}

void EMU_CALL psx_console_in(void *state, char c) {

}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE uint32 EMU_CALL get32lsb(uint8 *src) {
  return
    ((((uint32)(src[0])) & 0xFF) <<  0) |
    ((((uint32)(src[1])) & 0xFF) <<  8) |
    ((((uint32)(src[2])) & 0xFF) << 16) |
    ((((uint32)(src[3])) & 0xFF) << 24);
}

/////////////////////////////////////////////////////////////////////////////
//
// Determine if an ASCII string exists in a byte block
//
static int string_exists(const char *block, uint32 len, const char *string) {
  uint32 sl = (uint32)strlen(string);
  uint32 i;
  if(sl > len) return 0;
  len = (len - sl) + 1;
  for(i = 0; i < len; i++) {
    if(memcmp(block + i, string, sl) == 0) return 1;
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Upload PS-X EXE - must INCLUDE the header.
// If the header includes the strings "North America", "Japan", or "Europe",
// the appropriate refresh rate is set.
// Returns nonzero on error.
// Will return error for PS2.
//
sint32 EMU_CALL psx_upload_psxexe(void *state, void *program, uint32 size) {
  uint32 init_pc;
  uint32 init_sp;
  uint32 text_start;
  uint32 text_size;

  if(PSXSTATE->version != 1) return -1;

  if(size < 0x801) return -1;
  if(memcmp(program, "PS-X EXE", 8)) return -1;

  text_start = get32lsb(((uint8*)program) + 0x18);
  text_size  = get32lsb(((uint8*)program) + 0x1C);
  init_pc    = get32lsb(((uint8*)program) + 0x10);
  init_sp    = get32lsb(((uint8*)program) + 0x30);

  // Try to determine the region, or leave it at the default if it's not found
       if(string_exists(program, 0x800, "North America")) { psx_set_refresh(state, 60); }
  else if(string_exists(program, 0x800, "Japan"        )) { psx_set_refresh(state, 60); }
  else if(string_exists(program, 0x800, "Europe"       )) { psx_set_refresh(state, 50); }

  if(text_size > (size - 0x800)) { text_size = (size - 0x800); }

  iop_upload_to_ram(IOPSTATE, text_start, ((uint8*)program) + 0x800, text_size);

  r3000_setreg(iop_get_r3000_state(IOPSTATE), R3000_REG_PC    , init_pc);
  r3000_setreg(iop_get_r3000_state(IOPSTATE), R3000_REG_GEN+29, init_sp);

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Set the screen refresh rate in Hz (50 or 60 for PAL or NTSC)
// Only 50 or 60 are valid; other values will be ignored
//
void EMU_CALL psx_set_refresh(void *state, uint32 refresh) {
  iop_set_refresh(IOPSTATE, refresh);
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
// -1     Halted successfully (only applicable to PS2 environment)
// <= -2  Unrecoverable error
//
sint32 EMU_CALL psx_execute(
  void   *state,
  sint32  cycles,
  sint16 *sound_buf,
  uint32 *sound_samples,
  uint32  event_mask
) {
  return iop_execute(IOPSTATE, state, cycles, sound_buf, sound_samples, event_mask);
}

/////////////////////////////////////////////////////////////////////////////
//
// emu virtual I/O
//
static sint32 EMU_CALL vopen(void *vfsstate, uint8 *ram_native, uint32 ram_size, sint32 ofs) {
  char path[256];
  sint32 i;
  for(i = 0; i < 255; i++) {
    char c = ram_native[(ofs & (ram_size-1)) ^ (EMU_ENDIAN_XOR(3))]; ofs++;
    if(!c) break;
    path[i] = c;
  }
  if(!i) return -2; // ENOENT if path is empty
  path[i] = 0;
  i = vfs_open(vfsstate, path);
  return i;
}

static sint32 EMU_CALL vclose(void *vfsstate, sint32 emufd) {
  if(emufd < 0) return -9;
  return vfs_close(vfsstate, emufd);
}

#ifdef PSX_BIG_ENDIAN
//
// This byte swapper performs NO bounds checking.  Prepare accordingly.
// Also, it assumes you're only calling it if PSX_BIG_ENDIAN is defined.
//
static void EMU_CALL vreadswap(uint8 *base, uint32 ofs, uint32 len) {
  uint32 slop_0 = ofs & 3;
  uint32 slop_1 = (0-(ofs+len)) & 3;
  uint32 start = ofs - slop_0;
  uint32 end = ofs + len + slop_1;
  for(; start < end; start += 4) {
    uint8 a, b, c, d;
    a = base[start+0];
    b = base[start+1];
    c = base[start+2];
    d = base[start+3];
    base[start+0] = d;
    base[start+1] = c;
    base[start+2] = b;
    base[start+3] = a;
  }
}
#endif

static sint32 EMU_CALL vread(void *vfsstate, uint8 *ram_native, uint32 ram_size, sint32 emufd, sint32 ofs, sint32 len) {
  sint32 r;
  if(emufd < 0) return -9;
  if(len < 0) return -22;
  if(len >= (sint32)ram_size) return -22;
  ofs &= (ram_size-1);
  if((len + ofs) > (sint32)ram_size) return -22;
#ifdef PSX_BIG_ENDIAN
  vreadswap(ram_native, ofs, len);
#endif
  r = vfs_read(vfsstate, emufd, (char *)ram_native + ofs, len);
#ifdef PSX_BIG_ENDIAN
  vreadswap(ram_native, ofs, len);
#endif
  return r;
}

static sint32 EMU_CALL vlseek(void *vfsstate, sint32 emufd, sint32 offset, sint32 whence) {
  if(emufd < 0) return -9;
  if(whence < 0 || whence > 2) return -22;
  if(whence == 0 && offset < 0) return -22;
  return vfs_lseek(vfsstate, emufd, offset, whence);
}

/////////////////////////////////////////////////////////////////////////////
//
// hefile emucall handler
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
) {
  if(type == 0) {
    if(PSXSTATE->console_callback) {
      sint32 i;
      for(i = 0; i < arg1; i++) {
        char c = ram_native[ofs & (ram_size-1)]; ofs++;
        if(c == 'H') { PSXSTATE->console_enable = 1; }
        if(PSXSTATE->console_enable) {
          (PSXSTATE->console_callback)(PSXSTATE->console_context, c);
        }
      }
    }
    return arg1;
  }
  if(!(HAVE_VFS)) return -5;
  switch(type) {
  case 3: return vopen (VFSSTATE, ram_native, ram_size, ofs);
  case 4: return vclose(VFSSTATE, emufd);
  case 5: return vread (VFSSTATE, ram_native, ram_size, emufd, ofs, arg1);
  case 6: return -13; // EACCES permission denied
  case 7: return vlseek(VFSSTATE, emufd, arg1, arg2);
  default: return -5;
  }
}

/////////////////////////////////////////////////////////////////////////////
