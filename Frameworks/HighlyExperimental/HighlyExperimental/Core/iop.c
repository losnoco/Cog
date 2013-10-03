/////////////////////////////////////////////////////////////////////////////
//
// iop - PS2 IOP emulation; can also do PS1 via compatibility mode
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "iop.h"

#include "psx.h"
#include "ioptimer.h"
#include "r3000.h"
#include "spu.h"
#include "bios.h"

//#include <windows.h>

/////////////////////////////////////////////////////////////////////////////
//
// Simulate DMA delay for harsh compat mode
//
#define DMA_SPU_CYCLES_PER_HALFWORD (8)

/////////////////////////////////////////////////////////////////////////////
//
// Static information
//
sint32 EMU_CALL iop_init(void) {
  return 0;
}

////////////////////////////////////////////////////////////////////////////////
/*
** State information
*/

//
// names from INTRMAN.H
//
#define IOP_INT_VBLANK  (1<<0)
#define IOP_INT_GM      (1<<1)
#define IOP_INT_CDROM   (1<<2)
#define IOP_INT_DMA     (1<<3)
#define IOP_INT_RTC0    (1<<4)
#define IOP_INT_RTC1    (1<<5)
#define IOP_INT_RTC2    (1<<6)
#define IOP_INT_SIO0    (1<<7)
#define IOP_INT_SIO1    (1<<8)
#define IOP_INT_SPU     (1<<9)
#define IOP_INT_PIO     (1<<10)
#define IOP_INT_EVBLANK (1<<11)
#define IOP_INT_DVD     (1<<12)
#define IOP_INT_PCMCIA  (1<<13)
#define IOP_INT_RTC3    (1<<14)
#define IOP_INT_RTC4    (1<<15)
#define IOP_INT_RTC5    (1<<16)
#define IOP_INT_SIO2    (1<<17)
#define IOP_INT_HTR0    (1<<18)
#define IOP_INT_HTR1    (1<<19)
#define IOP_INT_HTR2    (1<<20)
#define IOP_INT_HTR3    (1<<21)
#define IOP_INT_USB     (1<<22)
#define IOP_INT_EXTR    (1<<23)
#define IOP_INT_FWRE    (1<<24)
#define IOP_INT_FDMA    (1<<25)

struct IOP_INTR {
  uint32 en;
  uint32 signaled;
  uint8 is_masked;
};

struct IOP_DMA_CHAN {
  uint32 MADR;
  uint32 BCR;
  uint32 CHCR;
  uint64 cycles_until_interrupt;
};

struct IOP_DMA {
  struct IOP_DMA_CHAN chan[7];
  uint32 DPCR;
  uint32 DICR;
};

struct IOP_EVENT {
  uint64 time;
  uint32 type;
  char  *fmt; // Always points into constant static data. safe.
  uint32 arg[4];
};

#define IOP_MAX_EVENTS (16)

struct IOP_STATE {
  struct IOP_STATE *myself; // Pointer used to check location invariance

  uint8  version;
  uint32 offset_to_map_load;
  uint32 offset_to_map_store;
  uint32 offset_to_ioptimer;
  uint32 offset_to_r3000;
  uint32 offset_to_spu;
  uint32 ram[0x200000/4];
  uint32 scratch[0x400/4];
  uint64 odometer;
//  struct IOP_INTR      intr[2];
  struct IOP_INTR      intr;
  struct IOP_DMA       dma[2];
  sint16 *sound_buffer; // TEMPORARY pointer. safe.
  uint32  sound_buffer_samples_free;
  uint32  sound_cycles_pending;
  uint32  sound_cycles_until_interrupt;
  struct IOP_EVENT     event[IOP_MAX_EVENTS];
  uint32              event_write_index;
  uint32              event_count;
  uint32              event_mask;
  uint8 *audit_map; // Externally-registered pointer. safe.
  uint32 audit_bytes_used;
  uint8 compat_level;
  uint8 quitflag;
  uint8 fatalflag;
  uint32 cycles_per_sample;
  void *psx_state; // TEMPORARY pointer. safe.
};

#define IOPSTATE      ((struct IOP_STATE*)(state))
#define MAPLOAD       ((void*)(((char*)(state))+(IOPSTATE->offset_to_map_load)))
#define MAPSTORE      ((void*)(((char*)(state))+(IOPSTATE->offset_to_map_store)))
#define IOPTIMERSTATE ((void*)(((char*)(state))+(IOPSTATE->offset_to_ioptimer)))
#define R3000STATE    ((void*)(((char*)(state))+(IOPSTATE->offset_to_r3000)))
#define SPUSTATE      ((void*)(((char*)(state))+(IOPSTATE->offset_to_spu)))

extern const uint32 iop_map_load_entries;
extern const uint32 iop_map_store_entries;

uint32 EMU_CALL iop_get_state_size(int version) {
  uint32 size = 0;
  if(version != 2) { version = 1; }
  size += sizeof(struct IOP_STATE);
  size += sizeof(struct R3000_MEMORY_MAP) * iop_map_load_entries;
  size += sizeof(struct R3000_MEMORY_MAP) * iop_map_store_entries;
  size += ioptimer_get_state_size();
  size += r3000_get_state_size();
  size += spu_get_state_size(version);
  return size;
}

#define PSXRAM_BYTE_NATIVE ((uint8*)(IOPSTATE->ram))
#define BIOS_BYTE_NATIVE ((uint8*)(bios_rom_native))
#define SCRATCH_BYTE_NATIVE ((uint8*)(IOPSTATE->scratch))

static void EMU_CALL recompute_memory_maps(struct IOP_STATE *state);
static void EMU_CALL iop_advance(void *state, uint32 elapse);

void EMU_CALL iop_clear_state(void *state, int version) {
  uint32 offset;
  if(version != 2) { version = 1; }
  /* Clear local struct */
  memset(state, 0, sizeof(struct IOP_STATE));
  /* Set version */
  IOPSTATE->version = version;
  /* Set up offsets */
  offset = sizeof(struct IOP_STATE);
  IOPSTATE->offset_to_map_load  = offset; offset += sizeof(struct R3000_MEMORY_MAP) * iop_map_load_entries;
  IOPSTATE->offset_to_map_store = offset; offset += sizeof(struct R3000_MEMORY_MAP) * iop_map_store_entries;
  IOPSTATE->offset_to_ioptimer  = offset; offset += ioptimer_get_state_size();
  IOPSTATE->offset_to_r3000     = offset; offset += r3000_get_state_size();
  IOPSTATE->offset_to_spu       = offset; offset += spu_get_state_size(version);
  //
  // Set other variables
  //
  IOPSTATE->cycles_per_sample = 768;
  //
  // Take care of substructures: memory map, R3000 state, SPU state
  //
  recompute_memory_maps(IOPSTATE);

  ioptimer_clear_state(IOPTIMERSTATE);
  // default to NTSC / 60Hz
  iop_set_refresh(IOPSTATE, 60);

  r3000_clear_state(R3000STATE);
  r3000_set_prid(R3000STATE, (version == 1) ? 0x02 : 0x10);

  r3000_set_advance_callback(R3000STATE, iop_advance, IOPSTATE);
  r3000_set_memory_maps(R3000STATE, MAPLOAD, MAPSTORE);

  spu_clear_state(SPUSTATE, version);

  // Done
}

/////////////////////////////////////////////////////////////////////////////
//
// Set the screen refresh rate in Hz (50 or 60 for PAL or NTSC)
// Only 50 or 60 are valid; other values will be ignored
//
void EMU_CALL iop_set_refresh(void *state, uint32 refresh) {
  if(refresh == 50 || refresh == 60) {
    ioptimer_set_rates(IOPTIMERSTATE,
      (IOPSTATE->version == 1) ? 33868800 : 36864000,
      (IOPSTATE->version == 1) ? 429 : 858,
      (refresh == 60) ? 262 : 312,
      (refresh == 60) ? 224 : 240,
      refresh
    );
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Location invariance handlers
//

//
// Recompute all internally-kept pointers
//
static void EMU_CALL location_recompute(struct IOP_STATE *state) {
  recompute_memory_maps(state);
  r3000_set_advance_callback(R3000STATE, iop_advance, IOPSTATE);
  r3000_set_memory_maps(R3000STATE, MAPLOAD, MAPSTORE);
  state->myself = state;
}

//
// Check to see if this structure has moved, and if so, recompute
//
// This is currently ONLY done on iop_execute
//
static EMU_INLINE void EMU_CALL location_check(struct IOP_STATE *state) {
  if(state->myself == state) return;
  location_recompute(state);
}

////////////////////////////////////////////////////////////////////////////////
//
// Obtain substates
//
// These require no location checks as the substates are obtained via
// relative offsets
//
void* EMU_CALL iop_get_r3000_state(void *state) { return R3000STATE; }
void* EMU_CALL iop_get_spu_state  (void *state) { return SPUSTATE  ; }

////////////////////////////////////////////////////////////////////////////////
/*
** Register a map for auditing purposes
** (must contain 1 byte for every RAM byte)
**
** Pass NULL to disable auditing.
*/
void EMU_CALL iop_register_map_for_auditing(void *state, uint8 *map) {
  IOPSTATE->audit_map = map;
  IOPSTATE->audit_bytes_used = 0;
  recompute_memory_maps(IOPSTATE);
}

/*
** Auditing functions
*/
static EMU_INLINE void EMU_CALL audit(struct IOP_STATE *state, uint32 address, uint32 length, uint8 t) {
  for(; length; length--, address++) {
    if(!(state->audit_map[address & 0x1FFFFF])) {
      if(t == IOP_AUDIT_READ) { state->audit_bytes_used++; }
      state->audit_map[address & 0x1FFFFF] = t;
    }
  }
}

uint32 EMU_CALL iop_get_bytes_used_in_audit(void *state) {
  // No location check required
  return IOPSTATE->audit_bytes_used;
}

/*
** R3000 memory map callbacks for auditing
*/
static uint32 EMU_CALL audit_lw(void *state, uint32 a, uint32 mask) {

//PSXTRACE3("audit_lw(%08X,%08X,%08X)\n",state,a,mask);

  a &= 0x1FFFFC;
  if((mask & 0x000000FF) && (!(IOPSTATE->audit_map[a+0]))) { IOPSTATE->audit_map[a+0] = IOP_AUDIT_READ; IOPSTATE->audit_bytes_used++; }
  if((mask & 0x0000FF00) && (!(IOPSTATE->audit_map[a+1]))) { IOPSTATE->audit_map[a+1] = IOP_AUDIT_READ; IOPSTATE->audit_bytes_used++; }
  if((mask & 0x00FF0000) && (!(IOPSTATE->audit_map[a+2]))) { IOPSTATE->audit_map[a+2] = IOP_AUDIT_READ; IOPSTATE->audit_bytes_used++; }
  if((mask & 0xFF000000) && (!(IOPSTATE->audit_map[a+3]))) { IOPSTATE->audit_map[a+3] = IOP_AUDIT_READ; IOPSTATE->audit_bytes_used++; }
  return (*((uint32*)((PSXRAM_BYTE_NATIVE)+a))) & mask;
}

static void EMU_CALL audit_sw(void *state, uint32 a, uint32 d, uint32 mask) {

//PSXTRACE3("audit_sw(state,%08X,%08X,%08X)\n",a,d,mask);

  a &= 0x1FFFFC;
  if((mask & 0x000000FF) && (!(IOPSTATE->audit_map[a+0]))) IOPSTATE->audit_map[a+0] = IOP_AUDIT_WRITE;
  if((mask & 0x0000FF00) && (!(IOPSTATE->audit_map[a+1]))) IOPSTATE->audit_map[a+1] = IOP_AUDIT_WRITE;
  if((mask & 0x00FF0000) && (!(IOPSTATE->audit_map[a+2]))) IOPSTATE->audit_map[a+2] = IOP_AUDIT_WRITE;
  if((mask & 0xFF000000) && (!(IOPSTATE->audit_map[a+3]))) IOPSTATE->audit_map[a+3] = IOP_AUDIT_WRITE;
  (*((uint32*)((PSXRAM_BYTE_NATIVE)+a))) &= (~mask);
  (*((uint32*)((PSXRAM_BYTE_NATIVE)+a))) |= (d & mask);
}

////////////////////////////////////////////////////////////////////////////////
//
// Event handling
//
// None of this needs location checking.
//
static void EMU_CALL event(
  struct IOP_STATE *state,
  uint32 type,
  char *fmt,
  uint32 arg0,
  uint32 arg1,
  uint32 arg2,
  uint32 arg3
) {
  if(state->event_mask & (1 << type)) {
    struct IOP_EVENT *ev = state->event + state->event_write_index;
    state->event_write_index++;
    if(state->event_write_index >= IOP_MAX_EVENTS) { state->event_write_index = 0; }
    if(state->event_count < IOP_MAX_EVENTS) { state->event_count++; }
    /*
    ** Copy event info
    */
    ev->time = state->odometer;
    ev->type = type;
    ev->fmt  = fmt;
    ev->arg[0] = arg0;
    ev->arg[1] = arg1;
    ev->arg[2] = arg2;
    ev->arg[3] = arg3;
  }
}

void EMU_CALL iop_clear_events(void *state) {
  IOPSTATE->event_count = 0;
}

uint32 EMU_CALL iop_get_event_count(void *state) {
  return IOPSTATE->event_count;
}

void EMU_CALL iop_get_event(void *state, uint64 *time, uint32 *type, char **fmt, uint32 *arg) {
  struct IOP_EVENT *ev;
  if(!IOPSTATE->event_count) return;
  ev = IOPSTATE->event + (((IOPSTATE->event_write_index + IOP_MAX_EVENTS) - IOPSTATE->event_count) % IOP_MAX_EVENTS);
  if(time) *time = ev->time;
  if(type) *type = ev->type;
  if(fmt) *fmt = ev->fmt;
  if(arg) {
    arg[0] = ev->arg[0];
    arg[1] = ev->arg[1];
    arg[2] = ev->arg[2];
    arg[3] = ev->arg[3];
  }
}

void EMU_CALL iop_dismiss_event(void *state) {
  if(IOPSTATE->event_count) { IOPSTATE->event_count--; }
}

/////////////////////////////////////////////////////////////////////////////
//
// misc register load
//
static uint32 EMU_CALL misc_lw(struct IOP_STATE *state, uint32 a, uint32 mask) {
  uint32 d = 0;
  switch(a & 0x1FFFFFFC) {
  case 0x1F801450:
    if(state->version == 1) { d = 0x0000008; }
    else                    { d = 0x0000000; }
    break;
  }
  event(state, IOP_EVENT_REG_LOAD, "Misc. load (%08X,%08X)=%08X", a, mask, d, 0);
  return d;
}

////////////////////////////////////////////////////////////////////////////////
/*
** Interrupt controller
*/

/*
** Update the CPU-side interrupt pending flag
*/
static void EMU_CALL intr_update_ip(struct IOP_STATE *state) {
  uint32 ip = 0;
  if(
    (!(state->intr.is_masked)) &&
    ((state->intr.signaled & state->intr.en) != 0)
  ) { ip |= 0x4; }
  r3000_setinterrupt(R3000STATE, ip);
}

/*
** Signal an interrupt (hardware-side interrupt bits are used)
*/
static void EMU_CALL intr_signal(struct IOP_STATE *state, uint32 i) {

  event(state, IOP_EVENT_INTR_SIGNAL, "Interrupt %X signaled", i, 0, 0, 0);

  if(!(state->intr.signaled & i)) {
    state->intr.signaled |= i;
    intr_update_ip(state);
  }
}

/*
** INTR register load
*/
static uint32 EMU_CALL intr_lw(struct IOP_STATE *state, uint32 a, uint32 mask) {
  uint32 d = 0;

  switch(a & 0x7C) {
  case 0x70: d = state->intr.signaled; break;
  case 0x74: d = state->intr.en; break;
  case 0x78:
    d = (state->intr.is_masked) ? 0 : 1;
    state->intr.is_masked = 1;
    intr_update_ip(state);
    break;
  }

//end:
  d &= mask;
  event(state, IOP_EVENT_REG_LOAD, "INTR load (%08X,%08X)=%08X", a, mask, d, 0);
  return d;
}

/*
** INTR register store
*/
static void EMU_CALL intr_sw(struct IOP_STATE *state, uint32 a, uint32 d, uint32 mask) {

  event(state, IOP_EVENT_REG_STORE, "INTR store (%08X,%08X,%08X)", a, d, mask, 0);

  switch(a & 0x7C) {
  case 0x70:
    // Acknowledge
    state->intr.signaled &= (d | (~mask));
    intr_update_ip(state);
    break;
  case 0x74:
    // Change enable bits
    state->intr.en = (state->intr.en & (~mask)) | (d & mask);
    intr_update_ip(state);
    break;
  case 0x78:
    // Mask/unmask
    state->intr.is_masked = (d ^ 1) & 1;
    intr_update_ip(state);
    break;
  }

}

////////////////////////////////////////////////////////////////////////////////
/*
** DMA
*/

/*
** Signal completion on a DMA channel
*/
static void EMU_CALL dma_signal_completion(struct IOP_STATE *state, uint32 chan) {
  uint32 core = chan / 7;
  chan %= 7;

  /* Reset transfer busy bit */
  state->dma[core].chan[chan].CHCR &= (~0x01000000);

  /* TODO: see how the core affects the int numbers */

  if(state->dma[core].DICR & (0x00010000 << (chan))) {
    state->dma[core].DICR |= (0x01000000 << (chan));
    intr_signal(state, IOP_INT_DMA);
  }
}

/*
** Initiate a DMA transfer
*/
static void EMU_CALL dma_transfer(struct IOP_STATE *state, uint32 core, uint32 chan) {
  uint32 realchan      = 7 * core + chan;
  uint32 mem_address   = (state->dma[core].chan[chan].MADR      );
  uint32 blocksize     = (state->dma[core].chan[chan].BCR  >>  0) & 0xFFFF;
  uint32 blockcount    = (state->dma[core].chan[chan].BCR  >> 16) & 0xFFFF;
  int    is_writing    = (state->dma[core].chan[chan].CHCR >>  0) & 1;
//  int    is_continuous = (state->dma[core].chan[chan].CHCR >>  9) & 1;
//  int    is_linked     = (state->dma[core].chan[chan].CHCR >> 10) & 1;
  uint32 len = blocksize * 4 * blockcount;
//  uint32 i;
  uint64 cycles_delay = 0;

  event(state, IOP_EVENT_DMA_TRANSFER, 
    is_writing ? 
    "DMA ch.%d write (%08X, %08X)" :
    "DMA ch.%d read  (%08X, %08X)",
    realchan, mem_address, len, 0
  );

  mem_address &= 0x1FFFFC;
  if(!len) return;

  /*
  ** Remember to audit!
  ** TODO: audit linking: SPU with IOP etc.
  */
  if(state->audit_map) {
    audit(state, mem_address, len, is_writing ? IOP_AUDIT_READ : IOP_AUDIT_WRITE);
  }

  switch(realchan) {
  case 4: // SPU CORE0
    spu_dma(SPUSTATE, 0, PSXRAM_BYTE_NATIVE, mem_address & 0x1FFFFC, 0x1FFFFC, len, is_writing);
    cycles_delay = DMA_SPU_CYCLES_PER_HALFWORD * (len / 2);
    break;
  case 7: // SPU CORE1
    spu_dma(SPUSTATE, 1, PSXRAM_BYTE_NATIVE, mem_address & 0x1FFFFC, 0x1FFFFC, len, is_writing);
    cycles_delay = DMA_SPU_CYCLES_PER_HALFWORD * (len / 2);
    break;
  case 10: // SIF
    break;
  }

  /*
  ** Behavior here depends on compat level
  */
  switch(state->compat_level) {
  case IOP_COMPAT_FRIENDLY:
    /* Interrupt immediately */
    dma_signal_completion(state, realchan);
    break;
  case IOP_COMPAT_HARSH:
    /* Schedule interrupt for later */
    if(!cycles_delay) { cycles_delay = 1; }
    state->dma[core].chan[chan].cycles_until_interrupt = cycles_delay;
    break;
  default:
    /* Default behavior: friendly, interrupt immediately */
    dma_signal_completion(state, realchan);
    break;
  }

}

/*
** DMA register load
*/
static uint32 EMU_CALL dma_lw(struct IOP_STATE *state, uint32 core, uint32 a, uint32 mask) {
  uint32 d = 0;
  struct IOP_DMA *dma;
  uint8 chan;

  dma = &(state->dma[core]);
  chan = (a >> 4) & 7;
  if(chan == 7) {
    switch(a & 0xC) {
    case 0x0: d = dma->DPCR; break;
    case 0x4: d = dma->DICR; break;
    case 0x8: d = 0; break; // sometimes this is blocked until it's 0
    }
  }

//end:
  d &= mask;
  event(state, IOP_EVENT_REG_LOAD, "DMA%d load (%08X,%08X)=%08X", core, a, mask, d);
  return d;
}

static uint32 EMU_CALL dma0_lw(struct IOP_STATE *state, uint32 a, uint32 mask) { return dma_lw(state, 0, a, mask); }
static uint32 EMU_CALL dma1_lw(struct IOP_STATE *state, uint32 a, uint32 mask) { return dma_lw(state, 1, a, mask); }

/*
** DMA register store
*/
static void EMU_CALL dma_sw(struct IOP_STATE *state, uint32 core, uint32 a, uint32 d, uint32 mask) {
  struct IOP_DMA *dma;
  uint8 chan;

  event(state, IOP_EVENT_REG_STORE, "DMA%d store (%08X,%08X,%08X)", core, a, d, mask);

  dma = &(state->dma[core]);
  chan = (a >> 4) & 7;
  if(chan == 7) {
    switch(a & 0xC) {
    case 0x0: dma->DPCR = (dma->DPCR & (~mask)) | (d & mask); break;
    case 0x4:
      if(mask & 0xFF000000) { dma->DICR &= ((~d) & 0xFF000000); }
      if(mask & 0x00FF0000) { dma->DICR |= (( d) & 0x00FF0000); }
      break;
    case 0x8: // sometimes 0 is stored here
      break;
    }
  } else {
    switch(a & 0xC) {
    case 0x0: dma->chan[chan].MADR = (dma->chan[chan].MADR & (~mask)) | (d & mask); break;
    case 0x4: dma->chan[chan].BCR  = (dma->chan[chan].BCR  & (~mask)) | (d & mask); break;
    case 0x8:
      dma->chan[chan].CHCR = (dma->chan[chan].CHCR & (~mask)) | (d & mask);
      /* Transfer being initiated */
      if(d & mask & 0x01000000) dma_transfer(state, core, chan);
      break;
    }
  }

}

static void EMU_CALL dma0_sw(struct IOP_STATE *state, uint32 a, uint32 d, uint32 mask) { dma_sw(state, 0, a, d, mask); }
static void EMU_CALL dma1_sw(struct IOP_STATE *state, uint32 a, uint32 d, uint32 mask) { dma_sw(state, 1, a, d, mask); }

////////////////////////////////////////////////////////////////////////////////
/*
** Timers
*/

static uint32 EMU_CALL timer_lw(struct IOP_STATE *state, uint32 a, uint32 mask) {
  uint32 d = 0;
  event(state, IOP_EVENT_REG_LOAD, "Timer load (%08X,%08X)=%08X", a, mask, d, 0);
  d = ioptimer_lw(IOPTIMERSTATE, a, mask);
  return d & mask;
}

static void EMU_CALL timer_sw(struct IOP_STATE *state, uint32 a, uint32 d, uint32 mask) {
  event(state, IOP_EVENT_REG_STORE, "Timer store (%08X,%08X,%08X)", a, d, mask, 0);
  ioptimer_sw(IOPTIMERSTATE, a, d, mask);
}

////////////////////////////////////////////////////////////////////////////////
/*
** SPU: Forward read/write/advance calls to the SPU code
*/

static void EMU_CALL flush_sound(struct IOP_STATE *state) {
  uint32 samples_needed = state->sound_cycles_pending / (state->cycles_per_sample);
  if(samples_needed > state->sound_buffer_samples_free) {
    samples_needed = state->sound_buffer_samples_free;
  }
  if(!samples_needed) return;

  spu_render(SPUSTATE, state->sound_buffer, samples_needed);

  if(state->sound_buffer) state->sound_buffer += 2 * samples_needed;
  state->sound_buffer_samples_free -= samples_needed;
  state->sound_cycles_pending -= (state->cycles_per_sample) * samples_needed;
}

/*
** Register loads/stores
*/

static uint32 EMU_CALL iop_spu_lw(void *state, uint32 a, uint32 mask) {
  uint32 d = 0;
  flush_sound(IOPSTATE);
  if(mask & 0x0000FFFF) d |= ((((uint32)(spu_lh(SPUSTATE, (a&(~3))+0))) & 0xFFFF) <<  0);
  if(mask & 0xFFFF0000) d |= ((((uint32)(spu_lh(SPUSTATE, (a&(~3))+2))) & 0xFFFF) << 16);
//end:
  d &= mask;
  event(state, IOP_EVENT_REG_LOAD, "SPU load (%08X,%08X)=%08X", a, mask, d, 0);
  return d;
}

static void EMU_CALL iop_spu_sw(void *state, uint32 a, uint32 d, uint32 mask) {
  event(state, IOP_EVENT_REG_STORE, "SPU store (%08X,%08X,%08X)", a, d, mask, 0);
  flush_sound(IOPSTATE);
  if(mask & 0x0000FFFF) spu_sh(SPUSTATE, (a&(~3))+0, d >>  0);
  if(mask & 0xFFFF0000) spu_sh(SPUSTATE, (a&(~3))+2, d >> 16);
}

static uint32 EMU_CALL iop_spu2_lw(void *state, uint32 a, uint32 mask) {
  uint32 d = 0;
  if(IOPSTATE->version == 2) {
    flush_sound(IOPSTATE);
    if(mask & 0x0000FFFF) d |= ((((uint32)(spu_lh(SPUSTATE, (a&(~3))+0))) & 0xFFFF) <<  0);
    if(mask & 0xFFFF0000) d |= ((((uint32)(spu_lh(SPUSTATE, (a&(~3))+2))) & 0xFFFF) << 16);
  }
//end:
  d &= mask;
  event(state, IOP_EVENT_REG_LOAD, "SPU2 load (%08X,%08X)=%08X", a, mask, d, 0);
  return d;
}

static void EMU_CALL iop_spu2_sw(void *state, uint32 a, uint32 d, uint32 mask) {
  event(state, IOP_EVENT_REG_STORE, "SPU2 store (%08X,%08X,%08X)", a, d, mask, 0);
  if(IOPSTATE->version == 2) {
    flush_sound(IOPSTATE);
    if(mask & 0x0000FFFF) spu_sh(SPUSTATE, (a&(~3))+0, d >>  0);
    if(mask & 0xFFFF0000) spu_sh(SPUSTATE, (a&(~3))+2, d >> 16);
  }
}

/////////////////////////////////////////////////////////////////////////////
// 
// SIF register load
//
static uint32 EMU_CALL sif_lw(struct IOP_STATE *state, uint32 a, uint32 mask) {
  uint32 d = 0;
  switch(a & 0x7C) {
  case 0x20: d = 0x00010000; break;
  case 0x60: d = 0x1D000060; break;
  }
  d &= mask;
  event(state, IOP_EVENT_REG_LOAD, "SIF load (%08X,%08X)=%08X", a, mask, d, 0);
  return d;
}

//
// SIF register store
//
static void EMU_CALL sif_sw(struct IOP_STATE *state, uint32 a, uint32 d, uint32 mask) {

  event(state, IOP_EVENT_REG_STORE, "SIF store (%08X,%08X,%08X)", a, d, mask, 0);

  switch(a & 0x7C) {
  case 0x30: // seems to be a command of some kind
    //dma_signal_completion(state, 10);
    break;
  case 0x60:
    break;
  }

}

////////////////////////////////////////////////////////////////////////////////
/*
** hefile emucall
*/

static void EMU_CALL iop_emucall_sw(void *state, uint32 a, uint32 d, uint32 mask) {
  sint32 type;
  sint32 emufd;
  sint32 ofs;
  sint32 arg1;
  sint32 arg2;
  sint32 r;
  if(mask != 0xFFFFFFFF) return;
  if(d & 3) return;
  type  = *((sint32*)((PSXRAM_BYTE_NATIVE)+((d+(4*( 0 )))&0x1FFFFC)));
  emufd = *((sint32*)((PSXRAM_BYTE_NATIVE)+((d+(4*( 1 )))&0x1FFFFC)));
  ofs   = *((sint32*)((PSXRAM_BYTE_NATIVE)+((d+(4*( 2 )))&0x1FFFFC)));
  arg1  = *((sint32*)((PSXRAM_BYTE_NATIVE)+((d+(4*( 3 )))&0x1FFFFC)));
  arg2  = *((sint32*)((PSXRAM_BYTE_NATIVE)+((d+(4*( 4 )))&0x1FFFFC)));

  //
  // Log an event
  //
  switch(type) {
  case 0:  event(IOPSTATE, IOP_EVENT_VIRTUAL_IO, "Virtual console output(0x%X, 0x%X)", ofs, arg1, 0, 0); break;
  case 1:  event(IOPSTATE, IOP_EVENT_VIRTUAL_IO, "Virtual quit", 0, 0, 0, 0); break;
  case 3:  event(IOPSTATE, IOP_EVENT_VIRTUAL_IO, "Virtual open(%d, 0x%X, 0x%X, 0x%X)", emufd, ofs, arg1, arg2); break;
  case 4:  event(IOPSTATE, IOP_EVENT_VIRTUAL_IO, "Virtual close(%d)", emufd, 0, 0, 0); break;
  case 5:  event(IOPSTATE, IOP_EVENT_VIRTUAL_IO, "Virtual read(%d, 0x%X, 0x%X)", emufd, ofs, arg1, 0); break;
  case 6:  event(IOPSTATE, IOP_EVENT_VIRTUAL_IO, "Virtual write(%d, 0x%X, 0x%X)", emufd, ofs, arg1, 0); break;
  case 7:  event(IOPSTATE, IOP_EVENT_VIRTUAL_IO, "Virtual lseek(%d, 0x%X, 0x%X)", emufd, arg1, arg2, 0); break;
  default: event(IOPSTATE, IOP_EVENT_VIRTUAL_IO, "Virtual unknown event(0x%X, 0x%X, 0x%X, 0x%X)", emufd, ofs, arg1, arg2); break;
  }

  switch(type) {
  case 1: // HEFILE_PSX_QUIT
//MessageBox(NULL,"qfset",NULL,MB_OK);
    IOPSTATE->quitflag = 1;
    r3000_break(R3000STATE);
    r = 0;
    break;
  default:
    r = psx_emucall(
      IOPSTATE->psx_state,
      PSXRAM_BYTE_NATIVE,
      0x200000,
      type, emufd, ofs, arg1, arg2
    );
    //
    // Special case: fatal error if return value is -5
    //
    if(r == -5) {
      IOPSTATE->fatalflag = 1;
      r3000_break(R3000STATE);
    }
    break;
  }
  *((sint32*)((PSXRAM_BYTE_NATIVE)+((d+(4*( 0 )))&0x1FFFFC))) = r;
}

////////////////////////////////////////////////////////////////////////////////
/*
** Invalid-address catchers
*/

static uint32 EMU_CALL catcher_lw(void *state, uint32 a, uint32 mask) {
  uint32 d = 0;
  event(state, IOP_EVENT_REG_LOAD, "Catcher load (%08X,%08X)", a, mask, d, 0);
  return d;
}

static void EMU_CALL catcher_sw(void *state, uint32 a, uint32 d, uint32 mask) {
  event(state, IOP_EVENT_REG_STORE, "Catcher store (%08X,%08X,%08X)", a, d, mask, 0);
}

////////////////////////////////////////////////////////////////////////////////
/*
** Advance hardware activity by the given cycle count
*/
static void EMU_CALL iop_advance(void *state, uint32 elapse) {
  sint32 core, i; //, r;
  uint32 intr;
  if(!elapse) return;
  //
  // Check timers
  //
  intr = ioptimer_advance(IOPTIMERSTATE, elapse);
  if(intr) intr_signal(IOPSTATE, intr);
  /*
  ** Check DMA counters
  */
  for(core = 0; core < 2; core++) {
    for(i = 0; i < 7; i++) {
      uint64 u = IOPSTATE->dma[core].chan[i].cycles_until_interrupt;
      if(!u) continue;
      if(u > elapse) {
        u -= elapse;
      } else {
        u = 0;
        dma_signal_completion(IOPSTATE, core * 7 + i);
      }
      IOPSTATE->dma[core].chan[i].cycles_until_interrupt = u;
    }
  }
  /*
  ** Check SPU IRQ
  */
  if(elapse >= IOPSTATE->sound_cycles_until_interrupt) intr_signal(IOPSTATE, 0x200);
  /*
  ** Update pending sound cycles
  */
  IOPSTATE->sound_cycles_pending += elapse;
  /*
  ** Update odometer
  */
  IOPSTATE->odometer += ((uint64)elapse);
}

////////////////////////////////////////////////////////////////////////////////
/*
** Determine how many cycles until the next interrupt
**
** This is then used as an upper bound for how many cycles can be executed
** before checking for futher interrupts
*/
static uint32 EMU_CALL cycles_until_next_interrupt(struct IOP_STATE *state, uint32 min) {
  uint32 cyc;
  uint32 core, i; //, r;
  //
  // Timers
  //
  cyc = ioptimer_cycles_until_interrupt(IOPTIMERSTATE);
  if(cyc < min) min = cyc;
  //
  // DMA
  //
  for(core = 0; core < 2; core++) {
    for(i = 0; i < 7; i++) {
      uint64 u = state->dma[core].chan[i].cycles_until_interrupt;
      if(!u) continue;
      if(u < ((uint64)min)) { min = (uint32)u; }
    }
  }
  //
  // SPU
  //
  state->sound_cycles_until_interrupt = cyc = spu_cycles_until_interrupt(SPUSTATE, (min + 767) / 768);
  if(cyc < min) min = cyc;

  if(min < 1) min = 1;
  return min;
}

////////////////////////////////////////////////////////////////////////////////
/*
** Static memory map structures
*/

#define STATEOFS(thetype,thefield) ((void*)(&(((struct thetype*)0)->thefield)))

//
// Note that the first two entries here MUST both be state offsets.
//
static const struct R3000_MEMORY_MAP iop_map_load[] = {
  { 0x00000000, 0x007FFFFF, { 0x001FFFFF, R3000_MAP_TYPE_POINTER , STATEOFS(IOP_STATE, ram    ) } },
  { 0x1F800000, 0x1F800FFF, { 0x000003FF, R3000_MAP_TYPE_POINTER , STATEOFS(IOP_STATE, scratch) } },
  { 0x1FC00000, 0x1FFFFFFF, { 0x00000000, R3000_MAP_TYPE_POINTER , NULL                         } },

  { 0x1D000000, 0x1D00007F, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, sif_lw                       } },
  { 0x1F801000, 0x1F80107F, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, intr_lw                      } },
  { 0x1F801080, 0x1F8010FF, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, dma0_lw                      } },
  { 0x1F801100, 0x1F80112F, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, timer_lw                     } },
  { 0x1F801400, 0x1F80147F, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, misc_lw                      } },
  { 0x1F801480, 0x1F8014AF, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, timer_lw                     } },
  { 0x1F801500, 0x1F80157F, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, dma1_lw                      } },
  { 0x1F801C00, 0x1F801DFF, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, iop_spu_lw                   } },
  { 0x1F900000, 0x1F9007FF, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, iop_spu2_lw                  } },

  { 0x00000000, 0xFFFFFFFF, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, catcher_lw                   } }
};

static const struct R3000_MEMORY_MAP iop_map_store[] = {
  { 0x00000000, 0x007FFFFF, { 0x001FFFFF, R3000_MAP_TYPE_POINTER , STATEOFS(IOP_STATE, ram    ) } },
  { 0x1F800000, 0x1F800FFF, { 0x000003FF, R3000_MAP_TYPE_POINTER , STATEOFS(IOP_STATE, scratch) } },

  { 0x1D000000, 0x1D00007F, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, sif_sw                       } },
  { 0x1F801000, 0x1F80107F, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, intr_sw                      } },
  { 0x1F801080, 0x1F8010FF, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, dma0_sw                      } },
  { 0x1F801100, 0x1F80112F, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, timer_sw                     } },
//{ 0x1F801400, 0x1F80147F, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, intr1_sw                     } },
  { 0x1F801480, 0x1F8014AF, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, timer_sw                     } },
  { 0x1F801500, 0x1F80157F, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, dma1_sw                      } },
  { 0x1F801C00, 0x1F801DFF, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, iop_spu_sw                   } },
  { 0x1F900000, 0x1F9007FF, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, iop_spu2_sw                  } },

  { 0x1FC17120, 0x1FC17123, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, iop_emucall_sw               } },

  { 0x00000000, 0xFFFFFFFF, { 0x1FFFFFFF, R3000_MAP_TYPE_CALLBACK, catcher_sw                   } }
};

#define IOP_ARRAY_ENTRIES(x) (sizeof(x)/sizeof((x)[0]))

const uint32 iop_map_load_entries  = IOP_ARRAY_ENTRIES(iop_map_load );
const uint32 iop_map_store_entries = IOP_ARRAY_ENTRIES(iop_map_store);

////////////////////////////////////////////////////////////////////////////////
//
// Memory map recomputation
//
// Necessary on structure location change or audit change
//

static void perform_state_offset(struct R3000_MEMORY_MAP *map, struct IOP_STATE *state) {
  //
  // It's safe to typecast this "pointer" to a uint32 since, due to
  // the STATEOFS hack, it'll never be bigger than 4GB
  //
  uint32 o = (uint32)(map->type.p);
  map->type.p = ((uint8*)state) + o;
}

//
// Recompute the memory maps.
// PERFORMS NO REGISTRATION with the actual R3000 state.
//
static void recompute_memory_maps(struct IOP_STATE *state) {
  sint32 i;
  struct R3000_MEMORY_MAP *mapload  = MAPLOAD;
  struct R3000_MEMORY_MAP *mapstore = MAPSTORE;
  //
  // First, just copy from the static tables
  //
  memcpy(mapload , iop_map_load , sizeof(iop_map_load ));
  memcpy(mapstore, iop_map_store, sizeof(iop_map_store));
  //
  // Now perform state offsets on first _two_ entries in each map
  //
  for(i = 0; i < 2; i++) {
    perform_state_offset(mapload  + i, state);
    perform_state_offset(mapstore + i, state);
  }
  //
  // And importantly, set the third load entry to point to the BIOS
  //
  mapload[2].type.mask = bios_get_imagesize() - 1;
  mapload[2].type.n    = R3000_MAP_TYPE_POINTER;
  mapload[2].type.p    = bios_get_image_native();

  //
  // Finally, if we're auditing, we'll want to change the _first_ entry in
  // each map to an audit callback
  //
  if(state->audit_map) {
    mapload [0].type.n = R3000_MAP_TYPE_CALLBACK;
    mapload [0].type.p = audit_lw;
    mapstore[0].type.n = R3000_MAP_TYPE_CALLBACK;
    mapstore[0].type.p = audit_sw;
  }
}

////////////////////////////////////////////////////////////////////////////////
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
sint32 EMU_CALL iop_execute(
  void   *state,
  void   *psx_state,
  sint32  cycles,
  sint16 *sound_buf,
  uint32 *sound_samples,
  uint32  event_mask
) {
  sint32 r = 0;
  sint64 cyc_upperbound;
  uint64 old_odometer;
  uint64 target_odometer;

  location_check(IOPSTATE);

  IOPSTATE->psx_state = psx_state;

  old_odometer = IOPSTATE->odometer;

  IOPSTATE->event_mask = event_mask;
  IOPSTATE->sound_buffer              =  sound_buf;
  IOPSTATE->sound_buffer_samples_free = *sound_samples;
  //
  // If the quit flag was set, do nothing
  //
  if(IOPSTATE->quitflag) {
//MessageBox(NULL,"qfset_on_execute",NULL,MB_OK);
    return -1;
  }
  //
  // If the fatal flag was set, return -2
  //
  if(IOPSTATE->fatalflag) { return -2; }
  //
  // If we have a bogus cycle count, return error
  //
  if(cycles < 0) { return -2; }

  /*
  ** Begin by flushing any pending sound data into the newly available
  ** buffer, if necessary
  */
  flush_sound(IOPSTATE);
  /*
  ** Compute an upper bound for the number of cycles that can be generated
  ** while still fitting in the buffer
  */
  cyc_upperbound = 
    ((sint64)(IOPSTATE->cycles_per_sample)) *
    ((sint64)(IOPSTATE->sound_buffer_samples_free));
  if(cyc_upperbound > (IOPSTATE->sound_cycles_pending)) {
    cyc_upperbound -= IOPSTATE->sound_cycles_pending;
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
  target_odometer = (IOPSTATE->odometer)+((uint64)(cyc_upperbound));
  /*
  ** Execution loop
  */
  for(;;) {
    uint32 diff, ci;
    if(IOPSTATE->odometer >= target_odometer) break;

    diff = (uint32)((target_odometer) - (IOPSTATE->odometer));
    ci = cycles_until_next_interrupt(IOPSTATE, diff);
    if(diff > ci) diff = ci;
    r = r3000_execute(R3000STATE, diff);
    //
    // On a clean quit, break with -1
    //
    if(IOPSTATE->quitflag) {
      r = -1;
      break;
    }
    //
    // Create an error if the fatalflag got set
    //
    if(IOPSTATE->fatalflag) {
      IOPSTATE->fatalflag = 0;
      r = -5;
    }
    //
    // On an unrecoverable CPU error, break with -2
    //
    if(r < 0) {
      r = -2;
      break;
    }
  }
  /*
  ** End with a final sound flush
  */
  flush_sound(IOPSTATE);
  /*
  ** Determine how many sound samples we generated
  */
  (*sound_samples) -= (IOPSTATE->sound_buffer_samples_free);
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
  r = (sint32)(IOPSTATE->odometer - old_odometer);
  return r;
}

////////////////////////////////////////////////////////////////////////////////
//
// For debugger use
//
// Location checks not needed
//
uint32 EMU_CALL iop_getword(void *state, uint32 a) { a &= 0x1FFFFFFC;
         if(a < 0x00800000) { return (*((uint32*)(PSXRAM_BYTE_NATIVE+(a&0x1FFFFC))));
  } else if(a >=0x1FC00000) { return (*((uint32*)(bios_get_image_native()+(a&(bios_get_imagesize()-1)))));
  } else {                    return 0;
  }
}

void EMU_CALL iop_setword(void *state, uint32 a, uint32 d) { a &= 0x1FFFFFFC;
  if(a < 0x00800000) {
    (*((uint32*)(PSXRAM_BYTE_NATIVE+(a&0x1FFFFC)))) = d;
  }
}

////////////////////////////////////////////////////////////////////////////////
//
// Timing may be nonuniform
//
// Location checks not needed
//
uint64 EMU_CALL iop_get_odometer(void *state) {
  return IOPSTATE->odometer;
}

////////////////////////////////////////////////////////////////////////////////
//
// Compatibility level
//
// Location checks not needed
//
void EMU_CALL iop_set_compat(void *state, uint8 compat) {
  IOPSTATE->compat_level = compat;
}

////////////////////////////////////////////////////////////////////////////////
//
// Useful for playing with the tempo.
// 768 is normal.
// Higher is faster; lower is slower.
//
// Location checks not needed
//
void EMU_CALL iop_set_cycles_per_sample(void *state, uint32 cycles_per_sample) {
  if(!cycles_per_sample) cycles_per_sample = 1;
  IOPSTATE->cycles_per_sample = cycles_per_sample;
}

////////////////////////////////////////////////////////////////////////////////
//
// Upload a section to RAM
//
// Location checks not needed
//
void EMU_CALL iop_upload_to_ram(void *state, uint32 address, const void *src, uint32 len) {
  while(len) {
    uint32 advance;
    address &= 0x1FFFFF;
    advance = 0x200000 - address;
    if(advance > len) { advance = len; }
#ifdef EMU_BIG_ENDIAN
    { uint32 i;
      for(i = 0; i < advance; i++) {
        (PSXRAM_BYTE_NATIVE)[(address+i)^(EMU_ENDIAN_XOR(3))] = ((char*)src)[i];
      }
    }
#else
    memcpy((PSXRAM_BYTE_NATIVE) + address, src, advance);
#endif
    src = (((const char*)src) + advance);
    len -= advance;
    address += advance;
  }

}

////////////////////////////////////////////////////////////////////////////////
