/////////////////////////////////////////////////////////////////////////////
//
// z80 - Emulates Z80 CPU
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "z80.h"

// no 'conversion from _blah_ possible loss of data' warnings
#pragma warning (disable: 4244)

//#define TESTZ80

/////////////////////////////////////////////////////////////////////////////
//
// Static information
//
static const uint8 cycletable_normal[0x100] = {
 4,10, 7, 6, 4, 4, 7, 4, 4,11, 7, 6, 4, 4, 7, 4,
 8,10, 7, 6, 4, 4, 7, 4,12,11, 7, 6, 4, 4, 7, 4,
 7,10,16, 6, 4, 4, 7, 4, 7,11,16, 6, 4, 4, 7, 4,
 7,10,13, 6,11,11,10, 4, 7,11,13, 6, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 7, 7, 7, 7, 7, 7, 4, 7, 4, 4, 4, 4, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 4, 4, 4, 4, 4, 4, 7, 4, 4, 4, 4, 4, 4, 4, 7, 4,
 5,10,10,10,10,11, 7,11, 5,10,10, 0,10,17, 7,11,
 5,10,10,11,10,11, 7,11, 5, 4,10,11,10, 0, 7,11,
 5,10,10,19,10,11, 7,11, 5, 4,10, 4,10, 0, 7,11,
 5,10,10, 4,10,11, 7,11, 5, 6,10, 4,10, 0, 7,11};

static const uint8 cycletable_cb[0x100] = {
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
 8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
 8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
 8, 8, 8, 8, 8, 8,12, 8, 8, 8, 8, 8, 8, 8,12, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8,
 8, 8, 8, 8, 8, 8,15, 8, 8, 8, 8, 8, 8, 8,15, 8};

static const uint8 cycletable_ed[0x100] = {
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
12,12,15,20, 8, 8, 8, 9,12,12,15,20, 8, 8, 8, 9,
12,12,15,20, 8, 8, 8, 9,12,12,15,20, 8, 8, 8, 9,
12,12,15,20, 8, 8, 8,18,12,12,15,20, 8, 8, 8,18,
12,12,15,20, 8, 8, 8, 8,12,12,15,20, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
16,16,16,16, 8, 8, 8, 8,16,16,16,16, 8, 8, 8, 8,
16,16,16,16, 8, 8, 8, 8,16,16,16,16, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};

static const uint8 cycletable_xy[0x100] = {
 4, 4, 4, 4, 4, 4, 4, 4, 4,15, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4,15, 4, 4, 4, 4, 4, 4,
 4,14,20,10, 9, 9, 9, 4, 4,15,20,10, 9, 9, 9, 4,
 4, 4, 4, 4,23,23,19, 4, 4,15, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
 4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
 9, 9, 9, 9, 9, 9,19, 9, 9, 9, 9, 9, 9, 9,19, 9,
19,19,19,19,19,19, 4,19, 4, 4, 4, 4, 9, 9,19, 4,
 4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
 4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
 4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
 4, 4, 4, 4, 9, 9,19, 4, 4, 4, 4, 4, 9, 9,19, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 0, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 4,14, 4,23, 4,15, 4, 4, 4, 8, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4,10, 4, 4, 4, 4, 4, 4};

#define cycletable_dd cycletable_xy
#define cycletable_fd cycletable_xy

static const uint8 cycletable_xycb[0x100] = {
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,20,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,
23,23,23,23,23,23,23,23,23,23,23,23,23,23,23,23};

// extra cycles if jr/jp/call taken and 'interrupt latency' on rst 0-7
static const uint8 cycletable_ex[0x100] = {
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // DJNZ
 5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0,  // JR NZ/JR Z
 5, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0,  // JR NC/JR C
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 5, 5, 5, 5, 0, 0, 0, 0, 5, 5, 5, 5, 0, 0, 0, 0,  // LDIR/CPIR/INIR/OTIR LDDR/CPDR/INDR/OTDR
 6, 0, 0, 0, 7, 0, 0, 2, 6, 0, 0, 0, 7, 0, 0, 2,
 6, 0, 0, 0, 7, 0, 0, 2, 6, 0, 0, 0, 7, 0, 0, 2,
 6, 0, 0, 0, 7, 0, 0, 2, 6, 0, 0, 0, 7, 0, 0, 2,
 6, 0, 0, 0, 7, 0, 0, 2, 6, 0, 0, 0, 7, 0, 0, 2};

static uint8 SZ      [0x100];
static uint8 SZP     [0x100];
static uint8 SZHV_inc[0x100];
static uint8 SZHV_dec[0x100];

#define CF (0x01)
#define NF (0x02)
#define PF (0x04)
#define VF (PF)
#define XF (0x08)
#define HF (0x10)
#define YF (0x20)
#define ZF (0x40)
#define SF (0x80)

//
// Init: mainly just initialize the flag lookup tables
//
sint32 z80_init(void) {
  uint32 i;
  for (i = 0; i < 256; i++) {
    uint32 nbits = 0;
    if(i & 0x01) { nbits++; }
    if(i & 0x02) { nbits++; }
    if(i & 0x04) { nbits++; }
    if(i & 0x08) { nbits++; }
    if(i & 0x10) { nbits++; }
    if(i & 0x20) { nbits++; }
    if(i & 0x40) { nbits++; }
    if(i & 0x80) { nbits++; }
    SZ[i] = i ? i & SF : ZF;
    SZ[i] |= (i & (YF | XF));    /* undocumented flag bits 5+3 */
    SZP[i] = SZ[i] | ((nbits & 1) ? 0 : PF);
    SZHV_inc[i] = SZ[i];
    if(i == 0x80) { SZHV_inc[i] |= VF; }
    if((i & 0x0F) == 0x00) { SZHV_inc[i] |= HF; }
    SZHV_dec[i] = SZ[i] | NF;
    if(i == 0x7F) { SZHV_dec[i] |= VF; }
    if((i & 0x0F) == 0x0F) { SZHV_dec[i] |= HF; }
  }

//printf("z80 init\n");
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// State information
//
struct Z80_STATE {
  uint16 af;
  uint16 bc;
  uint16 de;
  uint16 hl;
  uint16 pc;
  uint16 sp;
  uint16 ix;
  uint16 iy;
  uint16 af2;
  uint16 bc2;
  uint16 de2;
  uint16 hl2;
  uint8 r;
  uint8 r2;
  uint8 i;
  uint8 irq_vector;
  uint32 imflags; // nmistate,intstate,halt,badins,iff2,iff1,im(1-0)

#define IMFLAGS_NMISTATE (0x80)
#define IMFLAGS_IRQSTATE (0x40)
#define IMFLAGS_HALT     (0x20)
#define IMFLAGS_BADINS   (0x10)
#define IMFLAGS_IFF2     (0x08)
#define IMFLAGS_IFF1     (0x04)
#define IMFLAGS_IM       (0x03)

  sint32 cycles_remaining;
  sint32 cycles_remaining_last_checkpoint;
  sint32 cycles_deferred_from_break;

  //
  // These are REGISTERED EXTERNAL POINTERS.
  //
  z80_advance_callback_t advance;
  void *hwstate;
  struct Z80_MEMORY_MAP *map_op;
  struct Z80_MEMORY_MAP *map_read;
  struct Z80_MEMORY_MAP *map_write;
  struct Z80_MEMORY_MAP *map_in;
  struct Z80_MEMORY_MAP *map_out;
};

#define STATE ((struct Z80_STATE*)(state))

/////////////////////////////////////////////////////////////////////////////
//
// TESTZ80 stuff
//
#ifdef TESTZ80

#define MAXRW (256)

static uint8 testz80_irq = 0;

static uint32 testz80_numrw = 0;
static uint32 testz80_subrw = 0;
static uint8  testz80_rwtype[MAXRW];
static uint16 testz80_rwaddr[MAXRW];
static uint8  testz80_rwdata[MAXRW];
#define TESTZ80_TYPE_READ  (0x00)
#define TESTZ80_TYPE_WRITE (0x01)
#define TESTZ80_TYPE_IN    (0x02)
#define TESTZ80_TYPE_OUT   (0x03)

static struct Z80_STATE *testz80_state = 0;

static const char *testz80_typestr[4] = { "R","W","I","O" };

static void testz80_setstate(struct Z80_STATE *mainstate) {
  testz80_state = mainstate;
}

static void testz80_newins(void) {
  testz80_numrw = 0;
  testz80_subrw = 0;
  testz80_irq = 0;
}

static void testz80_logrw(uint8 t, uint16 a, uint8 d) {
  if(testz80_numrw >= MAXRW) { *((volatile char*)0) = 0; }
  testz80_rwtype[testz80_numrw] = t;
  testz80_rwaddr[testz80_numrw] = a;
  testz80_rwdata[testz80_numrw] = d;
  testz80_numrw++;
}

static void testz80_logread (uint16 a, uint8 d) { testz80_logrw(TESTZ80_TYPE_READ , a, d); }
static void testz80_logwrite(uint16 a, uint8 d) { testz80_logrw(TESTZ80_TYPE_WRITE, a, d); }
static void testz80_login   (uint16 a, uint8 d) { testz80_logrw(TESTZ80_TYPE_IN   , a, d); }
static void testz80_logout  (uint16 a, uint8 d) { testz80_logrw(TESTZ80_TYPE_OUT  , a, d); }

uint16 testz80_getsub_af(void);
uint16 testz80_getsub_bc(void);
uint16 testz80_getsub_de(void);
uint16 testz80_getsub_hl(void);
uint16 testz80_getsub_pc(void);
uint16 testz80_getsub_sp(void);
uint16 testz80_getsub_ix(void);
uint16 testz80_getsub_iy(void);
uint16 testz80_getsub_af2(void);
uint16 testz80_getsub_bc2(void);
uint16 testz80_getsub_de2(void);
uint16 testz80_getsub_hl2(void);

static void testz80_dumprw(char *s) {
  uint32 i;
  for(i = 0; i < testz80_numrw; i++) {
    sprintf(s+strlen(s), "[%s] 0x%04X,0x%02X %s\n",
      (testz80_typestr[testz80_rwtype[i]&3]),
      testz80_rwaddr[i],
      testz80_rwdata[i],
      (testz80_subrw == i) ? "<--subrw" : ""
    );
  }
  if((testz80_numrw > 0) && (testz80_subrw >= i)) {
    sprintf(s+strlen(s), "<--subrw\n");
  }
}

static void testz80_dumpregs(char *s) {
  sprintf(s+strlen(s), "regs (main) (sub)\n");
  sprintf(s+strlen(s), "af  = %04X %04X\n", testz80_state->af , testz80_getsub_af ());
  sprintf(s+strlen(s), "bc  = %04X %04X\n", testz80_state->bc , testz80_getsub_bc ());
  sprintf(s+strlen(s), "de  = %04X %04X\n", testz80_state->de , testz80_getsub_de ());
  sprintf(s+strlen(s), "hl  = %04X %04X\n", testz80_state->hl , testz80_getsub_hl ());
  sprintf(s+strlen(s), "pc  = %04X %04X\n", testz80_state->pc , testz80_getsub_pc ());
  sprintf(s+strlen(s), "sp  = %04X %04X\n", testz80_state->sp , testz80_getsub_sp ());
  sprintf(s+strlen(s), "ix  = %04X %04X\n", testz80_state->ix , testz80_getsub_ix ());
  sprintf(s+strlen(s), "iy  = %04X %04X\n", testz80_state->iy , testz80_getsub_iy ());
  sprintf(s+strlen(s), "af2 = %04X %04X\n", testz80_state->af2, testz80_getsub_af2());
  sprintf(s+strlen(s), "bc2 = %04X %04X\n", testz80_state->bc2, testz80_getsub_bc2());
  sprintf(s+strlen(s), "de2 = %04X %04X\n", testz80_state->de2, testz80_getsub_de2());
  sprintf(s+strlen(s), "hl2 = %04X %04X\n", testz80_state->hl2, testz80_getsub_hl2());
}

//#include <windows.h>

static void testz80_errorout(const char *name) {
  char s[10000];
  uint32 i;
  strcpy(s, name);
  strcat(s,"\n");
  testz80_dumprw(s);
  testz80_dumpregs(s);

{ FILE *f=fopen("C:\\corlett\\testz80.out","wt");
fprintf(f,s);
fclose(f);
exit(1);
}
//  MessageBox(NULL,s,"testz80 error",MB_OK);
}

static void testz80_rwmismatch1(const char *name, uint16 a) {
  char s[100]; sprintf(s, "mismatch sub %s(0x%04X)", name, a); testz80_errorout(s);
}

static void testz80_rwmismatch2(const char *name, uint16 a, uint8 d) {
  char s[100]; sprintf(s, "mismatch sub %s(0x%04X,0x%02X)", name, a, d); testz80_errorout(s);
}

static void testz80_compareregs(void) {
  if(testz80_state->af  != testz80_getsub_af ()) testz80_errorout("af mismatch");
  if(testz80_state->bc  != testz80_getsub_bc ()) testz80_errorout("bc mismatch");
  if(testz80_state->de  != testz80_getsub_de ()) testz80_errorout("de mismatch");
  if(testz80_state->hl  != testz80_getsub_hl ()) testz80_errorout("hl mismatch");
  if(testz80_state->pc  != testz80_getsub_pc ()) testz80_errorout("pc mismatch");
  if(testz80_state->sp  != testz80_getsub_sp ()) testz80_errorout("sp mismatch");
  if(testz80_state->ix  != testz80_getsub_ix ()) testz80_errorout("ix mismatch");
  if(testz80_state->iy  != testz80_getsub_iy ()) testz80_errorout("iy mismatch");
  if(testz80_state->af2 != testz80_getsub_af2()) testz80_errorout("af2 mismatch");
  if(testz80_state->bc2 != testz80_getsub_bc2()) testz80_errorout("bc2 mismatch");
  if(testz80_state->de2 != testz80_getsub_de2()) testz80_errorout("de2 mismatch");
  if(testz80_state->hl2 != testz80_getsub_hl2()) testz80_errorout("hl2 mismatch");
  if(testz80_numrw != testz80_subrw) testz80_errorout("rw count mismatch");
}

uint8 testz80_subread(uint16 a) {
  if(
    (testz80_subrw >= testz80_numrw) ||
    (testz80_rwtype[testz80_subrw] != TESTZ80_TYPE_READ) ||
    (testz80_rwaddr[testz80_subrw] != a)
  ) {
    testz80_rwmismatch1("read", a);
  }
  testz80_subrw++;
  return testz80_rwdata[testz80_subrw - 1];
}

uint8 testz80_subin(uint16 a) {
  if(
    (testz80_subrw >= testz80_numrw) ||
    (testz80_rwtype[testz80_subrw] != TESTZ80_TYPE_IN) ||
    (testz80_rwaddr[testz80_subrw] != a)
  ) {
    testz80_rwmismatch1("in", a);
  }
  testz80_subrw++;
  return testz80_rwdata[testz80_subrw - 1];
}

void testz80_subwrite(uint16 a, uint8 d) {
  if(
    (testz80_subrw >= testz80_numrw) ||
    (testz80_rwtype[testz80_subrw] != TESTZ80_TYPE_WRITE) ||
    (testz80_rwaddr[testz80_subrw] != a) ||
    (testz80_rwdata[testz80_subrw] != d)
  ) {
    testz80_rwmismatch2("write", a, d);
  }
  testz80_subrw++;
}

void testz80_subout(uint16 a, uint8 d) {
  if(
    (testz80_subrw >= testz80_numrw) ||
    (testz80_rwtype[testz80_subrw] != TESTZ80_TYPE_OUT) ||
    (testz80_rwaddr[testz80_subrw] != a) ||
    (testz80_rwdata[testz80_subrw] != d)
  ) {
    testz80_rwmismatch2("out", a, d);
  }
  testz80_subrw++;
}

void testz80_subexec(void);
void testz80_subinit(void);
void testz80_subrst38h(void);

#endif

/////////////////////////////////////////////////////////////////////////////

#define OFS_LO (0^(EMU_ENDIAN_XOR_L2H(1)))
#define OFS_HI (1^(EMU_ENDIAN_XOR_L2H(1)))

#define _AF (STATE->af)
#define _BC (STATE->bc)
#define _DE (STATE->de)
#define _HL (STATE->hl)
#define _PC (STATE->pc)
#define _SP (STATE->sp)
#define _IX (STATE->ix)
#define _IY (STATE->iy)

#define _A  (((uint8*)(&(STATE->af)))[OFS_HI])
#define _F  (((uint8*)(&(STATE->af)))[OFS_LO])
#define _B  (((uint8*)(&(STATE->bc)))[OFS_HI])
#define _C  (((uint8*)(&(STATE->bc)))[OFS_LO])
#define _D  (((uint8*)(&(STATE->de)))[OFS_HI])
#define _E  (((uint8*)(&(STATE->de)))[OFS_LO])
#define _H  (((uint8*)(&(STATE->hl)))[OFS_HI])
#define _L  (((uint8*)(&(STATE->hl)))[OFS_LO])
#define _HX (((uint8*)(&(STATE->ix)))[OFS_HI])
#define _LX (((uint8*)(&(STATE->ix)))[OFS_LO])
#define _HY (((uint8*)(&(STATE->iy)))[OFS_HI])
#define _LY (((uint8*)(&(STATE->iy)))[OFS_LO])

#define _R  (STATE->r)
#define _I  (STATE->i)
#define _R2 (STATE->r2)

/////////////////////////////////////////////////////////////////////////////

uint32 EMU_CALL z80_get_state_size(void) {
  return sizeof(struct Z80_STATE);
}

void EMU_CALL z80_clear_state(void *state) {
  memset(state, 0, sizeof(struct Z80_STATE));
  _IX = 0xFFFF;
  _IY = 0xFFFF;
  _F = ZF;
#ifdef TESTZ80
testz80_subinit();
testz80_setstate(state);
#endif
}

/////////////////////////////////////////////////////////////////////////////

void EMU_CALL z80_set_memory_maps(
  void *state,
  struct Z80_MEMORY_MAP *map_op,
  struct Z80_MEMORY_MAP *map_read,
  struct Z80_MEMORY_MAP *map_write,
  struct Z80_MEMORY_MAP *map_in,
  struct Z80_MEMORY_MAP *map_out
) {
  STATE->map_op    = map_op;
  STATE->map_read  = map_read;
  STATE->map_write = map_write;
  STATE->map_in    = map_in;
  STATE->map_out   = map_out;
}

/////////////////////////////////////////////////////////////////////////////

void EMU_CALL z80_set_advance_callback(
  void *state,
  z80_advance_callback_t advance,
  void *hwstate
) {
  STATE->advance = advance;
  STATE->hwstate = hwstate;
}

/////////////////////////////////////////////////////////////////////////////

uint16 EMU_CALL z80_getpc(void *state) { return _PC; }

/////////////////////////////////////////////////////////////////////////////

void EMU_CALL z80_break(void *state) {
  if(STATE->cycles_remaining <= 0) return;
  STATE->cycles_deferred_from_break += STATE->cycles_remaining;
  STATE->cycles_remaining_last_checkpoint -= STATE->cycles_remaining;
  STATE->cycles_remaining                  = 0;
}

/////////////////////////////////////////////////////////////////////////////

void EMU_CALL z80_setirq(void *state, uint8 irq, uint8 vector) {
  if(!irq) {
    STATE->imflags &= ~(IMFLAGS_IRQSTATE);
  } else {
    STATE->imflags |= IMFLAGS_IRQSTATE;
    STATE->irq_vector = vector;
  }
  z80_break(state);
}

void EMU_CALL z80_setnmi(void *state, uint8 nmi) {
  if(!nmi) {
    STATE->imflags &= ~(IMFLAGS_NMISTATE);
  } else {
    STATE->imflags |= IMFLAGS_NMISTATE;
  }
  z80_break(state);
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE struct Z80_MEMORY_TYPE* mmwalk(
  struct Z80_MEMORY_MAP *map,
  uint16 a
) {
  for(;; map++) {
    if(a < (map->x) || a > (map->y)) continue;
    return &(map->type);
  }
}

static EMU_INLINE void hw_sync(struct Z80_STATE *state) {
  sint32 d = state->cycles_remaining_last_checkpoint - state->cycles_remaining;
  if(d > 0) {
    state->advance(state->hwstate, d);
    state->cycles_remaining_last_checkpoint = state->cycles_remaining;
  }
}

static EMU_INLINE uint8 map_readb(
  struct Z80_STATE *state,
  struct Z80_MEMORY_MAP *map,
  uint16 a
) {
  struct Z80_MEMORY_TYPE *t = mmwalk(map, a);
  a &= t->mask;
  if(t->n == Z80_MAP_TYPE_POINTER) {
    return *((uint8*)(((uint8*)(t->p))+a));
  } else {
    hw_sync(state);
    return ((z80_read_callback_t)(t->p))(state->hwstate, a);
  }
}

static EMU_INLINE void map_writeb(
  struct Z80_STATE *state,
  struct Z80_MEMORY_MAP *map,
  uint16 a,
  uint8 d
) {
  struct Z80_MEMORY_TYPE *t = mmwalk(map, a);
  a &= t->mask;
  if(t->n == Z80_MAP_TYPE_POINTER) {
    *((uint8*)(((uint8*)(t->p))+a)) = d;
  } else {
    hw_sync(state);
    ((z80_write_callback_t)(t->p))(state->hwstate, a, d);
  }
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE uint8 z80_opb(struct Z80_STATE *state, uint16 a) {
  uint8 d;
//printf("z80 op byte %04X", a); fflush(stdout);
  d = map_readb(state, state->map_op, a);
//printf(" = %02X\n", d); fflush(stdout);
#ifdef TESTZ80
testz80_logread(a, d);
#endif
  return d;
}
static EMU_INLINE uint8 z80_readb(struct Z80_STATE *state, uint16 a) {
  uint8 d;
//printf("z80 read byte %04X", a); fflush(stdout);
  d = map_readb(state, state->map_read, a);
//printf(" = %02X\n", d); fflush(stdout);
#ifdef TESTZ80
testz80_logread(a, d);
#endif
  return d;
}
static EMU_INLINE void z80_writeb(struct Z80_STATE *state, uint16 a, uint8 d) {
//printf("z80 write byte %04X = %02X\n", a, d); fflush(stdout);
  map_writeb(state, state->map_write, a, d);
#ifdef TESTZ80
testz80_logwrite(a, d);
#endif
}
static EMU_INLINE uint8 z80_inb(struct Z80_STATE *state, uint16 a) {
  uint8 d;
  d = map_readb(state, state->map_in, a);
#ifdef TESTZ80
testz80_login(a, d);
#endif
  return d;
}
static EMU_INLINE void z80_outb(struct Z80_STATE *state, uint16 a, uint8 d) {
  map_writeb(state, state->map_out, a, d);
#ifdef TESTZ80
testz80_logout(a, d);
#endif
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE uint16 z80_readw(struct Z80_STATE *state, uint16 a) {
  uint16 t;
  t  = ((uint32)(z80_readb(state, a  )));
  t |= ((uint32)(z80_readb(state, a+1))) << 8;
  return t;
}
static EMU_INLINE void z80_writew(struct Z80_STATE *state, uint16 a, uint16 d) {
  z80_writeb(state, a  , (uint8)(d   ));
  z80_writeb(state, a+1, (uint8)(d>>8));
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE uint8 z80_ropb(struct Z80_STATE *state) {
  uint16 t = _PC; _PC++; return z80_opb(state, t);
}

static EMU_INLINE uint8 z80_readpcb(struct Z80_STATE *state) {
  uint16 t = _PC; _PC++; return z80_readb(state, t);
}

static EMU_INLINE uint16 z80_readpcw(struct Z80_STATE *state) {
  uint16 t = _PC; _PC+=2; return z80_readw(state, t);
}

/////////////////////////////////////////////////////////////////////////////

#define RM(a)       z80_readb(state,a)
#define RM16(a)     z80_readw(state,a)
#define WM(a,d)     z80_writeb(state,a,d)
#define WM16(a,d)   z80_writew(state,a,d)
#define ARG()       z80_readpcb(state)
#define ARG16()     z80_readpcw(state)
#define ROP()       z80_ropb(STATE)
#define Z80IN(a)    z80_inb(state,a)
#define Z80OUT(a,d) z80_outb(state,a,d)

/////////////////////////////////////////////////////////////////////////////

#define CC(table,op) do{STATE->cycles_remaining-=cycletable_##table[(op)];}while(0)

/////////////////////////////////////////////////////////////////////////////

#define EXEC(type,op) do{         \
  uint8 t=(op);                   \
  z80_exec_##type(state,t);       \
  CC(type,t);                     \
}while(0)

#define EXEC_EA(type,op,ea) do{   \
  uint8 t=(op);                   \
  z80_exec_ea_##type(state,t,ea); \
  CC(type,t);                     \
}while(0)

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE uint16 op_pop(struct Z80_STATE *state) {
  uint16 t = RM16(_SP); _SP += 2; return t;
}
#define POP() op_pop(state)

#define PUSH(x) do{ uint16 t=(x); _SP -= 1; WM(_SP, (t>>8)); _SP -= 1; WM(_SP, (t)); }while(0)

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE void checkinterrupts(struct Z80_STATE *state) {
  // IRQ
  if(
    (state->imflags & IMFLAGS_IRQSTATE) &&
    (state->imflags & IMFLAGS_IFF1)
  ) {
#ifdef TESTZ80
testz80_irq = 1;
#endif
    if(state->imflags & IMFLAGS_HALT) { _PC++; }
    // auto-acknowledge interrupts. TODO: is this right?
    state->imflags &= ~(IMFLAGS_HALT | IMFLAGS_IFF1 | IMFLAGS_IRQSTATE);
    PUSH( _PC );
//printf("z80 interrupt(mode=%d)\n",state->imflags & IMFLAGS_IM);fflush(stdout);
    switch(state->imflags & IMFLAGS_IM) {
    case 0:
      _PC = state->irq_vector & 0x38;
      CC(normal,0xFF);
      break;
    case 1:
      _PC = 0x38;
      CC(normal,0xFF);
      break;
    case 2:
      _PC = RM16(
        (((uint32)(state->irq_vector))     ) |
        (((uint32)(state->i         )) << 8)
      );
      CC(normal,0xCD);
      break;
    }
  }
  // NMI
  if(state->imflags & IMFLAGS_NMISTATE) {
    if(state->imflags & IMFLAGS_HALT) { _PC++; }
    state->imflags &= ~(IMFLAGS_HALT | IMFLAGS_IFF1 | IMFLAGS_NMISTATE);
    PUSH( _PC );
    _PC = 0x0066;
    CC(normal,0xFF);
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// TODO: perhaps there's some latency after EI before interrupts really are
// enabled
//
#define EI do{state->imflags|= (IMFLAGS_IFF1|IMFLAGS_IFF2);checkinterrupts(state);}while(0)
#define DI do{state->imflags&=~(IMFLAGS_IFF1|IMFLAGS_IFF2);}while(0)
#define IM(x) do{state->imflags&=~(IMFLAGS_IM);state->imflags|=(x);}while(0)

#define RETN do{                          \
  _PC = POP();                            \
  if(state->imflags & IMFLAGS_IFF2) {     \
    state->imflags |= IMFLAGS_IFF1;       \
    checkinterrupts(state);               \
  } else {                                \
    state->imflags &= ~(IMFLAGS_IFF1);    \
  }                                       \
}while(0)

#define RETI do{                          \
  _PC = POP();                            \
  if(state->imflags & IMFLAGS_IFF2) {     \
    state->imflags |= IMFLAGS_IFF1;       \
    checkinterrupts(state);               \
  } else {                                \
    state->imflags &= ~(IMFLAGS_IFF1);    \
  }                                       \
}while(0)

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE uint8 op_inc(struct Z80_STATE *state, uint8 value) {
  uint8 res = value + 1;
  _F = (_F & CF) | SZHV_inc[res];
  return res;
}
#define INC(d) op_inc(state,d)

static EMU_INLINE uint8 op_dec(struct Z80_STATE *state, uint8 value) {
  uint8 res = value - 1;
  _F = (_F & CF) | SZHV_dec[res];
  return res;
}
#define DEC(d) op_dec(state,d)

#define RLCA do{                                      \
  _A = (_A << 1) | (_A >> 7);                         \
  _F = (_F & (SF | ZF | PF)) | (_A & (YF | XF | CF)); \
}while(0)

#define RRCA do{                                      \
  _F = (_F & (SF | ZF | PF)) | (_A & CF);             \
  _A = (_A >> 1) | (_A << 7);                         \
  _F |= (_A & (YF | XF) );                            \
}while(0)

#define RLA do{                                       \
  uint8 res = (_A << 1) | (_F & CF);                  \
  uint8 c = (_A & 0x80) ? CF : 0;                     \
  _F = (_F & (SF | ZF | PF)) | c | (res & (YF | XF)); \
  _A = res;                                           \
}while(0)

#define RRA do{                                       \
  uint8 res = (_A >> 1) | (_F << 7);                  \
  uint8 c = (_A & 0x01) ? CF : 0;                     \
  _F = (_F & (SF | ZF | PF)) | c | (res & (YF | XF)); \
  _A = res;                                           \
}while(0)

#define RRD do{                          \
  uint8 n = RM(_HL);                      \
  WM( _HL, (n >> 4) | (_A << 4) );              \
  _A = (_A & 0xf0) | (n & 0x0f);                \
  _F = (_F & CF) | SZP[_A];                  \
}while(0)

#define RLD do{                          \
  uint8 n = RM(_HL);                      \
  WM( _HL, (n << 4) | (_A & 0x0f) );              \
  _A = (_A & 0xf0) | (n >> 4);                \
  _F = (_F & CF) | SZP[_A];                  \
}while(0)

#define ADD(value) do{                                \
  uint32 val = (value);                               \
  uint32 res = ((uint32)_A) + val;                  \
  _F = SZ[(uint8)res] | ((res >> 8) & CF) |           \
    ((_A ^ res ^ val) & HF) |                         \
    (((val ^ _A ^ 0x80) & (val ^ res) & 0x80) >> 5);  \
  _A = (uint8)res;                                    \
}while(0)

#define ADC(value) do{                               \
  uint32 val = (value);                              \
  uint32 res = ((uint32)_A) + val + ((uint32)(_F & CF)); \
  _F = SZ[res & 0xff] | ((res >> 8) & CF) |          \
    ((_A ^ res ^ val) & HF) |                        \
    (((val ^ _A ^ 0x80) & (val ^ res) & 0x80) >> 5); \
  _A = res;                                          \
}while(0)

#define SUB(value) do{                               \
  uint32 val = (value);                              \
  uint32 res = ((uint32)_A) - val;                   \
  _F = SZ[res & 0xff] | ((res >> 8) & CF) | NF |     \
    ((_A ^ res ^ val) & HF) |                        \
    (((val ^ _A) & (_A ^ res) & 0x80) >> 5);         \
  _A = res;                                          \
}while(0)

#define SBC(value) do{                               \
  uint32 val = (value);                              \
  uint32 res = ((uint32)_A) - val - ((uint32)(_F & CF)); \
  _F = SZ[res & 0xff] | ((res >> 8) & CF) | NF |     \
    ((_A ^ res ^ val) & HF) |                        \
    (((val ^ _A) & (_A ^ res) & 0x80) >> 5);         \
  _A = res;                                          \
}while(0)

#define NEG do{                          \
  uint8 value = _A;                      \
  _A = 0;                                \
  SUB(value);                            \
}while(0)

#define DAA do{                          \
  uint8 cf, nf, hf, lo, hi, diff;                \
  cf = _F & CF;                        \
  nf = _F & NF;                        \
  hf = _F & HF;                        \
  lo = _A & 15;                        \
  hi = _A / 16;                        \
                                \
  if (cf)                            \
  {                              \
    diff = (lo <= 9 && !hf) ? 0x60 : 0x66;          \
  }                              \
  else                            \
  {                              \
    if (lo >= 10)                      \
    {                            \
      diff = hi <= 8 ? 0x06 : 0x66;            \
    }                            \
    else                          \
    {                            \
      if (hi >= 10)                    \
      {                          \
        diff = hf ? 0x66 : 0x60;            \
      }                          \
      else                        \
      {                          \
        diff = hf ? 0x06 : 0x00;            \
      }                          \
    }                            \
  }                              \
  if (nf) _A -= diff;                      \
  else _A += diff;                      \
                                \
  _F = SZP[_A] | (_F & NF);                  \
  if (cf || (lo <= 9 ? hi >= 10 : hi >= 9)) _F |= CF;      \
  if (nf ? hf && lo <= 5 : lo >= 10)  _F |= HF;        \
}while(0)

#define AND(value) do{ _A &= (value); _F = SZP[_A] | HF; }while(0)
#define OR(value)  do{ _A |= (value); _F = SZP[_A]; }while(0)
#define XOR(value) do{ _A ^= (value); _F = SZP[_A]; }while(0)

#define CP(value) do{                          \
  uint32 val = (value);                     \
  uint32 res = ((uint32)_A) - val;                  \
  _F = (SZ[res & 0xff] & (SF | ZF)) |              \
    (val & (YF | XF)) | ((res >> 8) & CF) | NF |      \
    ((_A ^ res ^ val) & HF) |                \
    ((((val ^ _A) & (_A ^ res)) >> 5) & VF);        \
}while(0)

#define EX_AF    do{uint16 t=state->af;state->af=state->af2;state->af2=t;}while(0)
#define EX_DE_HL do{uint16 t=state->de;state->de=state->hl ;state->hl =t;}while(0)
#define EXX do{ uint16 t;                        \
  t=state->bc;state->bc=state->bc2;state->bc2=t; \
  t=state->de;state->de=state->de2;state->de2=t; \
  t=state->hl;state->hl=state->hl2;state->hl2=t; \
}while(0)

static EMU_INLINE uint16 op_exsp16(struct Z80_STATE *state,uint16 x) {
  uint16 t = RM16(_SP); WM16(_SP,x); return t;
}
#define EXSP16(x) op_exsp16(state,x)

#define ADD16(DR,SR) do{           \
  uint32 res = ((uint32)(DR))+((uint32)(SR));      \
  _F = (_F & (SF | ZF | VF)) |     \
    (((DR ^ res ^ SR) >> 8) & HF) | \
    ((res >> 16) & CF) | ((res >> 8) & (YF | XF)); \
  DR = res;                        \
}while(0)

#define ADC16(Reg) do{                     \
  uint32 res = \
    ((uint32)(_HL)) + \
    ((uint32)(Reg)) + \
    ((uint32)(_F & CF));      \
  _F = (((_HL ^ res ^ Reg) >> 8) & HF) |   \
    ((res >> 16) & CF) |                   \
    ((res >> 8) & (SF | YF | XF)) |        \
    ((res & 0xFFFF) ? 0 : ZF) |            \
    (((Reg ^ _HL ^ 0x8000) & (Reg ^ res) & 0x8000) >> 13); \
  _HL = res;                               \
}while(0)

#define SBC16(Reg) do{                         \
  uint32 res = \
    ((uint32)(_HL)) - \
    ((uint32)(Reg)) - \
    ((uint32)(_F & CF)); \
/* printf("SBC HL=%04X," #Reg "=%04X-F=%02X -> %08X\n",_HL,Reg,_F,res); */ \
  _F = (((_HL ^ res ^ Reg) >> 8) & HF) | NF |  \
    ((res >> 16) & CF) |                       \
    ((res >> 8) & (SF | YF | XF)) |            \
    ((res & 0xFFFF) ? 0 : ZF) |                \
    (((Reg ^ _HL) & (_HL ^ res) &0x8000) >> 13); \
/* printf("_F=%02X\n",_F); */ \
  _HL = res;                                   \
}while(0)

static EMU_INLINE uint8 op_rlc(struct Z80_STATE *state, uint8 value) {
  uint32 res = value;
  uint32 c = (res & 0x80) ? CF : 0;
  res = ((res << 1) | (res >> 7)) & 0xff;
  _F = SZP[res] | c;
  return res;
}
#define RLC(d) op_rlc(state,d)

static EMU_INLINE uint8 op_rrc(struct Z80_STATE *state, uint8 value) {
  uint32 res = value;
  uint32 c = (res & 0x01) ? CF : 0;
  res = ((res >> 1) | (res << 7)) & 0xff;
  _F = SZP[res] | c;
  return res;
}
#define RRC(d) op_rrc(state,d)

static EMU_INLINE uint8 op_rl(struct Z80_STATE *state, uint8 value) {
  uint32 res = value;
  uint32 c = (res & 0x80) ? CF : 0;
  res = ((res << 1) | (_F & CF)) & 0xff;
  _F = SZP[res] | c;
  return res;
}
#define RL(d) op_rl(state,d)

static EMU_INLINE uint8 op_rr(struct Z80_STATE *state, uint8 value) {
  uint32 res = value;
  uint32 c = (res & 0x01) ? CF : 0;
  res = ((res >> 1) | (_F << 7)) & 0xff;
  _F = SZP[res] | c;
  return res;
}
#define RR(d) op_rr(state,d)

static EMU_INLINE uint8 op_sla(struct Z80_STATE *state, uint8 value) {
  uint32 res = value;
  uint32 c = (res & 0x80) ? CF : 0;
  res = (res << 1) & 0xff;
  _F = SZP[res] | c;
  return res;
}
#define SLA(d) op_sla(state,d)

static EMU_INLINE uint8 op_sra(struct Z80_STATE *state, uint8 value) {
  uint32 res = value;
  uint32 c = (res & 0x01) ? CF : 0;
  res = ((res >> 1) | (res & 0x80)) & 0xff;
  _F = SZP[res] | c;
  return res;
}
#define SRA(d) op_sra(state,d)

static EMU_INLINE uint8 op_sll(struct Z80_STATE *state, uint8 value) {
  uint32 res = value;
  uint32 c = (res & 0x80) ? CF : 0;
  res = ((res << 1) | 0x01) & 0xff;
  _F = SZP[res] | c;
  return res;
}
#define SLL(d) op_sll(state,d)

static EMU_INLINE uint8 op_srl(struct Z80_STATE *state, uint8 value) {
  uint32 res = value;
  uint32 c = (res & 0x01) ? CF : 0;
  res = (res >> 1) & 0xff;
  _F = SZP[res] | c;
  return res;
}
#define SRL(d) op_srl(state,d)

#define BIT(bit,reg) do{ _F = (_F & CF) | HF | SZP[(reg) & (1<<(bit))]; }while(0)
//#define BIT(bit,reg) do{ _F = (_F & CF) | HF | SZP[(reg) & (1<<(bit))]; _F&=~(YF|XF);_F|=(reg)&(YF|XF);}while(0)
#define BIT_XY(bit,reg) do{ _F = (_F & CF) | HF | (SZP[(reg) & (1<<(bit))] & ~(YF|XF)) | ((EA>>8) & (YF|XF)); }while(0)

static EMU_INLINE uint8 RES(uint8 bit, uint8 value) { return value & ~(1<<bit); }
static EMU_INLINE uint8 SET(uint8 bit, uint8 value) { return value | (1<<bit); }

#define LDI do{                          \
  uint8 io = RM(_HL);                      \
  WM( _DE, io );                        \
/* printf("z80 ldi (%04X->%04X)\n",_HL,_DE); */ \
  _F &= SF | ZF | CF;                      \
  io += _A;                                \
  if(io & 0x02) _F |= YF; /* bit 1 -> flag 5 */    \
  if(io & 0x08) _F |= XF; /* bit 3 -> flag 3 */    \
  _HL++; _DE++; _BC--;                    \
  if( _BC ) _F |= VF;                      \
}while(0)

#define CPI do{                          \
  uint8 val = RM(_HL);                    \
  uint8 res = _A - val;                    \
  _HL++; _BC--;                        \
  _F = (_F & CF) | (SZ[res] & ~(YF|XF)) | ((_A ^ val ^ res) & HF) | NF;  \
  if( _F & HF ) res -= 1;                    \
  if(res & 0x02) _F |= YF; /* bit 1 -> flag 5 */      \
  if(res & 0x08) _F |= XF; /* bit 3 -> flag 3 */      \
  if( _BC ) _F |= VF;                      \
}while(0)

#define INI do{                          \
  uint32 t;                          \
  uint8 io = Z80IN(_BC);                      \
  _B--;                            \
  WM( _HL, io );                        \
  _HL++;                            \
  _F = SZ[_B];                        \
  t = (uint32)((_C + 1) & 0xff) + (uint32)io;        \
  if( io & SF ) _F |= NF;                    \
  if( t & 0x100 ) _F |= HF | CF;                \
  _F |= SZP[(uint8)(t & 0x07) ^ _B] & PF;            \
}while(0)

#define OUTI do{                          \
  uint32 t;                          \
  uint8 io = RM(_HL);                      \
  _B--;                            \
  Z80OUT( _BC, io );                        \
  _HL++;                            \
  _F = SZ[_B];                        \
  t = (uint32)_L + (uint32)io;              \
  if( io & SF ) _F |= NF;                    \
  if( t & 0x100 ) _F |= HF | CF;                \
  _F |= SZP[(uint8)(t & 0x07) ^ _B] & PF;            \
}while(0)

#define LDD do{                          \
  uint8 io = RM(_HL);                      \
  WM( _DE, io );                        \
  _F &= SF | ZF | CF;                      \
  if( (_A + io) & 0x02 ) _F |= YF; /* bit 1 -> flag 5 */    \
  if( (_A + io) & 0x08 ) _F |= XF; /* bit 3 -> flag 3 */    \
  _HL--; _DE--; _BC--;                    \
  if( _BC ) _F |= VF;                      \
}while(0)

#define CPD do{                          \
  uint8 val = RM(_HL);                    \
  uint8 res = _A - val;                    \
  _HL--; _BC--;                        \
  _F = (_F & CF) | (SZ[res] & ~(YF|XF)) | ((_A ^ val ^ res) & HF) | NF;  \
  if( _F & HF ) res -= 1;                    \
  if( res & 0x02 ) _F |= YF; /* bit 1 -> flag 5 */      \
  if( res & 0x08 ) _F |= XF; /* bit 3 -> flag 3 */      \
  if( _BC ) _F |= VF;                      \
}while(0)

#define IND do{                          \
  uint32 t;                          \
  uint8 io = Z80IN(_BC);                      \
  _B--;                            \
  WM( _HL, io );                        \
  _HL--;                            \
  _F = SZ[_B];                        \
  t = ((uint32)(_C - 1) & 0xff) + (uint32)io;        \
  if( io & SF ) _F |= NF;                    \
  if( t & 0x100 ) _F |= HF | CF;                \
  _F |= SZP[(uint8)(t & 0x07) ^ _B] & PF;            \
}while(0)

#define OUTD do{                          \
  uint32 t;                          \
  uint8 io = RM(_HL);                      \
  _B--;                            \
  Z80OUT( _BC, io );                        \
  _HL--;                            \
  _F = SZ[_B];                        \
  t = (uint32)_L + (uint32)io;              \
  if( io & SF ) _F |= NF;                    \
  if( t & 0x100 ) _F |= HF | CF;                \
  _F |= SZP[(uint8)(t & 0x07) ^ _B] & PF;            \
}while(0)

#define LDIR do{                          \
  LDI;                            \
  if( _BC ) {                              \
    _PC -= 2;                        \
    CC(ex,0xb0);                      \
  } \
}while(0)

#define CPIR do{                          \
  CPI;                            \
  if( _BC && !(_F & ZF) ) {                              \
    _PC -= 2;                        \
    CC(ex,0xb1);                      \
  } \
}while(0)

#define INIR do{                         \
  INI;                            \
  if( _B ) {                              \
    _PC -= 2;                        \
    CC(ex,0xb2);                      \
  } \
}while(0)

#define OTIR do{                          \
  OUTI;                            \
  if( _B ) {                              \
    _PC -= 2;                        \
    CC(ex,0xb3);                      \
  } \
}while(0)

#define LDDR do{                          \
  LDD;                            \
  if( _BC ) {                              \
    _PC -= 2;                        \
    CC(ex,0xb8);                      \
  } \
}while(0)

#define CPDR do{                          \
  CPD;                            \
  if( _BC && !(_F & ZF) ) {                              \
    _PC -= 2;                        \
    CC(ex,0xb9);                      \
  } \
}while(0)

#define INDR do{                          \
  IND;                            \
  if( _B ) {                              \
    _PC -= 2;                        \
    CC(ex,0xba);                      \
  } \
}while(0)

#define OTDR do{                          \
  OUTD;                            \
  if( _B ) {                              \
    _PC -= 2;                        \
    CC(ex,0xbb);                      \
  } \
}while(0)

/////////////////////////////////////////////////////////////////////////////

#define EAX do{EA=((uint32)((uint16)(_IX+((sint8)ARG()))));}while(0)
#define EAY do{EA=((uint32)((uint16)(_IY+((sint8)ARG()))));}while(0)

#define JP do{_PC=ARG16();}while(0)
#define JP_COND(cond) do{if(cond){JP;}else{_PC+=2;}}while(0)

#define JR do{sint16 o=((sint16)((sint8)(ARG())));_PC+=o;}while(0)
#define JR_COND(cond,op) do{if(cond){JR;CC(ex,op);}else{_PC+=1;}}while(0)

#define CALL do{ uint16 t=ARG16(); PUSH(_PC); _PC = t; }while(0)
#define CALL_COND(cond,op) do{if(cond){CALL;CC(ex,op);}else{_PC+=2;}}while(0)

#define RST(addr) do{ PUSH( _PC ); _PC = (addr); }while(0)

#define RET_COND(cond,op) do{if(cond){_PC=POP();CC(ex,op);}}while(0)

#define LD_R_A do{                        \
  _R = _A;                          \
  _R2 = _A & 0x80;        /* keep bit 7 of R */    \
}while(0)

#define LD_A_R do{                        \
  _A = (_R & 0x7f) | _R2;                    \
  _F = (_F & CF) | SZ[_A];          \
  if(state->imflags & IMFLAGS_IFF2) _F |= (1<<2);  \
}while(0)

#define LD_I_A do{                        \
  _I = _A;                          \
}while(0)

#define LD_A_I do{                        \
  _A = _I;                          \
  _F = (_F & CF) | SZ[_A]; \
  if(state->imflags & IMFLAGS_IFF2) _F |= (1<<2);  \
}while(0)

/////////////////////////////////////////////////////////////////////////////

#define illegal_1() do{ state->imflags = IMFLAGS_BADINS; z80_break(state); }while(0)
#define illegal_2() do{ state->imflags = IMFLAGS_BADINS; z80_break(state); }while(0)

#define HALT do{                  \
  state->imflags |= IMFLAGS_HALT; \
  state->cycles_remaining = 0;    \
}while(0)

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE void z80_exec_cb(struct Z80_STATE *state, uint8 op) {

//printf("PC=%04X exec cb %02X\n",_PC, op);fflush(stdout);

  switch(op) {
  case /*CB*/0x00: _B = RLC(_B);                      break; // RLC  B
  case /*CB*/0x01: _C = RLC(_C);                      break; // RLC  C
  case /*CB*/0x02: _D = RLC(_D);                      break; // RLC  D
  case /*CB*/0x03: _E = RLC(_E);                      break; // RLC  E
  case /*CB*/0x04: _H = RLC(_H);                      break; // RLC  H
  case /*CB*/0x05: _L = RLC(_L);                      break; // RLC  L
  case /*CB*/0x06: WM( _HL, RLC(RM(_HL)) );           break; // RLC  (HL)
  case /*CB*/0x07: _A = RLC(_A);                      break; // RLC  A

  case /*CB*/0x08: _B = RRC(_B);                      break; // RRC  B
  case /*CB*/0x09: _C = RRC(_C);                      break; // RRC  C
  case /*CB*/0x0a: _D = RRC(_D);                      break; // RRC  D
  case /*CB*/0x0b: _E = RRC(_E);                      break; // RRC  E
  case /*CB*/0x0c: _H = RRC(_H);                      break; // RRC  H
  case /*CB*/0x0d: _L = RRC(_L);                      break; // RRC  L
  case /*CB*/0x0e: WM( _HL, RRC(RM(_HL)) );           break; // RRC  (HL)
  case /*CB*/0x0f: _A = RRC(_A);                      break; // RRC  A

  case /*CB*/0x10: _B = RL(_B);                      break; // RL   B
  case /*CB*/0x11: _C = RL(_C);                      break; // RL   C
  case /*CB*/0x12: _D = RL(_D);                      break; // RL   D
  case /*CB*/0x13: _E = RL(_E);                      break; // RL   E
  case /*CB*/0x14: _H = RL(_H);                      break; // RL   H
  case /*CB*/0x15: _L = RL(_L);                      break; // RL   L
  case /*CB*/0x16: WM( _HL, RL(RM(_HL)) );           break; // RL   (HL)
  case /*CB*/0x17: _A = RL(_A);                      break; // RL   A

  case /*CB*/0x18: _B = RR(_B);                      break; // RR   B
  case /*CB*/0x19: _C = RR(_C);                      break; // RR   C
  case /*CB*/0x1a: _D = RR(_D);                      break; // RR   D
  case /*CB*/0x1b: _E = RR(_E);                      break; // RR   E
  case /*CB*/0x1c: _H = RR(_H);                      break; // RR   H
  case /*CB*/0x1d: _L = RR(_L);                      break; // RR   L
  case /*CB*/0x1e: WM( _HL, RR(RM(_HL)) );           break; // RR   (HL)
  case /*CB*/0x1f: _A = RR(_A);                      break; // RR   A

  case /*CB*/0x20: _B = SLA(_B);                      break; // SLA  B
  case /*CB*/0x21: _C = SLA(_C);                      break; // SLA  C
  case /*CB*/0x22: _D = SLA(_D);                      break; // SLA  D
  case /*CB*/0x23: _E = SLA(_E);                      break; // SLA  E
  case /*CB*/0x24: _H = SLA(_H);                      break; // SLA  H
  case /*CB*/0x25: _L = SLA(_L);                      break; // SLA  L
  case /*CB*/0x26: WM( _HL, SLA(RM(_HL)) );           break; // SLA  (HL)
  case /*CB*/0x27: _A = SLA(_A);                      break; // SLA  A

  case /*CB*/0x28: _B = SRA(_B);                      break; // SRA  B
  case /*CB*/0x29: _C = SRA(_C);                      break; // SRA  C
  case /*CB*/0x2a: _D = SRA(_D);                      break; // SRA  D
  case /*CB*/0x2b: _E = SRA(_E);                      break; // SRA  E
  case /*CB*/0x2c: _H = SRA(_H);                      break; // SRA  H
  case /*CB*/0x2d: _L = SRA(_L);                      break; // SRA  L
  case /*CB*/0x2e: WM( _HL, SRA(RM(_HL)) );           break; // SRA  (HL)
  case /*CB*/0x2f: _A = SRA(_A);                      break; // SRA  A

  case /*CB*/0x30: _B = SLL(_B);                      break; // SLL  B
  case /*CB*/0x31: _C = SLL(_C);                      break; // SLL  C
  case /*CB*/0x32: _D = SLL(_D);                      break; // SLL  D
  case /*CB*/0x33: _E = SLL(_E);                      break; // SLL  E
  case /*CB*/0x34: _H = SLL(_H);                      break; // SLL  H
  case /*CB*/0x35: _L = SLL(_L);                      break; // SLL  L
  case /*CB*/0x36: WM( _HL, SLL(RM(_HL)) );           break; // SLL  (HL)
  case /*CB*/0x37: _A = SLL(_A);                      break; // SLL  A

  case /*CB*/0x38: _B = SRL(_B);                      break; // SRL  B
  case /*CB*/0x39: _C = SRL(_C);                      break; // SRL  C
  case /*CB*/0x3a: _D = SRL(_D);                      break; // SRL  D
  case /*CB*/0x3b: _E = SRL(_E);                      break; // SRL  E
  case /*CB*/0x3c: _H = SRL(_H);                      break; // SRL  H
  case /*CB*/0x3d: _L = SRL(_L);                      break; // SRL  L
  case /*CB*/0x3e: WM( _HL, SRL(RM(_HL)) );           break; // SRL  (HL)
  case /*CB*/0x3f: _A = SRL(_A);                      break; // SRL  A

  case /*CB*/0x40: BIT(0,_B);                        break; // BIT  0,B
  case /*CB*/0x41: BIT(0,_C);                        break; // BIT  0,C
  case /*CB*/0x42: BIT(0,_D);                        break; // BIT  0,D
  case /*CB*/0x43: BIT(0,_E);                        break; // BIT  0,E
  case /*CB*/0x44: BIT(0,_H);                        break; // BIT  0,H
  case /*CB*/0x45: BIT(0,_L);                        break; // BIT  0,L
  case /*CB*/0x46: BIT(0,RM(_HL));                   break; // BIT  0,(HL)
  case /*CB*/0x47: BIT(0,_A);                        break; // BIT  0,A

  case /*CB*/0x48: BIT(1,_B);                        break; // BIT  1,B
  case /*CB*/0x49: BIT(1,_C);                        break; // BIT  1,C
  case /*CB*/0x4a: BIT(1,_D);                        break; // BIT  1,D
  case /*CB*/0x4b: BIT(1,_E);                        break; // BIT  1,E
  case /*CB*/0x4c: BIT(1,_H);                        break; // BIT  1,H
  case /*CB*/0x4d: BIT(1,_L);                        break; // BIT  1,L
  case /*CB*/0x4e: BIT(1,RM(_HL));                   break; // BIT  1,(HL)
  case /*CB*/0x4f: BIT(1,_A);                        break; // BIT  1,A

  case /*CB*/0x50: BIT(2,_B);                        break; // BIT  2,B
  case /*CB*/0x51: BIT(2,_C);                        break; // BIT  2,C
  case /*CB*/0x52: BIT(2,_D);                        break; // BIT  2,D
  case /*CB*/0x53: BIT(2,_E);                        break; // BIT  2,E
  case /*CB*/0x54: BIT(2,_H);                        break; // BIT  2,H
  case /*CB*/0x55: BIT(2,_L);                        break; // BIT  2,L
  case /*CB*/0x56: BIT(2,RM(_HL));                   break; // BIT  2,(HL)
  case /*CB*/0x57: BIT(2,_A);                        break; // BIT  2,A

  case /*CB*/0x58: BIT(3,_B);                        break; // BIT  3,B
  case /*CB*/0x59: BIT(3,_C);                        break; // BIT  3,C
  case /*CB*/0x5a: BIT(3,_D);                        break; // BIT  3,D
  case /*CB*/0x5b: BIT(3,_E);                        break; // BIT  3,E
  case /*CB*/0x5c: BIT(3,_H);                        break; // BIT  3,H
  case /*CB*/0x5d: BIT(3,_L);                        break; // BIT  3,L
  case /*CB*/0x5e: BIT(3,RM(_HL));                   break; // BIT  3,(HL)
  case /*CB*/0x5f: BIT(3,_A);                        break; // BIT  3,A

  case /*CB*/0x60: BIT(4,_B);                        break; // BIT  4,B
  case /*CB*/0x61: BIT(4,_C);                        break; // BIT  4,C
  case /*CB*/0x62: BIT(4,_D);                        break; // BIT  4,D
  case /*CB*/0x63: BIT(4,_E);                        break; // BIT  4,E
  case /*CB*/0x64: BIT(4,_H);                        break; // BIT  4,H
  case /*CB*/0x65: BIT(4,_L);                        break; // BIT  4,L
  case /*CB*/0x66: BIT(4,RM(_HL));                   break; // BIT  4,(HL)
  case /*CB*/0x67: BIT(4,_A);                        break; // BIT  4,A

  case /*CB*/0x68: BIT(5,_B);                        break; // BIT  5,B
  case /*CB*/0x69: BIT(5,_C);                        break; // BIT  5,C
  case /*CB*/0x6a: BIT(5,_D);                        break; // BIT  5,D
  case /*CB*/0x6b: BIT(5,_E);                        break; // BIT  5,E
  case /*CB*/0x6c: BIT(5,_H);                        break; // BIT  5,H
  case /*CB*/0x6d: BIT(5,_L);                        break; // BIT  5,L
  case /*CB*/0x6e: BIT(5,RM(_HL));                   break; // BIT  5,(HL)
  case /*CB*/0x6f: BIT(5,_A);                        break; // BIT  5,A

  case /*CB*/0x70: BIT(6,_B);                        break; // BIT  6,B
  case /*CB*/0x71: BIT(6,_C);                        break; // BIT  6,C
  case /*CB*/0x72: BIT(6,_D);                        break; // BIT  6,D
  case /*CB*/0x73: BIT(6,_E);                        break; // BIT  6,E
  case /*CB*/0x74: BIT(6,_H);                        break; // BIT  6,H
  case /*CB*/0x75: BIT(6,_L);                        break; // BIT  6,L
  case /*CB*/0x76: BIT(6,RM(_HL));                   break; // BIT  6,(HL)
  case /*CB*/0x77: BIT(6,_A);                        break; // BIT  6,A

  case /*CB*/0x78: BIT(7,_B);                        break; // BIT  7,B
  case /*CB*/0x79: BIT(7,_C);                        break; // BIT  7,C
  case /*CB*/0x7a: BIT(7,_D);                        break; // BIT  7,D
  case /*CB*/0x7b: BIT(7,_E);                        break; // BIT  7,E
  case /*CB*/0x7c: BIT(7,_H);                        break; // BIT  7,H
  case /*CB*/0x7d: BIT(7,_L);                        break; // BIT  7,L
  case /*CB*/0x7e: BIT(7,RM(_HL));                   break; // BIT  7,(HL)
  case /*CB*/0x7f: BIT(7,_A);                        break; // BIT  7,A

  case /*CB*/0x80: _B = RES(0,_B);                      break; // RES  0,B
  case /*CB*/0x81: _C = RES(0,_C);                      break; // RES  0,C
  case /*CB*/0x82: _D = RES(0,_D);                      break; // RES  0,D
  case /*CB*/0x83: _E = RES(0,_E);                      break; // RES  0,E
  case /*CB*/0x84: _H = RES(0,_H);                      break; // RES  0,H
  case /*CB*/0x85: _L = RES(0,_L);                      break; // RES  0,L
  case /*CB*/0x86: WM( _HL, RES(0,RM(_HL)) );           break; // RES  0,(HL)
  case /*CB*/0x87: _A = RES(0,_A);                      break; // RES  0,A

  case /*CB*/0x88: _B = RES(1,_B);                      break; // RES  1,B
  case /*CB*/0x89: _C = RES(1,_C);                      break; // RES  1,C
  case /*CB*/0x8a: _D = RES(1,_D);                      break; // RES  1,D
  case /*CB*/0x8b: _E = RES(1,_E);                      break; // RES  1,E
  case /*CB*/0x8c: _H = RES(1,_H);                      break; // RES  1,H
  case /*CB*/0x8d: _L = RES(1,_L);                      break; // RES  1,L
  case /*CB*/0x8e: WM( _HL, RES(1,RM(_HL)) );           break; // RES  1,(HL)
  case /*CB*/0x8f: _A = RES(1,_A);                      break; // RES  1,A

  case /*CB*/0x90: _B = RES(2,_B);                      break; // RES  2,B
  case /*CB*/0x91: _C = RES(2,_C);                      break; // RES  2,C
  case /*CB*/0x92: _D = RES(2,_D);                      break; // RES  2,D
  case /*CB*/0x93: _E = RES(2,_E);                      break; // RES  2,E
  case /*CB*/0x94: _H = RES(2,_H);                      break; // RES  2,H
  case /*CB*/0x95: _L = RES(2,_L);                      break; // RES  2,L
  case /*CB*/0x96: WM( _HL, RES(2,RM(_HL)) );           break; // RES  2,(HL)
  case /*CB*/0x97: _A = RES(2,_A);                      break; // RES  2,A

  case /*CB*/0x98: _B = RES(3,_B);                      break; // RES  3,B
  case /*CB*/0x99: _C = RES(3,_C);                      break; // RES  3,C
  case /*CB*/0x9a: _D = RES(3,_D);                      break; // RES  3,D
  case /*CB*/0x9b: _E = RES(3,_E);                      break; // RES  3,E
  case /*CB*/0x9c: _H = RES(3,_H);                      break; // RES  3,H
  case /*CB*/0x9d: _L = RES(3,_L);                      break; // RES  3,L
  case /*CB*/0x9e: WM( _HL, RES(3,RM(_HL)) );           break; // RES  3,(HL)
  case /*CB*/0x9f: _A = RES(3,_A);                      break; // RES  3,A

  case /*CB*/0xa0: _B = RES(4,_B);                      break; // RES  4,B
  case /*CB*/0xa1: _C = RES(4,_C);                      break; // RES  4,C
  case /*CB*/0xa2: _D = RES(4,_D);                      break; // RES  4,D
  case /*CB*/0xa3: _E = RES(4,_E);                      break; // RES  4,E
  case /*CB*/0xa4: _H = RES(4,_H);                      break; // RES  4,H
  case /*CB*/0xa5: _L = RES(4,_L);                      break; // RES  4,L
  case /*CB*/0xa6: WM( _HL, RES(4,RM(_HL)) );           break; // RES  4,(HL)
  case /*CB*/0xa7: _A = RES(4,_A);                      break; // RES  4,A

  case /*CB*/0xa8: _B = RES(5,_B);                      break; // RES  5,B
  case /*CB*/0xa9: _C = RES(5,_C);                      break; // RES  5,C
  case /*CB*/0xaa: _D = RES(5,_D);                      break; // RES  5,D
  case /*CB*/0xab: _E = RES(5,_E);                      break; // RES  5,E
  case /*CB*/0xac: _H = RES(5,_H);                      break; // RES  5,H
  case /*CB*/0xad: _L = RES(5,_L);                      break; // RES  5,L
  case /*CB*/0xae: WM( _HL, RES(5,RM(_HL)) );           break; // RES  5,(HL)
  case /*CB*/0xaf: _A = RES(5,_A);                      break; // RES  5,A

  case /*CB*/0xb0: _B = RES(6,_B);                      break; // RES  6,B
  case /*CB*/0xb1: _C = RES(6,_C);                      break; // RES  6,C
  case /*CB*/0xb2: _D = RES(6,_D);                      break; // RES  6,D
  case /*CB*/0xb3: _E = RES(6,_E);                      break; // RES  6,E
  case /*CB*/0xb4: _H = RES(6,_H);                      break; // RES  6,H
  case /*CB*/0xb5: _L = RES(6,_L);                      break; // RES  6,L
  case /*CB*/0xb6: WM( _HL, RES(6,RM(_HL)) );           break; // RES  6,(HL)
  case /*CB*/0xb7: _A = RES(6,_A);                      break; // RES  6,A

  case /*CB*/0xb8: _B = RES(7,_B);                      break; // RES  7,B
  case /*CB*/0xb9: _C = RES(7,_C);                      break; // RES  7,C
  case /*CB*/0xba: _D = RES(7,_D);                      break; // RES  7,D
  case /*CB*/0xbb: _E = RES(7,_E);                      break; // RES  7,E
  case /*CB*/0xbc: _H = RES(7,_H);                      break; // RES  7,H
  case /*CB*/0xbd: _L = RES(7,_L);                      break; // RES  7,L
  case /*CB*/0xbe: WM( _HL, RES(7,RM(_HL)) );           break; // RES  7,(HL)
  case /*CB*/0xbf: _A = RES(7,_A);                      break; // RES  7,A

  case /*CB*/0xc0: _B = SET(0,_B);                      break; // SET  0,B
  case /*CB*/0xc1: _C = SET(0,_C);                      break; // SET  0,C
  case /*CB*/0xc2: _D = SET(0,_D);                      break; // SET  0,D
  case /*CB*/0xc3: _E = SET(0,_E);                      break; // SET  0,E
  case /*CB*/0xc4: _H = SET(0,_H);                      break; // SET  0,H
  case /*CB*/0xc5: _L = SET(0,_L);                      break; // SET  0,L
  case /*CB*/0xc6: WM( _HL, SET(0,RM(_HL)) );           break; // SET  0,(HL)
  case /*CB*/0xc7: _A = SET(0,_A);                      break; // SET  0,A

  case /*CB*/0xc8: _B = SET(1,_B);                      break; // SET  1,B
  case /*CB*/0xc9: _C = SET(1,_C);                      break; // SET  1,C
  case /*CB*/0xca: _D = SET(1,_D);                      break; // SET  1,D
  case /*CB*/0xcb: _E = SET(1,_E);                      break; // SET  1,E
  case /*CB*/0xcc: _H = SET(1,_H);                      break; // SET  1,H
  case /*CB*/0xcd: _L = SET(1,_L);                      break; // SET  1,L
  case /*CB*/0xce: WM( _HL, SET(1,RM(_HL)) );           break; // SET  1,(HL)
  case /*CB*/0xcf: _A = SET(1,_A);                      break; // SET  1,A

  case /*CB*/0xd0: _B = SET(2,_B);                      break; // SET  2,B
  case /*CB*/0xd1: _C = SET(2,_C);                      break; // SET  2,C
  case /*CB*/0xd2: _D = SET(2,_D);                      break; // SET  2,D
  case /*CB*/0xd3: _E = SET(2,_E);                      break; // SET  2,E
  case /*CB*/0xd4: _H = SET(2,_H);                      break; // SET  2,H
  case /*CB*/0xd5: _L = SET(2,_L);                      break; // SET  2,L
  case /*CB*/0xd6: WM( _HL, SET(2,RM(_HL)) );           break; // SET  2,(HL)
  case /*CB*/0xd7: _A = SET(2,_A);                      break; // SET  2,A

  case /*CB*/0xd8: _B = SET(3,_B);                      break; // SET  3,B
  case /*CB*/0xd9: _C = SET(3,_C);                      break; // SET  3,C
  case /*CB*/0xda: _D = SET(3,_D);                      break; // SET  3,D
  case /*CB*/0xdb: _E = SET(3,_E);                      break; // SET  3,E
  case /*CB*/0xdc: _H = SET(3,_H);                      break; // SET  3,H
  case /*CB*/0xdd: _L = SET(3,_L);                      break; // SET  3,L
  case /*CB*/0xde: WM( _HL, SET(3,RM(_HL)) );           break; // SET  3,(HL)
  case /*CB*/0xdf: _A = SET(3,_A);                      break; // SET  3,A

  case /*CB*/0xe0: _B = SET(4,_B);                      break; // SET  4,B
  case /*CB*/0xe1: _C = SET(4,_C);                      break; // SET  4,C
  case /*CB*/0xe2: _D = SET(4,_D);                      break; // SET  4,D
  case /*CB*/0xe3: _E = SET(4,_E);                      break; // SET  4,E
  case /*CB*/0xe4: _H = SET(4,_H);                      break; // SET  4,H
  case /*CB*/0xe5: _L = SET(4,_L);                      break; // SET  4,L
  case /*CB*/0xe6: WM( _HL, SET(4,RM(_HL)) );           break; // SET  4,(HL)
  case /*CB*/0xe7: _A = SET(4,_A);                      break; // SET  4,A

  case /*CB*/0xe8: _B = SET(5,_B);                      break; // SET  5,B
  case /*CB*/0xe9: _C = SET(5,_C);                      break; // SET  5,C
  case /*CB*/0xea: _D = SET(5,_D);                      break; // SET  5,D
  case /*CB*/0xeb: _E = SET(5,_E);                      break; // SET  5,E
  case /*CB*/0xec: _H = SET(5,_H);                      break; // SET  5,H
  case /*CB*/0xed: _L = SET(5,_L);                      break; // SET  5,L
  case /*CB*/0xee: WM( _HL, SET(5,RM(_HL)) );           break; // SET  5,(HL)
  case /*CB*/0xef: _A = SET(5,_A);                      break; // SET  5,A

  case /*CB*/0xf0: _B = SET(6,_B);                      break; // SET  6,B
  case /*CB*/0xf1: _C = SET(6,_C);                      break; // SET  6,C
  case /*CB*/0xf2: _D = SET(6,_D);                      break; // SET  6,D
  case /*CB*/0xf3: _E = SET(6,_E);                      break; // SET  6,E
  case /*CB*/0xf4: _H = SET(6,_H);                      break; // SET  6,H
  case /*CB*/0xf5: _L = SET(6,_L);                      break; // SET  6,L
  case /*CB*/0xf6: WM( _HL, SET(6,RM(_HL)) );           break; // SET  6,(HL)
  case /*CB*/0xf7: _A = SET(6,_A);                      break; // SET  6,A

  case /*CB*/0xf8: _B = SET(7,_B);                      break; // SET  7,B
  case /*CB*/0xf9: _C = SET(7,_C);                      break; // SET  7,C
  case /*CB*/0xfa: _D = SET(7,_D);                      break; // SET  7,D
  case /*CB*/0xfb: _E = SET(7,_E);                      break; // SET  7,E
  case /*CB*/0xfc: _H = SET(7,_H);                      break; // SET  7,H
  case /*CB*/0xfd: _L = SET(7,_L);                      break; // SET  7,L
  case /*CB*/0xfe: WM( _HL, SET(7,RM(_HL)) );           break; // SET  7,(HL)
  case /*CB*/0xff: _A = SET(7,_A);                      break; // SET  7,A
  }
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE void z80_exec_ea_xycb(
  struct Z80_STATE *state, uint8 op, uint16 EA
) {

//printf("PC=%04X exec xycb %02X\n",_PC, op);fflush(stdout);

  switch(op) {
  case /*xycb*/0x00: _B = RLC( RM(EA) ); WM( EA,_B );            break; // RLC  B=(XY+o)
  case /*xycb*/0x01: _C = RLC( RM(EA) ); WM( EA,_C );            break; // RLC  C=(XY+o)
  case /*xycb*/0x02: _D = RLC( RM(EA) ); WM( EA,_D );            break; // RLC  D=(XY+o)
  case /*xycb*/0x03: _E = RLC( RM(EA) ); WM( EA,_E );            break; // RLC  E=(XY+o)
  case /*xycb*/0x04: _H = RLC( RM(EA) ); WM( EA,_H );            break; // RLC  H=(XY+o)
  case /*xycb*/0x05: _L = RLC( RM(EA) ); WM( EA,_L );            break; // RLC  L=(XY+o)
  case /*xycb*/0x06: WM( EA, RLC( RM(EA) ) );                    break; // RLC  (XY+o)
  case /*xycb*/0x07: _A = RLC( RM(EA) ); WM( EA,_A );            break; // RLC  A=(XY+o)

  case /*xycb*/0x08: _B = RRC( RM(EA) ); WM( EA,_B );            break; // RRC  B=(XY+o)
  case /*xycb*/0x09: _C = RRC( RM(EA) ); WM( EA,_C );            break; // RRC  C=(XY+o)
  case /*xycb*/0x0a: _D = RRC( RM(EA) ); WM( EA,_D );            break; // RRC  D=(XY+o)
  case /*xycb*/0x0b: _E = RRC( RM(EA) ); WM( EA,_E );            break; // RRC  E=(XY+o)
  case /*xycb*/0x0c: _H = RRC( RM(EA) ); WM( EA,_H );            break; // RRC  H=(XY+o)
  case /*xycb*/0x0d: _L = RRC( RM(EA) ); WM( EA,_L );            break; // RRC  L=(XY+o)
  case /*xycb*/0x0e: WM( EA,RRC( RM(EA) ) );                     break; // RRC  (XY+o)
  case /*xycb*/0x0f: _A = RRC( RM(EA) ); WM( EA,_A );            break; // RRC  A=(XY+o)

  case /*xycb*/0x10: _B = RL( RM(EA) ); WM( EA,_B );            break; // RL   B=(XY+o)
  case /*xycb*/0x11: _C = RL( RM(EA) ); WM( EA,_C );            break; // RL   C=(XY+o)
  case /*xycb*/0x12: _D = RL( RM(EA) ); WM( EA,_D );            break; // RL   D=(XY+o)
  case /*xycb*/0x13: _E = RL( RM(EA) ); WM( EA,_E );            break; // RL   E=(XY+o)
  case /*xycb*/0x14: _H = RL( RM(EA) ); WM( EA,_H );            break; // RL   H=(XY+o)
  case /*xycb*/0x15: _L = RL( RM(EA) ); WM( EA,_L );            break; // RL   L=(XY+o)
  case /*xycb*/0x16: WM( EA,RL( RM(EA) ) );                     break; // RL   (XY+o)
  case /*xycb*/0x17: _A = RL( RM(EA) ); WM( EA,_A );            break; // RL   A=(XY+o)

  case /*xycb*/0x18: _B = RR( RM(EA) ); WM( EA,_B );            break; // RR   B=(XY+o)
  case /*xycb*/0x19: _C = RR( RM(EA) ); WM( EA,_C );            break; // RR   C=(XY+o)
  case /*xycb*/0x1a: _D = RR( RM(EA) ); WM( EA,_D );            break; // RR   D=(XY+o)
  case /*xycb*/0x1b: _E = RR( RM(EA) ); WM( EA,_E );            break; // RR   E=(XY+o)
  case /*xycb*/0x1c: _H = RR( RM(EA) ); WM( EA,_H );            break; // RR   H=(XY+o)
  case /*xycb*/0x1d: _L = RR( RM(EA) ); WM( EA,_L );            break; // RR   L=(XY+o)
  case /*xycb*/0x1e: WM( EA,RR( RM(EA) ) );                     break; // RR   (XY+o)
  case /*xycb*/0x1f: _A = RR( RM(EA) ); WM( EA,_A );            break; // RR   A=(XY+o)

  case /*xycb*/0x20: _B = SLA( RM(EA) ); WM( EA,_B );            break; // SLA  B=(XY+o)
  case /*xycb*/0x21: _C = SLA( RM(EA) ); WM( EA,_C );            break; // SLA  C=(XY+o)
  case /*xycb*/0x22: _D = SLA( RM(EA) ); WM( EA,_D );            break; // SLA  D=(XY+o)
  case /*xycb*/0x23: _E = SLA( RM(EA) ); WM( EA,_E );            break; // SLA  E=(XY+o)
  case /*xycb*/0x24: _H = SLA( RM(EA) ); WM( EA,_H );            break; // SLA  H=(XY+o)
  case /*xycb*/0x25: _L = SLA( RM(EA) ); WM( EA,_L );            break; // SLA  L=(XY+o)
  case /*xycb*/0x26: WM( EA,SLA( RM(EA) ) );                     break; // SLA  (XY+o)
  case /*xycb*/0x27: _A = SLA( RM(EA) ); WM( EA,_A );            break; // SLA  A=(XY+o)

  case /*xycb*/0x28: _B = SRA( RM(EA) ); WM( EA,_B );            break; // SRA  B=(XY+o)
  case /*xycb*/0x29: _C = SRA( RM(EA) ); WM( EA,_C );            break; // SRA  C=(XY+o)
  case /*xycb*/0x2a: _D = SRA( RM(EA) ); WM( EA,_D );            break; // SRA  D=(XY+o)
  case /*xycb*/0x2b: _E = SRA( RM(EA) ); WM( EA,_E );            break; // SRA  E=(XY+o)
  case /*xycb*/0x2c: _H = SRA( RM(EA) ); WM( EA,_H );            break; // SRA  H=(XY+o)
  case /*xycb*/0x2d: _L = SRA( RM(EA) ); WM( EA,_L );            break; // SRA  L=(XY+o)
  case /*xycb*/0x2e: WM( EA,SRA( RM(EA) ) );                     break; // SRA  (XY+o)
  case /*xycb*/0x2f: _A = SRA( RM(EA) ); WM( EA,_A );            break; // SRA  A=(XY+o)

  case /*xycb*/0x30: _B = SLL( RM(EA) ); WM( EA,_B );            break; // SLL  B=(XY+o)
  case /*xycb*/0x31: _C = SLL( RM(EA) ); WM( EA,_C );            break; // SLL  C=(XY+o)
  case /*xycb*/0x32: _D = SLL( RM(EA) ); WM( EA,_D );            break; // SLL  D=(XY+o)
  case /*xycb*/0x33: _E = SLL( RM(EA) ); WM( EA,_E );            break; // SLL  E=(XY+o)
  case /*xycb*/0x34: _H = SLL( RM(EA) ); WM( EA,_H );            break; // SLL  H=(XY+o)
  case /*xycb*/0x35: _L = SLL( RM(EA) ); WM( EA,_L );            break; // SLL  L=(XY+o)
  case /*xycb*/0x36: WM( EA,SLL( RM(EA) ) );                     break; // SLL  (XY+o)
  case /*xycb*/0x37: _A = SLL( RM(EA) ); WM( EA,_A );            break; // SLL  A=(XY+o)

  case /*xycb*/0x38: _B = SRL( RM(EA) ); WM( EA,_B );            break; // SRL  B=(XY+o)
  case /*xycb*/0x39: _C = SRL( RM(EA) ); WM( EA,_C );            break; // SRL  C=(XY+o)
  case /*xycb*/0x3a: _D = SRL( RM(EA) ); WM( EA,_D );            break; // SRL  D=(XY+o)
  case /*xycb*/0x3b: _E = SRL( RM(EA) ); WM( EA,_E );            break; // SRL  E=(XY+o)
  case /*xycb*/0x3c: _H = SRL( RM(EA) ); WM( EA,_H );            break; // SRL  H=(XY+o)
  case /*xycb*/0x3d: _L = SRL( RM(EA) ); WM( EA,_L );            break; // SRL  L=(XY+o)
  case /*xycb*/0x3e: WM( EA,SRL( RM(EA) ) );                     break; // SRL  (XY+o)
  case /*xycb*/0x3f: _A = SRL( RM(EA) ); WM( EA,_A );            break; // SRL  A=(XY+o)

  case /*xycb*/0x40: BIT_XY(0,RM(EA));                           break; // BIT  0,B=(XY+o)
  case /*xycb*/0x41: BIT_XY(0,RM(EA));                           break; // BIT  0,C=(XY+o)
  case /*xycb*/0x42: BIT_XY(0,RM(EA));                           break; // BIT  0,D=(XY+o)
  case /*xycb*/0x43: BIT_XY(0,RM(EA));                           break; // BIT  0,E=(XY+o)
  case /*xycb*/0x44: BIT_XY(0,RM(EA));                           break; // BIT  0,H=(XY+o)
  case /*xycb*/0x45: BIT_XY(0,RM(EA));                           break; // BIT  0,L=(XY+o)
  case /*xycb*/0x46: BIT_XY(0,RM(EA));                           break; // BIT  0,(XY+o)
  case /*xycb*/0x47: BIT_XY(0,RM(EA));                           break; // BIT  0,A=(XY+o)

  case /*xycb*/0x48: BIT_XY(1,RM(EA));                           break; // BIT  1,B=(XY+o)
  case /*xycb*/0x49: BIT_XY(1,RM(EA));                           break; // BIT  1,C=(XY+o)
  case /*xycb*/0x4a: BIT_XY(1,RM(EA));                           break; // BIT  1,D=(XY+o)
  case /*xycb*/0x4b: BIT_XY(1,RM(EA));                           break; // BIT  1,E=(XY+o)
  case /*xycb*/0x4c: BIT_XY(1,RM(EA));                           break; // BIT  1,H=(XY+o)
  case /*xycb*/0x4d: BIT_XY(1,RM(EA));                           break; // BIT  1,L=(XY+o)
  case /*xycb*/0x4e: BIT_XY(1,RM(EA));                           break; // BIT  1,(XY+o)
  case /*xycb*/0x4f: BIT_XY(1,RM(EA));                           break; // BIT  1,A=(XY+o)

  case /*xycb*/0x50: BIT_XY(2,RM(EA));                           break; // BIT  2,B=(XY+o)
  case /*xycb*/0x51: BIT_XY(2,RM(EA));                           break; // BIT  2,C=(XY+o)
  case /*xycb*/0x52: BIT_XY(2,RM(EA));                           break; // BIT  2,D=(XY+o)
  case /*xycb*/0x53: BIT_XY(2,RM(EA));                           break; // BIT  2,E=(XY+o)
  case /*xycb*/0x54: BIT_XY(2,RM(EA));                           break; // BIT  2,H=(XY+o)
  case /*xycb*/0x55: BIT_XY(2,RM(EA));                           break; // BIT  2,L=(XY+o)
  case /*xycb*/0x56: BIT_XY(2,RM(EA));                           break; // BIT  2,(XY+o)
  case /*xycb*/0x57: BIT_XY(2,RM(EA));                           break; // BIT  2,A=(XY+o)

  case /*xycb*/0x58: BIT_XY(3,RM(EA));                           break; // BIT  3,B=(XY+o)
  case /*xycb*/0x59: BIT_XY(3,RM(EA));                           break; // BIT  3,C=(XY+o)
  case /*xycb*/0x5a: BIT_XY(3,RM(EA));                           break; // BIT  3,D=(XY+o)
  case /*xycb*/0x5b: BIT_XY(3,RM(EA));                           break; // BIT  3,E=(XY+o)
  case /*xycb*/0x5c: BIT_XY(3,RM(EA));                           break; // BIT  3,H=(XY+o)
  case /*xycb*/0x5d: BIT_XY(3,RM(EA));                           break; // BIT  3,L=(XY+o)
  case /*xycb*/0x5e: BIT_XY(3,RM(EA));                           break; // BIT  3,(XY+o)
  case /*xycb*/0x5f: BIT_XY(3,RM(EA));                           break; // BIT  3,A=(XY+o)

  case /*xycb*/0x60: BIT_XY(4,RM(EA));                           break; // BIT  4,B=(XY+o)
  case /*xycb*/0x61: BIT_XY(4,RM(EA));                           break; // BIT  4,C=(XY+o)
  case /*xycb*/0x62: BIT_XY(4,RM(EA));                           break; // BIT  4,D=(XY+o)
  case /*xycb*/0x63: BIT_XY(4,RM(EA));                           break; // BIT  4,E=(XY+o)
  case /*xycb*/0x64: BIT_XY(4,RM(EA));                           break; // BIT  4,H=(XY+o)
  case /*xycb*/0x65: BIT_XY(4,RM(EA));                           break; // BIT  4,L=(XY+o)
  case /*xycb*/0x66: BIT_XY(4,RM(EA));                           break; // BIT  4,(XY+o)
  case /*xycb*/0x67: BIT_XY(4,RM(EA));                           break; // BIT  4,A=(XY+o)

  case /*xycb*/0x68: BIT_XY(5,RM(EA));                           break; // BIT  5,B=(XY+o)
  case /*xycb*/0x69: BIT_XY(5,RM(EA));                           break; // BIT  5,C=(XY+o)
  case /*xycb*/0x6a: BIT_XY(5,RM(EA));                           break; // BIT  5,D=(XY+o)
  case /*xycb*/0x6b: BIT_XY(5,RM(EA));                           break; // BIT  5,E=(XY+o)
  case /*xycb*/0x6c: BIT_XY(5,RM(EA));                           break; // BIT  5,H=(XY+o)
  case /*xycb*/0x6d: BIT_XY(5,RM(EA));                           break; // BIT  5,L=(XY+o)
  case /*xycb*/0x6e: BIT_XY(5,RM(EA));                           break; // BIT  5,(XY+o)
  case /*xycb*/0x6f: BIT_XY(5,RM(EA));                           break; // BIT  5,A=(XY+o)

  case /*xycb*/0x70: BIT_XY(6,RM(EA));                           break; // BIT  6,B=(XY+o)
  case /*xycb*/0x71: BIT_XY(6,RM(EA));                           break; // BIT  6,C=(XY+o)
  case /*xycb*/0x72: BIT_XY(6,RM(EA));                           break; // BIT  6,D=(XY+o)
  case /*xycb*/0x73: BIT_XY(6,RM(EA));                           break; // BIT  6,E=(XY+o)
  case /*xycb*/0x74: BIT_XY(6,RM(EA));                           break; // BIT  6,H=(XY+o)
  case /*xycb*/0x75: BIT_XY(6,RM(EA));                           break; // BIT  6,L=(XY+o)
  case /*xycb*/0x76: BIT_XY(6,RM(EA));                           break; // BIT  6,(XY+o)
  case /*xycb*/0x77: BIT_XY(6,RM(EA));                           break; // BIT  6,A=(XY+o)

  case /*xycb*/0x78: BIT_XY(7,RM(EA));                           break; // BIT  7,B=(XY+o)
  case /*xycb*/0x79: BIT_XY(7,RM(EA));                           break; // BIT  7,C=(XY+o)
  case /*xycb*/0x7a: BIT_XY(7,RM(EA));                           break; // BIT  7,D=(XY+o)
  case /*xycb*/0x7b: BIT_XY(7,RM(EA));                           break; // BIT  7,E=(XY+o)
  case /*xycb*/0x7c: BIT_XY(7,RM(EA));                           break; // BIT  7,H=(XY+o)
  case /*xycb*/0x7d: BIT_XY(7,RM(EA));                           break; // BIT  7,L=(XY+o)
  case /*xycb*/0x7e: BIT_XY(7,RM(EA));                           break; // BIT  7,(XY+o)
  case /*xycb*/0x7f: BIT_XY(7,RM(EA));                           break; // BIT  7,A=(XY+o)

  case /*xycb*/0x80: _B = RES(0, RM(EA) ); WM( EA,_B );          break; // RES  0,B=(XY+o)
  case /*xycb*/0x81: _C = RES(0, RM(EA) ); WM( EA,_C );          break; // RES  0,C=(XY+o)
  case /*xycb*/0x82: _D = RES(0, RM(EA) ); WM( EA,_D );          break; // RES  0,D=(XY+o)
  case /*xycb*/0x83: _E = RES(0, RM(EA) ); WM( EA,_E );          break; // RES  0,E=(XY+o)
  case /*xycb*/0x84: _H = RES(0, RM(EA) ); WM( EA,_H );          break; // RES  0,H=(XY+o)
  case /*xycb*/0x85: _L = RES(0, RM(EA) ); WM( EA,_L );          break; // RES  0,L=(XY+o)
  case /*xycb*/0x86: WM( EA, RES(0,RM(EA)) );                    break; // RES  0,(XY+o)
  case /*xycb*/0x87: _A = RES(0, RM(EA) ); WM( EA,_A );          break; // RES  0,A=(XY+o)

  case /*xycb*/0x88: _B = RES(1, RM(EA) ); WM( EA,_B );          break; // RES  1,B=(XY+o)
  case /*xycb*/0x89: _C = RES(1, RM(EA) ); WM( EA,_C );          break; // RES  1,C=(XY+o)
  case /*xycb*/0x8a: _D = RES(1, RM(EA) ); WM( EA,_D );          break; // RES  1,D=(XY+o)
  case /*xycb*/0x8b: _E = RES(1, RM(EA) ); WM( EA,_E );          break; // RES  1,E=(XY+o)
  case /*xycb*/0x8c: _H = RES(1, RM(EA) ); WM( EA,_H );          break; // RES  1,H=(XY+o)
  case /*xycb*/0x8d: _L = RES(1, RM(EA) ); WM( EA,_L );          break; // RES  1,L=(XY+o)
  case /*xycb*/0x8e: WM( EA, RES(1,RM(EA)) );                    break; // RES  1,(XY+o)
  case /*xycb*/0x8f: _A = RES(1, RM(EA) ); WM( EA,_A );          break; // RES  1,A=(XY+o)

  case /*xycb*/0x90: _B = RES(2, RM(EA) ); WM( EA,_B );          break; // RES  2,B=(XY+o)
  case /*xycb*/0x91: _C = RES(2, RM(EA) ); WM( EA,_C );          break; // RES  2,C=(XY+o)
  case /*xycb*/0x92: _D = RES(2, RM(EA) ); WM( EA,_D );          break; // RES  2,D=(XY+o)
  case /*xycb*/0x93: _E = RES(2, RM(EA) ); WM( EA,_E );          break; // RES  2,E=(XY+o)
  case /*xycb*/0x94: _H = RES(2, RM(EA) ); WM( EA,_H );          break; // RES  2,H=(XY+o)
  case /*xycb*/0x95: _L = RES(2, RM(EA) ); WM( EA,_L );          break; // RES  2,L=(XY+o)
  case /*xycb*/0x96: WM( EA, RES(2,RM(EA)) );                    break; // RES  2,(XY+o)
  case /*xycb*/0x97: _A = RES(2, RM(EA) ); WM( EA,_A );          break; // RES  2,A=(XY+o)

  case /*xycb*/0x98: _B = RES(3, RM(EA) ); WM( EA,_B );          break; // RES  3,B=(XY+o)
  case /*xycb*/0x99: _C = RES(3, RM(EA) ); WM( EA,_C );          break; // RES  3,C=(XY+o)
  case /*xycb*/0x9a: _D = RES(3, RM(EA) ); WM( EA,_D );          break; // RES  3,D=(XY+o)
  case /*xycb*/0x9b: _E = RES(3, RM(EA) ); WM( EA,_E );          break; // RES  3,E=(XY+o)
  case /*xycb*/0x9c: _H = RES(3, RM(EA) ); WM( EA,_H );          break; // RES  3,H=(XY+o)
  case /*xycb*/0x9d: _L = RES(3, RM(EA) ); WM( EA,_L );          break; // RES  3,L=(XY+o)
  case /*xycb*/0x9e: WM( EA, RES(3,RM(EA)) );                    break; // RES  3,(XY+o)
  case /*xycb*/0x9f: _A = RES(3, RM(EA) ); WM( EA,_A );          break; // RES  3,A=(XY+o)

  case /*xycb*/0xa0: _B = RES(4, RM(EA) ); WM( EA,_B );          break; // RES  4,B=(XY+o)
  case /*xycb*/0xa1: _C = RES(4, RM(EA) ); WM( EA,_C );          break; // RES  4,C=(XY+o)
  case /*xycb*/0xa2: _D = RES(4, RM(EA) ); WM( EA,_D );          break; // RES  4,D=(XY+o)
  case /*xycb*/0xa3: _E = RES(4, RM(EA) ); WM( EA,_E );          break; // RES  4,E=(XY+o)
  case /*xycb*/0xa4: _H = RES(4, RM(EA) ); WM( EA,_H );          break; // RES  4,H=(XY+o)
  case /*xycb*/0xa5: _L = RES(4, RM(EA) ); WM( EA,_L );          break; // RES  4,L=(XY+o)
  case /*xycb*/0xa6: WM( EA, RES(4,RM(EA)) );                    break; // RES  4,(XY+o)
  case /*xycb*/0xa7: _A = RES(4, RM(EA) ); WM( EA,_A );          break; // RES  4,A=(XY+o)

  case /*xycb*/0xa8: _B = RES(5, RM(EA) ); WM( EA,_B );          break; // RES  5,B=(XY+o)
  case /*xycb*/0xa9: _C = RES(5, RM(EA) ); WM( EA,_C );          break; // RES  5,C=(XY+o)
  case /*xycb*/0xaa: _D = RES(5, RM(EA) ); WM( EA,_D );          break; // RES  5,D=(XY+o)
  case /*xycb*/0xab: _E = RES(5, RM(EA) ); WM( EA,_E );          break; // RES  5,E=(XY+o)
  case /*xycb*/0xac: _H = RES(5, RM(EA) ); WM( EA,_H );          break; // RES  5,H=(XY+o)
  case /*xycb*/0xad: _L = RES(5, RM(EA) ); WM( EA,_L );          break; // RES  5,L=(XY+o)
  case /*xycb*/0xae: WM( EA, RES(5,RM(EA)) );                    break; // RES  5,(XY+o)
  case /*xycb*/0xaf: _A = RES(5, RM(EA) ); WM( EA,_A );          break; // RES  5,A=(XY+o)

  case /*xycb*/0xb0: _B = RES(6, RM(EA) ); WM( EA,_B );          break; // RES  6,B=(XY+o)
  case /*xycb*/0xb1: _C = RES(6, RM(EA) ); WM( EA,_C );          break; // RES  6,C=(XY+o)
  case /*xycb*/0xb2: _D = RES(6, RM(EA) ); WM( EA,_D );          break; // RES  6,D=(XY+o)
  case /*xycb*/0xb3: _E = RES(6, RM(EA) ); WM( EA,_E );          break; // RES  6,E=(XY+o)
  case /*xycb*/0xb4: _H = RES(6, RM(EA) ); WM( EA,_H );          break; // RES  6,H=(XY+o)
  case /*xycb*/0xb5: _L = RES(6, RM(EA) ); WM( EA,_L );          break; // RES  6,L=(XY+o)
  case /*xycb*/0xb6: WM( EA, RES(6,RM(EA)) );                    break; // RES  6,(XY+o)
  case /*xycb*/0xb7: _A = RES(6, RM(EA) ); WM( EA,_A );          break; // RES  6,A=(XY+o)

  case /*xycb*/0xb8: _B = RES(7, RM(EA) ); WM( EA,_B );          break; // RES  7,B=(XY+o)
  case /*xycb*/0xb9: _C = RES(7, RM(EA) ); WM( EA,_C );          break; // RES  7,C=(XY+o)
  case /*xycb*/0xba: _D = RES(7, RM(EA) ); WM( EA,_D );          break; // RES  7,D=(XY+o)
  case /*xycb*/0xbb: _E = RES(7, RM(EA) ); WM( EA,_E );          break; // RES  7,E=(XY+o)
  case /*xycb*/0xbc: _H = RES(7, RM(EA) ); WM( EA,_H );          break; // RES  7,H=(XY+o)
  case /*xycb*/0xbd: _L = RES(7, RM(EA) ); WM( EA,_L );          break; // RES  7,L=(XY+o)
  case /*xycb*/0xbe: WM( EA, RES(7,RM(EA)) );                    break; // RES  7,(XY+o)
  case /*xycb*/0xbf: _A = RES(7, RM(EA) ); WM( EA,_A );          break; // RES  7,A=(XY+o)

  case /*xycb*/0xc0: _B = SET(0, RM(EA) ); WM( EA,_B );          break; // SET  0,B=(XY+o)
  case /*xycb*/0xc1: _C = SET(0, RM(EA) ); WM( EA,_C );          break; // SET  0,C=(XY+o)
  case /*xycb*/0xc2: _D = SET(0, RM(EA) ); WM( EA,_D );          break; // SET  0,D=(XY+o)
  case /*xycb*/0xc3: _E = SET(0, RM(EA) ); WM( EA,_E );          break; // SET  0,E=(XY+o)
  case /*xycb*/0xc4: _H = SET(0, RM(EA) ); WM( EA,_H );          break; // SET  0,H=(XY+o)
  case /*xycb*/0xc5: _L = SET(0, RM(EA) ); WM( EA,_L );          break; // SET  0,L=(XY+o)
  case /*xycb*/0xc6: WM( EA, SET(0,RM(EA)) );                    break; // SET  0,(XY+o)
  case /*xycb*/0xc7: _A = SET(0, RM(EA) ); WM( EA,_A );          break; // SET  0,A=(XY+o)

  case /*xycb*/0xc8: _B = SET(1, RM(EA) ); WM( EA,_B );          break; // SET  1,B=(XY+o)
  case /*xycb*/0xc9: _C = SET(1, RM(EA) ); WM( EA,_C );          break; // SET  1,C=(XY+o)
  case /*xycb*/0xca: _D = SET(1, RM(EA) ); WM( EA,_D );          break; // SET  1,D=(XY+o)
  case /*xycb*/0xcb: _E = SET(1, RM(EA) ); WM( EA,_E );          break; // SET  1,E=(XY+o)
  case /*xycb*/0xcc: _H = SET(1, RM(EA) ); WM( EA,_H );          break; // SET  1,H=(XY+o)
  case /*xycb*/0xcd: _L = SET(1, RM(EA) ); WM( EA,_L );          break; // SET  1,L=(XY+o)
  case /*xycb*/0xce: WM( EA, SET(1,RM(EA)) );                    break; // SET  1,(XY+o)
  case /*xycb*/0xcf: _A = SET(1, RM(EA) ); WM( EA,_A );          break; // SET  1,A=(XY+o)

  case /*xycb*/0xd0: _B = SET(2, RM(EA) ); WM( EA,_B );          break; // SET  2,B=(XY+o)
  case /*xycb*/0xd1: _C = SET(2, RM(EA) ); WM( EA,_C );          break; // SET  2,C=(XY+o)
  case /*xycb*/0xd2: _D = SET(2, RM(EA) ); WM( EA,_D );          break; // SET  2,D=(XY+o)
  case /*xycb*/0xd3: _E = SET(2, RM(EA) ); WM( EA,_E );          break; // SET  2,E=(XY+o)
  case /*xycb*/0xd4: _H = SET(2, RM(EA) ); WM( EA,_H );          break; // SET  2,H=(XY+o)
  case /*xycb*/0xd5: _L = SET(2, RM(EA) ); WM( EA,_L );          break; // SET  2,L=(XY+o)
  case /*xycb*/0xd6: WM( EA, SET(2,RM(EA)) );                    break; // SET  2,(XY+o)
  case /*xycb*/0xd7: _A = SET(2, RM(EA) ); WM( EA,_A );          break; // SET  2,A=(XY+o)

  case /*xycb*/0xd8: _B = SET(3, RM(EA) ); WM( EA,_B );          break; // SET  3,B=(XY+o)
  case /*xycb*/0xd9: _C = SET(3, RM(EA) ); WM( EA,_C );          break; // SET  3,C=(XY+o)
  case /*xycb*/0xda: _D = SET(3, RM(EA) ); WM( EA,_D );          break; // SET  3,D=(XY+o)
  case /*xycb*/0xdb: _E = SET(3, RM(EA) ); WM( EA,_E );          break; // SET  3,E=(XY+o)
  case /*xycb*/0xdc: _H = SET(3, RM(EA) ); WM( EA,_H );          break; // SET  3,H=(XY+o)
  case /*xycb*/0xdd: _L = SET(3, RM(EA) ); WM( EA,_L );          break; // SET  3,L=(XY+o)
  case /*xycb*/0xde: WM( EA, SET(3,RM(EA)) );                    break; // SET  3,(XY+o)
  case /*xycb*/0xdf: _A = SET(3, RM(EA) ); WM( EA,_A );          break; // SET  3,A=(XY+o)

  case /*xycb*/0xe0: _B = SET(4, RM(EA) ); WM( EA,_B );          break; // SET  4,B=(XY+o)
  case /*xycb*/0xe1: _C = SET(4, RM(EA) ); WM( EA,_C );          break; // SET  4,C=(XY+o)
  case /*xycb*/0xe2: _D = SET(4, RM(EA) ); WM( EA,_D );          break; // SET  4,D=(XY+o)
  case /*xycb*/0xe3: _E = SET(4, RM(EA) ); WM( EA,_E );          break; // SET  4,E=(XY+o)
  case /*xycb*/0xe4: _H = SET(4, RM(EA) ); WM( EA,_H );          break; // SET  4,H=(XY+o)
  case /*xycb*/0xe5: _L = SET(4, RM(EA) ); WM( EA,_L );          break; // SET  4,L=(XY+o)
  case /*xycb*/0xe6: WM( EA, SET(4,RM(EA)) );                    break; // SET  4,(XY+o)
  case /*xycb*/0xe7: _A = SET(4, RM(EA) ); WM( EA,_A );          break; // SET  4,A=(XY+o)

  case /*xycb*/0xe8: _B = SET(5, RM(EA) ); WM( EA,_B );          break; // SET  5,B=(XY+o)
  case /*xycb*/0xe9: _C = SET(5, RM(EA) ); WM( EA,_C );          break; // SET  5,C=(XY+o)
  case /*xycb*/0xea: _D = SET(5, RM(EA) ); WM( EA,_D );          break; // SET  5,D=(XY+o)
  case /*xycb*/0xeb: _E = SET(5, RM(EA) ); WM( EA,_E );          break; // SET  5,E=(XY+o)
  case /*xycb*/0xec: _H = SET(5, RM(EA) ); WM( EA,_H );          break; // SET  5,H=(XY+o)
  case /*xycb*/0xed: _L = SET(5, RM(EA) ); WM( EA,_L );          break; // SET  5,L=(XY+o)
  case /*xycb*/0xee: WM( EA, SET(5,RM(EA)) );                    break; // SET  5,(XY+o)
  case /*xycb*/0xef: _A = SET(5, RM(EA) ); WM( EA,_A );          break; // SET  5,A=(XY+o)

  case /*xycb*/0xf0: _B = SET(6, RM(EA) ); WM( EA,_B );          break; // SET  6,B=(XY+o)
  case /*xycb*/0xf1: _C = SET(6, RM(EA) ); WM( EA,_C );          break; // SET  6,C=(XY+o)
  case /*xycb*/0xf2: _D = SET(6, RM(EA) ); WM( EA,_D );          break; // SET  6,D=(XY+o)
  case /*xycb*/0xf3: _E = SET(6, RM(EA) ); WM( EA,_E );          break; // SET  6,E=(XY+o)
  case /*xycb*/0xf4: _H = SET(6, RM(EA) ); WM( EA,_H );          break; // SET  6,H=(XY+o)
  case /*xycb*/0xf5: _L = SET(6, RM(EA) ); WM( EA,_L );          break; // SET  6,L=(XY+o)
  case /*xycb*/0xf6: WM( EA, SET(6,RM(EA)) );                    break; // SET  6,(XY+o)
  case /*xycb*/0xf7: _A = SET(6, RM(EA) ); WM( EA,_A );          break; // SET  6,A=(XY+o)

  case /*xycb*/0xf8: _B = SET(7, RM(EA) ); WM( EA,_B );          break; // SET  7,B=(XY+o)
  case /*xycb*/0xf9: _C = SET(7, RM(EA) ); WM( EA,_C );          break; // SET  7,C=(XY+o)
  case /*xycb*/0xfa: _D = SET(7, RM(EA) ); WM( EA,_D );          break; // SET  7,D=(XY+o)
  case /*xycb*/0xfb: _E = SET(7, RM(EA) ); WM( EA,_E );          break; // SET  7,E=(XY+o)
  case /*xycb*/0xfc: _H = SET(7, RM(EA) ); WM( EA,_H );          break; // SET  7,H=(XY+o)
  case /*xycb*/0xfd: _L = SET(7, RM(EA) ); WM( EA,_L );          break; // SET  7,L=(XY+o)
  case /*xycb*/0xfe: WM( EA, SET(7,RM(EA)) );                    break; // SET  7,(XY+o)
  case /*xycb*/0xff: _A = SET(7, RM(EA) ); WM( EA,_A );          break; // SET  7,A=(XY+o)
  }
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE void z80_exec_dd(struct Z80_STATE *state, uint8 op) {
  uint16 EA;

//printf("PC=%04X exec dd %02X\n",_PC, op);fflush(stdout);

  switch(op) {
  case /*DD*/0x00: illegal_1(); /*op_00();*/                  break; // DB   DD
  case /*DD*/0x01: illegal_1(); /*op_01();*/                  break; // DB   DD
  case /*DD*/0x02: illegal_1(); /*op_02();*/                  break; // DB   DD
  case /*DD*/0x03: illegal_1(); /*op_03();*/                  break; // DB   DD
  case /*DD*/0x04: illegal_1(); /*op_04();*/                  break; // DB   DD
  case /*DD*/0x05: illegal_1(); /*op_05();*/                  break; // DB   DD
  case /*DD*/0x06: illegal_1(); /*op_06();*/                  break; // DB   DD
  case /*DD*/0x07: illegal_1(); /*op_07();*/                  break; // DB   DD

  case /*DD*/0x08: illegal_1(); /*op_08();*/                  break; // DB   DD
  case /*DD*/0x09: _R++; ADD16(_IX,_BC);                      break; // ADD  IX,BC
  case /*DD*/0x0a: illegal_1(); /*op_0a();*/                  break; // DB   DD
  case /*DD*/0x0b: illegal_1(); /*op_0b();*/                  break; // DB   DD
  case /*DD*/0x0c: illegal_1(); /*op_0c();*/                  break; // DB   DD
  case /*DD*/0x0d: illegal_1(); /*op_0d();*/                  break; // DB   DD
  case /*DD*/0x0e: illegal_1(); /*op_0e();*/                  break; // DB   DD
  case /*DD*/0x0f: illegal_1(); /*op_0f();*/                  break; // DB   DD

  case /*DD*/0x10: illegal_1(); /*op_10();*/                  break; // DB   DD
  case /*DD*/0x11: illegal_1(); /*op_11();*/                  break; // DB   DD
  case /*DD*/0x12: illegal_1(); /*op_12();*/                  break; // DB   DD
  case /*DD*/0x13: illegal_1(); /*op_13();*/                  break; // DB   DD
  case /*DD*/0x14: illegal_1(); /*op_14();*/                  break; // DB   DD
  case /*DD*/0x15: illegal_1(); /*op_15();*/                  break; // DB   DD
  case /*DD*/0x16: illegal_1(); /*op_16();*/                  break; // DB   DD
  case /*DD*/0x17: illegal_1(); /*op_17();*/                  break; // DB   DD

  case /*DD*/0x18: illegal_1(); /*op_18();*/                  break; // DB   DD
  case /*DD*/0x19: _R++; ADD16(_IX,_DE);                      break; // ADD  IX,DE
  case /*DD*/0x1a: illegal_1(); /*op_1a();*/                  break; // DB   DD
  case /*DD*/0x1b: illegal_1(); /*op_1b();*/                  break; // DB   DD
  case /*DD*/0x1c: illegal_1(); /*op_1c();*/                  break; // DB   DD
  case /*DD*/0x1d: illegal_1(); /*op_1d();*/                  break; // DB   DD
  case /*DD*/0x1e: illegal_1(); /*op_1e();*/                  break; // DB   DD
  case /*DD*/0x1f: illegal_1(); /*op_1f();*/                  break; // DB   DD

  case /*DD*/0x20: illegal_1(); /*op_20();*/                  break; // DB   DD
  case /*DD*/0x21: _R++; _IX = ARG16();                       break; // LD   IX,w
  case /*DD*/0x22: _R++; WM16( ARG16(), _IX );                break; // LD   (w),IX
  case /*DD*/0x23: _R++; _IX++;                               break; // INC  IX
  case /*DD*/0x24: _R++; _HX = INC(_HX);                      break; // INC  HX
  case /*DD*/0x25: _R++; _HX = DEC(_HX);                      break; // DEC  HX
  case /*DD*/0x26: _R++; _HX = ARG();                         break; // LD   HX,n
  case /*DD*/0x27: illegal_1(); /*op_27();*/                  break; // DB   DD

  case /*DD*/0x28: illegal_1(); /*op_28();*/                  break; // DB   DD
  case /*DD*/0x29: _R++; ADD16(_IX,_IX);                      break; // ADD  IX,IX
  case /*DD*/0x2a: _R++; _IX = RM16( ARG16() );               break; // LD   IX,(w)
  case /*DD*/0x2b: _R++; _IX--;                               break; // DEC  IX
  case /*DD*/0x2c: _R++; _LX = INC(_LX);                      break; // INC  LX
  case /*DD*/0x2d: _R++; _LX = DEC(_LX);                      break; // DEC  LX
  case /*DD*/0x2e: _R++; _LX = ARG();                         break; // LD   LX,n
  case /*DD*/0x2f: illegal_1(); /*op_2f();*/                  break; // DB   DD

  case /*DD*/0x30: illegal_1(); /*op_30();*/                  break; // DB   DD
  case /*DD*/0x31: illegal_1(); /*op_31();*/                  break; // DB   DD
  case /*DD*/0x32: illegal_1(); /*op_32();*/                  break; // DB   DD
  case /*DD*/0x33: illegal_1(); /*op_33();*/                  break; // DB   DD
  case /*DD*/0x34: _R++; EAX; WM( EA, INC(RM(EA)) );          break; // INC  (IX+o)
  case /*DD*/0x35: _R++; EAX; WM( EA, DEC(RM(EA)) );          break; // DEC  (IX+o)
  case /*DD*/0x36: _R++; EAX; WM( EA, ARG() );                break; // LD   (IX+o),n
  case /*DD*/0x37: illegal_1(); /*op_37();*/                  break; // DB   DD

  case /*DD*/0x38: illegal_1(); /*op_38();*/                  break; // DB   DD
  case /*DD*/0x39: _R++; ADD16(_IX,_SP);                      break; // ADD  IX,SP
  case /*DD*/0x3a: illegal_1(); /*op_3a();*/                  break; // DB   DD
  case /*DD*/0x3b: illegal_1(); /*op_3b();*/                  break; // DB   DD
  case /*DD*/0x3c: illegal_1(); /*op_3c();*/                  break; // DB   DD
  case /*DD*/0x3d: illegal_1(); /*op_3d();*/                  break; // DB   DD
  case /*DD*/0x3e: illegal_1(); /*op_3e();*/                  break; // DB   DD
  case /*DD*/0x3f: illegal_1(); /*op_3f();*/                  break; // DB   DD

  case /*DD*/0x40: illegal_1(); /*op_40();*/                  break; // DB   DD
  case /*DD*/0x41: illegal_1(); /*op_41();*/                  break; // DB   DD
  case /*DD*/0x42: illegal_1(); /*op_42();*/                  break; // DB   DD
  case /*DD*/0x43: illegal_1(); /*op_43();*/                  break; // DB   DD
  case /*DD*/0x44: _R++; _B = _HX;                            break; // LD   B,HX
  case /*DD*/0x45: _R++; _B = _LX;                            break; // LD   B,LX
  case /*DD*/0x46: _R++; EAX; _B = RM(EA);                    break; // LD   B,(IX+o)
  case /*DD*/0x47: illegal_1(); /*op_47();*/                  break; // DB   DD

  case /*DD*/0x48: illegal_1(); /*op_48();*/                  break; // DB   DD
  case /*DD*/0x49: illegal_1(); /*op_49();*/                  break; // DB   DD
  case /*DD*/0x4a: illegal_1(); /*op_4a();*/                  break; // DB   DD
  case /*DD*/0x4b: illegal_1(); /*op_4b();*/                  break; // DB   DD
  case /*DD*/0x4c: _R++; _C = _HX;                            break; // LD   C,HX
  case /*DD*/0x4d: _R++; _C = _LX;                            break; // LD   C,LX
  case /*DD*/0x4e: _R++; EAX; _C = RM(EA);                    break; // LD   C,(IX+o)
  case /*DD*/0x4f: illegal_1(); /*op_4f();*/                  break; // DB   DD

  case /*DD*/0x50: illegal_1(); /*op_50();*/                  break; // DB   DD
  case /*DD*/0x51: illegal_1(); /*op_51();*/                  break; // DB   DD
  case /*DD*/0x52: illegal_1(); /*op_52();*/                  break; // DB   DD
  case /*DD*/0x53: illegal_1(); /*op_53();*/                  break; // DB   DD
  case /*DD*/0x54: _R++; _D = _HX;                            break; // LD   D,HX
  case /*DD*/0x55: _R++; _D = _LX;                            break; // LD   D,LX
  case /*DD*/0x56: _R++; EAX; _D = RM(EA);                    break; // LD   D,(IX+o)
  case /*DD*/0x57: illegal_1(); /*op_57();*/                  break; // DB   DD

  case /*DD*/0x58: illegal_1(); /*op_58();*/                  break; // DB   DD
  case /*DD*/0x59: illegal_1(); /*op_59();*/                  break; // DB   DD
  case /*DD*/0x5a: illegal_1(); /*op_5a();*/                  break; // DB   DD
  case /*DD*/0x5b: illegal_1(); /*op_5b();*/                  break; // DB   DD
  case /*DD*/0x5c: _R++; _E = _HX;                            break; // LD   E,HX
  case /*DD*/0x5d: _R++; _E = _LX;                            break; // LD   E,LX
  case /*DD*/0x5e: _R++; EAX; _E = RM(EA);                    break; // LD   E,(IX+o)
  case /*DD*/0x5f: illegal_1(); /*op_5f();*/                  break; // DB   DD

  case /*DD*/0x60: _R++; _HX = _B;                            break; // LD   HX,B
  case /*DD*/0x61: _R++; _HX = _C;                            break; // LD   HX,C
  case /*DD*/0x62: _R++; _HX = _D;                            break; // LD   HX,D
  case /*DD*/0x63: _R++; _HX = _E;                            break; // LD   HX,E
  case /*DD*/0x64:                                            break; // LD   HX,HX
  case /*DD*/0x65: _R++; _HX = _LX;                           break; // LD   HX,LX
  case /*DD*/0x66: _R++; EAX; _H = RM(EA);                    break; // LD   H,(IX+o)
  case /*DD*/0x67: _R++; _HX = _A;                            break; // LD   HX,A

  case /*DD*/0x68: _R++; _LX = _B;                            break; // LD   LX,B
  case /*DD*/0x69: _R++; _LX = _C;                            break; // LD   LX,C
  case /*DD*/0x6a: _R++; _LX = _D;                            break; // LD   LX,D
  case /*DD*/0x6b: _R++; _LX = _E;                            break; // LD   LX,E
  case /*DD*/0x6c: _R++; _LX = _HX;                           break; // LD   LX,HX
  case /*DD*/0x6d:                                            break; // LD   LX,LX
  case /*DD*/0x6e: _R++; EAX; _L = RM(EA);                    break; // LD   L,(IX+o)
  case /*DD*/0x6f: _R++; _LX = _A;                            break; // LD   LX,A

  case /*DD*/0x70: _R++; EAX; WM( EA, _B );                   break; // LD   (IX+o),B
  case /*DD*/0x71: _R++; EAX; WM( EA, _C );                   break; // LD   (IX+o),C
  case /*DD*/0x72: _R++; EAX; WM( EA, _D );                   break; // LD   (IX+o),D
  case /*DD*/0x73: _R++; EAX; WM( EA, _E );                   break; // LD   (IX+o),E
  case /*DD*/0x74: _R++; EAX; WM( EA, _H );                   break; // LD   (IX+o),H
  case /*DD*/0x75: _R++; EAX; WM( EA, _L );                   break; // LD   (IX+o),L
  case /*DD*/0x76: illegal_1(); /*op_76();*/                  break; // DB   DD
  case /*DD*/0x77: _R++; EAX; WM( EA, _A );                   break; // LD   (IX+o),A

  case /*DD*/0x78: illegal_1(); /*op_78();*/                  break; // DB   DD
  case /*DD*/0x79: illegal_1(); /*op_79();*/                  break; // DB   DD
  case /*DD*/0x7a: illegal_1(); /*op_7a();*/                  break; // DB   DD
  case /*DD*/0x7b: illegal_1(); /*op_7b();*/                  break; // DB   DD
  case /*DD*/0x7c: _R++; _A = _HX;                            break; // LD   A,HX
  case /*DD*/0x7d: _R++; _A = _LX;                            break; // LD   A,LX
  case /*DD*/0x7e: _R++; EAX; _A = RM(EA);                    break; // LD   A,(IX+o)
  case /*DD*/0x7f: illegal_1(); /*op_7f();*/                  break; // DB   DD

  case /*DD*/0x80: illegal_1(); /*op_80();*/                  break; // DB   DD
  case /*DD*/0x81: illegal_1(); /*op_81();*/                  break; // DB   DD
  case /*DD*/0x82: illegal_1(); /*op_82();*/                  break; // DB   DD
  case /*DD*/0x83: illegal_1(); /*op_83();*/                  break; // DB   DD
  case /*DD*/0x84: _R++; ADD(_HX);                            break; // ADD  A,HX
  case /*DD*/0x85: _R++; ADD(_LX);                            break; // ADD  A,LX
  case /*DD*/0x86: _R++; EAX; ADD(RM(EA));                    break; // ADD  A,(IX+o)
  case /*DD*/0x87: illegal_1(); /*op_87();*/                  break; // DB   DD

  case /*DD*/0x88: illegal_1(); /*op_88();*/                  break; // DB   DD
  case /*DD*/0x89: illegal_1(); /*op_89();*/                  break; // DB   DD
  case /*DD*/0x8a: illegal_1(); /*op_8a();*/                  break; // DB   DD
  case /*DD*/0x8b: illegal_1(); /*op_8b();*/                  break; // DB   DD
  case /*DD*/0x8c: _R++; ADC(_HX);                            break; // ADC  A,HX
  case /*DD*/0x8d: _R++; ADC(_LX);                            break; // ADC  A,LX
  case /*DD*/0x8e: _R++; EAX; ADC(RM(EA));                    break; // ADC  A,(IX+o)
  case /*DD*/0x8f: illegal_1(); /*op_8f();*/                  break; // DB   DD

  case /*DD*/0x90: illegal_1(); /*op_90();*/                  break; // DB   DD
  case /*DD*/0x91: illegal_1(); /*op_91();*/                  break; // DB   DD
  case /*DD*/0x92: illegal_1(); /*op_92();*/                  break; // DB   DD
  case /*DD*/0x93: illegal_1(); /*op_93();*/                  break; // DB   DD
  case /*DD*/0x94: _R++; SUB(_HX);                            break; // SUB  HX
  case /*DD*/0x95: _R++; SUB(_LX);                            break; // SUB  LX
  case /*DD*/0x96: _R++; EAX; SUB(RM(EA));                    break; // SUB  (IX+o)
  case /*DD*/0x97: illegal_1(); /*op_97();*/                  break; // DB   DD

  case /*DD*/0x98: illegal_1(); /*op_98();*/                  break; // DB   DD
  case /*DD*/0x99: illegal_1(); /*op_99();*/                  break; // DB   DD
  case /*DD*/0x9a: illegal_1(); /*op_9a();*/                  break; // DB   DD
  case /*DD*/0x9b: illegal_1(); /*op_9b();*/                  break; // DB   DD
  case /*DD*/0x9c: _R++; SBC(_HX);                            break; // SBC  A,HX
  case /*DD*/0x9d: _R++; SBC(_LX);                            break; // SBC  A,LX
  case /*DD*/0x9e: _R++; EAX; SBC(RM(EA));                    break; // SBC  A,(IX+o)
  case /*DD*/0x9f: illegal_1(); /*op_9f();*/                  break; // DB   DD

  case /*DD*/0xa0: illegal_1(); /*op_a0();*/                  break; // DB   DD
  case /*DD*/0xa1: illegal_1(); /*op_a1();*/                  break; // DB   DD
  case /*DD*/0xa2: illegal_1(); /*op_a2();*/                  break; // DB   DD
  case /*DD*/0xa3: illegal_1(); /*op_a3();*/                  break; // DB   DD
  case /*DD*/0xa4: _R++; AND(_HX);                            break; // AND  HX
  case /*DD*/0xa5: _R++; AND(_LX);                            break; // AND  LX
  case /*DD*/0xa6: _R++; EAX; AND(RM(EA));                    break; // AND  (IX+o)
  case /*DD*/0xa7: illegal_1(); /*op_a7();*/                  break; // DB   DD

  case /*DD*/0xa8: illegal_1(); /*op_a8();*/                  break; // DB   DD
  case /*DD*/0xa9: illegal_1(); /*op_a9();*/                  break; // DB   DD
  case /*DD*/0xaa: illegal_1(); /*op_aa();*/                  break; // DB   DD
  case /*DD*/0xab: illegal_1(); /*op_ab();*/                  break; // DB   DD
  case /*DD*/0xac: _R++; XOR(_HX);                            break; // XOR  HX
  case /*DD*/0xad: _R++; XOR(_LX);                            break; // XOR  LX
  case /*DD*/0xae: _R++; EAX; XOR(RM(EA));                    break; // XOR  (IX+o)
  case /*DD*/0xaf: illegal_1(); /*op_af();*/                  break; // DB   DD

  case /*DD*/0xb0: illegal_1(); /*op_b0();*/                  break; // DB   DD
  case /*DD*/0xb1: illegal_1(); /*op_b1();*/                  break; // DB   DD
  case /*DD*/0xb2: illegal_1(); /*op_b2();*/                  break; // DB   DD
  case /*DD*/0xb3: illegal_1(); /*op_b3();*/                  break; // DB   DD
  case /*DD*/0xb4: _R++; OR(_HX);                             break; // OR   HX
  case /*DD*/0xb5: _R++; OR(_LX);                             break; // OR   LX
  case /*DD*/0xb6: _R++; EAX; OR(RM(EA));                     break; // OR   (IX+o)
  case /*DD*/0xb7: illegal_1(); /*op_b7();*/                  break; // DB   DD

  case /*DD*/0xb8: illegal_1(); /*op_b8();*/                  break; // DB   DD
  case /*DD*/0xb9: illegal_1(); /*op_b9();*/                  break; // DB   DD
  case /*DD*/0xba: illegal_1(); /*op_ba();*/                  break; // DB   DD
  case /*DD*/0xbb: illegal_1(); /*op_bb();*/                  break; // DB   DD
  case /*DD*/0xbc: _R++; CP(_HX);                             break; // CP   HX
  case /*DD*/0xbd: _R++; CP(_LX);                             break; // CP   LX
  case /*DD*/0xbe: _R++; EAX; CP(RM(EA));                     break; // CP   (IX+o)
  case /*DD*/0xbf: illegal_1(); /*op_bf();*/                  break; // DB   DD

  case /*DD*/0xc0: illegal_1(); /*op_c0();*/                  break; // DB   DD
  case /*DD*/0xc1: illegal_1(); /*op_c1();*/                  break; // DB   DD
  case /*DD*/0xc2: illegal_1(); /*op_c2();*/                  break; // DB   DD
  case /*DD*/0xc3: illegal_1(); /*op_c3();*/                  break; // DB   DD
  case /*DD*/0xc4: illegal_1(); /*op_c4();*/                  break; // DB   DD
  case /*DD*/0xc5: illegal_1(); /*op_c5();*/                  break; // DB   DD
  case /*DD*/0xc6: illegal_1(); /*op_c6();*/                  break; // DB   DD
  case /*DD*/0xc7: illegal_1(); /*op_c7();*/                  break; // DB   DD

  case /*DD*/0xc8: illegal_1(); /*op_c8();*/                  break; // DB   DD
  case /*DD*/0xc9: illegal_1(); /*op_c9();*/                  break; // DB   DD
  case /*DD*/0xca: illegal_1(); /*op_ca();*/                  break; // DB   DD
// pretty sure this is supposed to be ARG()
  case /*DD*/0xcb: _R++; EAX; EXEC_EA(xycb,ARG(),EA);         break; // **   DD CB xx
  case /*DD*/0xcc: illegal_1(); /*op_cc();*/                  break; // DB   DD
  case /*DD*/0xcd: illegal_1(); /*op_cd();*/                  break; // DB   DD
  case /*DD*/0xce: illegal_1(); /*op_ce();*/                  break; // DB   DD
  case /*DD*/0xcf: illegal_1(); /*op_cf();*/                  break; // DB   DD

  case /*DD*/0xd0: illegal_1(); /*op_d0();*/                  break; // DB   DD
  case /*DD*/0xd1: illegal_1(); /*op_d1();*/                  break; // DB   DD
  case /*DD*/0xd2: illegal_1(); /*op_d2();*/                  break; // DB   DD
  case /*DD*/0xd3: illegal_1(); /*op_d3();*/                  break; // DB   DD
  case /*DD*/0xd4: illegal_1(); /*op_d4();*/                  break; // DB   DD
  case /*DD*/0xd5: illegal_1(); /*op_d5();*/                  break; // DB   DD
  case /*DD*/0xd6: illegal_1(); /*op_d6();*/                  break; // DB   DD
  case /*DD*/0xd7: illegal_1(); /*op_d7();*/                  break; // DB   DD

  case /*DD*/0xd8: illegal_1(); /*op_d8();*/                  break; // DB   DD
  case /*DD*/0xd9: illegal_1(); /*op_d9();*/                  break; // DB   DD
  case /*DD*/0xda: illegal_1(); /*op_da();*/                  break; // DB   DD
  case /*DD*/0xdb: illegal_1(); /*op_db();*/                  break; // DB   DD
  case /*DD*/0xdc: illegal_1(); /*op_dc();*/                  break; // DB   DD
  case /*DD*/0xdd: illegal_1(); /*op_dd();*/                  break; // DB   DD
  case /*DD*/0xde: illegal_1(); /*op_de();*/                  break; // DB   DD
  case /*DD*/0xdf: illegal_1(); /*op_df();*/                  break; // DB   DD

  case /*DD*/0xe0: illegal_1(); /*op_e0();*/                  break; // DB   DD
  case /*DD*/0xe1: _R++; _IX = POP();                         break; // POP  IX
  case /*DD*/0xe2: illegal_1(); /*op_e2();*/                  break; // DB   DD
  case /*DD*/0xe3: _R++; _IX = EXSP16(_IX);                   break; // EX   (SP),IX
  case /*DD*/0xe4: illegal_1(); /*op_e4();*/                  break; // DB   DD
  case /*DD*/0xe5: _R++; PUSH( _IX );                         break; // PUSH IX
  case /*DD*/0xe6: illegal_1(); /*op_e6();*/                  break; // DB   DD
  case /*DD*/0xe7: illegal_1(); /*op_e7();*/                  break; // DB   DD

  case /*DD*/0xe8: illegal_1(); /*op_e8();*/                  break; // DB   DD
  case /*DD*/0xe9: _R++; _PC = _IX;                           break; // JP   (IX)
  case /*DD*/0xea: illegal_1(); /*op_ea();*/                  break; // DB   DD
  case /*DD*/0xeb: illegal_1(); /*op_eb();*/                  break; // DB   DD
  case /*DD*/0xec: illegal_1(); /*op_ec();*/                  break; // DB   DD
  case /*DD*/0xed: illegal_1(); /*op_ed();*/                  break; // DB   DD
  case /*DD*/0xee: illegal_1(); /*op_ee();*/                  break; // DB   DD
  case /*DD*/0xef: illegal_1(); /*op_ef();*/                  break; // DB   DD

  case /*DD*/0xf0: illegal_1(); /*op_f0();*/                  break; // DB   DD
  case /*DD*/0xf1: illegal_1(); /*op_f1();*/                  break; // DB   DD
  case /*DD*/0xf2: illegal_1(); /*op_f2();*/                  break; // DB   DD
  case /*DD*/0xf3: illegal_1(); /*op_f3();*/                  break; // DB   DD
  case /*DD*/0xf4: illegal_1(); /*op_f4();*/                  break; // DB   DD
  case /*DD*/0xf5: illegal_1(); /*op_f5();*/                  break; // DB   DD
  case /*DD*/0xf6: illegal_1(); /*op_f6();*/                  break; // DB   DD
  case /*DD*/0xf7: illegal_1(); /*op_f7();*/                  break; // DB   DD

  case /*DD*/0xf8: illegal_1(); /*op_f8();*/                  break; // DB   DD
  case /*DD*/0xf9: _R++; _SP = _IX;                           break; // LD   SP,IX
  case /*DD*/0xfa: illegal_1(); /*op_fa();*/                  break; // DB   DD
  case /*DD*/0xfb: illegal_1(); /*op_fb();*/                  break; // DB   DD
  case /*DD*/0xfc: illegal_1(); /*op_fc();*/                  break; // DB   DD
  case /*DD*/0xfd: illegal_1(); /*op_fd();*/                  break; // DB   DD
  case /*DD*/0xfe: illegal_1(); /*op_fe();*/                  break; // DB   DD
  case /*DD*/0xff: illegal_1(); /*op_ff();*/                  break; // DB   DD
  }
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE void z80_exec_ed(struct Z80_STATE *state, uint8 op) {

//printf("PC=%04X exec ed %02X\n",_PC, op);fflush(stdout);

  switch(op) {
  case /*ED*/0x00: illegal_2();                      break; // DB   ED
  case /*ED*/0x01: illegal_2();                      break; // DB   ED
  case /*ED*/0x02: illegal_2();                      break; // DB   ED
  case /*ED*/0x03: illegal_2();                      break; // DB   ED
  case /*ED*/0x04: illegal_2();                      break; // DB   ED
  case /*ED*/0x05: illegal_2();                      break; // DB   ED
  case /*ED*/0x06: illegal_2();                      break; // DB   ED
  case /*ED*/0x07: illegal_2();                      break; // DB   ED

  case /*ED*/0x08: illegal_2();                      break; // DB   ED
  case /*ED*/0x09: illegal_2();                      break; // DB   ED
  case /*ED*/0x0a: illegal_2();                      break; // DB   ED
  case /*ED*/0x0b: illegal_2();                      break; // DB   ED
  case /*ED*/0x0c: illegal_2();                      break; // DB   ED
  case /*ED*/0x0d: illegal_2();                      break; // DB   ED
  case /*ED*/0x0e: illegal_2();                      break; // DB   ED
  case /*ED*/0x0f: illegal_2();                      break; // DB   ED

  case /*ED*/0x10: illegal_2();                      break; // DB   ED
  case /*ED*/0x11: illegal_2();                      break; // DB   ED
  case /*ED*/0x12: illegal_2();                      break; // DB   ED
  case /*ED*/0x13: illegal_2();                      break; // DB   ED
  case /*ED*/0x14: illegal_2();                      break; // DB   ED
  case /*ED*/0x15: illegal_2();                      break; // DB   ED
  case /*ED*/0x16: illegal_2();                      break; // DB   ED
  case /*ED*/0x17: illegal_2();                      break; // DB   ED

  case /*ED*/0x18: illegal_2();                      break; // DB   ED
  case /*ED*/0x19: illegal_2();                      break; // DB   ED
  case /*ED*/0x1a: illegal_2();                      break; // DB   ED
  case /*ED*/0x1b: illegal_2();                      break; // DB   ED
  case /*ED*/0x1c: illegal_2();                      break; // DB   ED
  case /*ED*/0x1d: illegal_2();                      break; // DB   ED
  case /*ED*/0x1e: illegal_2();                      break; // DB   ED
  case /*ED*/0x1f: illegal_2();                      break; // DB   ED

  case /*ED*/0x20: illegal_2();                      break; // DB   ED
  case /*ED*/0x21: illegal_2();                      break; // DB   ED
  case /*ED*/0x22: illegal_2();                      break; // DB   ED
  case /*ED*/0x23: illegal_2();                      break; // DB   ED
  case /*ED*/0x24: illegal_2();                      break; // DB   ED
  case /*ED*/0x25: illegal_2();                      break; // DB   ED
  case /*ED*/0x26: illegal_2();                      break; // DB   ED
  case /*ED*/0x27: illegal_2();                      break; // DB   ED

  case /*ED*/0x28: illegal_2();                      break; // DB   ED
  case /*ED*/0x29: illegal_2();                      break; // DB   ED
  case /*ED*/0x2a: illegal_2();                      break; // DB   ED
  case /*ED*/0x2b: illegal_2();                      break; // DB   ED
  case /*ED*/0x2c: illegal_2();                      break; // DB   ED
  case /*ED*/0x2d: illegal_2();                      break; // DB   ED
  case /*ED*/0x2e: illegal_2();                      break; // DB   ED
  case /*ED*/0x2f: illegal_2();                      break; // DB   ED

  case /*ED*/0x30: illegal_2();                      break; // DB   ED
  case /*ED*/0x31: illegal_2();                      break; // DB   ED
  case /*ED*/0x32: illegal_2();                      break; // DB   ED
  case /*ED*/0x33: illegal_2();                      break; // DB   ED
  case /*ED*/0x34: illegal_2();                      break; // DB   ED
  case /*ED*/0x35: illegal_2();                      break; // DB   ED
  case /*ED*/0x36: illegal_2();                      break; // DB   ED
  case /*ED*/0x37: illegal_2();                      break; // DB   ED

  case /*ED*/0x38: illegal_2();                      break; // DB   ED
  case /*ED*/0x39: illegal_2();                      break; // DB   ED
  case /*ED*/0x3a: illegal_2();                      break; // DB   ED
  case /*ED*/0x3b: illegal_2();                      break; // DB   ED
  case /*ED*/0x3c: illegal_2();                      break; // DB   ED
  case /*ED*/0x3d: illegal_2();                      break; // DB   ED
  case /*ED*/0x3e: illegal_2();                      break; // DB   ED
  case /*ED*/0x3f: illegal_2();                      break; // DB   ED

  case /*ED*/0x40: _B = Z80IN(_BC); _F = (_F & CF) | SZP[_B]; break; // IN   B,(C)
  case /*ED*/0x41: Z80OUT(_BC,_B);                   break; // OUT  (C),B
  case /*ED*/0x42: SBC16( _BC );                     break; // SBC  HL,BC
  case /*ED*/0x43: WM16( ARG16(), _BC );             break; // LD   (w),BC
  case /*ED*/0x44: NEG;                              break; // NEG
  case /*ED*/0x45: RETN;                             break; // RETN;
  case /*ED*/0x46: IM(0);                            break; // IM   0
  case /*ED*/0x47: LD_I_A;                           break; // LD   I,A

  case /*ED*/0x48: _C = Z80IN(_BC); _F = (_F & CF) | SZP[_C]; break; // IN   C,(C)
  case /*ED*/0x49: Z80OUT(_BC,_C);                   break; // OUT  (C),C
  case /*ED*/0x4a: ADC16( _BC );                     break; // ADC  HL,BC
  case /*ED*/0x4b: _BC = RM16( ARG16() );            break; // LD   BC,(w)
  case /*ED*/0x4c: NEG;                              break; // NEG
  case /*ED*/0x4d: RETI;                             break; // RETI
  case /*ED*/0x4e: IM(0);                            break; // IM   0
  case /*ED*/0x4f: LD_R_A;                           break; // LD   R,A

  case /*ED*/0x50: _D = Z80IN(_BC); _F = (_F & CF) | SZP[_D]; break; // IN   D,(C)
  case /*ED*/0x51: Z80OUT(_BC,_D);                   break; // OUT  (C),D
  case /*ED*/0x52: SBC16( _DE );                     break; // SBC  HL,DE
  case /*ED*/0x53: WM16( ARG16(), _DE );             break; // LD   (w),DE
  case /*ED*/0x54: NEG;                              break; // NEG
  case /*ED*/0x55: RETN;                             break; // RETN;
  case /*ED*/0x56: IM(1);                            break; // IM   1
  case /*ED*/0x57: LD_A_I;                           break; // LD   A,I

  case /*ED*/0x58: _E = Z80IN(_BC); _F = (_F & CF) | SZP[_E]; break; // IN   E,(C)
  case /*ED*/0x59: Z80OUT(_BC,_E);                   break; // OUT  (C),E
  case /*ED*/0x5a: ADC16( _DE );                     break; // ADC  HL,DE
  case /*ED*/0x5b: _DE = RM16( ARG16() );            break; // LD   DE,(w)
  case /*ED*/0x5c: NEG;                              break; // NEG
  case /*ED*/0x5d: RETI;                             break; // RETI
  case /*ED*/0x5e: IM(2);                            break; // IM   2
  case /*ED*/0x5f: LD_A_R;                           break; // LD   A,R

  case /*ED*/0x60: _H = Z80IN(_BC); _F = (_F & CF) | SZP[_H]; break; // IN   H,(C)
  case /*ED*/0x61: Z80OUT(_BC,_H);                   break; // OUT  (C),H
  case /*ED*/0x62: SBC16( _HL );                     break; // SBC  HL,HL
  case /*ED*/0x63: WM16( ARG16(), _HL );             break; // LD   (w),HL
  case /*ED*/0x64: NEG;                              break; // NEG
  case /*ED*/0x65: RETN;                             break; // RETN;
  case /*ED*/0x66: IM (0);                           break; // IM   0
  case /*ED*/0x67: RRD;                              break; // RRD  (HL)

  case /*ED*/0x68: _L = Z80IN(_BC); _F = (_F & CF) | SZP[_L]; break; // IN   L,(C)
  case /*ED*/0x69: Z80OUT(_BC,_L);                   break; // OUT  (C),L
  case /*ED*/0x6a: ADC16( _HL );                     break; // ADC  HL,HL
  case /*ED*/0x6b: _HL = RM16( ARG16() );            break; // LD   HL,(w)
  case /*ED*/0x6c: NEG;                              break; // NEG
  case /*ED*/0x6d: RETI;                             break; // RETI
  case /*ED*/0x6e: IM(0);                            break; // IM   0
  case /*ED*/0x6f: RLD;                              break; // RLD  (HL)

  case /*ED*/0x70: _F = (_F & CF) | SZP[Z80IN(_BC)];    break; // IN   0,(C)
  case /*ED*/0x71: Z80OUT(_BC,0);                    break; // OUT  (C),0
  case /*ED*/0x72: SBC16( _SP );                     break; // SBC  HL,SP
  case /*ED*/0x73: WM16( ARG16(), _SP );             break; // LD   (w),SP
  case /*ED*/0x74: NEG;                              break; // NEG
  case /*ED*/0x75: RETN;                             break; // RETN;
  case /*ED*/0x76: IM(1);                            break; // IM   1
  case /*ED*/0x77: illegal_2();                      break; // DB   ED,77

  case /*ED*/0x78: _A = Z80IN(_BC); _F = (_F & CF) | SZP[_A]; break; // IN   E,(C)
  case /*ED*/0x79: Z80OUT(_BC,_A);                   break; // OUT  (C),E
  case /*ED*/0x7a: ADC16( _SP );                     break; // ADC  HL,SP
  case /*ED*/0x7b: _SP = RM16( ARG16() );            break; // LD   SP,(w)
  case /*ED*/0x7c: NEG;                              break; // NEG
  case /*ED*/0x7d: RETI;                             break; // RETI
  case /*ED*/0x7e: IM(2);                            break; // IM   2
  case /*ED*/0x7f: illegal_2();                      break; // DB   ED,7F

  case /*ED*/0x80: illegal_2();                      break; // DB   ED
  case /*ED*/0x81: illegal_2();                      break; // DB   ED
  case /*ED*/0x82: illegal_2();                      break; // DB   ED
  case /*ED*/0x83: illegal_2();                      break; // DB   ED
  case /*ED*/0x84: illegal_2();                      break; // DB   ED
  case /*ED*/0x85: illegal_2();                      break; // DB   ED
  case /*ED*/0x86: illegal_2();                      break; // DB   ED
  case /*ED*/0x87: illegal_2();                      break; // DB   ED

  case /*ED*/0x88: illegal_2();                      break; // DB   ED
  case /*ED*/0x89: illegal_2();                      break; // DB   ED
  case /*ED*/0x8a: illegal_2();                      break; // DB   ED
  case /*ED*/0x8b: illegal_2();                      break; // DB   ED
  case /*ED*/0x8c: illegal_2();                      break; // DB   ED
  case /*ED*/0x8d: illegal_2();                      break; // DB   ED
  case /*ED*/0x8e: illegal_2();                      break; // DB   ED
  case /*ED*/0x8f: illegal_2();                      break; // DB   ED

  case /*ED*/0x90: illegal_2();                      break; // DB   ED
  case /*ED*/0x91: illegal_2();                      break; // DB   ED
  case /*ED*/0x92: illegal_2();                      break; // DB   ED
  case /*ED*/0x93: illegal_2();                      break; // DB   ED
  case /*ED*/0x94: illegal_2();                      break; // DB   ED
  case /*ED*/0x95: illegal_2();                      break; // DB   ED
  case /*ED*/0x96: illegal_2();                      break; // DB   ED
  case /*ED*/0x97: illegal_2();                      break; // DB   ED

  case /*ED*/0x98: illegal_2();                      break; // DB   ED
  case /*ED*/0x99: illegal_2();                      break; // DB   ED
  case /*ED*/0x9a: illegal_2();                      break; // DB   ED
  case /*ED*/0x9b: illegal_2();                      break; // DB   ED
  case /*ED*/0x9c: illegal_2();                      break; // DB   ED
  case /*ED*/0x9d: illegal_2();                      break; // DB   ED
  case /*ED*/0x9e: illegal_2();                      break; // DB   ED
  case /*ED*/0x9f: illegal_2();                      break; // DB   ED

  case /*ED*/0xa0: LDI;                              break; // LDI
  case /*ED*/0xa1: CPI;                              break; // CPI
  case /*ED*/0xa2: INI;                              break; // INI
  case /*ED*/0xa3: OUTI;                             break; // OUTI
  case /*ED*/0xa4: illegal_2();                      break; // DB   ED
  case /*ED*/0xa5: illegal_2();                      break; // DB   ED
  case /*ED*/0xa6: illegal_2();                      break; // DB   ED
  case /*ED*/0xa7: illegal_2();                      break; // DB   ED

  case /*ED*/0xa8: LDD;                              break; // LDD
  case /*ED*/0xa9: CPD;                              break; // CPD
  case /*ED*/0xaa: IND;                              break; // IND
  case /*ED*/0xab: OUTD;                             break; // OUTD
  case /*ED*/0xac: illegal_2();                      break; // DB   ED
  case /*ED*/0xad: illegal_2();                      break; // DB   ED
  case /*ED*/0xae: illegal_2();                      break; // DB   ED
  case /*ED*/0xaf: illegal_2();                      break; // DB   ED

  case /*ED*/0xb0: LDIR;                             break; // LDIR
  case /*ED*/0xb1: CPIR;                             break; // CPIR
  case /*ED*/0xb2: INIR;                             break; // INIR
  case /*ED*/0xb3: OTIR;                             break; // OTIR
  case /*ED*/0xb4: illegal_2();                      break; // DB   ED
  case /*ED*/0xb5: illegal_2();                      break; // DB   ED
  case /*ED*/0xb6: illegal_2();                      break; // DB   ED
  case /*ED*/0xb7: illegal_2();                      break; // DB   ED

  case /*ED*/0xb8: LDDR;                             break; // LDDR
  case /*ED*/0xb9: CPDR;                             break; // CPDR
  case /*ED*/0xba: INDR;                             break; // INDR
  case /*ED*/0xbb: OTDR;                             break; // OTDR
  case /*ED*/0xbc: illegal_2();                      break; // DB   ED
  case /*ED*/0xbd: illegal_2();                      break; // DB   ED
  case /*ED*/0xbe: illegal_2();                      break; // DB   ED
  case /*ED*/0xbf: illegal_2();                      break; // DB   ED

  case /*ED*/0xc0: illegal_2();                      break; // DB   ED
  case /*ED*/0xc1: illegal_2();                      break; // DB   ED
  case /*ED*/0xc2: illegal_2();                      break; // DB   ED
  case /*ED*/0xc3: illegal_2();                      break; // DB   ED
  case /*ED*/0xc4: illegal_2();                      break; // DB   ED
  case /*ED*/0xc5: illegal_2();                      break; // DB   ED
  case /*ED*/0xc6: illegal_2();                      break; // DB   ED
  case /*ED*/0xc7: illegal_2();                      break; // DB   ED

  case /*ED*/0xc8: illegal_2();                      break; // DB   ED
  case /*ED*/0xc9: illegal_2();                      break; // DB   ED
  case /*ED*/0xca: illegal_2();                      break; // DB   ED
  case /*ED*/0xcb: illegal_2();                      break; // DB   ED
  case /*ED*/0xcc: illegal_2();                      break; // DB   ED
  case /*ED*/0xcd: illegal_2();                      break; // DB   ED
  case /*ED*/0xce: illegal_2();                      break; // DB   ED
  case /*ED*/0xcf: illegal_2();                      break; // DB   ED

  case /*ED*/0xd0: illegal_2();                      break; // DB   ED
  case /*ED*/0xd1: illegal_2();                      break; // DB   ED
  case /*ED*/0xd2: illegal_2();                      break; // DB   ED
  case /*ED*/0xd3: illegal_2();                      break; // DB   ED
  case /*ED*/0xd4: illegal_2();                      break; // DB   ED
  case /*ED*/0xd5: illegal_2();                      break; // DB   ED
  case /*ED*/0xd6: illegal_2();                      break; // DB   ED
  case /*ED*/0xd7: illegal_2();                      break; // DB   ED

  case /*ED*/0xd8: illegal_2();                      break; // DB   ED
  case /*ED*/0xd9: illegal_2();                      break; // DB   ED
  case /*ED*/0xda: illegal_2();                      break; // DB   ED
  case /*ED*/0xdb: illegal_2();                      break; // DB   ED
  case /*ED*/0xdc: illegal_2();                      break; // DB   ED
  case /*ED*/0xdd: illegal_2();                      break; // DB   ED
  case /*ED*/0xde: illegal_2();                      break; // DB   ED
  case /*ED*/0xdf: illegal_2();                      break; // DB   ED

  case /*ED*/0xe0: illegal_2();                      break; // DB   ED
  case /*ED*/0xe1: illegal_2();                      break; // DB   ED
  case /*ED*/0xe2: illegal_2();                      break; // DB   ED
  case /*ED*/0xe3: illegal_2();                      break; // DB   ED
  case /*ED*/0xe4: illegal_2();                      break; // DB   ED
  case /*ED*/0xe5: illegal_2();                      break; // DB   ED
  case /*ED*/0xe6: illegal_2();                      break; // DB   ED
  case /*ED*/0xe7: illegal_2();                      break; // DB   ED

  case /*ED*/0xe8: illegal_2();                      break; // DB   ED
  case /*ED*/0xe9: illegal_2();                      break; // DB   ED
  case /*ED*/0xea: illegal_2();                      break; // DB   ED
  case /*ED*/0xeb: illegal_2();                      break; // DB   ED
  case /*ED*/0xec: illegal_2();                      break; // DB   ED
  case /*ED*/0xed: illegal_2();                      break; // DB   ED
  case /*ED*/0xee: illegal_2();                      break; // DB   ED
  case /*ED*/0xef: illegal_2();                      break; // DB   ED

  case /*ED*/0xf0: illegal_2();                      break; // DB   ED
  case /*ED*/0xf1: illegal_2();                      break; // DB   ED
  case /*ED*/0xf2: illegal_2();                      break; // DB   ED
  case /*ED*/0xf3: illegal_2();                      break; // DB   ED
  case /*ED*/0xf4: illegal_2();                      break; // DB   ED
  case /*ED*/0xf5: illegal_2();                      break; // DB   ED
  case /*ED*/0xf6: illegal_2();                      break; // DB   ED
  case /*ED*/0xf7: illegal_2();                      break; // DB   ED

  case /*ED*/0xf8: illegal_2();                      break; // DB   ED
  case /*ED*/0xf9: illegal_2();                      break; // DB   ED
  case /*ED*/0xfa: illegal_2();                      break; // DB   ED
  case /*ED*/0xfb: illegal_2();                      break; // DB   ED
  case /*ED*/0xfc: illegal_2();                      break; // DB   ED
  case /*ED*/0xfd: illegal_2();                      break; // DB   ED
  case /*ED*/0xfe: illegal_2();                      break; // DB   ED
  case /*ED*/0xff: illegal_2();                      break; // DB   ED
  }
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE void z80_exec_fd(struct Z80_STATE *state, uint8 op) {
  uint16 EA;

//printf("PC=%04X exec fd %02X\n",_PC, op);fflush(stdout);

  switch(op) {
  case /*FD*/0x00: illegal_1(); /*op_00();*/                  break; // DB   FD
  case /*FD*/0x01: illegal_1(); /*op_01();*/                  break; // DB   FD
  case /*FD*/0x02: illegal_1(); /*op_02();*/                  break; // DB   FD
  case /*FD*/0x03: illegal_1(); /*op_03();*/                  break; // DB   FD
  case /*FD*/0x04: illegal_1(); /*op_04();*/                  break; // DB   FD
  case /*FD*/0x05: illegal_1(); /*op_05();*/                  break; // DB   FD
  case /*FD*/0x06: illegal_1(); /*op_06();*/                  break; // DB   FD
  case /*FD*/0x07: illegal_1(); /*op_07();*/                  break; // DB   FD

  case /*FD*/0x08: illegal_1(); /*op_08();*/                  break; // DB   FD
  case /*FD*/0x09: _R++; ADD16(_IY,_BC);                      break; // ADD  IY,BC
  case /*FD*/0x0a: illegal_1(); /*op_0a();*/                  break; // DB   FD
  case /*FD*/0x0b: illegal_1(); /*op_0b();*/                  break; // DB   FD
  case /*FD*/0x0c: illegal_1(); /*op_0c();*/                  break; // DB   FD
  case /*FD*/0x0d: illegal_1(); /*op_0d();*/                  break; // DB   FD
  case /*FD*/0x0e: illegal_1(); /*op_0e();*/                  break; // DB   FD
  case /*FD*/0x0f: illegal_1(); /*op_0f();*/                  break; // DB   FD

  case /*FD*/0x10: illegal_1(); /*op_10();*/                  break; // DB   FD
  case /*FD*/0x11: illegal_1(); /*op_11();*/                  break; // DB   FD
  case /*FD*/0x12: illegal_1(); /*op_12();*/                  break; // DB   FD
  case /*FD*/0x13: illegal_1(); /*op_13();*/                  break; // DB   FD
  case /*FD*/0x14: illegal_1(); /*op_14();*/                  break; // DB   FD
  case /*FD*/0x15: illegal_1(); /*op_15();*/                  break; // DB   FD
  case /*FD*/0x16: illegal_1(); /*op_16();*/                  break; // DB   FD
  case /*FD*/0x17: illegal_1(); /*op_17();*/                  break; // DB   FD

  case /*FD*/0x18: illegal_1(); /*op_18();*/                  break; // DB   FD
  case /*FD*/0x19: _R++; ADD16(_IY,_DE);                      break; // ADD  IY,DE
  case /*FD*/0x1a: illegal_1(); /*op_1a();*/                  break; // DB   FD
  case /*FD*/0x1b: illegal_1(); /*op_1b();*/                  break; // DB   FD
  case /*FD*/0x1c: illegal_1(); /*op_1c();*/                  break; // DB   FD
  case /*FD*/0x1d: illegal_1(); /*op_1d();*/                  break; // DB   FD
  case /*FD*/0x1e: illegal_1(); /*op_1e();*/                  break; // DB   FD
  case /*FD*/0x1f: illegal_1(); /*op_1f();*/                  break; // DB   FD

  case /*FD*/0x20: illegal_1(); /*op_20();*/                  break; // DB   FD
  case /*FD*/0x21: _R++; _IY = ARG16();                       break; // LD   IY,w
  case /*FD*/0x22: _R++; WM16( ARG16(), _IY );                break; // LD   (w),IY
  case /*FD*/0x23: _R++; _IY++;                               break; // INC  IY
  case /*FD*/0x24: _R++; _HY = INC(_HY);                      break; // INC  HY
  case /*FD*/0x25: _R++; _HY = DEC(_HY);                      break; // DEC  HY
  case /*FD*/0x26: _R++; _HY = ARG();                         break; // LD   HY,n
  case /*FD*/0x27: illegal_1(); /*op_27();*/                  break; // DB   FD

  case /*FD*/0x28: illegal_1(); /*op_28();*/                  break; // DB   FD
  case /*FD*/0x29: _R++; ADD16(_IY,_IY);                      break; // ADD  IY,IY
  case /*FD*/0x2a: _R++; _IY = RM16( ARG16() );               break; // LD   IY,(w)
  case /*FD*/0x2b: _R++; _IY--;                               break; // DEC  IY
  case /*FD*/0x2c: _R++; _LY = INC(_LY);                      break; // INC  LY
  case /*FD*/0x2d: _R++; _LY = DEC(_LY);                      break; // DEC  LY
  case /*FD*/0x2e: _R++; _LY = ARG();                         break; // LD   LY,n
  case /*FD*/0x2f: illegal_1(); /*op_2f();*/                  break; // DB   FD

  case /*FD*/0x30: illegal_1(); /*op_30();*/                  break; // DB   FD
  case /*FD*/0x31: illegal_1(); /*op_31();*/                  break; // DB   FD
  case /*FD*/0x32: illegal_1(); /*op_32();*/                  break; // DB   FD
  case /*FD*/0x33: illegal_1(); /*op_33();*/                  break; // DB   FD
  case /*FD*/0x34: _R++; EAY; WM( EA, INC(RM(EA)) );          break; // INC  (IY+o)
  case /*FD*/0x35: _R++; EAY; WM( EA, DEC(RM(EA)) );          break; // DEC  (IY+o)
  case /*FD*/0x36: _R++; EAY; WM( EA, ARG() );                break; // LD   (IY+o),n
  case /*FD*/0x37: illegal_1(); /*op_37();*/                  break; // DB   FD

  case /*FD*/0x38: illegal_1(); /*op_38();*/                  break; // DB   FD
  case /*FD*/0x39: _R++; ADD16(_IY,_SP);                      break; // ADD  IY,SP
  case /*FD*/0x3a: illegal_1(); /*op_3a();*/                  break; // DB   FD
  case /*FD*/0x3b: illegal_1(); /*op_3b();*/                  break; // DB   FD
  case /*FD*/0x3c: illegal_1(); /*op_3c();*/                  break; // DB   FD
  case /*FD*/0x3d: illegal_1(); /*op_3d();*/                  break; // DB   FD
  case /*FD*/0x3e: illegal_1(); /*op_3e();*/                  break; // DB   FD
  case /*FD*/0x3f: illegal_1(); /*op_3f();*/                  break; // DB   FD

  case /*FD*/0x40: illegal_1(); /*op_40();*/                  break; // DB   FD
  case /*FD*/0x41: illegal_1(); /*op_41();*/                  break; // DB   FD
  case /*FD*/0x42: illegal_1(); /*op_42();*/                  break; // DB   FD
  case /*FD*/0x43: illegal_1(); /*op_43();*/                  break; // DB   FD
  case /*FD*/0x44: _R++; _B = _HY;                            break; // LD   B,HY
  case /*FD*/0x45: _R++; _B = _LY;                            break; // LD   B,LY
  case /*FD*/0x46: _R++; EAY; _B = RM(EA);                    break; // LD   B,(IY+o)
  case /*FD*/0x47: illegal_1(); /*op_47();*/                  break; // DB   FD

  case /*FD*/0x48: illegal_1(); /*op_48();*/                  break; // DB   FD
  case /*FD*/0x49: illegal_1(); /*op_49();*/                  break; // DB   FD
  case /*FD*/0x4a: illegal_1(); /*op_4a();*/                  break; // DB   FD
  case /*FD*/0x4b: illegal_1(); /*op_4b();*/                  break; // DB   FD
  case /*FD*/0x4c: _R++; _C = _HY;                            break; // LD   C,HY
  case /*FD*/0x4d: _R++; _C = _LY;                            break; // LD   C,LY
  case /*FD*/0x4e: _R++; EAY; _C = RM(EA);                    break; // LD   C,(IY+o)
  case /*FD*/0x4f: illegal_1(); /*op_4f();*/                  break; // DB   FD

  case /*FD*/0x50: illegal_1(); /*op_50();*/                  break; // DB   FD
  case /*FD*/0x51: illegal_1(); /*op_51();*/                  break; // DB   FD
  case /*FD*/0x52: illegal_1(); /*op_52();*/                  break; // DB   FD
  case /*FD*/0x53: illegal_1(); /*op_53();*/                  break; // DB   FD
  case /*FD*/0x54: _R++; _D = _HY;                            break; // LD   D,HY
  case /*FD*/0x55: _R++; _D = _LY;                            break; // LD   D,LY
  case /*FD*/0x56: _R++; EAY; _D = RM(EA);                    break; // LD   D,(IY+o)
  case /*FD*/0x57: illegal_1(); /*op_57();*/                  break; // DB   FD

  case /*FD*/0x58: illegal_1(); /*op_58();*/                  break; // DB   FD
  case /*FD*/0x59: illegal_1(); /*op_59();*/                  break; // DB   FD
  case /*FD*/0x5a: illegal_1(); /*op_5a();*/                  break; // DB   FD
  case /*FD*/0x5b: illegal_1(); /*op_5b();*/                  break; // DB   FD
  case /*FD*/0x5c: _R++; _E = _HY;                            break; // LD   E,HY
  case /*FD*/0x5d: _R++; _E = _LY;                            break; // LD   E,LY
  case /*FD*/0x5e: _R++; EAY; _E = RM(EA);                    break; // LD   E,(IY+o)
  case /*FD*/0x5f: illegal_1(); /*op_5f();*/                  break; // DB   FD

  case /*FD*/0x60: _R++; _HY = _B;                            break; // LD   HY,B
  case /*FD*/0x61: _R++; _HY = _C;                            break; // LD   HY,C
  case /*FD*/0x62: _R++; _HY = _D;                            break; // LD   HY,D
  case /*FD*/0x63: _R++; _HY = _E;                            break; // LD   HY,E
  case /*FD*/0x64: _R++;                                      break; // LD   HY,HY
  case /*FD*/0x65: _R++; _HY = _LY;                           break; // LD   HY,LY
  case /*FD*/0x66: _R++; EAY; _H = RM(EA);                    break; // LD   H,(IY+o)
  case /*FD*/0x67: _R++; _HY = _A;                            break; // LD   HY,A

  case /*FD*/0x68: _R++; _LY = _B;                            break; // LD   LY,B
  case /*FD*/0x69: _R++; _LY = _C;                            break; // LD   LY,C
  case /*FD*/0x6a: _R++; _LY = _D;                            break; // LD   LY,D
  case /*FD*/0x6b: _R++; _LY = _E;                            break; // LD   LY,E
  case /*FD*/0x6c: _R++; _LY = _HY;                           break; // LD   LY,HY
  case /*FD*/0x6d: _R++;                                      break; // LD   LY,LY
  case /*FD*/0x6e: _R++; EAY; _L = RM(EA);                    break; // LD   L,(IY+o)
  case /*FD*/0x6f: _R++; _LY = _A;                            break; // LD   LY,A

  case /*FD*/0x70: _R++; EAY; WM( EA, _B );                   break; // LD   (IY+o),B
  case /*FD*/0x71: _R++; EAY; WM( EA, _C );                   break; // LD   (IY+o),C
  case /*FD*/0x72: _R++; EAY; WM( EA, _D );                   break; // LD   (IY+o),D
  case /*FD*/0x73: _R++; EAY; WM( EA, _E );                   break; // LD   (IY+o),E
  case /*FD*/0x74: _R++; EAY; WM( EA, _H );                   break; // LD   (IY+o),H
  case /*FD*/0x75: _R++; EAY; WM( EA, _L );                   break; // LD   (IY+o),L
  case /*FD*/0x76: illegal_1(); /*op_76();*/                  break; // DB   FD
  case /*FD*/0x77: _R++; EAY; WM( EA, _A );                   break; // LD   (IY+o),A

  case /*FD*/0x78: illegal_1(); /*op_78();*/                  break; // DB   FD
  case /*FD*/0x79: illegal_1(); /*op_79();*/                  break; // DB   FD
  case /*FD*/0x7a: illegal_1(); /*op_7a();*/                  break; // DB   FD
  case /*FD*/0x7b: illegal_1(); /*op_7b();*/                  break; // DB   FD
  case /*FD*/0x7c: _R++; _A = _HY;                            break; // LD   A,HY
  case /*FD*/0x7d: _R++; _A = _LY;                            break; // LD   A,LY
  case /*FD*/0x7e: _R++; EAY; _A = RM(EA);                    break; // LD   A,(IY+o)
  case /*FD*/0x7f: illegal_1(); /*op_7f();*/                  break; // DB   FD

  case /*FD*/0x80: illegal_1(); /*op_80();*/                  break; // DB   FD
  case /*FD*/0x81: illegal_1(); /*op_81();*/                  break; // DB   FD
  case /*FD*/0x82: illegal_1(); /*op_82();*/                  break; // DB   FD
  case /*FD*/0x83: illegal_1(); /*op_83();*/                  break; // DB   FD
  case /*FD*/0x84: _R++; ADD(_HY);                            break; // ADD  A,HY
  case /*FD*/0x85: _R++; ADD(_LY);                            break; // ADD  A,LY
  case /*FD*/0x86: _R++; EAY; ADD(RM(EA));                    break; // ADD  A,(IY+o)
  case /*FD*/0x87: illegal_1(); /*op_87();*/                  break; // DB   FD

  case /*FD*/0x88: illegal_1(); /*op_88();*/                  break; // DB   FD
  case /*FD*/0x89: illegal_1(); /*op_89();*/                  break; // DB   FD
  case /*FD*/0x8a: illegal_1(); /*op_8a();*/                  break; // DB   FD
  case /*FD*/0x8b: illegal_1(); /*op_8b();*/                  break; // DB   FD
  case /*FD*/0x8c: _R++; ADC(_HY);                            break; // ADC  A,HY
  case /*FD*/0x8d: _R++; ADC(_LY);                            break; // ADC  A,LY
  case /*FD*/0x8e: _R++; EAY; ADC(RM(EA));                    break; // ADC  A,(IY+o)
  case /*FD*/0x8f: illegal_1(); /*op_8f();*/                  break; // DB   FD

  case /*FD*/0x90: illegal_1(); /*op_90();*/                  break; // DB   FD
  case /*FD*/0x91: illegal_1(); /*op_91();*/                  break; // DB   FD
  case /*FD*/0x92: illegal_1(); /*op_92();*/                  break; // DB   FD
  case /*FD*/0x93: illegal_1(); /*op_93();*/                  break; // DB   FD
  case /*FD*/0x94: _R++; SUB(_HY);                            break; // SUB  HY
  case /*FD*/0x95: _R++; SUB(_LY);                            break; // SUB  LY
  case /*FD*/0x96: _R++; EAY; SUB(RM(EA));                    break; // SUB  (IY+o)
  case /*FD*/0x97: illegal_1(); /*op_97();*/                  break; // DB   FD

  case /*FD*/0x98: illegal_1(); /*op_98();*/                  break; // DB   FD
  case /*FD*/0x99: illegal_1(); /*op_99();*/                  break; // DB   FD
  case /*FD*/0x9a: illegal_1(); /*op_9a();*/                  break; // DB   FD
  case /*FD*/0x9b: illegal_1(); /*op_9b();*/                  break; // DB   FD
  case /*FD*/0x9c: _R++; SBC(_HY);                            break; // SBC  A,HY
  case /*FD*/0x9d: _R++; SBC(_LY);                            break; // SBC  A,LY
  case /*FD*/0x9e: _R++; EAY; SBC(RM(EA));                    break; // SBC  A,(IY+o)
  case /*FD*/0x9f: illegal_1(); /*op_9f();*/                  break; // DB   FD

  case /*FD*/0xa0: illegal_1(); /*op_a0();*/                  break; // DB   FD
  case /*FD*/0xa1: illegal_1(); /*op_a1();*/                  break; // DB   FD
  case /*FD*/0xa2: illegal_1(); /*op_a2();*/                  break; // DB   FD
  case /*FD*/0xa3: illegal_1(); /*op_a3();*/                  break; // DB   FD
  case /*FD*/0xa4: _R++; AND(_HY);                            break; // AND  HY
  case /*FD*/0xa5: _R++; AND(_LY);                            break; // AND  LY
  case /*FD*/0xa6: _R++; EAY; AND(RM(EA));                    break; // AND  (IY+o)
  case /*FD*/0xa7: illegal_1(); /*op_a7();*/                  break; // DB   FD

  case /*FD*/0xa8: illegal_1(); /*op_a8();*/                  break; // DB   FD
  case /*FD*/0xa9: illegal_1(); /*op_a9();*/                  break; // DB   FD
  case /*FD*/0xaa: illegal_1(); /*op_aa();*/                  break; // DB   FD
  case /*FD*/0xab: illegal_1(); /*op_ab();*/                  break; // DB   FD
  case /*FD*/0xac: _R++; XOR(_HY);                            break; // XOR  HY
  case /*FD*/0xad: _R++; XOR(_LY);                            break; // XOR  LY
  case /*FD*/0xae: _R++; EAY; XOR(RM(EA));                    break; // XOR  (IY+o)
  case /*FD*/0xaf: illegal_1(); /*op_af();*/                  break; // DB   FD

  case /*FD*/0xb0: illegal_1(); /*op_b0();*/                  break; // DB   FD
  case /*FD*/0xb1: illegal_1(); /*op_b1();*/                  break; // DB   FD
  case /*FD*/0xb2: illegal_1(); /*op_b2();*/                  break; // DB   FD
  case /*FD*/0xb3: illegal_1(); /*op_b3();*/                  break; // DB   FD
  case /*FD*/0xb4: _R++; OR(_HY);                             break; // OR   HY
  case /*FD*/0xb5: _R++; OR(_LY);                             break; // OR   LY
  case /*FD*/0xb6: _R++; EAY; OR(RM(EA));                     break; // OR   (IY+o)
  case /*FD*/0xb7: illegal_1(); /*op_b7();*/                  break; // DB   FD

  case /*FD*/0xb8: illegal_1(); /*op_b8();*/                  break; // DB   FD
  case /*FD*/0xb9: illegal_1(); /*op_b9();*/                  break; // DB   FD
  case /*FD*/0xba: illegal_1(); /*op_ba();*/                  break; // DB   FD
  case /*FD*/0xbb: illegal_1(); /*op_bb();*/                  break; // DB   FD
  case /*FD*/0xbc: _R++; CP(_HY);                             break; // CP   HY
  case /*FD*/0xbd: _R++; CP(_LY);                             break; // CP   LY
  case /*FD*/0xbe: _R++; EAY; CP(RM(EA));                     break; // CP   (IY+o)
  case /*FD*/0xbf: illegal_1(); /*op_bf();*/                  break; // DB   FD

  case /*FD*/0xc0: illegal_1(); /*op_c0();*/                  break; // DB   FD
  case /*FD*/0xc1: illegal_1(); /*op_c1();*/                  break; // DB   FD
  case /*FD*/0xc2: illegal_1(); /*op_c2();*/                  break; // DB   FD
  case /*FD*/0xc3: illegal_1(); /*op_c3();*/                  break; // DB   FD
  case /*FD*/0xc4: illegal_1(); /*op_c4();*/                  break; // DB   FD
  case /*FD*/0xc5: illegal_1(); /*op_c5();*/                  break; // DB   FD
  case /*FD*/0xc6: illegal_1(); /*op_c6();*/                  break; // DB   FD
  case /*FD*/0xc7: illegal_1(); /*op_c7();*/                  break; // DB   FD

  case /*FD*/0xc8: illegal_1(); /*op_c8();*/                  break; // DB   FD
  case /*FD*/0xc9: illegal_1(); /*op_c9();*/                  break; // DB   FD
  case /*FD*/0xca: illegal_1(); /*op_ca();*/                  break; // DB   FD
// pretty sure this is supposed to be ARG
  case /*FD*/0xcb: _R++; EAY; EXEC_EA(xycb,ARG(),EA);         break; // **   FD CB xx
  case /*FD*/0xcc: illegal_1(); /*op_cc();*/                  break; // DB   FD
  case /*FD*/0xcd: illegal_1(); /*op_cd();*/                  break; // DB   FD
  case /*FD*/0xce: illegal_1(); /*op_ce();*/                  break; // DB   FD
  case /*FD*/0xcf: illegal_1(); /*op_cf();*/                  break; // DB   FD

  case /*FD*/0xd0: illegal_1(); /*op_d0();*/                  break; // DB   FD
  case /*FD*/0xd1: illegal_1(); /*op_d1();*/                  break; // DB   FD
  case /*FD*/0xd2: illegal_1(); /*op_d2();*/                  break; // DB   FD
  case /*FD*/0xd3: illegal_1(); /*op_d3();*/                  break; // DB   FD
  case /*FD*/0xd4: illegal_1(); /*op_d4();*/                  break; // DB   FD
  case /*FD*/0xd5: illegal_1(); /*op_d5();*/                  break; // DB   FD
  case /*FD*/0xd6: illegal_1(); /*op_d6();*/                  break; // DB   FD
  case /*FD*/0xd7: illegal_1(); /*op_d7();*/                  break; // DB   FD

  case /*FD*/0xd8: illegal_1(); /*op_d8();*/                  break; // DB   FD
  case /*FD*/0xd9: illegal_1(); /*op_d9();*/                  break; // DB   FD
  case /*FD*/0xda: illegal_1(); /*op_da();*/                  break; // DB   FD
  case /*FD*/0xdb: illegal_1(); /*op_db();*/                  break; // DB   FD
  case /*FD*/0xdc: illegal_1(); /*op_dc();*/                  break; // DB   FD
  case /*FD*/0xdd: illegal_1(); /*op_dd();*/                  break; // DB   FD
  case /*FD*/0xde: illegal_1(); /*op_de();*/                  break; // DB   FD
  case /*FD*/0xdf: illegal_1(); /*op_df();*/                  break; // DB   FD

  case /*FD*/0xe0: illegal_1(); /*op_e0();*/                  break; // DB   FD
  case /*FD*/0xe1: _R++; _IY = POP();                         break; // POP  IY
  case /*FD*/0xe2: illegal_1(); /*op_e2();*/                  break; // DB   FD
  case /*FD*/0xe3: _R++; _IY = EXSP16(_IY);                   break; // EX   (SP),IY
  case /*FD*/0xe4: illegal_1(); /*op_e4();*/                  break; // DB   FD
  case /*FD*/0xe5: _R++; PUSH( _IY );                         break; // PUSH IY
  case /*FD*/0xe6: illegal_1(); /*op_e6();*/                  break; // DB   FD
  case /*FD*/0xe7: illegal_1(); /*op_e7();*/                  break; // DB   FD

  case /*FD*/0xe8: illegal_1(); /*op_e8();*/                  break; // DB   FD
  case /*FD*/0xe9: _R++; _PC = _IY;                           break; // JP   (IY)
  case /*FD*/0xea: illegal_1(); /*op_ea();*/                  break; // DB   FD
  case /*FD*/0xeb: illegal_1(); /*op_eb();*/                  break; // DB   FD
  case /*FD*/0xec: illegal_1(); /*op_ec();*/                  break; // DB   FD
  case /*FD*/0xed: illegal_1(); /*op_ed();*/                  break; // DB   FD
  case /*FD*/0xee: illegal_1(); /*op_ee();*/                  break; // DB   FD
  case /*FD*/0xef: illegal_1(); /*op_ef();*/                  break; // DB   FD

  case /*FD*/0xf0: illegal_1(); /*op_f0();*/                  break; // DB   FD
  case /*FD*/0xf1: illegal_1(); /*op_f1();*/                  break; // DB   FD
  case /*FD*/0xf2: illegal_1(); /*op_f2();*/                  break; // DB   FD
  case /*FD*/0xf3: illegal_1(); /*op_f3();*/                  break; // DB   FD
  case /*FD*/0xf4: illegal_1(); /*op_f4();*/                  break; // DB   FD
  case /*FD*/0xf5: illegal_1(); /*op_f5();*/                  break; // DB   FD
  case /*FD*/0xf6: illegal_1(); /*op_f6();*/                  break; // DB   FD
  case /*FD*/0xf7: illegal_1(); /*op_f7();*/                  break; // DB   FD

  case /*FD*/0xf8: illegal_1(); /*op_f8();*/                  break; // DB   FD
  case /*FD*/0xf9: _R++; _SP = _IY;                           break; // LD   SP,IY
  case /*FD*/0xfa: illegal_1(); /*op_fa();*/                  break; // DB   FD
  case /*FD*/0xfb: illegal_1(); /*op_fb();*/                  break; // DB   FD
  case /*FD*/0xfc: illegal_1(); /*op_fc();*/                  break; // DB   FD
  case /*FD*/0xfd: illegal_1(); /*op_fd();*/                  break; // DB   FD
  case /*FD*/0xfe: illegal_1(); /*op_fe();*/                  break; // DB   FD
  case /*FD*/0xff: illegal_1(); /*op_ff();*/                  break; // DB   FD
  }
}

/////////////////////////////////////////////////////////////////////////////

static EMU_INLINE void z80_exec_normal(struct Z80_STATE *state, uint8 op) {

//printf("PC=%04X exec normal %02X\n",_PC, op);fflush(stdout);

  switch(op) {
  case /*op*/0x00:                                    break; // NOP
  case /*op*/0x01: _BC = ARG16();                     break; // LD   BC,w
  case /*op*/0x02: WM(_BC, _A);                       break; // LD   (BC),A
  case /*op*/0x03: _BC++;                             break; // INC  BC
  case /*op*/0x04: _B = INC(_B);                      break; // INC  B
  case /*op*/0x05: _B = DEC(_B);                      break; // DEC  B
  case /*op*/0x06: _B = ARG();                        break; // LD   B,n
  case /*op*/0x07: RLCA;                              break; // RLCA

  case /*op*/0x08: EX_AF;                             break; // EX   AF,AF'
  case /*op*/0x09: ADD16(_HL,_BC);                    break; // ADD  HL,BC
  case /*op*/0x0A: _A = RM(_BC);                      break; // LD   A,(BC)
  case /*op*/0x0B: _BC--;                             break; // DEC  BC
  case /*op*/0x0C: _C = INC(_C);                      break; // INC  C
  case /*op*/0x0D: _C = DEC(_C);                      break; // DEC  C
  case /*op*/0x0E: _C = ARG();                        break; // LD   C,n
  case /*op*/0x0F: RRCA;                              break; // RRCA

  case /*op*/0x10: _B--; JR_COND( _B, 0x10 );         break; // DJNZ o
  case /*op*/0x11: _DE = ARG16();                     break; // LD   DE,w
  case /*op*/0x12: WM( _DE, _A );                     break; // LD   (DE),A
  case /*op*/0x13: _DE++;                             break; // INC  DE
  case /*op*/0x14: _D = INC(_D);                      break; // INC  D
  case /*op*/0x15: _D = DEC(_D);                      break; // DEC  D
  case /*op*/0x16: _D = ARG();                        break; // LD   D,n
  case /*op*/0x17: RLA;                               break; // RLA

  case /*op*/0x18: JR;                                break; // JR   o
  case /*op*/0x19: ADD16(_HL,_DE);                    break; // ADD  HL,DE
  case /*op*/0x1A: _A = RM(_DE);                      break; // LD   A,(DE)
  case /*op*/0x1B: _DE--;                             break; // DEC  DE
  case /*op*/0x1C: _E = INC(_E);                      break; // INC  E
  case /*op*/0x1D: _E = DEC(_E);                      break; // DEC  E
  case /*op*/0x1E: _E = ARG();                        break; // LD   E,n
  case /*op*/0x1F: RRA;                               break; // RRA

  case /*op*/0x20: JR_COND( !(_F & ZF), 0x20 );       break; // JR   NZ,o
  case /*op*/0x21: _HL = ARG16();                     break; // LD   HL,w
  case /*op*/0x22: WM16( ARG16(), _HL );              break; // LD   (w),HL
  case /*op*/0x23: _HL++;                             break; // INC  HL
  case /*op*/0x24: _H = INC(_H);                      break; // INC  H
  case /*op*/0x25: _H = DEC(_H);                      break; // DEC  H
  case /*op*/0x26: _H = ARG();                        break; // LD   H,n
  case /*op*/0x27: DAA;                               break; // DAA

  case /*op*/0x28: JR_COND( _F & ZF, 0x28 );          break; // JR   Z,o
  case /*op*/0x29: ADD16(_HL,_HL);                    break; // ADD  HL,HL
  case /*op*/0x2A: _HL = RM16( ARG16() );             break; // LD   HL,(w)
  case /*op*/0x2B: _HL--;                             break; // DEC  HL
  case /*op*/0x2C: _L = INC(_L);                      break; // INC  L
  case /*op*/0x2D: _L = DEC(_L);                      break; // DEC  L
  case /*op*/0x2E: _L = ARG();                        break; // LD   L,n
  case /*op*/0x2F: _A ^= 0xff; _F = (_F&(SF|ZF|PF|CF))|HF|NF|(_A&(YF|XF)); break; // CPL

  case /*op*/0x30: JR_COND( !(_F & CF), 0x30 );       break; // JR   NC,o
  case /*op*/0x31: _SP = ARG16();                     break; // LD   SP,w
  case /*op*/0x32: WM( ARG16(), _A );                 break; // LD   (w),A
  case /*op*/0x33: _SP++;                             break; // INC  SP
  case /*op*/0x34: WM( _HL, INC(RM(_HL)) );           break; // INC  (HL)
  case /*op*/0x35: WM( _HL, DEC(RM(_HL)) );           break; // DEC  (HL)
  case /*op*/0x36: WM( _HL, ARG() );                  break; // LD   (HL),n
  case /*op*/0x37: _F = (_F & (SF|ZF|PF)) | CF | (_A & (YF|XF)); break; // SCF

  case /*op*/0x38: JR_COND( _F & CF, 0x38 );          break; // JR   C,o
  case /*op*/0x39: ADD16(_HL,_SP);                    break; // ADD  HL,SP
  case /*op*/0x3A: _A = RM( ARG16() );                break; // LD   A,(w)
  case /*op*/0x3B: _SP--;                             break; // DEC  SP
  case /*op*/0x3C: _A = INC(_A);                      break; // INC  A
  case /*op*/0x3D: _A = DEC(_A);                      break; // DEC  A
  case /*op*/0x3E: _A = ARG();                        break; // LD   A,n
  case /*op*/0x3F: _F = ((_F&(SF|ZF|PF|CF))|((_F&CF)<<4)|(_A&(YF|XF)))^CF; break; // CCF

  case /*op*/0x40:                                    break; // LD   B,B
  case /*op*/0x41: _B = _C;                           break; // LD   B,C
  case /*op*/0x42: _B = _D;                           break; // LD   B,D
  case /*op*/0x43: _B = _E;                           break; // LD   B,E
  case /*op*/0x44: _B = _H;                           break; // LD   B,H
  case /*op*/0x45: _B = _L;                           break; // LD   B,L
  case /*op*/0x46: _B = RM(_HL);                      break; // LD   B,(HL)
  case /*op*/0x47: _B = _A;                           break; // LD   B,A

  case /*op*/0x48: _C = _B;                           break; // LD   C,B
  case /*op*/0x49:                                    break; // LD   C,C
  case /*op*/0x4A: _C = _D;                           break; // LD   C,D
  case /*op*/0x4B: _C = _E;                           break; // LD   C,E
  case /*op*/0x4C: _C = _H;                           break; // LD   C,H
  case /*op*/0x4D: _C = _L;                           break; // LD   C,L
  case /*op*/0x4E: _C = RM(_HL);                      break; // LD   C,(HL)
  case /*op*/0x4F: _C = _A;                           break; // LD   C,A

  case /*op*/0x50: _D = _B;                           break; // LD   D,B
  case /*op*/0x51: _D = _C;                           break; // LD   D,C
  case /*op*/0x52:                                    break; // LD   D,D
  case /*op*/0x53: _D = _E;                           break; // LD   D,E
  case /*op*/0x54: _D = _H;                           break; // LD   D,H
  case /*op*/0x55: _D = _L;                           break; // LD   D,L
  case /*op*/0x56: _D = RM(_HL);                      break; // LD   D,(HL)
  case /*op*/0x57: _D = _A;                           break; // LD   D,A

  case /*op*/0x58: _E = _B;                           break; // LD   E,B
  case /*op*/0x59: _E = _C;                           break; // LD   E,C
  case /*op*/0x5A: _E = _D;                           break; // LD   E,D
  case /*op*/0x5B:                                    break; // LD   E,E
  case /*op*/0x5C: _E = _H;                           break; // LD   E,H
  case /*op*/0x5D: _E = _L;                           break; // LD   E,L
  case /*op*/0x5E: _E = RM(_HL);                      break; // LD   E,(HL)
  case /*op*/0x5F: _E = _A;                           break; // LD   E,A

  case /*op*/0x60: _H = _B;                           break; // LD   H,B
  case /*op*/0x61: _H = _C;                           break; // LD   H,C
  case /*op*/0x62: _H = _D;                           break; // LD   H,D
  case /*op*/0x63: _H = _E;                           break; // LD   H,E
  case /*op*/0x64:                                    break; // LD   H,H
  case /*op*/0x65: _H = _L;                           break; // LD   H,L
  case /*op*/0x66: _H = RM(_HL);                      break; // LD   H,(HL)
  case /*op*/0x67: _H = _A;                           break; // LD   H,A

  case /*op*/0x68: _L = _B;                           break; // LD   L,B
  case /*op*/0x69: _L = _C;                           break; // LD   L,C
  case /*op*/0x6A: _L = _D;                           break; // LD   L,D
  case /*op*/0x6B: _L = _E;                           break; // LD   L,E
  case /*op*/0x6C: _L = _H;                           break; // LD   L,H
  case /*op*/0x6D:                                    break; // LD   L,L
  case /*op*/0x6E: _L = RM(_HL);                      break; // LD   L,(HL)
  case /*op*/0x6F: _L = _A;                           break; // LD   L,A

  case /*op*/0x70: WM( _HL, _B );                     break; // LD   (HL),B
  case /*op*/0x71: WM( _HL, _C );                     break; // LD   (HL),C
  case /*op*/0x72: WM( _HL, _D );                     break; // LD   (HL),D
  case /*op*/0x73: WM( _HL, _E );                     break; // LD   (HL),E
  case /*op*/0x74: WM( _HL, _H );                     break; // LD   (HL),H
  case /*op*/0x75: WM( _HL, _L );                     break; // LD   (HL),L
  case /*op*/0x76: HALT;                              break; // HALT
  case /*op*/0x77: WM( _HL, _A );                     break; // LD   (HL),A

  case /*op*/0x78: _A = _B;                           break; // LD   A,B
  case /*op*/0x79: _A = _C;                           break; // LD   A,C
  case /*op*/0x7A: _A = _D;                           break; // LD   A,D
  case /*op*/0x7B: _A = _E;                           break; // LD   A,E
  case /*op*/0x7C: _A = _H;                           break; // LD   A,H
  case /*op*/0x7D: _A = _L;                           break; // LD   A,L
  case /*op*/0x7E: _A = RM(_HL);                      break; // LD   A,(HL)
  case /*op*/0x7F:                                    break; // LD   A,A

  case /*op*/0x80: ADD(_B);                           break; // ADD  A,B
  case /*op*/0x81: ADD(_C);                           break; // ADD  A,C
  case /*op*/0x82: ADD(_D);                           break; // ADD  A,D
  case /*op*/0x83: ADD(_E);                           break; // ADD  A,E
  case /*op*/0x84: ADD(_H);                           break; // ADD  A,H
  case /*op*/0x85: ADD(_L);                           break; // ADD  A,L
  case /*op*/0x86: ADD(RM(_HL));                      break; // ADD  A,(HL)
  case /*op*/0x87: ADD(_A);                           break; // ADD  A,A

  case /*op*/0x88: ADC(_B);                           break; // ADC  A,B
  case /*op*/0x89: ADC(_C);                           break; // ADC  A,C
  case /*op*/0x8A: ADC(_D);                           break; // ADC  A,D
  case /*op*/0x8B: ADC(_E);                           break; // ADC  A,E
  case /*op*/0x8C: ADC(_H);                           break; // ADC  A,H
  case /*op*/0x8D: ADC(_L);                           break; // ADC  A,L
  case /*op*/0x8E: ADC(RM(_HL));                      break; // ADC  A,(HL)
  case /*op*/0x8F: ADC(_A);                           break; // ADC  A,A

  case /*op*/0x90: SUB(_B);                           break; // SUB  B
  case /*op*/0x91: SUB(_C);                           break; // SUB  C
  case /*op*/0x92: SUB(_D);                           break; // SUB  D
  case /*op*/0x93: SUB(_E);                           break; // SUB  E
  case /*op*/0x94: SUB(_H);                           break; // SUB  H
  case /*op*/0x95: SUB(_L);                           break; // SUB  L
  case /*op*/0x96: SUB(RM(_HL));                      break; // SUB  (HL)
  case /*op*/0x97: SUB(_A);                           break; // SUB  A

  case /*op*/0x98: SBC(_B);                           break; // SBC  A,B
  case /*op*/0x99: SBC(_C);                           break; // SBC  A,C
  case /*op*/0x9A: SBC(_D);                           break; // SBC  A,D
  case /*op*/0x9B: SBC(_E);                           break; // SBC  A,E
  case /*op*/0x9C: SBC(_H);                           break; // SBC  A,H
  case /*op*/0x9D: SBC(_L);                           break; // SBC  A,L
  case /*op*/0x9E: SBC(RM(_HL));                      break; // SBC  A,(HL)
  case /*op*/0x9F: SBC(_A);                           break; // SBC  A,A

  case /*op*/0xA0: AND(_B);                           break; // AND  B
  case /*op*/0xA1: AND(_C);                           break; // AND  C
  case /*op*/0xA2: AND(_D);                           break; // AND  D
  case /*op*/0xA3: AND(_E);                           break; // AND  E
  case /*op*/0xA4: AND(_H);                           break; // AND  H
  case /*op*/0xA5: AND(_L);                           break; // AND  L
  case /*op*/0xA6: AND(RM(_HL));                      break; // AND  (HL)
  case /*op*/0xA7: AND(_A);                           break; // AND  A

  case /*op*/0xA8: XOR(_B);                           break; // XOR  B
  case /*op*/0xA9: XOR(_C);                           break; // XOR  C
  case /*op*/0xAA: XOR(_D);                           break; // XOR  D
  case /*op*/0xAB: XOR(_E);                           break; // XOR  E
  case /*op*/0xAC: XOR(_H);                           break; // XOR  H
  case /*op*/0xAD: XOR(_L);                           break; // XOR  L
  case /*op*/0xAE: XOR(RM(_HL));                      break; // XOR  (HL)
  case /*op*/0xAF: XOR(_A);                           break; // XOR  A

  case /*op*/0xB0: OR(_B);                            break; // OR   B
  case /*op*/0xB1: OR(_C);                            break; // OR   C
  case /*op*/0xB2: OR(_D);                            break; // OR   D
  case /*op*/0xB3: OR(_E);                            break; // OR   E
  case /*op*/0xB4: OR(_H);                            break; // OR   H
  case /*op*/0xB5: OR(_L);                            break; // OR   L
  case /*op*/0xB6: OR(RM(_HL));                       break; // OR   (HL)
  case /*op*/0xB7: OR(_A);                            break; // OR   A

  case /*op*/0xB8: CP(_B);                            break; // CP   B
  case /*op*/0xB9: CP(_C);                            break; // CP   C
  case /*op*/0xBA: CP(_D);                            break; // CP   D
  case /*op*/0xBB: CP(_E);                            break; // CP   E
  case /*op*/0xBC: CP(_H);                            break; // CP   H
  case /*op*/0xBD: CP(_L);                            break; // CP   L
  case /*op*/0xBE: CP(RM(_HL));                       break; // CP   (HL)
  case /*op*/0xBF: CP(_A);                            break; // CP   A

  case /*op*/0xC0: RET_COND( !(_F & ZF), 0xc0 );      break; // RET  NZ
  case /*op*/0xC1: _BC = POP();                       break; // POP  BC
  case /*op*/0xC2: JP_COND( !(_F & ZF) );             break; // JP   NZ,a
  case /*op*/0xC3: JP;                                break; // JP   a
  case /*op*/0xC4: CALL_COND( !(_F & ZF), 0xc4 );     break; // CALL NZ,a
  case /*op*/0xC5: PUSH( _BC );                       break; // PUSH BC
  case /*op*/0xC6: ADD(ARG());                        break; // ADD  A,n
  case /*op*/0xC7: RST(0x00);                         break; // RST  0

  case /*op*/0xC8: RET_COND( _F & ZF, 0xc8 );         break; // RET  Z
  case /*op*/0xC9: _PC = POP();                       break; // RET
  case /*op*/0xCA: JP_COND( _F & ZF );                break; // JP   Z,a
  case /*op*/0xCB: _R++; EXEC(cb,ROP());              break; // **** CB xx
  case /*op*/0xCC: CALL_COND( _F & ZF, 0xcc );        break; // CALL Z,a
  case /*op*/0xCD: CALL;                              break; // CALL a
  case /*op*/0xCE: ADC(ARG());                        break; // ADC  A,n
  case /*op*/0xCF: RST(0x08);                         break; // RST  1

  case /*op*/0xD0: RET_COND( !(_F & CF), 0xd0 );      break; // RET  NC
  case /*op*/0xD1: _DE = POP();                       break; // POP  DE
  case /*op*/0xD2: JP_COND( !(_F & CF) );             break; // JP   NC,a
  case /*op*/0xD3: Z80OUT( ARG() | (_A << 8), _A );   break; // OUT  (n),A
  case /*op*/0xD4: CALL_COND( !(_F & CF), 0xd4 );     break; // CALL NC,a
  case /*op*/0xD5: PUSH( _DE );                       break; // PUSH DE
  case /*op*/0xD6: SUB(ARG());                        break; // SUB  n
  case /*op*/0xD7: RST(0x10);                         break; // RST  2

  case /*op*/0xD8: RET_COND( _F & CF, 0xd8 );         break; // RET  C
  case /*op*/0xD9: EXX;                               break; // EXX
  case /*op*/0xDA: JP_COND( _F & CF );                break; // JP   C,a
  case /*op*/0xDB: _A = Z80IN( ARG() | (_A << 8) );   break; // IN   A,(n)
  case /*op*/0xDC: CALL_COND( _F & CF, 0xdc );        break; // CALL C,a
  case /*op*/0xDD: _R++; EXEC(dd,ROP());              break; // **** DD xx
  case /*op*/0xDE: SBC(ARG());                        break; // SBC  A,n
  case /*op*/0xDF: RST(0x18);                         break; // RST  3

  case /*op*/0xE0: RET_COND( !(_F & PF), 0xe0 );      break; // RET  PO
  case /*op*/0xE1: _HL = POP();                       break; // POP  HL
  case /*op*/0xE2: JP_COND( !(_F & PF) );             break; // JP   PO,a
  case /*op*/0xE3: _HL = EXSP16(_HL);                 break; // EX   HL,(SP)
  case /*op*/0xE4: CALL_COND( !(_F & PF), 0xe4 );     break; // CALL PO,a
  case /*op*/0xE5: PUSH( _HL );                       break; // PUSH HL
  case /*op*/0xE6: AND(ARG());                        break; // AND  n
  case /*op*/0xE7: RST(0x20);                         break; // RST  4

  case /*op*/0xE8: RET_COND( _F & PF, 0xe8 );         break; // RET  PE
  case /*op*/0xE9: _PC = _HL;                         break; // JP   (HL)
  case /*op*/0xEA: JP_COND( _F & PF );                break; // JP   PE,a
  case /*op*/0xEB: EX_DE_HL;                          break; // EX   DE,HL
  case /*op*/0xEC: CALL_COND( _F & PF, 0xec );        break; // CALL PE,a
  case /*op*/0xED: _R++; EXEC(ed,ROP());              break; // **** ED xx
  case /*op*/0xEE: XOR(ARG());                        break; // XOR  n
  case /*op*/0xEF: RST(0x28);                         break; // RST  5

  case /*op*/0xF0: RET_COND( !(_F & SF), 0xf0 );      break; // RET  P
  case /*op*/0xF1: _AF = POP();                       break; // POP  AF
  case /*op*/0xF2: JP_COND( !(_F & SF) );             break; // JP   P,a
  case /*op*/0xF3: DI;                                break; // DI
  case /*op*/0xF4: CALL_COND( !(_F & SF), 0xf4 );     break; // CALL P,a
  case /*op*/0xF5: PUSH( _AF );                       break; // PUSH AF
  case /*op*/0xF6: OR(ARG());                         break; // OR   n
  case /*op*/0xF7: RST(0x30);                         break; // RST  6

  case /*op*/0xF8: RET_COND( _F & SF, 0xf8 );         break; // RET  M
  case /*op*/0xF9: _SP = _HL;                         break; // LD   SP,HL
  case /*op*/0xFA: JP_COND(_F & SF);                  break; // JP   M,a
  case /*op*/0xFB: EI;                                break; // EI
  case /*op*/0xFC: CALL_COND( _F & SF, 0xfc );        break; // CALL M,a
  case /*op*/0xFD: _R++; EXEC(fd,ROP());              break; // **** FD xx
  case /*op*/0xFE: CP(ARG());                         break; // CP   n
  case /*op*/0xFF: RST(0x38);                         break; // RST  7
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Returns -1 on error
//
sint32 EMU_CALL z80_execute(void *state, sint32 cycles) {

  if(STATE->imflags & IMFLAGS_BADINS) return -1;

  STATE->cycles_remaining_last_checkpoint = cycles;
  STATE->cycles_remaining = cycles;
  STATE->cycles_deferred_from_break = 0;

  if(STATE->imflags & IMFLAGS_HALT) { STATE->cycles_remaining = 0; }

#ifdef TESTZ80
testz80_newins();
#endif
  checkinterrupts(STATE);
#ifdef TESTZ80
if(testz80_irq) { testz80_subrst38h(); testz80_irq = 0; }
testz80_compareregs();
#endif

  while(STATE->cycles_remaining > 0) {
//printf("z80 pc=%04X\n",_PC);fflush(stdout);
#ifdef TESTZ80
testz80_newins();
#endif
    _R++; EXEC(normal,ROP());
#ifdef TESTZ80
testz80_subexec();
if(testz80_irq) { testz80_subrst38h(); testz80_irq = 0; }
testz80_compareregs();
#endif
  }

  STATE->cycles_remaining_last_checkpoint += STATE->cycles_deferred_from_break;
  STATE->cycles_remaining                 += STATE->cycles_deferred_from_break;
  STATE->cycles_deferred_from_break = 0;

  hw_sync(state);
  return (STATE->imflags & IMFLAGS_BADINS) ? -1 : 0;
}

/////////////////////////////////////////////////////////////////////////////
