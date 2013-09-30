/////////////////////////////////////////////////////////////////////////////
//
// spu - Top-level SPU emulation, for SPU and SPU2
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "spu.h"

#include "spucore.h"

////////////////////////////////////////////////////////////////////////////////
/*
** Static information
*/
sint32 EMU_CALL spu_init(void) { return 0; }

////////////////////////////////////////////////////////////////////////////////
/*
** State information
*/
#define SPUSTATE ((struct SPU_STATE*)(state))

#define CORESTATE(n) ((void*)(((char*)(state))+(SPUSTATE->offset_to_core[(n)])))
#define SPURAM       ((void*)(((char*)(state))+(SPUSTATE->offset_to_ram)))


struct SPU_STATE {
  uint8 version;
  uint32 offset_to_ram;
  uint32 offset_to_core[2];

  /* actual plugin option */
  uint8 global_main_on;
  uint8 global_effect_on;

  uint32 tsa[2];
  uint8 dma_mode[2];

  uint8 is_on[2];
  uint8 is_audible[2];
  uint8 is_reverb_enabled[2];
  uint8 is_irq_enabled[2];

  uint16 mystery_dma[2];

};

/*
** Get size depending on version
*/
uint32 EMU_CALL spu_get_state_size(uint8 version) {
  uint32 size = 0;
  if(version != 2) { version = 1; }
  size += sizeof(struct SPU_STATE);
  switch(version) {
  case 1:
    size += spucore_get_state_size();
    size += 0x80000;
    break;
  case 2:
    size += spucore_get_state_size() * 2;
    size += 0x200000;
    break;
  }
  return size;
}

/*
** Initialize SPU state
*/
void EMU_CALL spu_clear_state(void *state, uint8 version) {
  uint32 offset;
  if(version != 2) { version = 1; }
  /*
  ** Clear to zero
  */
  memset(state, 0, sizeof(struct SPU_STATE));
  /*
  ** Set version
  */
  SPUSTATE->version = version;
  /*
  ** Set offsets
  */
  offset = sizeof(struct SPU_STATE);
  switch(version) {
  case 1:
    /* Both cores point to the same place (safety) */
    SPUSTATE->offset_to_core[0] = offset;
    SPUSTATE->offset_to_core[1] = offset; offset += spucore_get_state_size();
    SPUSTATE->offset_to_ram     = offset;
    break;
  case 2:
    SPUSTATE->offset_to_core[0] = offset; offset += spucore_get_state_size();
    SPUSTATE->offset_to_core[1] = offset; offset += spucore_get_state_size();
    SPUSTATE->offset_to_ram     = offset;
    break;
  }

  //
  // Set local flags
  //
  SPUSTATE->global_main_on = 1;
  SPUSTATE->global_effect_on = 1;

  /*
  ** Init SPU core states / clear SPU RAM
  */
  switch(version) {
  case 1:
    spucore_clear_state(CORESTATE(0));
    spucore_set_mem_size(CORESTATE(0), 0x80000);
    memset(SPURAM, 0, 0x80000);
    break;
  case 2:
    spucore_clear_state(CORESTATE(0));
    spucore_clear_state(CORESTATE(1));
    spucore_set_mem_size(CORESTATE(0), 0x200000);
    spucore_set_mem_size(CORESTATE(1), 0x200000);
    memset(SPURAM, 0, 0x200000);
    break;
  }

}

/*
** Enable/disable reverb
*/
void EMU_CALL spu_enable_reverb(void *state, uint8 enable) {
  SPUSTATE->global_effect_on = enable;
}

void EMU_CALL spu_enable_main(void *state, uint8 enable) {
  SPUSTATE->global_main_on = enable;
}

////////////////////////////////////////////////////////////////////////////////
/*
** Hardware register load/store
*/

static uint32 EMU_CALL get_tsa(struct SPU_STATE *state, uint32 core) {
  return state->tsa[core];
}

static void EMU_CALL set_tsa(struct SPU_STATE *state, uint32 core, uint32 a, uint32 mask) {
  state->tsa[core] &= ~mask;
  state->tsa[core] |= a & mask;
}

static EMU_INLINE uint16 EMU_CALL get_transfer(struct SPU_STATE *state, uint32 core) {
  uint32 memmask = (state->version == 2) ? 0x001FFFFE : 0x0007FFFE;
  uint16 d = *((uint16*)(((uint8*)(SPURAM)) + ((state->tsa[core]) & memmask)));
  state->tsa[core] += 2;
  state->tsa[core] &= memmask;
  return d;
}

static EMU_INLINE void EMU_CALL set_transfer(struct SPU_STATE *state, uint32 core, uint16 d) {
  uint32 memmask = (state->version == 2) ? 0x001FFFFE : 0x0007FFFE;
  *((uint16*)(((uint8*)(SPURAM)) + ((state->tsa[core]) & memmask))) = d;
  state->tsa[core] += 2;
  state->tsa[core] &= memmask;
}

void EMU_CALL spu_dma(void *state, uint32 core, void *mem, uint32 mem_ofs, uint32 mem_mask, uint32 bytes, int iswrite) {
  uint32 words = (bytes + 3) / 4;
  mem_ofs &= (~3);
  if(iswrite) {
    while(words--) {
      uint32 d;
      mem_ofs &= mem_mask;
      d = *((uint32*)(((uint8*)mem)+mem_ofs));
      set_transfer(state, core, d      );
      set_transfer(state, core, d >> 16);
      mem_ofs += 4;
    }
  } else {
    while(words--) {
      uint32 d;
      mem_ofs &= mem_mask;
      d  = ((uint32)(get_transfer(state, core)))      ;
      d |= ((uint32)(get_transfer(state, core))) << 16;
      *((uint32*)(((uint8*)mem)+mem_ofs)) = d;
      mem_ofs += 4;
    }
  }
  // complete flag?
  SPUSTATE->mystery_dma[core] |= 0x80;
}

static uint16 EMU_CALL get_mystery_dma(struct SPU_STATE *state, uint32 core) {
  uint16 m = state->mystery_dma[core];
  state->mystery_dma[core] = 0;
  return m;
}

static void EMU_CALL set_mystery_dma(struct SPU_STATE *state, uint32 core, uint16 n) {
}

static uint16 EMU_CALL get_ctrl(struct SPU_STATE *state, uint32 core) {
  uint32 on            = !!(spucore_getflag(CORESTATE(core), SPUREG_FLAG_ON));
  uint32 main_enable   = !!(spucore_getflag(CORESTATE(core), SPUREG_FLAG_MAIN_ENABLE));
  uint32 noiseclock    = spucore_getreg(CORESTATE(core), SPUREG_NOISECLOCK) & 0x3F;
  uint32 reverb_enable = !!(spucore_getflag(CORESTATE(core), SPUREG_FLAG_REVERB_ENABLE));
  uint32 irq_enable    = !!(spucore_getflag(CORESTATE(core), SPUREG_FLAG_IRQ_ENABLE));
  uint32 dmamode       = state->dma_mode[core] & 3;
  uint32 er            = !!(spucore_getflag(CORESTATE(core), SPUREG_FLAG_ER));
  uint32 cr            = !!(spucore_getflag(CORESTATE(core), SPUREG_FLAG_CR));
  uint32 ee            = !!(spucore_getflag(CORESTATE(core), SPUREG_FLAG_EE));
  uint32 ce            = !!(spucore_getflag(CORESTATE(core), SPUREG_FLAG_CE));
  return (
    (on << 15) |
    (main_enable << 14) |
    (noiseclock << 8) |
    (reverb_enable << 7) |
    (irq_enable << 6) |
    (dmamode << 4) |
    (er << 3) |
    (cr << 2) |
    (ee << 1) |
    (ce << 0)
  );
}

static void EMU_CALL set_ctrl(struct SPU_STATE *state, uint32 core, uint16 d) {
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_ON           , !!(d & (1<<15)));
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_MAIN_ENABLE  , !!(d & (1<<14)));
  spucore_setreg (CORESTATE(core), SPUREG_NOISECLOCK, (d >> 8) & 0x3F, 0x3F);
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_REVERB_ENABLE, !!(d & (1<< 7)));
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_IRQ_ENABLE   , !!(d & (1<< 6)));
  state->dma_mode[core] = (d >> 4) & 3;
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_ER           , !!(d & (1<< 3)));
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_CR           , !!(d & (1<< 2)));
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_EE           , !!(d & (1<< 1)));
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_CE           , !!(d & (1<< 0)));
}

static uint16 EMU_CALL get_mmix(struct SPU_STATE *state, uint32 core) {
  uint16 d = 0;
  if(spucore_getflag(CORESTATE(core), SPUREG_FLAG_MSNDL )) d |= (1<<11);
  if(spucore_getflag(CORESTATE(core), SPUREG_FLAG_MSNDR )) d |= (1<<10);
  if(spucore_getflag(CORESTATE(core), SPUREG_FLAG_MSNDEL)) d |= (1<< 9);
  if(spucore_getflag(CORESTATE(core), SPUREG_FLAG_MSNDER)) d |= (1<< 8);
  if(spucore_getflag(CORESTATE(core), SPUREG_FLAG_MINL  )) d |= (1<< 7);
  if(spucore_getflag(CORESTATE(core), SPUREG_FLAG_MINR  )) d |= (1<< 6);
  if(spucore_getflag(CORESTATE(core), SPUREG_FLAG_MINEL )) d |= (1<< 5);
  if(spucore_getflag(CORESTATE(core), SPUREG_FLAG_MINER )) d |= (1<< 4);
  if(spucore_getflag(CORESTATE(core), SPUREG_FLAG_SINL  )) d |= (1<< 3);
  if(spucore_getflag(CORESTATE(core), SPUREG_FLAG_SINR  )) d |= (1<< 2);
  if(spucore_getflag(CORESTATE(core), SPUREG_FLAG_SINEL )) d |= (1<< 1);
  if(spucore_getflag(CORESTATE(core), SPUREG_FLAG_SINER )) d |= (1<< 0);
  return d;
}

static void EMU_CALL set_mmix(struct SPU_STATE *state, uint32 core, uint16 d) {
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_MSNDL , (d >> 11) & 1);
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_MSNDR , (d >> 10) & 1);
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_MSNDEL, (d >>  9) & 1);
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_MSNDER, (d >>  8) & 1);
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_MINL  , (d >>  7) & 1);
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_MINR  , (d >>  6) & 1);
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_MINEL , (d >>  5) & 1);
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_MINER , (d >>  4) & 1);
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_SINL  , (d >>  3) & 1);
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_SINR  , (d >>  2) & 1);
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_SINEL , (d >>  1) & 1);
  spucore_setflag(CORESTATE(core), SPUREG_FLAG_SINER , (d >>  0) & 1);
}

////////////////////////////////////////////////////////////////////////////////
/*
** SPU1 register accesses: lh1/sh1
*/

static uint16 EMU_CALL lh1(struct SPU_STATE *state, uint32 a) {
  a &= 0x1FE;
  if(a < 0x180) {
    uint32 voice = a >> 4;
    switch(a & 0xE) {
    case 0x0: return spucore_getreg_voice(CORESTATE(0), voice, SPUREG_VOICE_VOLL );
    case 0x2: return spucore_getreg_voice(CORESTATE(0), voice, SPUREG_VOICE_VOLR );
    case 0x4: return spucore_getreg_voice(CORESTATE(0), voice, SPUREG_VOICE_PITCH);
    case 0x6: return spucore_getreg_voice(CORESTATE(0), voice, SPUREG_VOICE_SSA  ) >> 3;
    case 0x8: return spucore_getreg_voice(CORESTATE(0), voice, SPUREG_VOICE_ADSR1);
    case 0xA: return spucore_getreg_voice(CORESTATE(0), voice, SPUREG_VOICE_ADSR2);
    case 0xC: return spucore_getreg_voice(CORESTATE(0), voice, SPUREG_VOICE_ENVX );
    case 0xE: return spucore_getreg_voice(CORESTATE(0), voice, SPUREG_VOICE_LSAX ) >> 3;
    }
  } else {
    switch(a) {
    case 0x180: return spucore_getreg(CORESTATE(0), SPUREG_MVOLL);
    case 0x182: return spucore_getreg(CORESTATE(0), SPUREG_MVOLR);
    case 0x184: return spucore_getreg(CORESTATE(0), SPUREG_EVOLL);
    case 0x186: return spucore_getreg(CORESTATE(0), SPUREG_EVOLR);
    case 0x188: return spucore_getreg(CORESTATE(0), SPUREG_KON);
    case 0x18A: return spucore_getreg(CORESTATE(0), SPUREG_KON) >> 16;
    case 0x18C: return spucore_getreg(CORESTATE(0), SPUREG_KOFF);
    case 0x18E: return spucore_getreg(CORESTATE(0), SPUREG_KOFF) >> 16;
    case 0x190: return spucore_getreg(CORESTATE(0), SPUREG_FM);
    case 0x192: return spucore_getreg(CORESTATE(0), SPUREG_FM) >> 16;
    case 0x194: return spucore_getreg(CORESTATE(0), SPUREG_NOISE);
    case 0x196: return spucore_getreg(CORESTATE(0), SPUREG_NOISE) >> 16;
    case 0x198: return spucore_getreg(CORESTATE(0), SPUREG_VMIXE);
    case 0x19A: return spucore_getreg(CORESTATE(0), SPUREG_VMIXE) >> 16;
//  case 0x19C: return spucore_getreg(CORESTATE(0), SPUREG_VMIX);
//  case 0x19E: return spucore_getreg(CORESTATE(0), SPUREG_VMIX) >> 16;
    case 0x19C: return 0;
    case 0x19E: return 0;
    case 0x1A0: return 0;
    case 0x1A2: return spucore_getreg(CORESTATE(0), SPUREG_ESA) >> 3;
    case 0x1A4: return spucore_getreg(CORESTATE(0), SPUREG_IRQA) >> 3;
    case 0x1A6: return get_tsa     (state, 0) >> 3;
    case 0x1A8: return get_transfer(state, 0);
    case 0x1AA: return get_ctrl    (state, 0);
    case 0x1AC: return 0; /* TODO: what does this reg do? */
    case 0x1AE: return 0; /* TODO: implement status bits (0 should be ok though) */
    case 0x1B0: return spucore_getreg(CORESTATE(0), SPUREG_AVOLL);
    case 0x1B2: return spucore_getreg(CORESTATE(0), SPUREG_AVOLR);
    case 0x1B4: return spucore_getreg(CORESTATE(0), SPUREG_BVOLL);
    case 0x1B6: return spucore_getreg(CORESTATE(0), SPUREG_BVOLR);
    case 0x1B8: return 0;
    case 0x1BA: return 0;
    case 0x1BC: return 0;
    case 0x1BE: return 0;
    case 0x1C0: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_FB_SRC_A   ) >> 3;
    case 0x1C2: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_FB_SRC_B   ) >> 3;
    case 0x1C4: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_IIR_ALPHA  );
    case 0x1C6: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_ACC_COEF_A );
    case 0x1C8: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_ACC_COEF_B );
    case 0x1CA: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_ACC_COEF_C );
    case 0x1CC: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_ACC_COEF_D );
    case 0x1CE: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_IIR_COEF   );
    case 0x1D0: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_FB_ALPHA   );
    case 0x1D2: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_FB_X       );
    case 0x1D4: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_IIR_DEST_A0) >> 3;
    case 0x1D6: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_IIR_DEST_A1) >> 3;
    case 0x1D8: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_A0 ) >> 3;
    case 0x1DA: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_A1 ) >> 3;
    case 0x1DC: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_B0 ) >> 3;
    case 0x1DE: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_B1 ) >> 3;
    case 0x1E0: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_IIR_SRC_A0 ) >> 3;
    case 0x1E2: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_IIR_SRC_A1 ) >> 3;
    case 0x1E4: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_IIR_DEST_B0) >> 3;
    case 0x1E6: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_IIR_DEST_B1) >> 3;
    case 0x1E8: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_C0 ) >> 3;
    case 0x1EA: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_C1 ) >> 3;
    case 0x1EC: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_D0 ) >> 3;
    case 0x1EE: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_D1 ) >> 3;
    case 0x1F0: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_IIR_SRC_B1 ) >> 3;
    case 0x1F2: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_IIR_SRC_B0 ) >> 3;
    case 0x1F4: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_MIX_DEST_A0) >> 3;
    case 0x1F6: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_MIX_DEST_A1) >> 3;
    case 0x1F8: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_MIX_DEST_B0) >> 3;
    case 0x1FA: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_MIX_DEST_B1) >> 3;
    case 0x1FC: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_IN_COEF_L  );
    case 0x1FE: return spucore_getreg(CORESTATE(0), SPUREG_REVERB_IN_COEF_R  );
    }
  }
  return 0;
}

static void EMU_CALL sh1(struct SPU_STATE *state, uint32 a, uint16 d) {
  a &= 0x1FE;
  if(a < 0x180) {
    uint32 voice = a >> 4;
    switch(a & 0xE) {
    case 0x0: spucore_setreg_voice(CORESTATE(0), voice, SPUREG_VOICE_VOLL , d, 0xFFFF); break;
    case 0x2: spucore_setreg_voice(CORESTATE(0), voice, SPUREG_VOICE_VOLR , d, 0xFFFF); break;
    case 0x4: spucore_setreg_voice(CORESTATE(0), voice, SPUREG_VOICE_PITCH, d, 0xFFFF); break;
    case 0x6: spucore_setreg_voice(CORESTATE(0), voice, SPUREG_VOICE_SSA  , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x8: spucore_setreg_voice(CORESTATE(0), voice, SPUREG_VOICE_ADSR1, d, 0xFFFF); break;
    case 0xA: spucore_setreg_voice(CORESTATE(0), voice, SPUREG_VOICE_ADSR2, d, 0xFFFF); break;
    case 0xC: spucore_setreg_voice(CORESTATE(0), voice, SPUREG_VOICE_ENVX , d, 0xFFFF); break;
    case 0xE: spucore_setreg_voice(CORESTATE(0), voice, SPUREG_VOICE_LSAX , ((uint32)d) << 3, 0xFFFFFFFF); break;
    }
  } else {
    switch(a) {
    case 0x180: spucore_setreg  (CORESTATE(0), SPUREG_MVOLL, d, 0xFFFF); break;
    case 0x182: spucore_setreg  (CORESTATE(0), SPUREG_MVOLR, d, 0xFFFF); break;
    case 0x184: spucore_setreg  (CORESTATE(0), SPUREG_EVOLL, d, 0xFFFF); break;
    case 0x186: spucore_setreg  (CORESTATE(0), SPUREG_EVOLR, d, 0xFFFF); break;
    case 0x188: spucore_setreg  (CORESTATE(0), SPUREG_KON  , ((uint32)d) <<  0, 0x0000FFFF); break;
    case 0x18A: spucore_setreg  (CORESTATE(0), SPUREG_KON  , ((uint32)d) << 16, 0xFFFF0000); break;
    case 0x18C: spucore_setreg  (CORESTATE(0), SPUREG_KOFF , ((uint32)d) <<  0, 0x0000FFFF); break;
    case 0x18E: spucore_setreg  (CORESTATE(0), SPUREG_KOFF , ((uint32)d) << 16, 0xFFFF0000); break;
    case 0x190: spucore_setreg  (CORESTATE(0), SPUREG_FM   , ((uint32)d) <<  0, 0x0000FFFF); break;
    case 0x192: spucore_setreg  (CORESTATE(0), SPUREG_FM   , ((uint32)d) << 16, 0xFFFF0000); break;
    case 0x194: spucore_setreg  (CORESTATE(0), SPUREG_NOISE, ((uint32)d) <<  0, 0x0000FFFF); break;
    case 0x196: spucore_setreg  (CORESTATE(0), SPUREG_NOISE, ((uint32)d) << 16, 0xFFFF0000); break;
    case 0x198: spucore_setreg  (CORESTATE(0), SPUREG_VMIXE, ((uint32)d) <<  0, 0x0000FFFF); break;
    case 0x19A: spucore_setreg  (CORESTATE(0), SPUREG_VMIXE, ((uint32)d) << 16, 0xFFFF0000); break;
//  case 0x19C: spucore_setreg  (CORESTATE(0), SPUREG_VMIX , ((uint32)d) <<  0, 0x0000FFFF); break;
//  case 0x19E: spucore_setreg  (CORESTATE(0), SPUREG_VMIX , ((uint32)d) << 16, 0xFFFF0000); break;
    case 0x19C: break;
    case 0x19E: break;
    case 0x1A0: break;
    case 0x1A2: spucore_setreg  (CORESTATE(0), SPUREG_ESA , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1A4: spucore_setreg  (CORESTATE(0), SPUREG_IRQA, ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1A6: set_tsa     (state, 0, ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1A8: set_transfer(state, 0, d); break;
    case 0x1AA: set_ctrl    (state, 0, d); break;
    case 0x1AC: break; /* TODO: what does this reg do? */
    case 0x1AE: break; /* TODO: implement status bits (0 should be ok though) */
    case 0x1B0: spucore_setreg  (CORESTATE(0), SPUREG_AVOLL, d, 0xFFFF); break;
    case 0x1B2: spucore_setreg  (CORESTATE(0), SPUREG_AVOLR, d, 0xFFFF); break;
    case 0x1B4: spucore_setreg  (CORESTATE(0), SPUREG_BVOLL, d, 0xFFFF); break;
    case 0x1B6: spucore_setreg  (CORESTATE(0), SPUREG_BVOLR, d, 0xFFFF); break;
    case 0x1B8: break;
    case 0x1BA: break;
    case 0x1BC: break;
    case 0x1BE: break;
    case 0x1C0: spucore_setreg(CORESTATE(0), SPUREG_REVERB_FB_SRC_A   , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1C2: spucore_setreg(CORESTATE(0), SPUREG_REVERB_FB_SRC_B   , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1C4: spucore_setreg(CORESTATE(0), SPUREG_REVERB_IIR_ALPHA  , ((uint32)d)     , 0x0000FFFF); break;
    case 0x1C6: spucore_setreg(CORESTATE(0), SPUREG_REVERB_ACC_COEF_A , ((uint32)d)     , 0x0000FFFF); break;
    case 0x1C8: spucore_setreg(CORESTATE(0), SPUREG_REVERB_ACC_COEF_B , ((uint32)d)     , 0x0000FFFF); break;
    case 0x1CA: spucore_setreg(CORESTATE(0), SPUREG_REVERB_ACC_COEF_C , ((uint32)d)     , 0x0000FFFF); break;
    case 0x1CC: spucore_setreg(CORESTATE(0), SPUREG_REVERB_ACC_COEF_D , ((uint32)d)     , 0x0000FFFF); break;
    case 0x1CE: spucore_setreg(CORESTATE(0), SPUREG_REVERB_IIR_COEF   , ((uint32)d)     , 0x0000FFFF); break;
    case 0x1D0: spucore_setreg(CORESTATE(0), SPUREG_REVERB_FB_ALPHA   , ((uint32)d)     , 0x0000FFFF); break;
    case 0x1D2: spucore_setreg(CORESTATE(0), SPUREG_REVERB_FB_X       , ((uint32)d)     , 0x0000FFFF); break;
    case 0x1D4: spucore_setreg(CORESTATE(0), SPUREG_REVERB_IIR_DEST_A0, ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1D6: spucore_setreg(CORESTATE(0), SPUREG_REVERB_IIR_DEST_A1, ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1D8: spucore_setreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_A0 , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1DA: spucore_setreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_A1 , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1DC: spucore_setreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_B0 , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1DE: spucore_setreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_B1 , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1E0: spucore_setreg(CORESTATE(0), SPUREG_REVERB_IIR_SRC_A0 , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1E2: spucore_setreg(CORESTATE(0), SPUREG_REVERB_IIR_SRC_A1 , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1E4: spucore_setreg(CORESTATE(0), SPUREG_REVERB_IIR_DEST_B0, ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1E6: spucore_setreg(CORESTATE(0), SPUREG_REVERB_IIR_DEST_B1, ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1E8: spucore_setreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_C0 , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1EA: spucore_setreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_C1 , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1EC: spucore_setreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_D0 , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1EE: spucore_setreg(CORESTATE(0), SPUREG_REVERB_ACC_SRC_D1 , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1F0: spucore_setreg(CORESTATE(0), SPUREG_REVERB_IIR_SRC_B1 , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1F2: spucore_setreg(CORESTATE(0), SPUREG_REVERB_IIR_SRC_B0 , ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1F4: spucore_setreg(CORESTATE(0), SPUREG_REVERB_MIX_DEST_A0, ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1F6: spucore_setreg(CORESTATE(0), SPUREG_REVERB_MIX_DEST_A1, ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1F8: spucore_setreg(CORESTATE(0), SPUREG_REVERB_MIX_DEST_B0, ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1FA: spucore_setreg(CORESTATE(0), SPUREG_REVERB_MIX_DEST_B1, ((uint32)d) << 3, 0xFFFFFFFF); break;
    case 0x1FC: spucore_setreg(CORESTATE(0), SPUREG_REVERB_IN_COEF_L  , ((uint32)d)     , 0x0000FFFF); break;
    case 0x1FE: spucore_setreg(CORESTATE(0), SPUREG_REVERB_IN_COEF_R  , ((uint32)d)     , 0x0000FFFF); break;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** SPU2 register accesses: lh2/sh2
*/

/*
** Returns "normalized to core0" address
** Also sets *core to 0 or 1
*/
static EMU_INLINE uint32 EMU_CALL spu2_reg_addr_normal(uint32 a, uint32 *core) {
  a &= 0x7FE;
  if(a < 0x400) { *core = 0; return a; }
  if(a < 0x760) { *core = 1; return a-0x400; }
  if(a < 0x788) { *core = 0; return a; }
  if(a < 0x7B0) { *core = 1; return a-0x28; }
  *core = 0; return a;
}

static uint16 EMU_CALL lh2(struct SPU_STATE *state, uint32 a) {
  uint32 core = 0; a = spu2_reg_addr_normal(a, &core);
  if(a < 0x180) {
    uint32 voice = a >> 4;
    switch(a & 0xE) {
    case 0x0: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_VOLL );
    case 0x2: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_VOLR );
    case 0x4: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_PITCH);
    case 0x6: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_ADSR1);
    case 0x8: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_ADSR2);
    case 0xA: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_ENVX );
    case 0xC: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_VOLXL);
    case 0xE: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_VOLXR);
    }
  } else if(a < 0x1C0) {
    switch(a) {
    case 0x180: return spucore_getreg(CORESTATE(core), SPUREG_FM);
    case 0x182: return spucore_getreg(CORESTATE(core), SPUREG_FM) >> 16;
    case 0x184: return spucore_getreg(CORESTATE(core), SPUREG_NOISE);
    case 0x186: return spucore_getreg(CORESTATE(core), SPUREG_NOISE) >> 16;
    case 0x188: return spucore_getreg(CORESTATE(core), SPUREG_VMIXL);
    case 0x18A: return spucore_getreg(CORESTATE(core), SPUREG_VMIXL) >> 16;
    case 0x18C: return spucore_getreg(CORESTATE(core), SPUREG_VMIXEL);
    case 0x18E: return spucore_getreg(CORESTATE(core), SPUREG_VMIXEL) >> 16;
    case 0x190: return spucore_getreg(CORESTATE(core), SPUREG_VMIXR);
    case 0x192: return spucore_getreg(CORESTATE(core), SPUREG_VMIXR) >> 16;
    case 0x194: return spucore_getreg(CORESTATE(core), SPUREG_VMIXER);
    case 0x196: return spucore_getreg(CORESTATE(core), SPUREG_VMIXER) >> 16;
    case 0x198: return get_mmix(state, core);
    case 0x19A: return get_ctrl(state, core);
    case 0x19C: return spucore_getreg(CORESTATE(core), SPUREG_IRQA) >> 17;
    case 0x19E: return spucore_getreg(CORESTATE(core), SPUREG_IRQA) >> 1;
    case 0x1A0: return spucore_getreg(CORESTATE(core), SPUREG_KON);
    case 0x1A2: return spucore_getreg(CORESTATE(core), SPUREG_KON) >> 16;
    case 0x1A4: return spucore_getreg(CORESTATE(core), SPUREG_KOFF);
    case 0x1A6: return spucore_getreg(CORESTATE(core), SPUREG_KOFF) >> 16;
    case 0x1A8: return get_tsa(state, core) >> 17;
    case 0x1AA: return get_tsa(state, core) >> 1;
    case 0x1AC: return get_transfer(state, core);
    case 0x1AE: return 0; // do not know what this is yet but it may be important
    case 0x1B0: return 0; // do not know what this is yet but it may be important
    case 0x1B2: return 0;
    case 0x1B4: return 0;
    case 0x1B6: return 0;
    case 0x1B8: return 0;
    case 0x1BA: return 0;
    case 0x1BC: return 0;
    case 0x1BE: return 0;
    }
  } else if(a < 0x2E0) {
    uint32 voice = (a-0x1C0)/12;
    switch((a-0x1C0)%12) {
    case 0x0: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_SSA ) >> 17;
    case 0x2: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_SSA ) >> 1;
    case 0x4: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_LSAX) >> 17;
    case 0x6: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_LSAX) >> 1;
    case 0x8: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_NAX ) >> 17;
    case 0xA: return spucore_getreg_voice(CORESTATE(core), voice, SPUREG_VOICE_NAX ) >> 1;
    }
  } else {
    switch(a) {
    case 0x2E0: return spucore_getreg(CORESTATE(core), SPUREG_ESA) >> 17;
    case 0x2E2: return spucore_getreg(CORESTATE(core), SPUREG_ESA) >> 1;
    case 0x2E4: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_FB_SRC_A   ) >> 17;
    case 0x2E6: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_FB_SRC_A   ) >> 1;
    case 0x2E8: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_FB_SRC_B   ) >> 17;
    case 0x2EA: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_FB_SRC_B   ) >> 1;
    case 0x2EC: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_A0) >> 17;
    case 0x2EE: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_A0) >> 1;
    case 0x2F0: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_A1) >> 17;
    case 0x2F2: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_A1) >> 1;
    case 0x2F4: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_A0 ) >> 17;
    case 0x2F6: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_A0 ) >> 1;
    case 0x2F8: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_A1 ) >> 17;
    case 0x2FA: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_A1 ) >> 1;
    case 0x2FC: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_B0 ) >> 17;
    case 0x2FE: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_B0 ) >> 1;
    case 0x300: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_B1 ) >> 17;
    case 0x302: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_B1 ) >> 1;
    case 0x304: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_A0 ) >> 17;
    case 0x306: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_A0 ) >> 1;
    case 0x308: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_A1 ) >> 17;
    case 0x30A: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_A1 ) >> 1;
    case 0x30C: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_B0) >> 17;
    case 0x30E: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_B0) >> 1;
    case 0x310: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_B1) >> 17;
    case 0x312: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_B1) >> 1;
    case 0x314: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_C0 ) >> 17;
    case 0x316: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_C0 ) >> 1;
    case 0x318: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_C1 ) >> 17;
    case 0x31A: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_C1 ) >> 1;
    case 0x31C: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_D0 ) >> 17;
    case 0x31E: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_D0 ) >> 1;
    case 0x320: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_D1 ) >> 17;
    case 0x322: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_D1 ) >> 1;
    case 0x324: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_B1 ) >> 17;
    case 0x326: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_B1 ) >> 1;
    case 0x328: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_B0 ) >> 17;
    case 0x32A: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_B0 ) >> 1;
    case 0x32C: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_A0) >> 17;
    case 0x32E: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_A0) >> 1;
    case 0x330: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_A1) >> 17;
    case 0x332: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_A1) >> 1;
    case 0x334: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_B0) >> 17;
    case 0x336: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_B0) >> 1;
    case 0x338: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_B1) >> 17;
    case 0x33A: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_B1) >> 1;
    case 0x33C: return spucore_getreg(CORESTATE(core), SPUREG_EEA) >> 17;
    case 0x33E: return spucore_getreg(CORESTATE(core), SPUREG_EEA) >> 1;
    case 0x340: return spucore_getreg(CORESTATE(core), SPUREG_EAX) >> 17;
    case 0x342: return spucore_getreg(CORESTATE(core), SPUREG_EAX) >> 1;
    case 0x344: return get_mystery_dma(state, core);
    case 0x760: return spucore_getreg(CORESTATE(core), SPUREG_MVOLL);
    case 0x762: return spucore_getreg(CORESTATE(core), SPUREG_MVOLR);
    case 0x764: return spucore_getreg(CORESTATE(core), SPUREG_EVOLL);
    case 0x766: return spucore_getreg(CORESTATE(core), SPUREG_EVOLR);
    case 0x768: return spucore_getreg(CORESTATE(core), SPUREG_AVOLL);
    case 0x76A: return spucore_getreg(CORESTATE(core), SPUREG_AVOLR);
    case 0x76C: return spucore_getreg(CORESTATE(core), SPUREG_BVOLL);
    case 0x76E: return spucore_getreg(CORESTATE(core), SPUREG_BVOLR);
    case 0x770: return spucore_getreg(CORESTATE(core), SPUREG_MVOLXL);
    case 0x772: return spucore_getreg(CORESTATE(core), SPUREG_MVOLXR);
    case 0x774: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_ALPHA );
    case 0x776: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_COEF_A);
    case 0x778: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_COEF_B);
    case 0x77A: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_COEF_C);
    case 0x77C: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_ACC_COEF_D);
    case 0x77E: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IIR_COEF  );
    case 0x780: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_FB_ALPHA  );
    case 0x782: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_FB_X      );
    case 0x784: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IN_COEF_L );
    case 0x786: return spucore_getreg(CORESTATE(core), SPUREG_REVERB_IN_COEF_R );
    }
  }
  return 0;
}

static void EMU_CALL sh2(struct SPU_STATE *state, uint32 a, uint16 d) {
  uint32 core = 0; a = spu2_reg_addr_normal(a, &core);
  if(a < 0x180) {
    uint32 voice = a >> 4;
    switch(a & 0xE) {
    case 0x0: spucore_setreg_voice(CORESTATE(core), voice, SPUREG_VOICE_VOLL , d, 0xFFFF); break;
    case 0x2: spucore_setreg_voice(CORESTATE(core), voice, SPUREG_VOICE_VOLR , d, 0xFFFF); break;
    case 0x4: spucore_setreg_voice(CORESTATE(core), voice, SPUREG_VOICE_PITCH, d, 0xFFFF); break;
    case 0x6: spucore_setreg_voice(CORESTATE(core), voice, SPUREG_VOICE_ADSR1, d, 0xFFFF); break;
    case 0x8: spucore_setreg_voice(CORESTATE(core), voice, SPUREG_VOICE_ADSR2, d, 0xFFFF); break;
    case 0xA: break; /* ENVX  is read-only */
    case 0xC: break; /* VOLXL is read-only */
    case 0xE: break; /* VOLXR is read-only */
    }
  } else if(a < 0x1C0) {
    switch(a) {
    case 0x180: spucore_setreg(CORESTATE(core), SPUREG_FM    , ((uint32)d)      , 0x0000FFFF); break;
    case 0x182: spucore_setreg(CORESTATE(core), SPUREG_FM    , ((uint32)d) << 16, 0xFFFF0000); break;
    case 0x184: spucore_setreg(CORESTATE(core), SPUREG_NOISE , ((uint32)d)      , 0x0000FFFF); break;
    case 0x186: spucore_setreg(CORESTATE(core), SPUREG_NOISE , ((uint32)d) << 16, 0xFFFF0000); break;
    case 0x188: spucore_setreg(CORESTATE(core), SPUREG_VMIXL , ((uint32)d)      , 0x0000FFFF); break;
    case 0x18A: spucore_setreg(CORESTATE(core), SPUREG_VMIXL , ((uint32)d) << 16, 0xFFFF0000); break;
    case 0x18C: spucore_setreg(CORESTATE(core), SPUREG_VMIXEL, ((uint32)d)      , 0x0000FFFF); break;
    case 0x18E: spucore_setreg(CORESTATE(core), SPUREG_VMIXEL, ((uint32)d) << 16, 0xFFFF0000); break;
    case 0x190: spucore_setreg(CORESTATE(core), SPUREG_VMIXR , ((uint32)d)      , 0x0000FFFF); break;
    case 0x192: spucore_setreg(CORESTATE(core), SPUREG_VMIXR , ((uint32)d) << 16, 0xFFFF0000); break;
    case 0x194: spucore_setreg(CORESTATE(core), SPUREG_VMIXER, ((uint32)d)      , 0x0000FFFF); break;
    case 0x196: spucore_setreg(CORESTATE(core), SPUREG_VMIXER, ((uint32)d) << 16, 0xFFFF0000); break;
    case 0x198: set_mmix(state, core, d); break;
    case 0x19A: set_ctrl(state, core, d); break;
    case 0x19C: spucore_setreg(CORESTATE(core), SPUREG_IRQA, ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x19E: spucore_setreg(CORESTATE(core), SPUREG_IRQA, ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x1A0: spucore_setreg(CORESTATE(core), SPUREG_KON , ((uint32)d)      , 0x0000FFFF); break;
    case 0x1A2: spucore_setreg(CORESTATE(core), SPUREG_KON , ((uint32)d) << 16, 0xFFFF0000); break;
    case 0x1A4: spucore_setreg(CORESTATE(core), SPUREG_KOFF, ((uint32)d)      , 0x0000FFFF); break;
    case 0x1A6: spucore_setreg(CORESTATE(core), SPUREG_KOFF, ((uint32)d) << 16, 0xFFFF0000); break;
    case 0x1A8: set_tsa(state, core, ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x1AA: set_tsa(state, core, ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x1AC: set_transfer(state, core, d); break;
    case 0x1AE: break; // do not know what this is yet but it may be important
    case 0x1B0: break; // do not know what this is yet but it may be important
    case 0x1B2: break;
    case 0x1B4: break;
    case 0x1B6: break;
    case 0x1B8: break;
    case 0x1BA: break;
    case 0x1BC: break;
    case 0x1BE: break;
    }
  } else if(a < 0x2E0) {
    uint32 voice = (a-0x1C0)/12;
    switch((a-0x1C0)%12) {
    case 0x0: spucore_setreg_voice(CORESTATE(core), voice, SPUREG_VOICE_SSA , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x2: spucore_setreg_voice(CORESTATE(core), voice, SPUREG_VOICE_SSA , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x4: spucore_setreg_voice(CORESTATE(core), voice, SPUREG_VOICE_LSAX, ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x6: spucore_setreg_voice(CORESTATE(core), voice, SPUREG_VOICE_LSAX, ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x8: break; /* NAX is read-only */
    case 0xA: break; /* NAX is read-only */
    }
  } else {
    switch(a) {
    case 0x2E0: spucore_setreg(CORESTATE(core), SPUREG_ESA               , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x2E2: spucore_setreg(CORESTATE(core), SPUREG_ESA               , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x2E4: spucore_setreg(CORESTATE(core), SPUREG_REVERB_FB_SRC_A   , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x2E6: spucore_setreg(CORESTATE(core), SPUREG_REVERB_FB_SRC_A   , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x2E8: spucore_setreg(CORESTATE(core), SPUREG_REVERB_FB_SRC_B   , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x2EA: spucore_setreg(CORESTATE(core), SPUREG_REVERB_FB_SRC_B   , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x2EC: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_A0, ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x2EE: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_A0, ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x2F0: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_A1, ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x2F2: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_A1, ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x2F4: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_A0 , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x2F6: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_A0 , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x2F8: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_A1 , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x2FA: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_A1 , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x2FC: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_B0 , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x2FE: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_B0 , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x300: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_B1 , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x302: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_B1 , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x304: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_A0 , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x306: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_A0 , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x308: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_A1 , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x30A: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_A1 , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x30C: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_B0, ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x30E: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_B0, ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x310: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_B1, ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x312: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_DEST_B1, ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x314: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_C0 , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x316: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_C0 , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x318: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_C1 , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x31A: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_C1 , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x31C: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_D0 , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x31E: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_D0 , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x320: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_D1 , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x322: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_SRC_D1 , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x324: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_B1 , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x326: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_B1 , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x328: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_B0 , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x32A: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_SRC_B0 , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x32C: spucore_setreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_A0, ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x32E: spucore_setreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_A0, ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x330: spucore_setreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_A1, ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x332: spucore_setreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_A1, ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x334: spucore_setreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_B0, ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x336: spucore_setreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_B0, ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x338: spucore_setreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_B1, ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x33A: spucore_setreg(CORESTATE(core), SPUREG_REVERB_MIX_DEST_B1, ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x33C: spucore_setreg(CORESTATE(core), SPUREG_EEA               , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x33E: spucore_setreg(CORESTATE(core), SPUREG_EEA               , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x340: spucore_setreg(CORESTATE(core), SPUREG_EAX               , ((uint32)d) << 17, 0xFFFE0000); break;
    case 0x342: spucore_setreg(CORESTATE(core), SPUREG_EAX               , ((uint32)d) << 1 , 0x0001FFFF); break;
    case 0x344: set_mystery_dma(state, core, d); break;
    case 0x760: spucore_setreg(CORESTATE(core), SPUREG_MVOLL, d, 0xFFFF); break;
    case 0x762: spucore_setreg(CORESTATE(core), SPUREG_MVOLR, d, 0xFFFF); break;
    case 0x764: spucore_setreg(CORESTATE(core), SPUREG_EVOLL, d, 0xFFFF); break;
    case 0x766: spucore_setreg(CORESTATE(core), SPUREG_EVOLR, d, 0xFFFF); break;
    case 0x768: spucore_setreg(CORESTATE(core), SPUREG_AVOLL, d, 0xFFFF); break;
    case 0x76A: spucore_setreg(CORESTATE(core), SPUREG_AVOLR, d, 0xFFFF); break;
    case 0x76C: spucore_setreg(CORESTATE(core), SPUREG_BVOLL, d, 0xFFFF); break;
    case 0x76E: spucore_setreg(CORESTATE(core), SPUREG_BVOLR, d, 0xFFFF); break;
    case 0x770: break; /* MVOLXL is read-only */
    case 0x772: break; /* MVOLXR is read-only */
    case 0x774: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_ALPHA , d, 0xFFFF); break;
    case 0x776: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_COEF_A, d, 0xFFFF); break;
    case 0x778: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_COEF_B, d, 0xFFFF); break;
    case 0x77A: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_COEF_C, d, 0xFFFF); break;
    case 0x77C: spucore_setreg(CORESTATE(core), SPUREG_REVERB_ACC_COEF_D, d, 0xFFFF); break;
    case 0x77E: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IIR_COEF  , d, 0xFFFF); break;
    case 0x780: spucore_setreg(CORESTATE(core), SPUREG_REVERB_FB_ALPHA  , d, 0xFFFF); break;
    case 0x782: spucore_setreg(CORESTATE(core), SPUREG_REVERB_FB_X      , d, 0xFFFF); break;
    case 0x784: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IN_COEF_L , d, 0xFFFF); break;
    case 0x786: spucore_setreg(CORESTATE(core), SPUREG_REVERB_IN_COEF_R , d, 0xFFFF); break;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/*
** Externally-accessible lh/sh
*/

uint16 EMU_CALL spu_lh(void *state, uint32 a) {
  a &= 0x1FFFFFFE;
  if(a >= 0x1F801C00 && a <= 0x1F801DFF) {
    return lh1(SPUSTATE, a);
  } else if(a >= 0x1F900000 && a <= 0x1F9007FF) {
    if(SPUSTATE->version == 2) return lh2(SPUSTATE, a);
  }
  return 0;
}

void EMU_CALL spu_sh(void *state, uint32 a, uint16 d) {
  a &= 0x1FFFFFFE;
  if(a >= 0x1F801C00 && a <= 0x1F801DFF) {
    sh1(SPUSTATE, a, d);
  } else if(a >= 0x1F900000 && a <= 0x1F9007FF) {
    if(SPUSTATE->version == 2) sh2(SPUSTATE, a, d);
  }
}

////////////////////////////////////////////////////////////////////////////////

void EMU_CALL spu_render(void *state, sint16 *buf, uint32 samples) {

  uint8 mainout = SPUSTATE->global_main_on;
  uint8 effectout = SPUSTATE->global_effect_on;
//  mainout = 0;
  if(SPUSTATE->version == 1) {
    spucore_render(CORESTATE(0), SPURAM, buf, NULL, samples, mainout, effectout);
  } else {
    spucore_render(CORESTATE(0), SPURAM, buf, NULL, samples, mainout, effectout);
    spucore_render(CORESTATE(1), SPURAM, buf, buf , samples, mainout, effectout);
//    spucore_render(CORESTATE(1), SPURAM, buf, NULL, samples);
  }
}

////////////////////////////////////////////////////////////////////////////////

void EMU_CALL spu_render_ext(void *state, sint16 *buf, sint16 *ext, uint32 samples) {

  uint8 mainout = SPUSTATE->global_main_on;
  uint8 effectout = SPUSTATE->global_effect_on;
//  mainout = 0;
  if(SPUSTATE->version == 1) {
    spucore_render(CORESTATE(0), SPURAM, buf, ext, samples, mainout, effectout);
  } else {
    spucore_render(CORESTATE(0), SPURAM, buf, ext, samples, mainout, effectout);
    spucore_render(CORESTATE(1), SPURAM, buf, buf, samples, mainout, effectout);
//    spucore_render(CORESTATE(1), SPURAM, buf, NULL, samples);
  }
}

////////////////////////////////////////////////////////////////////////////////

uint32 EMU_CALL spu_cycles_until_interrupt(void *state, uint32 samples) {
  if(SPUSTATE->version == 1) {
    return spucore_cycles_until_interrupt(CORESTATE(0), SPURAM, samples);
  } else {
    uint32 cycles1 = spucore_cycles_until_interrupt(CORESTATE(0), SPURAM, samples);
    uint32 cycles2 = spucore_cycles_until_interrupt(CORESTATE(1), SPURAM, samples);
    return cycles1 < cycles2 ? cycles1 : cycles2;
  }
}

////////////////////////////////////////////////////////////////////////////////
