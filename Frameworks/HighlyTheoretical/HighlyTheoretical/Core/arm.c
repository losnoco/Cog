/////////////////////////////////////////////////////////////////////////////
//
// arm - Emulates ARM7DI CPU
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "arm.h"

//int armcount = 0;

//extern void subtimeon(void);
//extern void subtimeoff(void);
/*
static EMU_INLINE uint64 myrdtsc(void) {
  uint64 r;
  __asm {
    rdtsc
    mov dword ptr [r],eax
    mov dword ptr [r+4],edx
  }
  return r;
}

uint64 armsubtime = 0;
uint64 armsubtimestart = 0;

static EMU_INLINE void armsubtimeon(void) { armsubtimestart = myrdtsc(); }
static EMU_INLINE void armsubtimeoff(void) { armsubtime += myrdtsc() - armsubtimestart; }
*/
/////////////////////////////////////////////////////////////////////////////
//
// Static information
//

// rows: instruction condition field
// columns: values of nzcv
static uint8 condtable[256];

//
// initialize condtable
//
static void condtable_init(void) {
  uint8 nzcv, cond;
  uint8 *table = condtable;
  for(nzcv = 0; nzcv < 16; nzcv++) {
    for(cond = 0; cond < 16; cond++) {
      uint8 n = (nzcv >> 3) & 1;
      uint8 z = (nzcv >> 2) & 1;
      uint8 c = (nzcv >> 1) & 1;
      uint8 v = (nzcv >> 0) & 1;
      uint8 truth = 0;
      switch(cond & 0xE) {
      case 0x0: /* EQ */ truth = z; break;
      case 0x2: /* CS */ truth = c; break;
      case 0x4: /* MI */ truth = n; break;
      case 0x6: /* VS */ truth = v; break;
      case 0x8: /* HI */ truth = ((!z) && c); break;
      case 0xA: /* GE */ truth = (n == v); break;
      case 0xC: /* GT */ truth = ((!z) && (n == v)); break;
      case 0xE: /* AL */ truth = 1; break;
      }
      if(cond & 1) { truth = !truth; }
      *table++ = truth ? 1 : 0;
    }
  }
}

//
// Static init
//
sint32 EMU_CALL arm_init(void) {
//int i;
  condtable_init();

//for(i=0;i<256;i++){
//printf("%d",condtable[i]);
//if((i&15)==15)printf("\n");
//}

  return 0;
}

/////////////////////////////////////////////////////////////////////////////

#define ARMSTATE ((struct ARM_STATE*)(state))

#define MODE_USER   (0x10)
#define MODE_FIQ    (0x11)
#define MODE_IRQ    (0x12)
#define MODE_SVC    (0x13)
#define MODE_ABT    (0x17)
#define MODE_UND    (0x1B)
#define MODE_SYSTEM (0x1F)
#define MODE_MASK   (0x1F)

#define PSR_IRQMASK (0x80)
#define PSR_FIQMASK (0x40)

#define PSR_POS_N (31)
#define PSR_POS_Z (30)
#define PSR_POS_C (29)
#define PSR_POS_V (28)

#define PSR_NMASK (1<<31)
#define PSR_ZMASK (1<<30)
#define PSR_CMASK (1<<29)
#define PSR_VMASK (1<<28)

#define EXCEPTION_RESET    (0)
#define EXCEPTION_UDEF     (1)
#define EXCEPTION_SWI      (2)
#define EXCEPTION_PABT     (3)
#define EXCEPTION_DABT     (4)
#define EXCEPTION_RESERVED (5)
#define EXCEPTION_IRQ      (6)
#define EXCEPTION_FIQ      (7)

struct ARM_STATE {
  //
  // Registers
  //
  uint32 r[16];
  uint32 rfiq[7];
  uint32 rirq[2];
  uint32 rsvc[2];
  uint32 rabt[2];
  uint32 rund[2];
  uint32 cpsr; // top 4 bits: nzcv
  uint32 spsr;
  uint32 spsr_fiq;
  uint32 spsr_svc;
  uint32 spsr_abt;
  uint32 spsr_irq;
  uint32 spsr_und;

  sint32 cycles_remaining;
  sint32 cycles_remaining_last_checkpoint;

  //
  // These are REGISTERED EXTERNAL POINTERS.
  //
  arm_advance_callback_t advance;
  void *hwstate;
  struct ARM_MEMORY_MAP *map_load;
  struct ARM_MEMORY_MAP *map_store;

  //
  // The following are TEMPORARY.
  // There are no location invariance issues.
  //
  uint32 maxpc;
  void *fetchbase;
  uint32 fetchbox;

  int badinsflag;
};

uint32 EMU_CALL arm_get_state_size(void) {
  return sizeof(struct ARM_STATE);
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE void pcchanged(struct ARM_STATE *state) { state->maxpc = 0; }

/////////////////////////////////////////////////////////////////////////////
//
// Swap registers either to or from their proper places
//
static void regswap(struct ARM_STATE *state) {
  uint32 *a, *b, *s, i, n = 0, t;
  switch(state->cpsr & MODE_MASK) {
  case MODE_FIQ: a = (state->r) +  8; b = state->rfiq; n = 7; s = &(state->spsr_fiq); break; // FIQ
  case MODE_IRQ: a = (state->r) + 13; b = state->rirq; n = 2; s = &(state->spsr_irq); break; // IRQ
  case MODE_SVC: a = (state->r) + 13; b = state->rsvc; n = 2; s = &(state->spsr_svc); break; // SVC
  case MODE_ABT: a = (state->r) + 13; b = state->rabt; n = 2; s = &(state->spsr_abt); break; // ABT
  case MODE_UND: a = (state->r) + 13; b = state->rund; n = 2; s = &(state->spsr_und); break; // UND
  default: return;
  }
  for(i = 0; i < n; i++) { t = a[i]; a[i] = b[i]; b[i] = t; }
  t = state->spsr; state->spsr = *s; *s = t;
}

static uint32 getuserreg(struct ARM_STATE *state, uint32 n) {
  uint32 mode = state->cpsr & MODE_MASK;
  n &= 0xF;
  if((n < 8) || (n > 14)) return state->r[n];
  if(mode == MODE_FIQ) return state->rfiq[n - 8];
  if(n < 13) return state->r[n];
  if(mode == MODE_IRQ) return state->rirq[n - 13];
  if(mode == MODE_SVC) return state->rsvc[n - 13];
  if(mode == MODE_ABT) return state->rabt[n - 13];
  if(mode == MODE_UND) return state->rund[n - 13];
  return state->r[n];
}

static void setuserreg(struct ARM_STATE *state, uint32 n, uint32 v) {
  uint32 mode = state->cpsr & MODE_MASK;
  n &= 0xF;
  if((n < 8) || (n > 14)) { state->r[n] = v; return; }
  if(mode == MODE_FIQ) { state->rfiq[n - 8] = v; return; }
  if(n < 13) { state->r[n] = v; return; }
  if(mode == MODE_IRQ) { state->rirq[n - 13] = v; return; }
  if(mode == MODE_SVC) { state->rsvc[n - 13] = v; return; }
  if(mode == MODE_ABT) { state->rabt[n - 13] = v; return; }
  if(mode == MODE_UND) { state->rund[n - 13] = v; return; }
  state->r[n] = v; return;
}

static void setcpsr(struct ARM_STATE *state, uint32 psr) {
  regswap(state);
  state->cpsr = psr & 0xF00000FF;
  regswap(state);
}

static void exception(struct ARM_STATE *state, uint32 n) {
  static const uint32 mode_on_entry[8] = {
    MODE_SVC, MODE_UND, MODE_SVC, MODE_ABT,
    MODE_ABT,        0, MODE_IRQ, MODE_FIQ
  };
  static const uint32 lr_offset[8] = {
    0, 4, 4, 4, 8, 0, 4, 4
  };
  uint32 spsr;
  n &= 7;
  spsr = state->cpsr;
  setcpsr(state, ((state->cpsr) & (~(MODE_MASK))) | mode_on_entry[n]);
  state->spsr = spsr;
  state->r[14] = state->r[15] + lr_offset[n];
  state->r[15] = 4 * n;
  if(n == EXCEPTION_FIQ) { state->cpsr |= PSR_FIQMASK; }
  if(n == EXCEPTION_IRQ) { state->cpsr |= PSR_IRQMASK; }
}

/////////////////////////////////////////////////////////////////////////////

void EMU_CALL arm_clear_state(void *state) {
  memset(state, 0, sizeof(struct ARM_STATE));
  exception(ARMSTATE, EXCEPTION_RESET);
}

/////////////////////////////////////////////////////////////////////////////
//
// Registration of external pointers within the state
//
void EMU_CALL arm_set_memory_maps(
  void *state,
  struct ARM_MEMORY_MAP *map_load,
  struct ARM_MEMORY_MAP *map_store
) {
  ARMSTATE->map_load  = map_load;
  ARMSTATE->map_store = map_store;
}

void EMU_CALL arm_set_advance_callback(
  void *state,
  arm_advance_callback_t advance,
  void *hwstate
) {
  ARMSTATE->advance = advance;
  ARMSTATE->hwstate = hwstate;
}

/////////////////////////////////////////////////////////////////////////////

uint32 EMU_CALL arm_getreg(void *state, sint32 regnum) {
  if(regnum >= ARM_REG_GEN && regnum < (ARM_REG_GEN+16)) {
    sint32 n = regnum - ARM_REG_GEN;
    return ARMSTATE->r[n];
  }
  switch(regnum) {
  case ARM_REG_CPSR: return ARMSTATE->cpsr;
  case ARM_REG_SPSR: return ARMSTATE->spsr;
  }
  return 0;
}

// unimplemented!
//void EMU_CALL arm_setreg(void *state, sint32 regnum, uint32 value) { }

/////////////////////////////////////////////////////////////////////////////

void EMU_CALL arm_break(void *state) {
  if(ARMSTATE->cycles_remaining <= 0) return;
  ARMSTATE->cycles_remaining_last_checkpoint -= ARMSTATE->cycles_remaining;
  ARMSTATE->cycles_remaining                  = 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Memory map walker
//
static EMU_INLINE struct ARM_MEMORY_TYPE* mmwalk(struct ARM_MEMORY_MAP *map, uint32 a) {
  for(;; map++) {
    uint32 x = map->x;
    uint32 y = map->y;
    if(a < x || a > y) continue;
    return &(map->type);
  }
}

/////////////////////////////////////////////////////////////////////////////

static void hw_sync(struct ARM_STATE *state) {
  sint32 d = state->cycles_remaining_last_checkpoint - state->cycles_remaining;
  if(d > 0) { state->advance(state->hwstate, d); }
  state->cycles_remaining_last_checkpoint = state->cycles_remaining;
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE void renew_fetch_region(struct ARM_STATE *state) {
  struct ARM_MEMORY_TYPE *t;
  state->r[15] &= ~3;
  t = mmwalk(state->map_load, state->r[15]);
  if(t->n == ARM_MAP_TYPE_POINTER) {
    uint32 astart = (state->r[15]) & (~(t->mask));
    state->maxpc = astart + ((t->mask) + 1);
    state->fetchbase = ((uint8*)(t->p)) - astart;
  } else {
    state->maxpc = state->r[15] + 4;
    state->fetchbase = ((uint8*)(&(state->fetchbox)))-(state->r[15]);
    state->fetchbox = ((arm_load_callback_t)(t->p))(state->hwstate, state->r[15], 0xFFFFFFFF);
  }
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE uint32 lb(struct ARM_STATE *state, uint32 a) {
  struct ARM_MEMORY_TYPE *t;
//armsubtimeon();
  t = mmwalk(state->map_load, a);
  a &= t->mask;
  if(t->n == ARM_MAP_TYPE_POINTER) {
    a ^= EMU_ENDIAN_XOR(3);
//armsubtimeoff();
    return *((uint8*)(((uint8*)(t->p))+a));
  } else {
    uint32 sh = (a & 3) * 8;
    hw_sync(state);
    a = ((arm_load_callback_t)(t->p))(state->hwstate, a & (~3), 0xFF << sh);
    a >>= sh;
//armsubtimeoff();
    return a & 0xFF;
  }
}

static EMU_INLINE uint32 lh(struct ARM_STATE *state, uint32 a) {
  struct ARM_MEMORY_TYPE *t;
  uint32 sh;
//armsubtimeon();
  t = mmwalk(state->map_load, a);
  sh = (a & 3) * 8;
  a &= t->mask & (~3);
  if(t->n == ARM_MAP_TYPE_POINTER) {
    a = *((uint32*)(((uint8*)(t->p))+a));
  } else {
    hw_sync(state);
    a = ((arm_load_callback_t)(t->p))(state->hwstate, a, 0xFFFF << sh);
  }
  a >>= sh;
//armsubtimeoff();
  return a & 0xFFFF;
}

static EMU_INLINE uint32 lw(struct ARM_STATE *state, uint32 a) {
  struct ARM_MEMORY_TYPE *t;
  uint32 sh;
//armsubtimeon();
  t = mmwalk(state->map_load, a);
  sh = (a & 3) * 8;
  a &= t->mask & (~3);
  if(t->n == ARM_MAP_TYPE_POINTER) {
    a = *((uint32*)(((uint8*)(t->p))+a));
  } else {
    hw_sync(state);
    a = ((arm_load_callback_t)(t->p))(state->hwstate, a, 0xFFFFFFFF);
  }
//armsubtimeoff();
  return a >> sh;
}

static EMU_INLINE void sb(struct ARM_STATE *state, uint32 a, uint32 d) {
  struct ARM_MEMORY_TYPE *t;
//armsubtimeon();
  t = mmwalk(state->map_store, a);
  a &= t->mask;
  if(t->n == ARM_MAP_TYPE_POINTER) {
    a ^= EMU_ENDIAN_XOR(3);
    *((uint8*)(((uint8*)(t->p))+a)) = d;
  } else {
    uint32 sh = (a & 3) * 8;
    hw_sync(state);
    d &= 0xFF;
    ((arm_store_callback_t)(t->p))(state->hwstate, a & (~3), d << sh, 0xFF << sh);
  }
//armsubtimeoff();
}

static EMU_INLINE void sh(struct ARM_STATE *state, uint32 a, uint32 d) {
  struct ARM_MEMORY_TYPE *t;
  uint32 sh;
//armsubtimeon();
  t = mmwalk(state->map_store, a);
  sh = (a & 3) * 8;
  a &= t->mask & (~3);
  d &= 0xFFFF;
  if(t->n == ARM_MAP_TYPE_POINTER) {
    *((uint32*)(((uint8*)(t->p))+a)) &= ~(0xFFFF << sh);
    *((uint32*)(((uint8*)(t->p))+a)) |=  (d      << sh);
  } else {
    hw_sync(state);
    ((arm_store_callback_t)(t->p))(state->hwstate, a, d << sh, 0xFFFF << sh);
  }
//armsubtimeoff();
}

static EMU_INLINE void sw(struct ARM_STATE *state, uint32 a, uint32 d) {
  struct ARM_MEMORY_TYPE *t;
  uint32 sh;
//armsubtimeon();
  t = mmwalk(state->map_store, a);
  sh = (a & 3) * 8;
  a &= t->mask & (~3);
  if(t->n == ARM_MAP_TYPE_POINTER) {
    *((uint32*)(((uint8*)(t->p))+a)) &= ~(0xFFFFFFFF << sh);
    *((uint32*)(((uint8*)(t->p))+a)) |=  (d          << sh);
  } else {
    hw_sync(state);
    ((arm_store_callback_t)(t->p))(state->hwstate, a, d << sh, 0xFFFFFFFF << sh);
  }
//armsubtimeoff();
}

static EMU_INLINE uint32 fetch(struct ARM_STATE *state) {
  if(state->r[15] >= state->maxpc) renew_fetch_region(state);
  return *((uint32*)(((uint8*)(state->fetchbase))+(state->r[15])));
}

/////////////////////////////////////////////////////////////////////////////

static void EMU_CALL badins(struct ARM_STATE *state, uint32 insword) {
//armcount++;
  state->badinsflag = 1;
  arm_break(state);
}

/////////////////////////////////////////////////////////////////////////////

#define IFIELD(B,N) ((insword>>(B))&((1<<(N))-1))
#define WRITESTATUS(N) (((N)&1)!=0)
#define ISLOGIC(N) ((((N)&0x0C)==0x00)||(((N)&0x18)==0x18))

#define C_TO_CPSR { state->cpsr &= ~PSR_CMASK; state->cpsr |= (c&1) << PSR_POS_C; }
#define GET_NZ_TO_CPSR(V) { state->cpsr &= ~((PSR_NMASK)|(PSR_ZMASK)); state->cpsr |= ((V)&0x80000000) | (((uint32)((V)==0))<<(PSR_POS_Z)); }

#define DATAOP(N) (((N)>>1)&0xF)

#define DATA_AND (0x0)
#define DATA_EOR (0x1)
#define DATA_SUB (0x2)
#define DATA_RSB (0x3)
#define DATA_ADD (0x4)
#define DATA_ADC (0x5)
#define DATA_SBC (0x6)
#define DATA_RSC (0x7)
#define DATA_TST (0x8)
#define DATA_TEQ (0x9)
#define DATA_CMP (0xA)
#define DATA_CMN (0xB)
#define DATA_ORR (0xC)
#define DATA_MOV (0xD)
#define DATA_BIC (0xE)
#define DATA_MVN (0xF)

/////////////////////////////////////////////////////////////////////////////
//
// Template for a Data Processing instruction (0x00-0x3F in the list)
//
#define INSDATA(N)                                                           \
static void EMU_CALL insdata##N(struct ARM_STATE *state, uint32 insword) {   \
  uint32 v, c;                                                               \
  uint32 result, operand1, operand2;                                         \
  uint32 rd;                                                                 \
  /*                                                                 */      \
  /* Special-case check for nonstandard data processing instructions */      \
  /*                                                                 */      \
  if(!(N & 0x20)) {                                                         \
    /*               */                                                     \
    /* Multiply/swap */                                                     \
    /*               */                                                     \
    if((insword & 0xF0) == 0x90) {                                          \
      /*          */                                                        \
      /* MUL, MLA */                                                        \
      /*          */                                                        \
      if((N & 0xFC) == 0x00) {                                              \
        state->r[15] += 8;                                                  \
        rd = IFIELD(16,4);                                                  \
        result = state->r[IFIELD(0,4)] * state->r[IFIELD(8,4)];             \
        if(N & 2) result += state->r[IFIELD(12,4)];                         \
        /* set N and Z here if we want them  */                             \
        if(WRITESTATUS(N)) { GET_NZ_TO_CPSR(result); }                      \
        state->r[15] -= 4;                                                  \
        state->r[rd] = result;                                              \
        if(rd == 15) pcchanged(state);                                      \
        return;                                                             \
      /*               */                                                   \
      /* Multiply Long */                                                   \
      /*               */                                                   \
      } else if((N & 0xF8) == 0x08) {                                       \
        badins(state, insword); return;                                     \
      /*                  */                                                \
      /* Single Data Swap */                                                \
      /*                  */                                                \
      } else if((N & 0xFB) == 0x10) {                                       \
        badins(state, insword); return;                                    \
      } else {                                                              \
        badins(state, insword); return;                                     \
      }                                                                     \
    /*                     */                                               \
    /* Load/store halfword */                                               \
    /*                     */                                               \
    } else if((insword & 0x90) == 0x90) {                                   \
      badins(state, insword); return;                                         \
    }                                                                       \
  }                                                                         \
  /*                          */                               \
  /* check here for MSR / MRS */                               \
  /*                          */                               \
  if((N & 0x19) == 0x10) {                                     \
    if((insword & 0x0FFF0FFF) == 0x010F0000) {                 \
      state->r[15] += 4;                                       \
      rd = IFIELD(12, 4);                                      \
      if(rd != 15) state->r[rd] = state->cpsr;                 \
      return;                                                  \
    }                                                          \
    if((insword & 0x0FFF0FFF) == 0x014F0000) {                 \
      state->r[15] += 4;                                       \
      rd = IFIELD(12, 4);                                      \
      if(rd != 15) state->r[rd] = state->spsr;                 \
      return;                                                  \
    }                                                          \
    if((insword & 0x0FFFFFF0) == 0x0129F000) {                 \
      state->r[15] += 8;                                       \
      setcpsr(state, state->r[IFIELD(0,4)]);                   \
      state->r[15] -= 4;                                       \
      arm_break(state);                                        \
      return;                                                  \
    }                                                          \
    if((insword & 0x0FFFFFF0) == 0x0169F000) {                 \
      state->r[15] += 8;                                       \
      state->spsr = state->r[IFIELD(0,4)];                     \
      state->r[15] -= 4;                                       \
      return;                                                  \
    }                                                          \
    if((insword & 0x0FFFFFF0) == 0x0128F000) {                 \
      uint32 src;                                              \
      state->r[15] += 8;                                       \
      src = state->r[IFIELD(0,4)];                             \
      state->r[15] -= 4;                                       \
      state->cpsr &=        0x0FFFFFFF;                        \
      state->cpsr |= (src & 0xF0000000);                       \
      return;                                                  \
    }                                                          \
    if((insword & 0x0FFFFFF0) == 0x0168F000) {                 \
      uint32 src;                                              \
      state->r[15] += 8;                                       \
      src = state->r[IFIELD(0,4)];                             \
      state->r[15] -= 4;                                       \
      state->spsr &=        0x0FFFFFFF;                        \
      state->spsr |= (src & 0xF0000000);                       \
      return;                                                  \
    }                                                          \
    if((insword & 0x0FFFF000) == 0x0328F000) {                 \
      uint32 src = IFIELD(0,8);                                \
      uint32 ror = IFIELD(8,4) * 2;                            \
      src = (src >> ror) | (src << (32-ror));                  \
      state->cpsr &=        0x0FFFFFFF;                        \
      state->cpsr |= (src & 0xF0000000);                       \
      return;                                                  \
    }                                                          \
    if((insword & 0x0FFFF000) == 0x0368F000) {                 \
      uint32 src = IFIELD(0,8);                                \
      uint32 ror = IFIELD(8,4) * 2;                            \
      src = (src >> ror) | (src << (32-ror));                  \
      state->spsr &=        0x0FFFFFFF;                        \
      state->spsr |= (src & 0xF0000000);                       \
      return;                                                  \
    }                                                          \
  }                                                            \
  /*                                                         */ \
  /* from this point forward, everything is just normal data */ \
  /*                                                         */ \
  state->r[15] += 8;                                            \
  /*               */                                                       \
  /* Get operand 2 */                                                       \
  /*               */                                                       \
  if(!(N & 0x20)) {                                                         \
    operand2 = state->r[IFIELD(0,4)];                                       \
    /* shift                 */                                               \
    /* rrx is a special case */                                               \
    if((insword & 0xFF0) == 0x060) {                                        \
      c = operand2 & 1;                                                     \
      operand2 = (operand2 >> 1) | ((state->cpsr << 2) & 0x80000000);       \
      if(WRITESTATUS(N) && ISLOGIC(N)) { C_TO_CPSR; }                       \
    } else {                                                                \
      uint8 shiftby;                                                        \
      if((insword & 0x10) == 0) {                                           \
        shiftby = IFIELD(7,5);                                              \
        shiftby |= ((shiftby == 0) & ((insword & 0x60) != 0)) << 5;         \
      } else {                                                              \
        shiftby = state->r[IFIELD(8,4)];                                    \
      }                                                                     \
      if(shiftby) {                                                         \
        switch(IFIELD(5,2)) {                                               \
        case 0: /* LSL */                                                   \
          if(WRITESTATUS(N) && ISLOGIC(N)) {                                \
            if(shiftby > 32) { c = 0; } else { c = operand2 >> (32-shiftby); } \
            C_TO_CPSR;                                                         \
          }                                                                    \
          operand2 <<= shiftby;                                                \
          break;                                                               \
        case 1: /* LSR */                                                      \
          if(WRITESTATUS(N) && ISLOGIC(N)) {                                   \
            if(shiftby > 32) { c = 0; } else { c = operand2 >> (shiftby-1); }  \
            C_TO_CPSR;                                                         \
          }                                                                    \
          operand2 >>= shiftby;                                                \
          break;                                                               \
        case 2: /* ASR */                                                      \
          if(WRITESTATUS(N) && ISLOGIC(N)) {                                   \
            if(shiftby >= 32) { c = operand2 >> 31; } else { c = operand2 >> (shiftby-1); }      \
            C_TO_CPSR;                                                         \
          }                                                                    \
          operand2 = ((sint32)(((sint32)operand2) >> shiftby));                \
          break;                                                               \
        case 3: /* ROR */                                                      \
          if(WRITESTATUS(N) && ISLOGIC(N)) {                                   \
            c = operand2 >> ((shiftby-1)&31);                                  \
            C_TO_CPSR;                                                         \
          }                                                                    \
          shiftby &= 31;                                                       \
          operand2 = (operand2 >> shiftby) | (operand2 << (32-shiftby));       \
          break;                                                               \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  } else {                                                                     \
    uint32 ror = IFIELD(8,4) * 2;                                              \
    operand2 = IFIELD(0,8);                                                    \
    operand2 = (operand2 >> ror) | (operand2 << (32 - ror));                   \
  }                                                                            \
  /*                                                   */                        \
  /* Do stuff depending on the instruction             */                        \
  /*                                                   */                        \
  /* for anything except MOV and MVN, we want operand1 */                        \
  if(DATAOP(N) != DATA_MOV && DATAOP(N) != DATA_MVN) {                           \
    operand1 = state->r[IFIELD(16,4)];                                         \
  }                                                                            \
         if(DATAOP(N) == DATA_MOV) { result = operand2;                         \
  } else if(DATAOP(N) == DATA_MVN) { result = operand2 ^ 0xFFFFFFFF;            \
  } else if(DATAOP(N) == DATA_AND) { result = operand1 & operand2;              \
  } else if(DATAOP(N) == DATA_TST) { result = operand1 & operand2;              \
  } else if(DATAOP(N) == DATA_EOR) { result = operand1 ^ operand2;              \
  } else if(DATAOP(N) == DATA_TEQ) { result = operand1 ^ operand2;              \
  } else if(DATAOP(N) == DATA_ORR) { result = operand1 | operand2;              \
  } else if(DATAOP(N) == DATA_BIC) { result = operand1 & (~operand2);           \
  /*                                            */                             \
  /* (moving on to the arithmetic instructions) */                             \
  /*                                            */                             \
  /* ADD and CMN and ADC                        */                             \
  } else if(               \
    DATAOP(N)==DATA_ADD || \
    DATAOP(N)==DATA_CMN || \
    DATAOP(N)==DATA_ADC    \
  ) {       \
    result = operand1 + operand2;                                              \
    if(DATAOP(N)==DATA_ADC) result += (((state->cpsr) >> PSR_POS_C) & 1);       \
    if(WRITESTATUS(N)) {                                                       \
      v = ((operand2^result)&(~(operand1^operand2))) >> 31;                    \
      c = (result^((operand1^operand2)|(operand2^result))) >> 31;              \
      state->cpsr &= ~((PSR_CMASK)|(PSR_VMASK));                               \
      state->cpsr |= v << PSR_POS_V;                                           \
      state->cpsr |= c << PSR_POS_C;                                           \
    }                                                                          \
  /* SUB and RSB and CMP and SBC and RSC  */                                   \
  } else if(               \
    DATAOP(N)==DATA_SUB || \
    DATAOP(N)==DATA_RSB || \
    DATAOP(N)==DATA_CMP || \
    DATAOP(N)==DATA_SBC || \
    DATAOP(N)==DATA_RSC    \
  ) {                      \
    /* swap for RSB and RSC   */                                                            \
    if(DATAOP(N)==DATA_RSB || DATAOP(N)==DATA_RSC) {                 \
      result = operand1; operand1 = operand2; operand2 = result; }   \
    result = operand1 - operand2;                                                           \
    /* carry where needed */                                                                \
    if(DATAOP(N)==DATA_SBC || DATAOP(N)==DATA_RSC) {                                        \
      result += (((state->cpsr) >> PSR_POS_C) & 1);                                         \
      result--;                                                                             \
    }                                                                                       \
    if(WRITESTATUS(N)) {                                                                   \
      v = ((operand2^operand1)&(~(operand2^result))) >> 31;                                \
      c = (~(operand1^((operand2^operand1)|(operand1^result)))) >> 31;                     \
      state->cpsr &= ~((PSR_CMASK)|(PSR_VMASK));                                           \
      state->cpsr |= v << PSR_POS_V;                                                       \
      state->cpsr |= c << PSR_POS_C;                                                       \
    }                                                                                      \
  }                                                                                        \
  /* set N and Z here if we want them  */                                                  \
  if(WRITESTATUS(N)) { GET_NZ_TO_CPSR(result); }                                           \
  /* it's safe to decrement the program counter again here */                              \
  state->r[15] -= 4;                                                                       \
  /* write results to the destination register if applicable */                            \
  if((N & 0x18) != 0x10) {                                                                 \
    uint32 rd = IFIELD(12,4);                                                              \
    state->r[rd] = result;                                                                 \
    /* if the destination reg was the PC:   */                                             \
    if(rd == 15) {                                                                         \
      /* force fetch base recalculation    */                                              \
      pcchanged(state);                                                                    \
      /* and also, if S was set, return from exception  */                                 \
      if(WRITESTATUS(N)) {                                                                 \
        setcpsr(state, state->spsr);                                                       \
        arm_break(state);                                                                  \
      }                                                                                    \
    }                                                                                      \
  }                                                                                        \
}

/////////////////////////////////////////////////////////////////////////////

INSDATA(0x00) INSDATA(0x01) INSDATA(0x02) INSDATA(0x03) INSDATA(0x04) INSDATA(0x05) INSDATA(0x06) INSDATA(0x07)
INSDATA(0x08) INSDATA(0x09) INSDATA(0x0A) INSDATA(0x0B) INSDATA(0x0C) INSDATA(0x0D) INSDATA(0x0E) INSDATA(0x0F)
INSDATA(0x10) INSDATA(0x11) INSDATA(0x12) INSDATA(0x13) INSDATA(0x14) INSDATA(0x15) INSDATA(0x16) INSDATA(0x17)
INSDATA(0x18) INSDATA(0x19) INSDATA(0x1A) INSDATA(0x1B) INSDATA(0x1C) INSDATA(0x1D) INSDATA(0x1E) INSDATA(0x1F)
INSDATA(0x20) INSDATA(0x21) INSDATA(0x22) INSDATA(0x23) INSDATA(0x24) INSDATA(0x25) INSDATA(0x26) INSDATA(0x27)
INSDATA(0x28) INSDATA(0x29) INSDATA(0x2A) INSDATA(0x2B) INSDATA(0x2C) INSDATA(0x2D) INSDATA(0x2E) INSDATA(0x2F)
INSDATA(0x30) INSDATA(0x31) INSDATA(0x32) INSDATA(0x33) INSDATA(0x34) INSDATA(0x35) INSDATA(0x36) INSDATA(0x37)
INSDATA(0x38) INSDATA(0x39) INSDATA(0x3A) INSDATA(0x3B) INSDATA(0x3C) INSDATA(0x3D) INSDATA(0x3E) INSDATA(0x3F)

/////////////////////////////////////////////////////////////////////////////
//
// Template for a Single Data Transfer instruction (0x40-0x7F in the list)
//
#define SDT_I(N) (((N)>>5)&1)
#define SDT_P(N) (((N)>>4)&1)
#define SDT_U(N) (((N)>>3)&1)
#define SDT_B(N) (((N)>>2)&1)
#define SDT_W(N) (((N)>>1)&1)
#define SDT_L(N) (((N)>>0)&1)

#define INSSDT(N)                                                           \
static void EMU_CALL inssdt##N(struct ARM_STATE *state, uint32 insword) {   \
  uint32 rn = IFIELD(16,4);                                                 \
  uint32 rd = IFIELD(12,4);                                                 \
  uint32 address, offset;                                                   \
  /* PC is advanced by 8 here */                                            \
  state->r[15] += 8;                                                        \
  /* get base address */                                                    \
  address = state->r[rn];                                                   \
  /* get offset */                                                          \
  if(!(SDT_I(N))) {                                                          \
    offset = insword & 0xFFF;                                               \
  } else {                                                                  \
    offset = state->r[IFIELD(0,4)];                                         \
    /* shift                 */                                             \
    /* rrx is a special case */                                             \
    if((insword & 0xFF0) == 0x060) {                                        \
      offset = (offset >> 1) | ((state->cpsr << 2) & 0x80000000);           \
    } else {                                                                \
      uint8 shiftby = IFIELD(7,5);                                          \
      shiftby |= ((shiftby == 0) & ((insword & 0x60) != 0)) << 5;           \
      if(shiftby) {                                                         \
        switch(IFIELD(5,2)) {                                               \
        case 0: /* LSL */ offset <<= shiftby; break;                        \
        case 1: /* LSR */ offset >>= shiftby; break;                        \
        case 2: /* ASR */ offset = ((sint32)(((sint32)offset) >> shiftby)); break; \
        case 3: /* ROR */                                                   \
          shiftby &= 31;                                                    \
          offset = (offset >> shiftby) | (offset << (32-shiftby));          \
          break;                                                            \
        }                                                                   \
      }                                                                     \
    }                                                                       \
  }                                                                         \
  if(( SDT_P(N))) { if(SDT_U(N)) { address += offset; } else { address -= offset; } } \
  if(SDT_L(N)) {                                                                      \
    if(SDT_B(N)) { state->r[rd] = (uint8 )lb(state, address); }                       \
    else         { state->r[rd] = (uint32)lw(state, address); }                       \
    if(rd == 15) { state->r[15] += 4; pcchanged(state); }                             \
  } else {                                                                            \
    if(SDT_B(N)) { sb(state, address, state->r[rd] & 0xFF); }                         \
    else         { sw(state, address, state->r[rd]       ); }                         \
  }                                                                                   \
  if((!SDT_P(N))) { if(SDT_U(N)) { address += offset; } else { address -= offset; } } \
  /* remember: post implies writeback */                                              \
  if((!SDT_P(N)) || SDT_W(N)) { state->r[rn] = address; }                                \
  state->r[15] -= 4;                                                        \
}

/////////////////////////////////////////////////////////////////////////////

INSSDT(0x40) INSSDT(0x41) INSSDT(0x42) INSSDT(0x43) INSSDT(0x44) INSSDT(0x45) INSSDT(0x46) INSSDT(0x47)
INSSDT(0x48) INSSDT(0x49) INSSDT(0x4A) INSSDT(0x4B) INSSDT(0x4C) INSSDT(0x4D) INSSDT(0x4E) INSSDT(0x4F)
INSSDT(0x50) INSSDT(0x51) INSSDT(0x52) INSSDT(0x53) INSSDT(0x54) INSSDT(0x55) INSSDT(0x56) INSSDT(0x57)
INSSDT(0x58) INSSDT(0x59) INSSDT(0x5A) INSSDT(0x5B) INSSDT(0x5C) INSSDT(0x5D) INSSDT(0x5E) INSSDT(0x5F)
INSSDT(0x60) INSSDT(0x61) INSSDT(0x62) INSSDT(0x63) INSSDT(0x64) INSSDT(0x65) INSSDT(0x66) INSSDT(0x67)
INSSDT(0x68) INSSDT(0x69) INSSDT(0x6A) INSSDT(0x6B) INSSDT(0x6C) INSSDT(0x6D) INSSDT(0x6E) INSSDT(0x6F)
INSSDT(0x70) INSSDT(0x71) INSSDT(0x72) INSSDT(0x73) INSSDT(0x74) INSSDT(0x75) INSSDT(0x76) INSSDT(0x77)
INSSDT(0x78) INSSDT(0x79) INSSDT(0x7A) INSSDT(0x7B) INSSDT(0x7C) INSSDT(0x7D) INSSDT(0x7E) INSSDT(0x7F)

/////////////////////////////////////////////////////////////////////////////
//
// Template for a Block Data Transfer instruction (0x80-0x9F in the list)
//
#define BDT_P(N) (((N)>>4)&1)
#define BDT_U(N) (((N)>>3)&1)
#define BDT_S(N) (((N)>>2)&1)
#define BDT_W(N) (((N)>>1)&1)
#define BDT_L(N) (((N)>>0)&1)

#define INSBDT(N)                                                             \
static void EMU_CALL insbdt##N(struct ARM_STATE *state, uint32 insword) {     \
  uint32 address = state->r[IFIELD(16,4)];                                    \
  uint32 rfe = 0;                                                             \
  sint32 r = 0;                                                               \
  sint32 rend = 16;                                                           \
  sint32 rstep = 1;                                                           \
  /* R15 is stored as +12 - we can restore later if necessary */              \
  state->r[15] += 12;                                                         \
  /* count registers backwards if decrementing */                             \
  if(!(BDT_U(N))) { r=15; rend=-1; rstep=-1; }                                \
  for(; r!=rend; r+=rstep) if((insword>>r)&1) {                               \
    if(( BDT_P(N))) { if(BDT_U(N)) { address += 4; } else { address -= 4; } } \
    if(BDT_L(N)) {                                                            \
      if(BDT_S(N)) {                                                          \
        if((insword&0x8000)==0) { setuserreg(state, r, lw(state, address)); } \
        else { state->r[r] = lw(state, address); }                            \
      } else {                                                                \
        state->r[r] = lw(state, address);                                     \
      }                                                                       \
      /* if we just loaded the PC: */                                         \
      if(r == 15) {                                                           \
        /* adjust for it */                                                   \
        state->r[15] += 8; pcchanged(state);                                  \
        /* also if S is set, return from exception */                         \
        if(BDT_S(N)) {                                                        \
          rfe = 1;                                                            \
        }                                                                     \
      }                                                                       \
    } else {                                                                  \
      if(BDT_S(N)) { sw(state, address, getuserreg(state, r)); }              \
      else { sw(state, address, state->r[r]); }                               \
    }                                                                         \
    if((!BDT_P(N))) { if(BDT_U(N)) { address += 4; } else { address -= 4; } } \
  }                                                                           \
  if(BDT_W(N)) { state->r[IFIELD(16,4)] = address; if(IFIELD(16,4)==15) pcchanged(state); } \
  state->r[15] -= 8;                                                          \
  if(rfe) {                                                             \
    setcpsr(state, state->spsr);                                        \
    arm_break(state);                                                   \
  }                                                                     \
}

/////////////////////////////////////////////////////////////////////////////

INSBDT(0x80) INSBDT(0x81) INSBDT(0x82) INSBDT(0x83) INSBDT(0x84) INSBDT(0x85) INSBDT(0x86) INSBDT(0x87)
INSBDT(0x88) INSBDT(0x89) INSBDT(0x8A) INSBDT(0x8B) INSBDT(0x8C) INSBDT(0x8D) INSBDT(0x8E) INSBDT(0x8F)
INSBDT(0x90) INSBDT(0x91) INSBDT(0x92) INSBDT(0x93) INSBDT(0x94) INSBDT(0x95) INSBDT(0x96) INSBDT(0x97)
INSBDT(0x98) INSBDT(0x99) INSBDT(0x9A) INSBDT(0x9B) INSBDT(0x9C) INSBDT(0x9D) INSBDT(0x9E) INSBDT(0x9F)

/////////////////////////////////////////////////////////////////////////////

static void EMU_CALL insbranch(struct ARM_STATE *state, uint32 insword) {
  insword <<= 8;
  insword = ((sint32)(((sint32)insword) >> 6));
  state->r[15] += 8 + insword;
  pcchanged(state);
}

static void EMU_CALL insbranchlink(struct ARM_STATE *state, uint32 insword) {
  state->r[14] = state->r[15] + 4;
  insword <<= 8;
  insword = ((sint32)(((sint32)insword) >> 6));
  state->r[15] += 8 + insword;
  pcchanged(state);
}

/////////////////////////////////////////////////////////////////////////////

typedef void (EMU_CALL *inscallback)(struct ARM_STATE *state, uint32 insword);

static inscallback inscalltable[256] = {
// 00
  insdata0x00,insdata0x01,insdata0x02,insdata0x03,insdata0x04,insdata0x05,insdata0x06,insdata0x07,
  insdata0x08,insdata0x09,insdata0x0A,insdata0x0B,insdata0x0C,insdata0x0D,insdata0x0E,insdata0x0F,
// 10
  insdata0x10,insdata0x11,insdata0x12,insdata0x13,insdata0x14,insdata0x15,insdata0x16,insdata0x17,
  insdata0x18,insdata0x19,insdata0x1A,insdata0x1B,insdata0x1C,insdata0x1D,insdata0x1E,insdata0x1F,
// 20
  insdata0x20,insdata0x21,insdata0x22,insdata0x23,insdata0x24,insdata0x25,insdata0x26,insdata0x27,
  insdata0x28,insdata0x29,insdata0x2A,insdata0x2B,insdata0x2C,insdata0x2D,insdata0x2E,insdata0x2F,
// 30
  insdata0x30,insdata0x31,insdata0x32,insdata0x33,insdata0x34,insdata0x35,insdata0x36,insdata0x37,
  insdata0x38,insdata0x39,insdata0x3A,insdata0x3B,insdata0x3C,insdata0x3D,insdata0x3E,insdata0x3F,
// 40
  inssdt0x40,inssdt0x41,inssdt0x42,inssdt0x43,inssdt0x44,inssdt0x45,inssdt0x46,inssdt0x47,
  inssdt0x48,inssdt0x49,inssdt0x4A,inssdt0x4B,inssdt0x4C,inssdt0x4D,inssdt0x4E,inssdt0x4F,
// 50
  inssdt0x50,inssdt0x51,inssdt0x52,inssdt0x53,inssdt0x54,inssdt0x55,inssdt0x56,inssdt0x57,
  inssdt0x58,inssdt0x59,inssdt0x5A,inssdt0x5B,inssdt0x5C,inssdt0x5D,inssdt0x5E,inssdt0x5F,
// 60
  inssdt0x60,inssdt0x61,inssdt0x62,inssdt0x63,inssdt0x64,inssdt0x65,inssdt0x66,inssdt0x67,
  inssdt0x68,inssdt0x69,inssdt0x6A,inssdt0x6B,inssdt0x6C,inssdt0x6D,inssdt0x6E,inssdt0x6F,
// 70
  inssdt0x70,inssdt0x71,inssdt0x72,inssdt0x73,inssdt0x74,inssdt0x75,inssdt0x76,inssdt0x77,
  inssdt0x78,inssdt0x79,inssdt0x7A,inssdt0x7B,inssdt0x7C,inssdt0x7D,inssdt0x7E,inssdt0x7F,
// 80
  insbdt0x80,insbdt0x81,insbdt0x82,insbdt0x83,insbdt0x84,insbdt0x85,insbdt0x86,insbdt0x87,
  insbdt0x88,insbdt0x89,insbdt0x8A,insbdt0x8B,insbdt0x8C,insbdt0x8D,insbdt0x8E,insbdt0x8F,
// 90
  insbdt0x90,insbdt0x91,insbdt0x92,insbdt0x93,insbdt0x94,insbdt0x95,insbdt0x96,insbdt0x97,
  insbdt0x98,insbdt0x99,insbdt0x9A,insbdt0x9B,insbdt0x9C,insbdt0x9D,insbdt0x9E,insbdt0x9F,
// A0
  insbranch,insbranch,insbranch,insbranch,insbranch,insbranch,insbranch,insbranch,
  insbranch,insbranch,insbranch,insbranch,insbranch,insbranch,insbranch,insbranch,
// B0
  insbranchlink,insbranchlink,insbranchlink,insbranchlink,insbranchlink,insbranchlink,insbranchlink,insbranchlink,
  insbranchlink,insbranchlink,insbranchlink,insbranchlink,insbranchlink,insbranchlink,insbranchlink,insbranchlink,
// C0
  badins,badins,badins,badins,badins,badins,badins,badins,
  badins,badins,badins,badins,badins,badins,badins,badins,
// D0
  badins,badins,badins,badins,badins,badins,badins,badins,
  badins,badins,badins,badins,badins,badins,badins,badins,
// E0
  badins,badins,badins,badins,badins,badins,badins,badins,
  badins,badins,badins,badins,badins,badins,badins,badins,
// F0
  badins,badins,badins,badins,badins,badins,badins,badins,
  badins,badins,badins,badins,badins,badins,badins,badins
};

/////////////////////////////////////////////////////////////////////////////
//
// Returns 0 or positive on success
// Returns negative on error
// (value is otherwise meaningless for now)
//
sint32 EMU_CALL arm_execute(void *state, sint32 cycles, uint8 fiq) {
  uint32 instruction;
//cycles=1;
  //
  // a lot of checks are done here at the beginning.
  // and nowhere else, really, because it expects that it'll arm_break() if
  // anything groovy happens that would involve these checks
  //
//armcount=(uint32)(ARMSTATE);

//armcount++;
  //
  // check for a bad instruction (from last time around)
  //
  if(ARMSTATE->badinsflag) { return -1; }

  ARMSTATE->cycles_remaining_last_checkpoint = cycles;
  ARMSTATE->cycles_remaining = cycles;

  //
  // check for pending interrupts
  //
  // if FIQ unmasked:
  if((ARMSTATE->cpsr & PSR_FIQMASK) == 0) {
    // if there's a FIQ set and we can enter it, do so
    if(fiq) {
      exception(ARMSTATE, EXCEPTION_FIQ);
      ARMSTATE->cycles_remaining -= 2;
    }
  }

  //
  // This guarantees location invariance on fetchbox/fetchbase
  //
  ARMSTATE->maxpc = 0;

  while(ARMSTATE->cycles_remaining > 0) {
//armsubtimeon();
   // hw_sync(state);

//armcount++;

    instruction = fetch(ARMSTATE);

//armcount = (uint32)(ARMSTATE->maxpc);

//armsubtimeoff();
//printf("%08X: %08X\n",ARMSTATE->r[15], instruction);

    // instruction may be skipped due to condition
    if(!condtable[(instruction >> 28) + ((ARMSTATE->cpsr) >> 24)]) {
      ARMSTATE->r[15] += 4;
      ARMSTATE->cycles_remaining -= 2;
      continue;
    }
//armsubtimeon();
    inscalltable[(instruction>>20)&0xFF](ARMSTATE, instruction);
//armsubtimeoff();

    ARMSTATE->cycles_remaining -= 2;
  }

  //
  // finishing sync
  //
  hw_sync(state);

  if(ARMSTATE->badinsflag) { return -1; }

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
