/////////////////////////////////////////////////////////////////////////////
//
// spucore - Emulates a single SPU CORE
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __PSX_SPUCORE_H__
#define __PSX_SPUCORE_H__

#include "emuconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

sint32 EMU_CALL spucore_init(void);
uint32 EMU_CALL spucore_get_state_size(void);
void   EMU_CALL spucore_clear_state(void *state);

void   EMU_CALL spucore_set_mem_size(void *state, uint32 size);

void   EMU_CALL spucore_render(void *state, uint16 *ram, sint16 *buf, sint16 *extinput, uint32 samples, uint8 mainout, uint8 effectout);

uint32 EMU_CALL spucore_getreg      (void *state, uint32 n);
void   EMU_CALL spucore_setreg      (void *state, uint32 n, uint32 value, uint32 mask);
uint32 EMU_CALL spucore_getreg_voice(void *state, uint32 voice, uint32 n);
void   EMU_CALL spucore_setreg_voice(void *state, uint32 voice, uint32 n, uint32 value, uint32 mask);
int    EMU_CALL spucore_getflag     (void *state, uint32 n);
void   EMU_CALL spucore_setflag     (void *state, uint32 n, int value);

uint32 EMU_CALL spucore_cycles_until_interrupt(void *state, uint16 *ram, uint32 samples);

/*
** Register definitions
*/

/* Flags */

#define SPUREG_FLAG_ON             (1<<19)
#define SPUREG_FLAG_MAIN_ENABLE    (1<<18)
#define SPUREG_FLAG_REVERB_ENABLE  (1<<17)
#define SPUREG_FLAG_IRQ_ENABLE     (1<<16)

#define SPUREG_FLAG_ER             (1<<15)
#define SPUREG_FLAG_CR             (1<<14)
#define SPUREG_FLAG_EE             (1<<13)
#define SPUREG_FLAG_CE             (1<<12)

#define SPUREG_FLAG_MSNDL          (1<<11)
#define SPUREG_FLAG_MSNDR          (1<<10)
#define SPUREG_FLAG_MSNDEL         (1<< 9)
#define SPUREG_FLAG_MSNDER         (1<< 8)
#define SPUREG_FLAG_MINL           (1<< 7)
#define SPUREG_FLAG_MINR           (1<< 6)
#define SPUREG_FLAG_MINEL          (1<< 5)
#define SPUREG_FLAG_MINER          (1<< 4)
#define SPUREG_FLAG_SINL           (1<< 3)
#define SPUREG_FLAG_SINR           (1<< 2)
#define SPUREG_FLAG_SINEL          (1<< 1)
#define SPUREG_FLAG_SINER          (1<< 0)

/* Voice registers */

enum {
  SPUREG_VOICE_VOLL,
  SPUREG_VOICE_VOLR,
  SPUREG_VOICE_VOLXL,
  SPUREG_VOICE_VOLXR,
  SPUREG_VOICE_PITCH,
  SPUREG_VOICE_SSA,
  SPUREG_VOICE_ADSR1,
  SPUREG_VOICE_ADSR2,
  SPUREG_VOICE_ENVX,
  SPUREG_VOICE_LSAX,
  SPUREG_VOICE_NAX
};

/* Main registers */

enum {
  SPUREG_MVOLL,
  SPUREG_MVOLR,
  SPUREG_MVOLXL,
  SPUREG_MVOLXR,
  SPUREG_EVOLL,
  SPUREG_EVOLR,
  SPUREG_AVOLL,
  SPUREG_AVOLR,
  SPUREG_BVOLL,
  SPUREG_BVOLR,
  SPUREG_KON,
  SPUREG_KOFF,
  SPUREG_FM,
  SPUREG_NOISE,
  SPUREG_VMIXE,
  SPUREG_VMIX,
  SPUREG_VMIXEL,
  SPUREG_VMIXER,
  SPUREG_VMIXL,
  SPUREG_VMIXR,
  SPUREG_ESA,
  SPUREG_EEA,
  SPUREG_EAX,
  SPUREG_IRQA,

  SPUREG_NOISECLOCK,

  SPUREG_REVERB_FB_SRC_A,
  SPUREG_REVERB_FB_SRC_B,
  SPUREG_REVERB_IIR_ALPHA,
  SPUREG_REVERB_ACC_COEF_A,
  SPUREG_REVERB_ACC_COEF_B,
  SPUREG_REVERB_ACC_COEF_C,
  SPUREG_REVERB_ACC_COEF_D,
  SPUREG_REVERB_IIR_COEF,
  SPUREG_REVERB_FB_ALPHA,
  SPUREG_REVERB_FB_X,
  SPUREG_REVERB_IIR_DEST_A0,
  SPUREG_REVERB_IIR_DEST_A1,
  SPUREG_REVERB_ACC_SRC_A0,
  SPUREG_REVERB_ACC_SRC_A1,
  SPUREG_REVERB_ACC_SRC_B0,
  SPUREG_REVERB_ACC_SRC_B1,
  SPUREG_REVERB_IIR_SRC_A0,
  SPUREG_REVERB_IIR_SRC_A1,
  SPUREG_REVERB_IIR_DEST_B0,
  SPUREG_REVERB_IIR_DEST_B1,
  SPUREG_REVERB_ACC_SRC_C0,
  SPUREG_REVERB_ACC_SRC_C1,
  SPUREG_REVERB_ACC_SRC_D0,
  SPUREG_REVERB_ACC_SRC_D1,
  SPUREG_REVERB_IIR_SRC_B1,
  SPUREG_REVERB_IIR_SRC_B0,
  SPUREG_REVERB_MIX_DEST_A0,
  SPUREG_REVERB_MIX_DEST_A1,
  SPUREG_REVERB_MIX_DEST_B0,
  SPUREG_REVERB_MIX_DEST_B1,
  SPUREG_REVERB_IN_COEF_L,
  SPUREG_REVERB_IN_COEF_R

};


#ifdef __cplusplus
}
#endif

#endif
