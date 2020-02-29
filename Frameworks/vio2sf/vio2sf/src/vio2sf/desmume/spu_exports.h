#ifndef _SPU_EXPORTS_H
#define _SPU_EXPORTS_H

#ifndef _SPU_CPP_

#include "types.h"

typedef struct NDS_state NDS_state;

void SPU_WriteLong(NDS_state *, u32 addr, u32 val);
void SPU_WriteByte(NDS_state *, u32 addr, u8 val);
void SPU_WriteWord(NDS_state *, u32 addr, u16 val);
void SPU_EmulateSamples(NDS_state *, int numsamples);
int SPU_ChangeSoundCore(NDS_state *, int coreid, int buffersize);
void SPU_Reset(NDS_state *);
int SPU_Init(NDS_state *, int, int);
void SPU_DeInit(NDS_state *);
void SPU_Pause(NDS_state *state, int pause);
void SPU_SetVolume(NDS_state *state, int volume);

typedef struct SoundInterface_struct
{
   int id;
   const char *Name;
   int (*Init)(NDS_state *, int buffersize);
   void (*DeInit)(NDS_state *);
   void (*UpdateAudio)(NDS_state *, s16 *buffer, u32 num_samples);
   u32 (*GetAudioSpace)(NDS_state *);
   void (*MuteAudio)(NDS_state *);
   void (*UnMuteAudio)(NDS_state *);
   void (*SetVolume)(NDS_state *, int volume);
} SoundInterface_struct;

#endif //_SPU_CPP_

#endif //_SPU_EXPORTS_H
