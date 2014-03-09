#ifndef _PLAYPTMOD_H_
#define _PLAYPTMOD_H_

#ifdef __cplusplus
extern "C" {
#endif

void * playptmod_Create(int samplingFrequency);

#define PTMOD_OPTION_CLAMP_PERIODS 0
/* Set before loading module, or notes will be clamped accordingly
   1 (default) = Amiga / Protracker range
 * 0           = MSDOS / MTM / extended range */

#define PTMOD_OPTION_VSYNC_TIMING  1
/* 0 (default) = Speed command 20 or higher sets tempo
 * 1           = Speed command always sets speed */

void playptmod_Config(void *p, int option, int value);

int playptmod_LoadMem(void *p, const unsigned char *buf, unsigned long bufLength);
int playptmod_Load(void *p, const char *filename);

void playptmod_Play(void *p, unsigned int startOrder);
void playptmod_Stop(void *p);
void playptmod_Render(void *p, signed int *target, int length);
void playptmod_Render16(void *p, signed short *target, int length);

void playptmod_Mute(void *p, int channel, int mute);

unsigned int playptmod_LoopCounter(void *p);

typedef struct _ptmi
{
  unsigned char order;
  unsigned char pattern;
  unsigned char row;
  unsigned char speed;
  unsigned char tempo;
  unsigned char channelsPlaying;
} playptmod_info;

void playptmod_GetInfo(void *p, playptmod_info *i);

void playptmod_Free(void *p);

#ifdef __cplusplus
}
#endif

#endif
