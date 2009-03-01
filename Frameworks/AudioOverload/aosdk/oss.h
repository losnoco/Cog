#ifndef _OSS_H_
#define _OSS_H_

extern int audiofd;
extern void (*m1sdr_Callback)(unsigned long dwUser, signed short *smp);
extern unsigned long cbUserData;

// function protos

void m1sdr_Update(void);
INT16 m1sdr_Init(int sample_rate);
void m1sdr_Exit(void);
void m1sdr_PlayStart(void);
void m1sdr_PlayStop(void);
INT16 m1sdr_IsThere(void);
void m1sdr_TimeCheck(void);
void m1sdr_SetSamplesPerTick(UINT32 spf);
void m1sdr_SetHz(UINT32 hz);
void m1sdr_SetCallback(void *fn);
void m1sdr_SetCPUHog(int hog);
INT32 m1sdr_HwPresent(void);
void m1sdr_FlushAudio(void);
void m1sdr_Pause(int); 
void m1sdr_SetNoWait(int nw);
short *m1sdr_GetSamples(void);
int m1sdr_GetPlayTime(void);
#endif
