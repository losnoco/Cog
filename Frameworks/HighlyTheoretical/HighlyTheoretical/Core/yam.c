/////////////////////////////////////////////////////////////////////////////
//
// yam - Emulates Yamaha SCSP and AICA
//
/////////////////////////////////////////////////////////////////////////////

#ifndef EMU_COMPILE
#error "Hi I forgot to set EMU_COMPILE"
#endif

#include "yam.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined(HAVE_MPROTECT)
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#endif

#include <math.h>

#ifndef _WIN32
#define __cdecl
#define __fastcall __attribute__((regparm(3)))
#endif

/* No dynarec for x86_64 yet */
#if defined(_WIN32) || defined(__i386__)
#define ENABLE_DYNAREC
#endif
#if defined(_WIN64) || defined(__amd64__)
#undef ENABLE_DYNAREC
#endif

// no 'conversion from _blah_ possible loss of data' warnings
#pragma warning (disable: 4244)

/////////////////////////////////////////////////////////////////////////////

// meh, these don't like to work higher than 1 and 2
#define RENDERMAX (200)
#define RINGMAX   (256) // should be nearest power of two that's at least one greater than RENDERMAX

/////////////////////////////////////////////////////////////////////////////

#define INT_ONE_SAMPLE  (10)
#define INT_MIDI_OUTPUT (9)
#define INT_TIMER_C     (8)
#define INT_TIMER_B     (7)
#define INT_TIMER_A     (6)
#define INT_CPU         (5)
#define INT_DMA_END     (4)
#define INT_MIDI_INPUT  (3)
#define INT_RESERVED_2  (2)
#define INT_RESERVED_1  (1)
#define INT_EXTERNAL    (0)

/////////////////////////////////////////////////////////////////////////////
//
// Static information
//

#if 0
#define MAKELFOPHASEINC(x) (((uint64)(0x100000000)) / ((uint64)(x)))

static uint32 lfophaseinctable[0x20] = {
MAKELFOPHASEINC(0x3FC00),MAKELFOPHASEINC(0x37C00),MAKELFOPHASEINC(0x2FC00),MAKELFOPHASEINC(0x27C00),
MAKELFOPHASEINC(0x1FC00),MAKELFOPHASEINC(0x1BC00),MAKELFOPHASEINC(0x17C00),MAKELFOPHASEINC(0x13C00),
MAKELFOPHASEINC(0x0FC00),MAKELFOPHASEINC(0x0BC00),MAKELFOPHASEINC(0x0DC00),MAKELFOPHASEINC(0x09C00),
MAKELFOPHASEINC(0x07C00),MAKELFOPHASEINC(0x06C00),MAKELFOPHASEINC(0x05C00),MAKELFOPHASEINC(0x04C00),
MAKELFOPHASEINC(0x03C00),MAKELFOPHASEINC(0x03400),MAKELFOPHASEINC(0x02C00),MAKELFOPHASEINC(0x02400),
MAKELFOPHASEINC(0x01C00),MAKELFOPHASEINC(0x01800),MAKELFOPHASEINC(0x01400),MAKELFOPHASEINC(0x01000),
MAKELFOPHASEINC(0x00C00),MAKELFOPHASEINC(0x00A00),MAKELFOPHASEINC(0x00800),MAKELFOPHASEINC(0x00600),
MAKELFOPHASEINC(0x00400),MAKELFOPHASEINC(0x00300),MAKELFOPHASEINC(0x00200),MAKELFOPHASEINC(0x00100)
};

static uint8 envattackshift[0x3D][4] = {
/* 00-07 */ {4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},
/* 08-0F */ {4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},
/* 10-17 */ {4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},
/* 18-1F */ {4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},
/* 20-27 */ {4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},
/* 28-2F */ {4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},{4,4,4,4},
/* 30-37 */ {4,4,4,4},{3,4,4,4},{3,4,3,4},{3,3,3,4},{3,3,3,3},{2,3,3,3},{2,3,2,3},{2,2,2,3},
/* 38-3C */ {2,2,2,2},{1,2,2,2},{1,2,1,2},{1,1,1,2},{1,1,1,1}
};
#endif

static uint8 envdecayvalue[0x3D][4] = {
/* 00-07 */ {1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},
/* 08-0F */ {1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},
/* 10-17 */ {1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},
/* 18-1F */ {1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},
/* 20-27 */ {1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},
/* 28-2F */ {1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},{1,1,1,1},
/* 30-37 */ {1,1,1,1},{2,1,1,1},{2,1,2,1},{2,2,2,1},{2,2,2,2},{4,2,2,2},{4,2,4,2},{4,4,4,2},
/* 38-3C */ {4,4,4,4},{8,4,4,4},{8,4,8,4},{8,8,8,4},{8,8,8,8}
};

#if 0
static sint32 adpcmscale[8] = {
  0x0E6, 0x0E6, 0x0E6, 0x0E6, 0x133, 0x199, 0x200, 0x266
};

static sint32 adpcmdiff[16] = {
 1, 3, 5, 7, 9, 11, 13, 15,
-1,-3,-5,-7,-9,-11,-13,-15
};
#endif

static sint32 qtable[32] = {
0x0E00,0x0E80,0x0F00,0x0F80,
0x1000,0x1080,0x1100,0x1180,
0x1200,0x1280,0x1300,0x1280,
0x1400,0x1480,0x1500,0x1580,
0x1600,0x1680,0x1700,0x1780,
0x1800,0x1880,0x1900,0x1980,
0x1A00,0x1A80,0x1B00,0x1B80,
0x1C00,0x1D00,0x1E00,0x1F00
};

// Lifted from MAME/etc cores by kode54, in an attempt to fix some stuff
//Envelope times in ms
#define LFO_SHIFT   8

struct _LFO
{
  unsigned short phase;
  uint32 phase_step;
  int *table;
  int *scale;
};

#define LFIX(v)  ((unsigned int) ((float) (1<<LFO_SHIFT)*(v)))

//Convert DB to multiply amplitude
#define DB(v)   LFIX(pow(10.0,v/20.0))

//Convert cents to step increment
#define CENTS(v) LFIX(pow(2.0,v/1200.0))

static int PLFO_TRI[256],PLFO_SQR[256],PLFO_SAW[256],PLFO_NOI[256];
static int ALFO_TRI[256],ALFO_SQR[256],ALFO_SAW[256],ALFO_NOI[256];
static float LFOFreq[32]={0.17,0.19,0.23,0.27,0.34,0.39,0.45,0.55,0.68,0.78,0.92,1.10,1.39,1.60,1.87,2.27,
  2.87,3.31,3.92,4.79,6.15,7.18,8.60,10.8,14.4,17.2,21.5,28.7,43.1,57.4,86.1,172.3};
static float ASCALE[8]={0.0,0.4,0.8,1.5,3.0,6.0,12.0,24.0};
static float PSCALE[8]={0.0,7.0,13.5,27.0,55.0,112.0,230.0,494};
static int PSCALES[8][256];
static int ASCALES[8][256];

static const double ARTimes[64]={100000/*infinity*/,100000/*infinity*/,8100.0,6900.0,6000.0,4800.0,4000.0,3400.0,3000.0,2400.0,2000.0,1700.0,1500.0,
  1200.0,1000.0,860.0,760.0,600.0,500.0,430.0,380.0,300.0,250.0,220.0,190.0,150.0,130.0,110.0,95.0,
  76.0,63.0,55.0,47.0,38.0,31.0,27.0,24.0,19.0,15.0,13.0,12.0,9.4,7.9,6.8,6.0,4.7,3.8,3.4,3.0,2.4,
  2.0,1.8,1.6,1.3,1.1,0.93,0.85,0.65,0.53,0.44,0.40,0.35,0.0,0.0};
static const double DRTimes[64]={100000/*infinity*/,100000/*infinity*/,118200.0,101300.0,88600.0,70900.0,59100.0,50700.0,44300.0,35500.0,29600.0,25300.0,22200.0,17700.0,
  14800.0,12700.0,11100.0,8900.0,7400.0,6300.0,5500.0,4400.0,3700.0,3200.0,2800.0,2200.0,1800.0,1600.0,1400.0,1100.0,
  920.0,790.0,690.0,550.0,460.0,390.0,340.0,270.0,230.0,200.0,170.0,140.0,110.0,98.0,85.0,68.0,57.0,49.0,43.0,34.0,
  28.0,25.0,22.0,18.0,14.0,12.0,11.0,8.5,7.1,6.1,5.4,4.3,3.6,3.1};
static uint32 FNS_Table[0x400];
static sint32 EG_TABLE[0x400];
static int ARTABLE[64];
static int DRTABLE[64];

static const float SDLT_AICA[16]={-1000000.0,-42.0,-39.0,-36.0,-33.0,-30.0,-27.0,-24.0,-21.0,-18.0,-15.0,-12.0,-9.0,-6.0,-3.0,0.0};
static const float SDLT_SCSP[8]={-1000000.0,-36.0,-30.0,-24.0,-18.0,-12.0,-6.0,0.0};

static sint32 LPANTABLE_SCSP[0x10000];
static sint32 RPANTABLE_SCSP[0x10000];

static sint32 LPANTABLE_AICA[0x20000];
static sint32 RPANTABLE_AICA[0x20000];

#define EG_SHIFT  16
#define SHIFT   10
#define FIX(v)  ((sint32) ((float) (1<<SHIFT)*(v)))

#define log_base_2(n) (log((float) n)/log((float) 2))

static void yam_init_tables()
{
  sint32 i,s;

  for(i=0;i<256;++i) {
    int a,p;
    //    float TL;
    //Saw
    a=255-i;
    if(i<128)
      p=i;
    else
      p=i-256;    
    ALFO_SAW[i]=a;
    PLFO_SAW[i]=p;

    //Square
    if(i<128) {
      a=255;
      p=127;
    }
    else {
      a=0;
      p=-128;
    }
    ALFO_SQR[i]=a;
    PLFO_SQR[i]=p;

    //Tri
    if(i<128)
      a=255-(i*2);
    else
      a=(i*2)-256;
    if(i<64)
      p=i*2;
    else if(i<128)
      p=255-i*2;
    else if(i<192)
      p=256-i*2;
    else
      p=i*2-511;
    ALFO_TRI[i]=a;
    PLFO_TRI[i]=p;

    //noise
    //a=lfo_noise[i];
    a=rand()&0xff;
    p=128-a;
    ALFO_NOI[i]=a;
    PLFO_NOI[i]=p;
  }

  for(s=0;s<8;++s) {
    float limit=PSCALE[s];
    for(i=-128;i<128;++i) {
      PSCALES[s][i+128]=CENTS(((limit*(float) i)/128.0));
    }
    limit=-ASCALE[s];
    for(i=0;i<256;++i) {
      ASCALES[s][i]=DB(((limit*(float) i)/256.0));
    }
  }

  for(i=0;i<0x400;++i) {
    float fcent=(double) 1200.0*log_base_2((double)(((double) 1024.0+(double)i)/(double)1024.0));
    fcent=(double) 44100.0*pow(2.0,fcent/1200.0);
    FNS_Table[i]=(float) (1<<SHIFT) *fcent;
  }

  for(i=0;i<0x400;++i) {
    float envDB=((float)(3*(i-0x3ff)))/32.0;
    float scale=(float)(1<<SHIFT);
    EG_TABLE[i]=(sint32)(pow(10.0,envDB/20.0)*scale);
  }

  for(i=0;i<0x10000;++i) {
    int iTL =(i>>0x0)&0xff;
    int iPAN=(i>>0x8)&0x1f;
    int iSDL=(i>>0xD)&0x07;
    float TL=1.0;
    float SegaDB=0;
    float fSDL=1.0;
    float PAN=1.0;
    float LPAN,RPAN;

    if(iTL&0x01) SegaDB-=0.4;
    if(iTL&0x02) SegaDB-=0.8;
    if(iTL&0x04) SegaDB-=1.5;
    if(iTL&0x08) SegaDB-=3;
    if(iTL&0x10) SegaDB-=6;
    if(iTL&0x20) SegaDB-=12;
    if(iTL&0x40) SegaDB-=24;
    if(iTL&0x80) SegaDB-=48;

    TL=pow(10.0,SegaDB/20.0);

    SegaDB=0;
    if(iPAN&0x1) SegaDB-=3;
    if(iPAN&0x2) SegaDB-=6;
    if(iPAN&0x4) SegaDB-=12;
    if(iPAN&0x8) SegaDB-=24;

    if((iPAN&0xf)==0xf) PAN=0.0;
    else PAN=pow(10.0,SegaDB/20.0);

    if(iPAN<0x10)
    {
      LPAN=PAN;
      RPAN=1.0;
    }
    else
    {
      RPAN=PAN;
      LPAN=1.0;
    }

    if(iSDL)
      fSDL=pow(10.0,(SDLT_SCSP[iSDL])/20.0);
    else
      fSDL=0.0;

    LPANTABLE_SCSP[i]=FIX((4.0*LPAN*TL*fSDL));
    RPANTABLE_SCSP[i]=FIX((4.0*RPAN*TL*fSDL));
  }

  for(i=0;i<0x20000;++i) {
    int iTL =(i>>0x0)&0xff;
    int iPAN=(i>>0x8)&0x1f;
    int iSDL=(i>>0xD)&0x0F;
    float TL=1.0;
    float SegaDB=0;
    float fSDL=1.0;
    float PAN=1.0;
    float LPAN,RPAN;

    if(iTL&0x01) SegaDB-=0.4;
    if(iTL&0x02) SegaDB-=0.8;
    if(iTL&0x04) SegaDB-=1.5;
    if(iTL&0x08) SegaDB-=3;
    if(iTL&0x10) SegaDB-=6;
    if(iTL&0x20) SegaDB-=12;
    if(iTL&0x40) SegaDB-=24;
    if(iTL&0x80) SegaDB-=48;

    TL=pow(10.0,SegaDB/20.0);

    SegaDB=0;
    if(iPAN&0x1) SegaDB-=3;
    if(iPAN&0x2) SegaDB-=6;
    if(iPAN&0x4) SegaDB-=12;
    if(iPAN&0x8) SegaDB-=24;

    if((iPAN&0xf)==0xf) PAN=0.0;
    else PAN=pow(10.0,SegaDB/20.0);

    if(iPAN<0x10)
    {
      LPAN=PAN;
      RPAN=1.0;
    }
    else
    {
      RPAN=PAN;
      LPAN=1.0;
    }

    if(iSDL)
      fSDL=pow(10.0,(SDLT_AICA[iSDL])/20.0);
    else
      fSDL=0.0;

    LPANTABLE_AICA[i]=FIX((4.0*LPAN*TL*fSDL));
    RPANTABLE_AICA[i]=FIX((4.0*RPAN*TL*fSDL));
  }

  ARTABLE[0]=DRTABLE[0]=0;  //Infinite time
  ARTABLE[1]=DRTABLE[1]=0;  //Infinite time
  for(i=2;i<64;++i) {
    double t,step,scale;
    t=ARTimes[i];  //In ms
    if(t!=0.0) {
      step=(1023*1000.0)/((float) 44100.0f*t);
      scale=(double) (1<<EG_SHIFT);
      ARTABLE[i]=(int) (step*scale);
    }
    else
      ARTABLE[i]=1024<<EG_SHIFT;

    t=DRTimes[i];  //In ms
    step=(1023*1000.0)/((float) 44100.0f*t);
    scale=(double) (1<<EG_SHIFT);
    DRTABLE[i]=(int) (step*scale);
  }
}

static void convert_stereo_send_level(
  uint8 tl, uint8 sdl, uint8 pan,
  sint32 *vol_l, sint32 *vol_r,
  uint32 version
) {
  uint32 Enc = (tl<<0) | (pan<<8) | (sdl<<13);
  if (version == 1) {
    Enc &= 0xFFFF;
    *vol_l = LPANTABLE_SCSP[Enc];
    *vol_r = RPANTABLE_SCSP[Enc];
  } else {
    *vol_l = LPANTABLE_AICA[Enc];
    *vol_r = RPANTABLE_AICA[Enc];
  }
}

/////////////////////////////////////////////////////////////////////////////

sint32 EMU_CALL yam_init(void) {
  yam_init_tables();
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
/*
static int gfreq[201];

void yam_debugoutput(void) {
  int i;
  int t=0;
  int u=0;
  for(i=0;i<201;i++) {
    printf("gfreq[%3d]=%d\n",i,gfreq[i]);
    t += i*gfreq[i];
    u += gfreq[i];
  }
  printf("avg = %d/%d\n",t,u);
  
}
*/
/////////////////////////////////////////////////////////////////////////////
//
// State information
//
#define YAMSTATE ((struct YAM_STATE*)(state))

#define LOOP_NONE          (0)
#define LOOP_FORWARDS      (1)
#define LOOP_BACKWARDS     (2)
#define LOOP_BIDIRECTIONAL (3)

typedef enum {ATTACK,DECAY1,DECAY2,RELEASE} _STATE;
struct _EG
{
  int volume;  //
  _STATE state;
  int step;
  //step vals
  int AR;    //Attack
  int D1R;   //Decay1
  int D2R;   //Decay2
  int RR;    //Release

  int DL;    //Decay level
  uint8 EGHOLD;
  uint8 LPLINK;
};

struct YAM_CHAN {
  uint8 kyonb;
  uint8 ssctl;
  uint8 sampler_looptype;
  sint32 sampler_invert; // bits 15-31 = invert sign bit, bits 0-14 = invert other bits
  uint8 pcms;
  uint32 sampleaddr;
  sint32 loopstart;
  sint32 loopend;
  uint8 Backwards;  //the wave is playing backwards
  struct _EG EG;
  struct _LFO PLFO;    //Phase LFO
  struct _LFO ALFO;    //Amplitude LFO
  uint8 ar[4]; // amplitude envelope rate: attack, decay, sustain, release
  uint8 dl;
  uint8 krs;
  uint8 link;
  uint8 eghold;
  uint8 oct;
  uint16 fns;
  uint8 lfore;
  uint8 lfof;
  uint8 plfows;
  uint8 plfos;
  uint8 alfows;
  uint8 alfos;
  uint8 dspchan;
  uint8 dsplevel;
  uint8 disdl;
  uint8 dipan;
  uint8 tl;
  uint8 voff;
  uint8 lpoff;
  uint8 q;
  uint8 stwinh;
  uint8 mdl;
  uint8 mdxsl;
  uint8 mdysl;
  uint16 flv[5];
  uint8 fr[4]; // filter envelope rate: attack, decay, sustain, release
  uint16 lpflevel;
  uint8 lpfstate;
  uint8 lp;
  sint32 prv_addr;    // previous play address (for ADPCM)
  uint32 cur_addr;
  uint32 nxt_addr;
  uint32 step; //pitch step (24.8)
  uint32 frcphase;
  uint32 lfophase;
  sint32 samplebufcur; // these are 16-bit signed
  sint32 samplebufnext; // these are 16-bit signed
  int cur_sample;       //current ADPCM sample
  int cur_quant;        //current ADPCM step
  int curstep;
  int cur_lpquant, cur_lpsample, cur_lpstep;
  uint32 adbase, adlpbase;
  sint32 lpp1;
  sint32 lpp2;
};

struct MPRO {
  uint8 c_0rrrrrrr; // CRA (up to 7 bits)
  uint8 t_0rrrrrrr; // TRA
  uint8 t_Twwwwwww; // !TWT, TWA
  sint8 tablemask;  // -1 if table
  sint8 adrmask;    // -1 if adreb=1
  sint8 negb;       // -1 if negb
  uint8 __kisxzbon; // skip, interpolate, saturate, XSEL, ZERO, BSEL, NOFL, NXADR
  uint8 m_wrAFyyYh; // MWT, MRD, ADRL, FRCL, YSEL, YRL, shift-left-by
  uint8 i_00rrrrrr; // IRA
  uint8 i_0T0wwwww; // !IWT, IWA
  uint8 e_000Twwww; // !EWT, EWA
  uint8 m_00aaaaaa; // MASA
};

static uint64 mpro_scsp_read(struct MPRO *mpro) {
  uint64 value = 0;
  value |= ((uint64)(mpro->t_0rrrrrrr       )) << 56; // TRA
  value |= ((uint64)(mpro->t_Twwwwwww ^ 0x80)) << 48; // !TWT, TWA
  value |= ((uint64)(mpro->tablemask  &    1)) << 31; // TABLE
  value |= ((uint64)(mpro->adrmask    &    1)) <<  1; // ADREB
  value |= ((uint64)(mpro->negb       &    1)) << 18; // NEGB
  { uint64 sh = mpro->m_wrAFyyYh & 1;
    if((mpro->__kisxzbon & 0x20) == 0) { sh ^= 3; }
    value |= (sh << 20); // SHFT
  }
  value |= ((uint64)(mpro->__kisxzbon & 0x10)) << 43; // XSEL
  value |= ((uint64)(mpro->__kisxzbon & 0x0C)) << 14; // ZERO, BSEL
  value |= ((uint64)(mpro->__kisxzbon & 0x02)) <<  6; // NOFL *** THIS IS JUST A GUESS ***
  value |= ((uint64)(mpro->__kisxzbon & 0x01)) <<  0; // NXADR
  value |= ((uint64)(mpro->m_wrAFyyYh & 0xC0)) << 23; // MWT, MRD
  value |= ((uint64)(mpro->m_wrAFyyYh & 0x32)) << 18; // ADRL, FRCL, YRL
  value |= ((uint64)(mpro->m_wrAFyyYh & 0x0C)) << 43; // YSEL
  value |= ((uint64)(mpro->i_00rrrrrr & 0x3F)) << 38; // IRA
  value |= ((uint64)(mpro->i_0T0wwwww & 0x1F)) << 32; // IWA
  value |= ((uint64)((mpro->i_0T0wwwww & 0x40) ^ 0x40)) << 31; // !IWT
  value |= ((uint64)((mpro->e_000Twwww & 0x1F) ^ 0x10)) << 24; // !EWT, EWA
  value |= ((uint64)(mpro->m_00aaaaaa & 0x1F)) <<  2; // MASA (fewer bits on SCSP)
  value |= ((uint64)(mpro->c_0rrrrrrr & 0x3F)) <<  9; // CRA (SCSP exclusive)

  return value;
}

static void mpro_scsp_write(struct MPRO *mpro, uint64 value) {
  mpro->t_0rrrrrrr = ((value >> 56) & 0x7F); // TRA
  mpro->t_Twwwwwww = ((value >> 48) ^ 0x80); // !TWT, TWA
  mpro->tablemask  = ((value >> 31) & 1) ? (-1) : (0); // TABLE
  mpro->adrmask    = ((value >>  1) & 1) ? (-1) : (0); // ADREB
  mpro->negb       = ((value >> 18) & 1) ? (-1) : (0); // NEGB
  mpro->__kisxzbon = 0;
  if(!value) { mpro->__kisxzbon |= 0x80; } // skip
  if(((value >> 20) & 3) == 3) { mpro->__kisxzbon |= 0x40; } // interpolate
  if(((value >> 21) & 1) == 0) { mpro->__kisxzbon |= 0x20; } // saturate
  mpro->__kisxzbon |= (value >> 43) & 0x10; // XSEL
  mpro->__kisxzbon |= (value >> 14) & 0x0C; // ZERO, BSEL
  mpro->__kisxzbon |= (value >> 6) & 0x02; // NOFL *** THIS IS JUST A GUESS ***
  mpro->__kisxzbon |= (value >> 0) & 1; // NXADR
  mpro->m_wrAFyyYh  = (value >> 23) & 0xC0; // MWT, MRD
  mpro->m_wrAFyyYh |= (value >> 18) & 0x32; // ADRL, FRCL, YRL
  mpro->m_wrAFyyYh |= (value >> 43) & 0x0C; // YSEL
  mpro->m_wrAFyyYh |= ((value >> 20) & 1) ^ ((value >> 21) & 1); // shift left by
  mpro->i_00rrrrrr = (value >> 38) & 0x3F; // IRA
  mpro->i_0T0wwwww = (value >> 32) & 0x1F; // IWA
  mpro->i_0T0wwwww |= ((value >> 31) & 0x40) ^ 0x40; // !IWT
  mpro->e_000Twwww = ((value >> 24) & 0x1F) ^ 0x10; // !EWT, EWA
  mpro->m_00aaaaaa = (value >> 2) & 0x1F; // MASA (fewer bits on SCSP)
  mpro->c_0rrrrrrr = (value >> 9) & 0x3F; // CRA (SCSP exclusive)
}

static void mpro_aica_write(struct MPRO *mpro, uint64 value) {
  mpro->t_0rrrrrrr = ((value >> 57) & 0x7F); // TRA
  mpro->t_Twwwwwww = ((value >> 49) ^ 0x80); // !TWT, TWA
  mpro->tablemask  = ((value >> 31) & 1) ? (-1) : (0); // TABLE
  mpro->adrmask    = ((value >>  8) & 1) ? (-1) : (0); // ADREB
  mpro->negb       = ((value >> 18) & 1) ? (-1) : (0); // NEGB
  mpro->__kisxzbon = 0;
  if(!value) { mpro->__kisxzbon |= 0x80; } // skip
  if(((value >> 20) & 3) == 3) { mpro->__kisxzbon |= 0x40; } // interpolate
  if(((value >> 21) & 1) == 0) { mpro->__kisxzbon |= 0x20; } // saturate
  mpro->__kisxzbon |= (value >> 43) & 0x10; // XSEL
  mpro->__kisxzbon |= (value >> 14) & 0x0E; // ZERO, BSEL, NOFL
  mpro->__kisxzbon |= (value >> 7) & 1; // NXADR
  mpro->m_wrAFyyYh  = (value >> 23) & 0xC0; // MWT, MRD
  mpro->m_wrAFyyYh |= (value >> 18) & 0x32; // ADRL, FRCL, YRL
  mpro->m_wrAFyyYh |= (value >> 43) & 0x0C; // YSEL
  mpro->m_wrAFyyYh |= ((value >> 20) & 1) ^ ((value >> 21) & 1); // shift left by
  mpro->i_00rrrrrr = (value >> 39) & 0x3F; // IRA
  mpro->i_0T0wwwww = (value >> 33) & 0x1F; // IWA
  mpro->i_0T0wwwww |= ((value >> 32) & 0x40) ^ 0x40; // !IWT
  mpro->e_000Twwww = ((value >> 24) & 0x1F) ^ 0x10; // !EWT, EWA
  mpro->m_00aaaaaa = (value >> 9) & 0x3F; // MASA
}

static uint64 mpro_aica_read(struct MPRO *mpro) {
  uint64 value = 0;
  value |= ((uint64)(mpro->t_0rrrrrrr       )) << 57; // TRA
  value |= ((uint64)(mpro->t_Twwwwwww ^ 0x80)) << 49; // !TWT, TWA
  value |= ((uint64)(mpro->tablemask  &    1)) << 31; // TABLE
  value |= ((uint64)(mpro->adrmask    &    1)) <<  8; // ADREB
  value |= ((uint64)(mpro->negb       &    1)) << 18; // NEGB
  { uint64 sh = mpro->m_wrAFyyYh & 1;
    if((mpro->__kisxzbon & 0x20) == 0) { sh ^= 3; }
    value |= (sh << 20); // SHFT
  }
  value |= ((uint64)(mpro->__kisxzbon & 0x10)) << 43; // XSEL
  value |= ((uint64)(mpro->__kisxzbon & 0x0E)) << 14; // ZERO, BSEL, NOFL
  value |= ((uint64)(mpro->__kisxzbon & 0x01)) <<  7; // NXADR
  value |= ((uint64)(mpro->m_wrAFyyYh & 0xC0)) << 23; // MWT, MRD
  value |= ((uint64)(mpro->m_wrAFyyYh & 0x32)) << 18; // ADRL, FRCL, YRL
  value |= ((uint64)(mpro->m_wrAFyyYh & 0x0C)) << 43; // YSEL
  value |= ((uint64)(mpro->i_00rrrrrr & 0x3F)) << 39; // IRA
  value |= ((uint64)(mpro->i_0T0wwwww & 0x1F)) << 33; // IWA
  value |= ((uint64)((mpro->i_0T0wwwww & 0x40) ^ 0x40)) << 32; // !IWT
  value |= ((uint64)((mpro->e_000Twwww & 0x1F) ^ 0x10)) << 24; // !EWT, EWA
  value |= ((uint64)(mpro->m_00aaaaaa & 0x3F)) <<  9; // MASA
  return value;
}

#define DYNACODE_MAX_SIZE (0x6000)
#define DYNACODE_SLOP_SIZE (0x80)

struct YAM_STATE {
  //
  // Misc.
  //
  uint32 version;
  void *ram_ptr; // EXTERNALLY-REGISTERED pointer
  uint32 ram_mask;
  sint16 *out_buf; // EXTERNALLY-REGISTERED pointer
  uint32 out_pending;
  uint32 odometer;
  uint8 dry_out_enabled;
  uint8 dsp_emulation_enabled;
#ifdef ENABLE_DYNAREC
  uint8 dsp_dyna_enabled;
  uint8 dsp_dyna_valid;
#endif
  uint32 randseed;
  uint32 mem_word_address_xor;
  uint32 mem_byte_address_xor;
  //
  // Common regs
  //
  uint8 efsdl[18];
  uint8 efpan[18];
  uint8 mono;
  uint8 mvol;
  uint32 rbp;
  uint8 rbl;
  uint8 afsel;
  uint8 mslc;
  uint8 mrwinh;
  uint8 tctl[3], tim[3];
  uint16 mcieb, mcipd;
  uint16 scieb, scipd;
  uint8 scilv0, scilv1, scilv2;
  uint8 inton, intreq;
  uint32 rtc;
  //
  // DSP regs
  //
  sint16 coef[128]; // stored as 13-bit
  uint16 madrs[64];
  struct MPRO mpro[128];
  sint32 temp[128];
  // INPUTS all pre-promoted to 24-bit
  // 0x00-0x1F: MEMS
  // 0x20-0x2F: MIXS
  // 0x30-0x31: EXTS
  // 0x32-0x3F: always zero
  // 0x40-0x5F: slop area to handle IWTA
  sint32 inputs[0x60];
  // EFREG
  // 0x00-0x0F: EFREG
  // 0x10-0x1F: slop area to handle EWTA
  sint16 efreg[0x20];
  uint32 mdec_ct;
  uint32 adrs_reg;

  sint32 xzbchoice[5];
#define XZBCHOICE_TEMP     (0)
#define XZBCHOICE_ACC      (1)
#define XZBCHOICE_ZERO     (2)
#define XZBCHOICE_ZERO_ACC (3)
#define XZBCHOICE_INPUTS   (4)

  sint32 yychoice[4];
#define YYCHOICE_FRC_REG (0)
#define YYCHOICE_COEF    (1)
#define YYCHOICE_Y_REG_H (2)
#define YYCHOICE_Y_REG_L (3)

  sint32 mem_in_data[4];

  // SCSP modulation data
  sint16 ringbuf[32*RINGMAX];
  uint32 bufptr;
  // DMA registers
  uint32 dmea;
  uint16 drga;
  uint16 dtlg;
  //
  // Channel regs
  //
  struct YAM_CHAN chan[64];
  //
  // Buffer for dynarec code
  //
#ifdef ENABLE_DYNAREC
  uint8 dynacode[DYNACODE_MAX_SIZE];
#endif
};

//
// Get size
//
uint32 EMU_CALL yam_get_state_size(uint8 version) {
  return sizeof(struct YAM_STATE);
}

//
// Initialize DSP state
//
void EMU_CALL yam_clear_state(void *state, uint8 version) {
  int i;
  if(version != 2) { version = 1; }
  // Clear to zero
  memset(state, 0, sizeof(struct YAM_STATE));
  // Set version
  YAMSTATE->version = version;
  // Clear channel regs
  for(i = 0; i < 64; i++) {
    YAMSTATE->chan[i].EG.state = RELEASE;
    YAMSTATE->chan[i].lpfstate = 3;
    YAMSTATE->chan[i].eghold = 0;
    YAMSTATE->chan[i].lpflevel = 0x1FFF;
    // no lowpass on the SCSP
    if(version == 1) { YAMSTATE->chan[i].lpoff = 1; }
  }
  // Initialize MPRO
  for(i = 0; i < 128; i++) {
    switch(version) {
    case 1:
      mpro_scsp_write((YAMSTATE->mpro) + i, 0);
      break;
    case 2:
      YAMSTATE->mpro[i].c_0rrrrrrr = i;
      mpro_aica_write((YAMSTATE->mpro) + i, 0);
      break;
    }
  }
  // Enable dry output
  YAMSTATE->dry_out_enabled = 1;

  // Enable DSP emulation by default
  YAMSTATE->dsp_emulation_enabled = 1;

  // Enable DSP dynarec
#ifdef ENABLE_DYNAREC
  YAMSTATE->dsp_dyna_enabled = 1;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//
// Ugly hack to log debug output
//
/*
FILE *logfile = NULL;

static void logf(const char *fmt, ...) {
    va_list a;
    va_start(a, fmt);
  if(!logfile) {
    logfile=fopen("C:\\Corlett\\yam.log","wb");
  }
  if(logfile) { vfprintf(logfile, fmt, a); fflush(logfile); }
}

static void logupdate(struct YAM_STATE *state) {
  int i;
  logf("log update\n");
//  for(i=0;i<10;i++) {
//    logf("mpro %d: %08X %08X\n",i,state->mpro32[2*i+0],state->mpro32[2*i+1]);
//  }
  for(i = 0; i < 64; i++) {
    logf("chan %d: dsp %X level %X\n",i,state->chan[i].dspchan,state->chan[i].dsplevel);
  }
  for(i = 0; i < 16; i++) {
    logf("efsdl %d: %X pan %X\n",i,state->efsdl[i],state->efpan[i]);
  }
}

static void logstep(struct YAM_STATE *state, uint32 odometer) {
  static uint32 lastodometer = 0;
  if((odometer/50000) > (lastodometer/50000)) {
    lastodometer = odometer;
    logupdate(state);
  }
}

static int st = 0;

static void dumpch(struct YAM_STATE *state, struct YAM_CHAN *chan) {
  logf("st=%u (%us) envstate=%X level=%X\n",st,st/44100,chan->envstate,chan->envlevel);
  logf("  playpos=%X ls=%X le=%X\n",chan->playpos,chan->loopstart,chan->loopend);
  logf("  sample=%X tl=%X oct=%X fns=%X\n",chan->sampleaddr, chan->tl,chan->oct,chan->fns);
  logf("  rbp=%X rbl=%X\n",state->rbp,state->rbl);
}
*/
/////////////////////////////////////////////////////////////////////////////
//
// Set RAM pointer and size (must be a power of 2)
//
void EMU_CALL yam_setram(void *state, uint32 *ram, uint32 size, uint8 mbx, uint8 mwx) {
  YAMSTATE->ram_ptr = ram;
  if((size & (size-1)) == 0) {
    YAMSTATE->ram_mask = size-1;
  } else {
    YAMSTATE->ram_mask = 0;
  }
  YAMSTATE->mem_byte_address_xor = mbx;
  YAMSTATE->mem_word_address_xor = mwx;
  //
  // Invalidate dynarec code
  //
#ifdef ENABLE_DYNAREC
  YAMSTATE->dsp_dyna_valid = 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//
// Set output buffer pointer and begin new execution run
//
void EMU_CALL yam_beginbuffer(void *state, sint16 *buf) {
  YAMSTATE->out_buf = buf;
  YAMSTATE->out_pending = 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Enable or disable various things
//
void EMU_CALL yam_enable_dry(void *state, uint8 enable) {
  YAMSTATE->dry_out_enabled = (enable != 0);
}

void EMU_CALL yam_enable_dsp(void *state, uint8 enable) {
  YAMSTATE->dsp_emulation_enabled = (enable != 0);
#ifdef ENABLE_DYNAREC
  if(enable == 0) { YAMSTATE->dsp_dyna_valid = 0; }
#endif
}

void EMU_CALL yam_enable_dsp_dynarec(void *state, uint8 enable) {
#ifdef ENABLE_DYNAREC
  YAMSTATE->dsp_dyna_enabled = (enable != 0);
  if(enable == 0) { YAMSTATE->dsp_dyna_valid = 0; }
#endif
}

/////////////////////////////////////////////////////////////////////////////
//
// Timers / interrupts
//

//
// Recompute intreq
//
static void sci_recompute(struct YAM_STATE *state) {
  int i;
  uint16 scipd = (state->scipd) & (state->scieb);
  state->inton = 0;
  for(i = 10; i >= 0; i--) {
    if(((scipd) >> i) & 1) {
      if(i > 7) i = 7;
      state->intreq =
        ((((state->scilv0) >> i) & 1) << 0) |
        ((((state->scilv1) >> i) & 1) << 1) |
        ((((state->scilv2) >> i) & 1) << 2);
      state->inton = state->intreq;
      return;
    }
  }
}

//
// Signal an interrupt
//
static void sci_signal(struct YAM_STATE *state, int n) {
  state->scipd |= (1 << n);
  if(!(state->inton)) {
    sci_recompute(state);
  }
}

uint8* EMU_CALL yam_get_interrupt_pending_ptr(void *state) {
  return &(YAMSTATE->inton);
}

//
// Determine how many samples until the next interrupt
//
uint32 EMU_CALL yam_get_min_samples_until_interrupt(void *state) {
  uint32 min = 0xFFFFFFFF;
  uint32 t, samples;

//return 1;

  for(t = 0; t < 3; t++) {
    if(YAMSTATE->scieb & (1 << (INT_TIMER_A + t))) {
      samples = 0x100-((uint32)(YAMSTATE->tim[t]));
      samples <<= YAMSTATE->tctl[t];
      samples -= (YAMSTATE->odometer) & ((1<<YAMSTATE->tctl[t])-1);
      if(samples < min) { min = samples; }
    }
  }
//printf("yam min: ta=%X %02X tb=%X %02X tc=%X %02X min=%u\n",YAMSTATE->tctl[0],YAMSTATE->tim[0],YAMSTATE->tctl[1],YAMSTATE->tim[1],YAMSTATE->tctl[2],YAMSTATE->tim[2],min);
// min should never be 1 if the above is correct
//  if(min < 1) { min = 1; }
  return min;
}

/////////////////////////////////////////////////////////////////////////////
//
// Advance timers and interrupts
//
void EMU_CALL yam_advance(void *state, uint32 samples) {
  uint32 t;
//printf("yam_advance(%u)",samples);
  for(t = 0; t < 3; t++) {
    uint8 scale = YAMSTATE->tctl[t];
    uint32 whole = YAMSTATE->tim[t];
    uint32 frac = (YAMSTATE->odometer) & ((1<<scale)-1);
    uint32 remain = ((0x100 - whole) << scale) - frac;
    if(samples >= remain) { sci_signal(state, INT_TIMER_A + t); }
    YAMSTATE->tim[t] = ((frac + samples + (whole << scale)) >> scale) & 0xFF;
  }
  YAMSTATE->out_pending += samples;
  YAMSTATE->odometer += samples;
}

/////////////////////////////////////////////////////////////////////////////
//
// Envelope calculation
//
static int Get_AR(int base,int R)
{
  int Rate=base+(R<<1);
  if(Rate>63) Rate=63;
  if(Rate<0) Rate=0;
  return ARTABLE[Rate];
}

static int Get_DR(int base,int R)
{
  int Rate=base+(R<<1);
  if(Rate>63) Rate=63;
  if(Rate<0) Rate=0;
  return DRTABLE[Rate];
}

static int Get_RR(int base,int R)
{
  int Rate=base+(R<<1);
  if(Rate>63) Rate=63;
  if(Rate<0) Rate=0;
  return DRTABLE[Rate];
}

static void Compute_EG(struct YAM_CHAN *chan)
{
  int octave=chan->oct;
  int rate;
  if(octave&8) octave=octave-16;
  if(chan->krs!=0xf)
    rate=octave+2*chan->krs+((chan->fns>>9)&1);
  else
    rate=0; //rate=((FNS(slot)>>9)&1);

  chan->EG.volume=0x17F<<EG_SHIFT;
  chan->EG.AR=Get_AR(rate,chan->ar[0]);
  chan->EG.D1R=Get_DR(rate,chan->ar[1]);
  chan->EG.D2R=Get_DR(rate,chan->ar[2]);
  chan->EG.RR=Get_RR(rate,chan->ar[3]);
  chan->EG.DL=0x1f-chan->dl;
  chan->EG.EGHOLD=chan->eghold;
}

static void keyoff(struct YAM_CHAN *chan, int keyoff);

static int EG_Update(struct YAM_CHAN *chan)
{
  switch(chan->EG.state) {
    case ATTACK:
      chan->EG.volume+=chan->EG.AR;
      if(chan->EG.volume>=(0x3ff<<EG_SHIFT)) {
        if (!chan->link) {
          chan->EG.state=DECAY1;
          if(chan->EG.D1R>=(1024<<EG_SHIFT)) //Skip DECAY1, go directly to DECAY2
            chan->EG.state=DECAY2;
        }
        chan->EG.volume=0x3ff<<EG_SHIFT;
      }
      if(chan->EG.EGHOLD)
#if SHIFT >= 10
        return 0x3ff<<(SHIFT-10);
#else
        return 0x3ff>>(10-SHIFT);
#endif
      break;
    case DECAY1:
      chan->EG.volume-=chan->EG.D1R;
      if(chan->EG.volume<=0)
        chan->EG.volume=0;
      if(chan->EG.volume>>(EG_SHIFT+5)<=chan->EG.DL)
        chan->EG.state=DECAY2;
      break;
    case DECAY2:
      if(chan->ar[2]==0)
#if SHIFT >= 10
        return (chan->EG.volume>>EG_SHIFT)<<(SHIFT-10);
#else
        return (chan->EG.volume>>EG_SHIFT)>>(10-SHIFT);
#endif
      chan->EG.volume-=chan->EG.D2R;
      if(chan->EG.volume<=0)
        chan->EG.volume=0;
      break;
    case RELEASE:
      chan->EG.volume-=chan->EG.RR;
      if(chan->EG.volume<=0) {
        chan->EG.volume=0;
        keyoff(chan, 0);
      }
      break;
    default:
      return 1<<SHIFT;
  }
#if SHIFT >= 10
  return (chan->EG.volume>>EG_SHIFT)<<(SHIFT-10);
#else
  return (chan->EG.volume>>EG_SHIFT)>>(10-SHIFT);
#endif
}

static uint32 AICA_Step(struct YAM_CHAN *chan)
{
  int octave=chan->oct;
  uint32 Fn;

  Fn=(FNS_Table[chan->fns]);  //24.8
  if(octave&8)
    Fn>>=(16-octave);
  else
    Fn<<=octave;

  return Fn/(44100);
}

static signed int PLFO_Step(struct _LFO *LFO)
{
  int p;
  LFO->phase+=LFO->phase_step;    
#if LFO_SHIFT!=8    
  LFO->phase&=(1<<(LFO_SHIFT+8))-1;
#endif    
  p=LFO->table[LFO->phase>>LFO_SHIFT];
  p=LFO->scale[p+128];
  return p<<(SHIFT-LFO_SHIFT);
}

static signed int ALFO_Step(struct _LFO *LFO)
{
  int p;
  LFO->phase+=LFO->phase_step;    
#if LFO_SHIFT!=8    
  LFO->phase&=(1<<(LFO_SHIFT+8))-1;
#endif    
  p=LFO->table[LFO->phase>>LFO_SHIFT];
  p=LFO->scale[p];
  return p<<(SHIFT-LFO_SHIFT);
}

static void LFO_ComputeStep(struct _LFO *LFO,sint32 LFOF,sint32 LFOWS,sint32 LFOS,int ALFO)
{
  float step=(float) LFOFreq[LFOF]*256.0/(float)44100.0;
  LFO->phase_step=(unsigned int) ((float) (1<<LFO_SHIFT)*step);
  if(ALFO) {
    switch(LFOWS)
    {
    case 0: LFO->table=ALFO_SAW; break;
    case 1: LFO->table=ALFO_SQR; break;
    case 2: LFO->table=ALFO_TRI; break;
    case 3: LFO->table=ALFO_NOI; break;
    }
    LFO->scale=ASCALES[LFOS];
  }
  else {
    switch(LFOWS)
    {
    case 0: LFO->table=PLFO_SAW; break;
    case 1: LFO->table=PLFO_SQR; break;
    case 2: LFO->table=PLFO_TRI; break;
    case 3: LFO->table=PLFO_NOI; break;
    }
    LFO->scale=PSCALES[LFOS];
  }
}

static void Compute_LFO(struct YAM_CHAN *chan)
{
  if(chan->plfos!=0)
    LFO_ComputeStep(&(chan->PLFO),chan->lfof,chan->plfows,chan->plfos,0);
  if(chan->alfos!=0)
    LFO_ComputeStep(&(chan->ALFO),chan->lfof,chan->alfows,chan->alfos,1);
}

#define ADPCMSHIFT  8
#define ADFIX(f)    (int) ((float) f*(float) (1<<ADPCMSHIFT))

const int TableQuant[8]={ADFIX(0.8984375),ADFIX(0.8984375),ADFIX(0.8984375),ADFIX(0.8984375),ADFIX(1.19921875),ADFIX(1.59765625),ADFIX(2.0),ADFIX(2.3984375)};
const int quant_mul[16]= { 1, 3, 5, 7, 9, 11, 13, 15, -1, -3, -5, -7, -9, -11, -13, -15};

void InitADPCM(int *PrevSignal, int *PrevQuant)
{
  *PrevSignal=0;
  *PrevQuant=0x7f;
}

#define ICLIP16(x) ((((x)+0x8000)&0xFFFF0000)?(((x)>>31)^0x7FFF):(x))

static signed short DecodeADPCM(int *PrevSignal, unsigned char Delta, int *PrevQuant)
{
  int x = *PrevQuant * quant_mul [Delta & 15];
  x = *PrevSignal + ((int)(x + ((uint32)x >> 29)) >> 3);
  *PrevSignal=ICLIP16(x);
  *PrevQuant=(*PrevQuant*TableQuant[Delta&7])>>ADPCMSHIFT;
  *PrevQuant=(*PrevQuant<0x7f)?0x7f:((*PrevQuant>0x6000)?0x6000:*PrevQuant);
  return *PrevSignal;
}

/////////////////////////////////////////////////////////////////////////////
//
// Key on/off
//
static void keyon(struct YAM_STATE *state, struct YAM_CHAN *chan) {
//printf("keyon %08X\n",chan);
  // Ignore redundant key-ons
  if(chan->EG.state != RELEASE) return;
  chan->cur_addr = 0;
  chan->nxt_addr = 1 << SHIFT;
  chan->Backwards=0;
  chan->step=AICA_Step(chan);
  chan->lpflevel = chan->flv[0];
  chan->lpfstate = 0;
  chan->samplebufcur = 0;
  chan->samplebufnext = 0;
  Compute_EG(chan);
  chan->EG.state=ATTACK;
  chan->EG.volume=0x17F<<EG_SHIFT;
  Compute_LFO(chan);
//printf("keyon %08X passed\n",chan);
  if (chan->pcms >= 2)
  {
    uint8 *base;
    uint32 adbase;
    uint32 curstep, steps_to_go;

    chan->curstep = 0;
    chan->adbase = chan->sampleaddr & state->ram_mask;
    InitADPCM(&(chan->cur_sample), &(chan->cur_quant));
    InitADPCM(&(chan->cur_lpsample), &(chan->cur_lpquant));

    // walk to the ADPCM state at LSA
    curstep = 0;
    base = ((uint8*)state->ram_ptr);
    adbase = chan->adbase;
    steps_to_go = chan->loopstart;

    while (curstep < steps_to_go) {
      int shift1, delta1;
      shift1 = 4*((curstep&1));
      delta1 = (base[adbase]>>shift1)&0xf;
      DecodeADPCM(&(chan->cur_lpsample),delta1,&(chan->cur_lpquant));
      curstep++;
      if (!(curstep & 1)) {
        adbase = (adbase + 1) & state->ram_mask;
      }
    }

    chan->cur_lpstep = curstep;
    chan->adlpbase = adbase;

    // on real hardware this creates undefined behavior.
    if (chan->loopstart > chan->loopend) {
      chan->loopend = 0xFFFF;
    }
  }
}

static void keyoff(struct YAM_CHAN *chan,int keyoff) {
  chan->EG.state = RELEASE;
  chan->lpfstate = 3;
  if(!keyoff) chan->EG.volume = 0;
  chan->kyonb = 0;
}

/////////////////////////////////////////////////////////////////////////////
//
// Channel registers
//
static uint32 chan_scsp_load_reg(struct YAM_STATE *state, uint8 ch, uint8 a) {
  struct YAM_CHAN *chan = state->chan + (((uint32)ch) & 0x1F);
  uint16 d = 0;
  // don't really need a flush for loading chan regs
  switch(a & 0x1E) {
  case 0x00: // PlayControl
    d  = (((uint32)(chan->kyonb           )) & 0x0001) << 11;
    d |= (((uint32)(chan->sampler_invert  )) & 0xC000) >>  5;
    d |= (((uint32)(chan->ssctl           )) & 0x0003) <<  7;
    d |= (((uint32)(chan->sampler_looptype)) & 0x0003) <<  5;
    d |= (((uint32)(chan->pcms            )) & 0x0001) <<  4;
    d |= ((chan->sampleaddr) >> 16) & 0xF;
    break;
  case 0x02: // SampleAddrLow
    d = chan->sampleaddr;
    break;
  case 0x04: // LoopStart
    d = chan->loopstart;
    break;
  case 0x06: // LoopEnd
    d = chan->loopend;
    break;
  case 0x08: // AmpEnv1
    d  =  (((uint32)(chan->ar[2]          )) & 0x001F) << 11;
    d |=  (((uint32)(chan->ar[1]          )) & 0x001F) <<  6;
    d |=  (((uint32)(chan->eghold         )) & 1)      <<  5;
    d |=  (((uint32)(chan->ar[0]          )) & 0x001F) <<  0;
    break;
  case 0x0A: // AmpEnv2
    d  = (((uint32)(chan->link     )) & 0x0001) << 14;
    d |= (((uint32)(chan->krs      )) & 0x000F) << 10;
    d |= (((uint32)(chan->dl       )) & 0x001F) <<  5;
    d |= (((uint32)(chan->ar[3]    )) & 0x001F) <<  0;
    break;
  case 0x0C: // TotalLevel
    d  = (((uint32)(chan->stwinh   )) & 0x0001) <<  9;
    d |= (((uint32)(chan->voff     )) & 0x0001) <<  8;
    d |= (((uint32)(chan->tl       )) & 0x00FF) <<  0;
    break;
  case 0x0E: // Modulation
    d  = (((uint32)(chan->mdl      )) & 0x000F) << 12;
    d |= (((uint32)(chan->mdxsl    )) & 0x003F) <<  6;
    d |= (((uint32)(chan->mdysl    )) & 0x003F) <<  0;
    break;
  case 0x10: // SampleRatePitch
    d  = (((uint32)(chan->oct      )) & 0x000F) << 11;
    d |= (((uint32)(chan->fns      )) & 0x07FF) <<  0;
    break;
  case 0x12: // LFOControl
    d  = (((uint32)(chan->lfore    )) & 0x0001) << 15;
    d |= (((uint32)(chan->lfof     )) & 0x001F) << 10;
    d |= (((uint32)(chan->plfows   )) & 0x0003) <<  8;
    d |= (((uint32)(chan->plfos    )) & 0x0007) <<  5;
    d |= (((uint32)(chan->alfows   )) & 0x0003) <<  3;
    d |= (((uint32)(chan->alfos    )) & 0x0007) <<  0;
    break;
  case 0x14: // DSPInputSelect
    d  = (((uint32)(chan->dspchan  )) & 0x000F) <<  3;
    d |= (((uint32)(chan->dsplevel )) & 0x0007) >>  0;
    break;
  case 0x16: // SendLevels
    d  = (((uint32)(chan->disdl    )) & 0x0007) << 13;
    d |= (((uint32)(chan->dipan    )) & 0x001F) <<  8;
    if(ch < 18) {
      d |= (((uint32)(state->efsdl[ch])) & 0x07) << 5;
      d |= (((uint32)(state->efpan[ch])) & 0x1F) << 0;
    }
    break;
  }
  return d;
}

static void chan_scsp_store_reg(struct YAM_STATE *state, uint8 ch, uint8 a, uint32 d, uint32 mask) {
  struct YAM_CHAN *chan;
  a &= 0x1E;
  if(a >= 0x18) return;
  yam_flush(YAMSTATE);
  chan = state->chan + (((uint32)ch) & 0x1F);
  switch(a & 0x1E) {
  case 0x00: // PlayControl
    if(mask & 0x00FF) {
      chan->sampleaddr &= 0xFFFF;
      chan->sampleaddr |= (((uint32)d) & 0xF) << 16;
      chan->pcms = (d >> 4) & 1;
      chan->sampler_looptype = (d >> 5) & 3;
      chan->ssctl &= 2;
      chan->ssctl |= (d >> 7) & 1;
    }
    if(mask & 0xFF00) {
      chan->ssctl &= 1;
      chan->ssctl |= (d >> 7) & 2;
      chan->sampler_invert = 0;
      if(d & (1<< 9)) chan->sampler_invert |= 0x00007FFF;
      if(d & (1<<10)) chan->sampler_invert |= 0xFFFF8000;
      chan->kyonb = (d >> 11) & 1;
      if(d & 0x1000) { // kyonex
        int ch;
//for(ch=0;ch<32;ch++){printf("%d",state->chan[ch].envstate);}printf("\n");
        for(ch = 0; ch < 32; ch++) {
          if(state->chan[ch].kyonb) {
//printf("*");
            keyon(state, state->chan + ch);
          } else { 
//printf(".");
            keyoff(state->chan + ch, 1);
          }
        }
//printf("\n");
//for(ch=0;ch<32;ch++){printf("%d",state->chan[ch].envstate);}printf("\n");
      }
    }
    break;
  case 0x02: // SampleAddrLow
    chan->sampleaddr &= (0xFFFFF ^ mask);
    chan->sampleaddr |= (d & mask);
    break;
  case 0x04: // LoopStart
    chan->loopstart &= (0xFFFF ^ mask);
    chan->loopstart |= (d & mask);
    break;
  case 0x06: // LoopEnd
    chan->loopend &= (0xFFFF ^ mask);
    chan->loopend |= (d & mask);
    break;
  case 0x08: // AmpEnv1
    if(mask & 0x00FF) {
      chan->ar[0] = d & 0x1F;
      chan->eghold = !!(d & (1<<5));
//      chan->envlevelmask[0] = 0x1FFF;
      chan->ar[1] &= 0x1C;
      chan->ar[1] |= (d >> 6) & 0x03;
    }
    if(mask & 0xFF00) {
      chan->ar[1] &= 0x03;
      chan->ar[1] |= (d >> 6) & 0x1C;
      chan->ar[2] = (d >> 11) & 0x1F;
    }
    break;
  case 0x0A: // AmpEnv2
    if(mask & 0x00FF) {
      chan->ar[3] = d & 0x1F;
      chan->dl &= 0x18;
      chan->dl |= (d >> 5) & 0x07;
    }
    if(mask & 0xFF00) {
      chan->dl &= 0x07;
      chan->dl |= (d >> 5) & 0x18;
      chan->krs = (d >> 10) & 0xF;
      chan->link = (d >> 14) & 1;
    }
    break;
  case 0x0C: // TotalLevel
    if(mask & 0x00FF) {
      chan->tl = d & 0xFF;
    }
    if(mask & 0xFF00) {
      chan->voff = (d >> 8) & 1;
      chan->stwinh = (d >> 9) & 1;
    }
    break;
  case 0x0E: // Modulation
    if(mask & 0x00FF) {
      chan->mdysl = d & 0x3F;
      chan->mdxsl &= 0x3C;
      chan->mdxsl |= (d >> 6) & 0x03;
    }
    if(mask & 0xFF00) {
      chan->mdxsl &= 0x03;
      chan->mdxsl |= (d >> 6) & 0x3C;
      chan->mdl = (d >> 12) & 0xF;
    }
    break;
  case 0x10: // SampleRatePitch
    if(mask & 0x00FF) {
      chan->fns &= 0x700;
      chan->fns |= d & 0x0FF;
    }
    if(mask & 0xFF00) {
      chan->fns &= 0x0FF;
      chan->fns |= d & 0x700;
      chan->oct = (d >> 11) & 0xF;
    }
    chan->step = AICA_Step(chan);
    break;
  case 0x12: // LFOControl
    if(mask & 0x00FF) {
      chan->alfos  = d & 7;
      chan->alfows = (d >> 3) & 3;
      chan->plfos  = (d >> 5) & 7;
    }
    if(mask & 0xFF00) {
      chan->plfows = (d >> 8) & 3;
      chan->lfof   = (d >> 10) & 0x1F;
      chan->lfore  = (d >> 15) & 1;
    }
    Compute_LFO(chan);
    break;
  case 0x14: // DSPInputSelect
    if(mask & 0x00FF) {
      chan->dsplevel = d & 0x7;
      chan->dspchan = (d >> 3) & 0xF;
    }
    break;
  case 0x16: // SendLevels
    if(mask & 0x00FF) {
      if(ch < 18) {
        state->efpan[ch] = d & 0x1F;
        state->efsdl[ch] = (d >> 5) & 0x7;
      }
    }
    if(mask & 0xFF00) {
      chan->dipan = (d >> 8) & 0x1F;
      chan->disdl = (d >> 13) & 0x7;
    }
    break;
  }
}

static uint32 chan_aica_load_reg(struct YAM_STATE *state, uint8 ch, uint8 a) {
  struct YAM_CHAN *chan = state->chan + (((uint32)ch) & 0x3F);
  uint16 d = 0;
  // don't really need a flush for loading chan regs
  switch(a & 0x7C) {
  case 0x00: // PlayControl
    d  = (((uint32)(chan->kyonb           )) & 0x0001) << 14;
    d |= (((uint32)(chan->ssctl           )) & 0x0001) << 10;
    d |= (((uint32)(chan->sampler_looptype)) & 0x0001) <<  9;
    d |= (((uint32)(chan->pcms            )) & 0x0003) <<  7;
    d |= ((chan->sampleaddr) >> 16) & 0x7F;
    break;
  case 0x04: // SampleAddrLow
    d = chan->sampleaddr;
    break;
  case 0x08: // LoopStart
    d = chan->loopstart;
    break;
  case 0x0C: // LoopEnd
    d = chan->loopend;
    break;
  case 0x10: // AmpEnv1
    d  = (((uint32)(chan->ar[2]    )) & 0x001F) << 11;
    d |= (((uint32)(chan->ar[1]    )) & 0x001F) <<  6;
    d |= (((uint32)(chan->ar[0]    )) & 0x001F) <<  0;
    break;
  case 0x14: // AmpEnv2
    d  = (((uint32)(chan->link     )) & 0x0001) << 14;
    d |= (((uint32)(chan->krs      )) & 0x000F) << 10;
    d |= (((uint32)(chan->dl       )) & 0x001F) <<  5;
    d |= (((uint32)(chan->ar[3]    )) & 0x001F) <<  0;
    break;
  case 0x18: // SampleRatePitch
    d  = (((uint32)(chan->oct      )) & 0x000F) << 11;
    d |= (((uint32)(chan->fns      )) & 0x07FF) <<  0;
    break;
  case 0x1C: // LFOControl
    d  = (((uint32)(chan->lfore    )) & 0x0001) << 15;
    d |= (((uint32)(chan->lfof     )) & 0x001F) << 10;
    d |= (((uint32)(chan->plfows   )) & 0x0003) <<  8;
    d |= (((uint32)(chan->plfos    )) & 0x0007) <<  5;
    d |= (((uint32)(chan->alfows   )) & 0x0003) <<  3;
    d |= (((uint32)(chan->alfos    )) & 0x0007) <<  0;
    break;
  case 0x20: // DSPChannelSend
    d  = (((uint32)(chan->dsplevel )) & 0x000F) <<  4;
    d |= (((uint32)(chan->dspchan  )) & 0x000F) <<  0;
    break;
  case 0x24: // DirectPanVolSend
    d  = (((uint32)(chan->disdl    )) & 0x000F) <<  8;
    d |= (((uint32)(chan->dipan    )) & 0x001F) <<  0;
    break;
  case 0x28: // LPF1Volume
    d  = (((uint32)(chan->tl       )) & 0x00FF) <<  8;
    d |= (((uint32)(chan->voff     )) & 0x0001) <<  6;
    d |= (((uint32)(chan->lpoff    )) & 0x0001) <<  5;
    d |= (((uint32)(chan->q        )) & 0x001F) <<  0;
    break;
  case 0x2C: // LPF2
    d = (chan->flv[0]) & 0x1FFF;
    break;
  case 0x30: // LPF3
    d = (chan->flv[1]) & 0x1FFF;
    break;
  case 0x34: // LPF4
    d = (chan->flv[2]) & 0x1FFF;
    break;
  case 0x38: // LPF5
    d = (chan->flv[3]) & 0x1FFF;
    break;
  case 0x3C: // LPF6
    d = (chan->flv[4]) & 0x1FFF;
    break;
  case 0x40: // LPF7
    d  = (((uint32)(chan->fr[0]     )) & 0x001F) <<  8;
    d |= (((uint32)(chan->fr[1]     )) & 0x001F) <<  0;
    break;
  case 0x44: // LPF8
    d  = (((uint32)(chan->fr[2]     )) & 0x001F) <<  8;
    d |= (((uint32)(chan->fr[3]     )) & 0x001F) <<  0;
    break;
  }
  return d;
}

static void chan_aica_store_reg(struct YAM_STATE *state, uint8 ch, uint8 a, uint32 d, uint32 mask) {
  struct YAM_CHAN *chan;
  a &= 0x7C;
  if(a >= 0x48) return;
  yam_flush(YAMSTATE);
  chan = state->chan + (((uint32)ch) & 0x3F);
  switch(a) {
  case 0x00: // PlayControl
    if(mask & 0x00FF) {
      chan->sampleaddr &= 0xFFFF;
      chan->sampleaddr |= (((uint32)d) & 0x7F) << 16;
      chan->pcms &= 2;
      chan->pcms |= (d >> 7) & 1;
    }
    if(mask & 0xFF00) {
      chan->pcms &= 1;
      chan->pcms |= (d >> 7) & 2;
      chan->sampler_looptype = (d >> 9) & 1;
      chan->ssctl = (d >> 10) & 1;
      chan->kyonb = (d >> 14) & 1;
      if(d & 0x8000) { // kyonex
        int ch;
        for(ch = 0; ch < 64; ch++) {
          if(state->chan[ch].kyonb) { keyon(state, state->chan + ch); }
          else { keyoff(state->chan + ch, 1); }
        }
      }
    }
    break;
  case 0x04: // SampleAddrLow
    chan->sampleaddr &= (0x7FFFFF ^ mask);
    chan->sampleaddr |= (d & mask);
    break;
  case 0x08: // LoopStart
    chan->loopstart &= (0xFFFF ^ mask);
    chan->loopstart |= (d & mask);
    break;
  case 0x0C: // LoopEnd
    chan->loopend &= (0xFFFF ^ mask);
    chan->loopend |= (d & mask);
    break;
  case 0x10: // AmpEnv1
    if(mask & 0x00FF) {
      chan->ar[0] = d & 0x1F;
      chan->ar[1] &= 0x1C;
      chan->ar[1] |= (d >> 6) & 0x03;
    }
    if(mask & 0xFF00) {
      chan->ar[1] &= 0x03;
      chan->ar[1] |= (d >> 6) & 0x1C;
      chan->ar[2] = (d >> 11) & 0x1F;
    }
    break;
  case 0x14: // AmpEnv2
    if(mask & 0x00FF) {
      chan->ar[3] = d & 0x1F;
      chan->dl &= 0x18;
      chan->dl |= (d >> 5) & 0x07;
    }
    if(mask & 0xFF00) {
      chan->dl &= 0x07;
      chan->dl |= (d >> 5) & 0x18;
      chan->krs = (d >> 10) & 0xF;
      chan->link = (d >> 14) & 1;
    }
    break;
  case 0x18: // SampleRatePitch
    if(mask & 0x00FF) {
      chan->fns &= 0x700;
      chan->fns |= d & 0x0FF;
    }
    if(mask & 0xFF00) {
      chan->fns &= 0x0FF;
      chan->fns |= d & 0x700;
      chan->oct = (d >> 11) & 0xF;
    }
    chan->step = AICA_Step(chan);
    break;
  case 0x1C: // LFOControl
    if(mask & 0x00FF) {
      chan->alfos  = d & 7;
      chan->alfows = (d >> 3) & 3;
      chan->plfos  = (d >> 5) & 7;
    }
    if(mask & 0xFF00) {
      chan->plfows = (d >> 8) & 3;
      chan->lfof   = (d >> 10) & 0x1F;
      chan->lfore  = (d >> 15) & 1;
    }
    Compute_LFO(chan);
    break;
  case 0x20: // DSPChannelSend
    if(mask & 0x00FF) {
      chan->dspchan  = d & 0xF;
      chan->dsplevel = (d >> 4) & 0xF;
    }
    break;
  case 0x24: // DirectPanVolSend
    if(mask & 0x00FF) { chan->dipan = d & 0x1F; }
    if(mask & 0xFF00) { chan->disdl = (d >> 8) & 0xF; }
    break;
  case 0x28: // LPF1Volume
    if(mask & 0x00FF) {
      chan->q = d & 0x1F;
      chan->lpoff = (d >> 5) & 1;
      chan->voff = (d >> 6) & 1;
    }
    if(mask & 0xFF00) {
      chan->tl = (d >> 8) & 0xFF;
    }
    break;
  case 0x2C: // LPF2
    chan->flv[0] = (((chan->flv[0]) & (0xFFFF ^ mask)) | (d & mask)) & 0x1FFF;
    break;
  case 0x30: // LPF3
    chan->flv[1] = (((chan->flv[1]) & (0xFFFF ^ mask)) | (d & mask)) & 0x1FFF;
    break;
  case 0x34: // LPF4
    chan->flv[2] = (((chan->flv[2]) & (0xFFFF ^ mask)) | (d & mask)) & 0x1FFF;
    break;
  case 0x38: // LPF5
    chan->flv[3] = (((chan->flv[3]) & (0xFFFF ^ mask)) | (d & mask)) & 0x1FFF;
    break;
  case 0x3C: // LPF6
    chan->flv[4] = (((chan->flv[4]) & (0xFFFF ^ mask)) | (d & mask)) & 0x1FFF;
    break;
  case 0x40: // LPF7
    if(mask & 0x00FF) { chan->fr[1] = (d >> 0) & 0x1F; }
    if(mask & 0xFF00) { chan->fr[0] = (d >> 8) & 0x1F; }
    break;
  case 0x44: // LPF8
    if(mask & 0x00FF) { chan->fr[3] = (d >> 0) & 0x1F; }
    if(mask & 0xFF00) { chan->fr[2] = (d >> 8) & 0x1F; }
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// DSP registers
//
static void coef_write(struct YAM_STATE *state, uint32 n, uint32 d, uint32 mask) {
#ifdef ENABLE_DYNAREC
  sint16 old = state->coef[n];
#endif
  yam_flush(state);
  n &= 0x7F;
  state->coef[n] <<= 3;
  state->coef[n] &= ~mask;
  state->coef[n] |= d & mask;
  state->coef[n] = ((sint16)(state->coef[n])) >> 3;
#ifdef ENABLE_DYNAREC
  if(old != state->coef[n]) { state->dsp_dyna_valid = 0; }
#endif
}

static void madrs_write(struct YAM_STATE *state, uint32 n, uint32 d, uint32 mask) {
#ifdef ENABLE_DYNAREC
  uint16 old = state->madrs[n];
#endif
  yam_flush(state);
  n &= 0x3F;
  state->madrs[n] &= ~mask;
  state->madrs[n] |= d & mask;
#ifdef ENABLE_DYNAREC
  if(old != state->madrs[n]) { state->dsp_dyna_valid = 0; }
#endif
}

static uint32 temp_read(struct YAM_STATE *state, uint32 n) {
  yam_flush(state);
  if((n & 1) == 0) { return ((state->temp[(n/2)&0x7F]) >> 0) & 0x00FF; }
  else             { return ((state->temp[(n/2)&0x7F]) >> 8) & 0xFFFF; }
}

static void temp_write(struct YAM_STATE *state, uint32 n, uint32 d, uint32 mask) {
  yam_flush(state);
  switch(n & 1) {
  case 0: mask &= 0x00FF; break;
  case 1: mask &= 0xFFFF; mask <<= 8; d <<= 8; break;
  }
  n /= 2; n &= 0x1F;
  state->temp[n] &= ~mask;
  state->temp[n] |= d & mask;
  // redo sign extension
  state->temp[n] <<= 8;
  state->temp[n] >>= 8;
}

static uint32 mems_read(struct YAM_STATE *state, uint32 n) {
  yam_flush(state);
  if((n & 1) == 0) { return ((state->inputs[(n/2)&0x1F]) >> 0) & 0x00FF; }
  else             { return ((state->inputs[(n/2)&0x1F]) >> 8) & 0xFFFF; }
}

static void mems_write(struct YAM_STATE *state, uint32 n, uint32 d, uint32 mask) {
  yam_flush(state);
  switch(n & 1) {
  case 0: mask &= 0x00FF; break;
  case 1: mask &= 0xFFFF; mask <<= 8; d <<= 8; break;
  }
  n /= 2; n &= 0x1F;
  state->inputs[n] &= ~mask;
  state->inputs[n] |= d & mask;
  // redo sign extension
  state->inputs[n] <<= 8;
  state->inputs[n] >>= 8;
}

static uint32 mixs_read(struct YAM_STATE *state, uint32 n) {
  yam_flush(state);
  // MIXS is pre-promoted to 24-bit here, so shift it right by 4
  if((n & 1) == 0) { return ((state->inputs[0x20+((n/2)&0xF)]) >> 4) & 0x000F; }
  else             { return ((state->inputs[0x20+((n/2)&0xF)]) >> 8) & 0xFFFF; }
}

static uint32 efreg_read(struct YAM_STATE *state, uint32 n) {
  yam_flush(state);
  return ((uint32)(state->efreg[n & 0xF])) & 0xFFFF;
}

static void efreg_write(struct YAM_STATE *state, uint32 n, uint32 d, uint32 mask) {
  yam_flush(state);
  state->efreg[n & 0xF] &= ~mask;
  state->efreg[n & 0xF] |= d & mask;
}

static uint32 exts_read(struct YAM_STATE *state, uint32 n) {
  yam_flush(state);
  return (state->inputs[0x30 + (n & 1)] >> 8) & 0xFFFF;
}

static void exts_write(struct YAM_STATE *state, uint32 n, uint32 d, uint32 mask) {
  yam_flush(state);
  state->inputs[0x30 + (n & 1)] >>= 8;
  state->inputs[0x30 + (n & 1)] &= ~mask;
  state->inputs[0x30 + (n & 1)] |= d & mask;
  state->inputs[0x30 + (n & 1)] <<= 16;
  state->inputs[0x30 + (n & 1)] >>= 8;
}

static uint32 dsp_scsp_load_reg(struct YAM_STATE *state, uint32 a) {
  a &= 0xFFE;
  if(a < 0x700) return 0;
  if(a < 0x780) return state->coef[(a/2) & 0x3F] << 3;
  if(a < 0x7C0) return state->madrs[(a/2) & 0x1F];
  if(a < 0x800) return 0;
  if(a < 0xC00) {
    uint8 shift = ((a&6)^6) * 8;
    uint32 index = ((a-0x800)/8)&0x7F;
    return (mpro_scsp_read(state->mpro + index) >> shift) & 0xFFFF;
  }
  if(a < 0xE00) return temp_read(state, (a/2) & 0xFF);
  if(a < 0xE80) return mems_read(state, (a/2) & 0x3F);
  if(a < 0xEC0) return mixs_read(state, (a/2) & 0x1F);
  if(a < 0xEE0) return efreg_read(state, (a/2) & 0xF);
  if(a < 0xEE4) return exts_read(state, (a/2) & 1);
  return 0;
}

static void dsp_scsp_store_reg(
  struct YAM_STATE *state,
  uint32 a, uint32 d, uint32 mask
) {
  a &= 0xFFE;
  if(a < 0x700) { return; }
  if(a < 0x780) { coef_write(state, (a/2) & 0x3F, d, mask); return; }
  if(a < 0x7C0) { madrs_write(state, (a/2) & 0x1F, d, mask); return; }
  if(a < 0x800) { return; }
  if(a < 0xC00) {
    uint8 shift64 = ((a&6)^6) * 8;
    uint32 index64 = ((a-0x800)/8)&0x7F;
    uint64 mask64sh = ((uint64)(mask & 0xFFFF)) << shift64;
    uint64 dm64sh = ((uint64)(d & mask & 0xFFFF)) << shift64;
    uint64 oldvalue = mpro_scsp_read(state->mpro + index64);
    uint64 newvalue = (oldvalue & (~mask64sh)) | dm64sh;
    if(newvalue != oldvalue) {
      yam_flush(state);
      mpro_scsp_write(state->mpro + index64, newvalue);
#ifdef ENABLE_DYNAREC
      state->dsp_dyna_valid = 0;
#endif
    }
    return;
  }
  if(a < 0xE00) { temp_write(state, (a/2) & 0xFF, d, mask); return; }
  if(a < 0xE80) { mems_write(state, (a/2) & 0x3F, d, mask); return; }
  // you can't write to MIXS, at least not meaningfully
  if(a < 0xEC0) { return; }
  if(a < 0xEE0) { efreg_write(state, (a/2) & 0xF, d, mask); return; }
  if(a < 0xEE4) { exts_write(state, (a/2) & 1, d, mask); return; }
}

static uint32 dsp_aica_load_reg(struct YAM_STATE *state, uint32 a) {
  a &= 0xFFFC;
  if(a < 0x3000) return 0;
  if(a < 0x3200) return state->coef[(a/4) & 0x7F] << 3;
  if(a < 0x3300) return state->madrs[(a/4) & 0x3F];
  if(a < 0x3400) return 0;
  if(a < 0x3C00) {
    uint8 shift64 = ((a&0xC)^0xC) * 4;
    uint32 index64 = ((a-0x3400)/16)&0x7F;
    return (mpro_aica_read(state->mpro + index64) >> shift64) & 0xFFFF;
  }
  if(a < 0x4000) return 0;
  if(a < 0x4400) return temp_read(state, (a/4) & 0xFF);
  if(a < 0x4500) return mems_read(state, (a/4) & 0x3F);
  if(a < 0x4580) return mixs_read(state, (a/4) & 0x1F);
  if(a < 0x45C0) return efreg_read(state, (a/4) & 0xF);
  if(a < 0x45C8) return exts_read(state, (a/4) & 1);
  return 0;
}

static void dsp_aica_store_reg(
  struct YAM_STATE *state,
  uint32 a, uint32 d, uint32 mask
) {
  a &= 0xFFFC;
  if(a < 0x3000) { return; }
  if(a < 0x3200) { coef_write(state, (a/4) & 0x7F, d, mask); return; }
  if(a < 0x3300) { madrs_write(state, (a/4) & 0x3F, d, mask); return; }
  if(a < 0x3400) { return; }
  if(a < 0x3C00) {
    uint8 shift64 = ((a&0xC)^0xC) * 4;
    uint32 index64 = ((a-0x3400)/16)&0x7F;
    uint64 mask64sh = ((uint64)(mask & 0xFFFF)) << shift64;
    uint64 dm64sh = ((uint64)(d & mask & 0xFFFF)) << shift64;
    uint64 oldvalue = mpro_aica_read(state->mpro + index64);
    uint64 newvalue = (oldvalue & (~mask64sh)) | dm64sh;
    if(newvalue != oldvalue) {
      yam_flush(state);
      mpro_aica_write(state->mpro + index64, newvalue);
#ifdef ENABLE_DYNAREC
      state->dsp_dyna_valid = 0;
#endif
    }
    return;
  }
  if(a < 0x4000) { return; }
  if(a < 0x4400) { temp_write(state, (a/4) & 0xFF, d, mask); return; }
  if(a < 0x4500) { mems_write(state, (a/4) & 0x3F, d, mask); return; }
  // you can't write to MIXS, at least not meaningfully
  if(a < 0x4580) { return; }
  if(a < 0x45C0) { efreg_write(state, (a/4) & 0xF, d, mask); return; }
  if(a < 0x45C8) { exts_write(state, (a/4) & 1, d, mask); return; }
}

/////////////////////////////////////////////////////////////////////////////
//
// Externally-accessible load/store register
//

#if defined(SCSP_LOG) && !defined(USE_STARSCREAM)
#include <stdio.h>
  extern FILE * scsp_log;
  extern unsigned char ** scsp_pc, ** scsp_basepc;
#endif

uint32 EMU_CALL yam_scsp_load_reg(void *state, uint32 a, uint32 mask) {
  uint32 d = 0;
  a &= 0xFFE;
  if(a <  0x400) return chan_scsp_load_reg(YAMSTATE, a>>5, a&0x1E) & mask;
  if(a >= 0x700) return dsp_scsp_load_reg(YAMSTATE, a) & mask;
  if(a >= 0x600) return YAMSTATE->ringbuf[(YAMSTATE->bufptr-64+(a-0x600)/2)&(32*RINGMAX-1)] & mask;
  switch(a) {
  case 0x400: d = 0x0010; break; // MasterVolume (actually returns the LSI version)
  case 0x402: // RingBufferAddress
    d  = (((uint32)(YAMSTATE->rbl)) & 3) << 7;
    d |= ((YAMSTATE->rbp >> 13) & 0x7F);
    break;
  case 0x404: d = (1<<11) | (1 << 8); break; // MIDIInput, unimplemented
  case 0x406: d = 0; break; // MIDI output, unimplemented
  case 0x408: // CallAddress (playpos in increments of 4K)
    { int c = (YAMSTATE->mslc) & 0x1F;
	  int sgc, ca, eg;
	  struct YAM_CHAN * chan;

      if(YAMSTATE->out_pending > 0) yam_flush(YAMSTATE);

	  chan = YAMSTATE->chan + c;

	  sgc = chan->EG.state & 3;
	  ca = ( chan->cur_addr >> (SHIFT + 12) ) & 0xf;
	  eg = ( 0x1f - ( chan->EG.volume >> (EG_SHIFT + 5) ) ) & 0x1f;

	  d = (c << 11) | (ca << 7) | (sgc << 5) | eg;

//
// might only be checking when envstate is release anyway?
// 

//      d |= (YAMSTATE->chan[c].envstate != 3) << 4;
//d |= 1<<3;

//      d |= (YAMSTATE->chan[c].lp & 1) << 4;
//      YAMSTATE->chan[c].lp = 0;



//
// here bit 3 seems to be set to 1 to indicate idle.
// what idle means, I haven't determined
//

//d|=1<<3;

  //d|=(YAMSTATE->chan[c].sampler_dir!=0)<<4;
//  d|=(YAMSTATE->chan[c].envlevel>=0x80)<<4;

//  d|=((YAMSTATE->chan[c].envlevel)&0x3FF)>>5;

  //if(YAMSTATE->chan[c].envstate >= 1) {
  //  d |= 1<<4;
  //  d|=(YAMSTATE->chan[c].envlevel>=0x100)<<3;
  //}
//    //d |= 1<<3;
//    d |= (YAMSTATE->chan[c].envlevel >= 0x80) << 3;
//
//    d |= (YAMSTATE->chan[c].sampler_dir!=0) << 3;
//d|=1<<3;

    // if 1<<3 always set: missing notes
    // if 1<<3 never set: notes get cut off early, but most are there
    // if it's >=0x300: missing notes
    // if it's >=0x3C0: missing notes
    // if it's <0x300: notes get cut off early

// close but missing notes
//      d |= (YAMSTATE->chan[c].envstate == 3) << 4;
//      d |= (YAMSTATE->chan[c].envlevel >= 0x281) << 3;


//      d |= 1<<4;

//      d |= (YAMSTATE->chan[c].envlevel >= 0x281) << 3;
//      d |= (YAMSTATE->chan[c].envstate != 3) << 3;

//      d |= (YAMSTATE->chan[c].sampler_dir == 0) << 4;

      //d |= ((YAMSTATE->chan[c].envlevel) & 0x1FFF) >> 3;

//d ^= 0x00; // few
//d ^= 0x67; // few (these bits have no effect)
//d ^= 0x7F; // many missing notes
//d ^= 1<<4; // few
//d ^= 1<<3; // few
//d ^= 3<<3; // many missing notes

// only updates when hardware bit 1<<4 is 1
// hardware bit 1<<3 means idle?



// ok but wrong
//      d |= (((uint32)(YAMSTATE->chan[c].envstate)) & 3) << 5;
//      d |= ((YAMSTATE->chan[c].envlevel) & 0x3FF) >> 5;
//     d ^= 0x7F;


//      { uint32 l = (YAMSTATE->chan[c].envlevel) & (YAMSTATE->chan[c].envlevelmask[YAMSTATE->chan[c].envstate & 3]);
//        if(l>0x3BF)l=0x1FFF;
//        if(YAMSTATE->chan[c].sampler_dir == 0) l=0x1FFF;
//d|=(l>=0x3C0)<<3;
//d|=(l>=0x3C0)<<4;
//        d |= l >> 8;
//        if(l>=0x3BF) d|=0<<3;
//d|=l>>5;
//      }


//d^=0x1C;
//      d |= ((YAMSTATE->chan[c].envlevel) & 0x1FFF) >> 8;

//{ uint32 es = ((uint32)(YAMSTATE->chan[c].envstate)) & 3;
//  es = 0;
//  if(YAMSTATE->chan[c].sampler_dir == 0) es = 3;
//      d |= es << 3;
//}

//d ^= 0x18;
//d |= 0x00;

    }

    break;
  case 0x412:
    d = YAMSTATE->dmea & 0xFFFF;
    break;
  case 0x414:
    d = (((uint32)(YAMSTATE->dmea)) & 0xF0000) >> 4;
    d |= (((uint32)(YAMSTATE->drga)) & 0xFFE) << 0;
    break;
  case 0x416:
    d = (((uint32)(YAMSTATE->dtlg)) & 0xFFE) << 0;
    break;
  case 0x418:
    d  = (((uint32)(YAMSTATE->tctl[0])) & 0x7) << 8;
    d |= (((uint32)(YAMSTATE->tim[0])) & 0xFF) << 0;
    break;
  case 0x41A:
    d  = (((uint32)(YAMSTATE->tctl[1])) & 0x7) << 8;
    d |= (((uint32)(YAMSTATE->tim[1])) & 0xFF) << 0;
    break;
  case 0x41C:
    d  = (((uint32)(YAMSTATE->tctl[2])) & 0x7) << 8;
    d |= (((uint32)(YAMSTATE->tim[2])) & 0xFF) << 0;
    break;
  case 0x41E: d = YAMSTATE->scieb & 0x07FF; break;
  case 0x420: d = YAMSTATE->scipd & 0x07FF; break;
  case 0x424: d = YAMSTATE->scilv0 & 0xFF; break;
  case 0x426: d = YAMSTATE->scilv1 & 0xFF; break;
  case 0x428: d = YAMSTATE->scilv2 & 0xFF; break;
  case 0x42A: d = YAMSTATE->mcieb & 0x07FF; break;
  case 0x42C: d = YAMSTATE->mcipd & 0x07FF; break;
  }
#if defined(SCSP_LOG) && !defined(USE_STARSCREAM)
  fprintf(scsp_log, "%u - %08x r: %04x, %04x\n", YAMSTATE->odometer, (int)(*scsp_pc - *scsp_basepc), a, d);
#endif
  return d & mask;
}

void EMU_CALL yam_scsp_store_reg(void *state, uint32 a, uint32 d, uint32 mask, uint8 *breakcpu) {
  a &= 0xFFE;
  d &= 0xFFFF & mask;
#if defined(SCSP_LOG) && !defined(USE_STARSCREAM)
  fprintf(scsp_log, "%u - %08x w: %04x, %04x\n", YAMSTATE->odometer, (int)(*scsp_pc - *scsp_basepc), a, d);
#endif
  mask &= 0xFFFF;
  if(a <  0x400) { chan_scsp_store_reg(YAMSTATE, a>>5, a&0x1E, d, mask); return; }
  if(a >= 0x700) { dsp_scsp_store_reg(YAMSTATE, a, d, mask); return; }
  if(a >= 0x600) { uint32 offset = (YAMSTATE->bufptr-64+(a-0x600)/2)&(32*RINGMAX-1); YAMSTATE->ringbuf[offset] = (d & mask) | (YAMSTATE->ringbuf[offset] & ~mask); return; }
  switch(a) {
  case 0x400: // MasterVolume
    yam_flush(YAMSTATE);
    if(mask & 0x00FF) {
      YAMSTATE->mvol   = d & 0xF;
    }
    break;
  case 0x402: // RingBufferAddress
    { uint32 oldrbp = YAMSTATE->rbp;
      uint8 oldrbl = YAMSTATE->rbl;
      if(mask & 0x00FF) {
        YAMSTATE->rbp = (((uint32)d) & 0x7F) << 13;
        YAMSTATE->rbl &= 2;
        YAMSTATE->rbl |= (d >> 7) & 1;
      }
      if(mask & 0xFF00) {
        YAMSTATE->rbl &= 1;
        YAMSTATE->rbl |= (d >> 7) & 2;
      }
      if((oldrbp != YAMSTATE->rbp) || (oldrbl != YAMSTATE->rbl)) {
        uint32 newrbp = YAMSTATE->rbp;
        uint8 newrbl = YAMSTATE->rbl;
        YAMSTATE->rbp = oldrbp;
        YAMSTATE->rbl = oldrbl;
        yam_flush(YAMSTATE);
#ifdef ENABLE_DYNAREC
        YAMSTATE->dsp_dyna_valid = 0;
#endif
        YAMSTATE->rbp = newrbp;
        YAMSTATE->rbl = newrbl;
      }
    }
    break;
  case 0x408: // ChnInfoReq
    if(mask & 0xFF00) {
      YAMSTATE->mslc  = (d >> 11) & 0x1F;
    }
    break;
  case 0x412: // DMA 
    if(mask & 0x00FF) { YAMSTATE->dmea = (YAMSTATE->dmea & 0xFFF00) | (d & 0xFF); }
    if(mask & 0xFF00) { YAMSTATE->dmea = (YAMSTATE->dmea & 0xF00FF) | (d & 0xFF00); }
    break;
  case 0x414:
    if(mask & 0xFF) { YAMSTATE->drga = (YAMSTATE->drga & 0xF00) | (d & 0xFE); }
    if(mask & 0xFF00) { YAMSTATE->drga = (YAMSTATE->drga & 0x0FF) | (d & 0xF00); YAMSTATE->dmea = (YAMSTATE->dmea & 0xFFFF) | ((d & 0xF000) << 4); }
    break;
  case 0x416:
    if(mask & 0xFF) { YAMSTATE->dtlg = (YAMSTATE->dtlg & 0xF00) | (d & 0xFE); }
    if(mask & 0xFF00) { YAMSTATE->dtlg = (YAMSTATE->dtlg & 0xFF) | (d & 0xF00); }
    break;
  case 0x418: // TimerAControl
    if(mask & 0x00FF) { YAMSTATE->tim[0] = d & 0xFF; }
    if(mask & 0xFF00) { YAMSTATE->tctl[0] = (d >> 8) & 7; }
    if(breakcpu) *breakcpu = 1;
    break;
  case 0x41A: // TimerBControl
    if(mask & 0x00FF) { YAMSTATE->tim[1] = d & 0xFF; }
    if(mask & 0xFF00) { YAMSTATE->tctl[1] = (d >> 8) & 7; }
    if(breakcpu) *breakcpu = 1;
    break;
  case 0x41C: // TimerCControl
    if(mask & 0x00FF) { YAMSTATE->tim[2] = d & 0xFF; }
    if(mask & 0xFF00) { YAMSTATE->tctl[2] = (d >> 8) & 7; }
    if(breakcpu) *breakcpu = 1;
    break;
  case 0x41E: // SCIEB
    YAMSTATE->scieb = (((YAMSTATE->scieb) & (~mask)) | (d & mask)) & 0x7FF;
    if(breakcpu) *breakcpu = 1;
    break;
  case 0x420: // SCIPD
    YAMSTATE->scipd = (((YAMSTATE->scipd) & (~mask)) | (d & mask)) & 0x7FF;
    if(breakcpu) *breakcpu = 1;
    break;
  case 0x422: // SCIRE
    YAMSTATE->scipd &= ~(d & mask);
    // I guess this is how we acknowledge interrupts now
    sci_recompute(YAMSTATE);
    if(breakcpu) *breakcpu = 1;
    break;
  case 0x424: // SCILV0
    if(mask & 0x00FF) { YAMSTATE->scilv0 = d; }
    break;
  case 0x426: // SCILV1
    if(mask & 0x00FF) { YAMSTATE->scilv1 = d; }
    break;
  case 0x428: // SCILV2
    if(mask & 0x00FF) { YAMSTATE->scilv2 = d; }
    break;
  case 0x42A: // MCIEB
    YAMSTATE->mcieb = (((YAMSTATE->mcieb) & (~mask)) | (d & mask)) & 0x7FF;
    break;
  case 0x42C: // MCIPD
    YAMSTATE->mcipd = (((YAMSTATE->mcipd) & (~mask)) | (d & mask)) & 0x7FF;
    break;
  case 0x42E: // MCIRE
    YAMSTATE->mcipd &= ~(d & mask);
    break;
  }
}

uint32 EMU_CALL yam_aica_load_reg(void *state, uint32 a, uint32 mask) {
  uint32 d = 0;
  a &= 0xFFFC;
  if(a <  0x2000) return chan_aica_load_reg(YAMSTATE, a>>7, a&0x7C) & mask;
  if(a >= 0x3000) return dsp_aica_load_reg(YAMSTATE, a) & mask;
  if(a <  0x2048) {
    d =
      ((((uint32)(YAMSTATE->efsdl[(a - 0x2000) / 4])) & 0x0F) << 8) |
      ((((uint32)(YAMSTATE->efpan[(a - 0x2000) / 4])) & 0x1F) << 0);
    return d & mask;
  }
  switch(a) {
  case 0x2800: d = 0x0010; break; // MasterVolume (actually returns the LSI version)
  case 0x2804: // RingBufferAddress
    d  = (((uint32)(YAMSTATE->rbl)) & 3) << 13;
    d |= ((YAMSTATE->rbp >> 11) & 0xFFF);
    break;
  case 0x2808: d = (1<<11) | (1 << 8); break; // MIDIInput, unimplemented
  case 0x280C: d = 0; break; // ChnInfoReq, always seems to return 0 when read
  case 0x2810: // PlayStatus
//    if(YAMSTATE->out_pending > 100) yam_flush(YAMSTATE);
    if(YAMSTATE->out_pending > 0) yam_flush(YAMSTATE);
    { int c = (YAMSTATE->mslc) & 0x3F;
      d  = (((uint32)(YAMSTATE->chan[c].lp      )) & 1) << 15;
      if(YAMSTATE->afsel == 0) {
        d |= (((uint32)(YAMSTATE->chan[c].EG.state)) & 3) << 13;
        d |= (~YAMSTATE->chan[c].EG.volume >> (EG_SHIFT-3)) & 0x1FFF;
      } else {
        d |= (((uint32)(YAMSTATE->chan[c].lpfstate)) & 3) << 13;
        d |= (YAMSTATE->chan[c].lpflevel) & 0x1FFF;
      }
    }
    break;
  case 0x2814: d = (YAMSTATE->chan[YAMSTATE->mslc].cur_addr)>>(SHIFT+12); break;
  case 0x2880: d = YAMSTATE->mrwinh & 0xF; break;
  case 0x2884: d = 0; break;
  case 0x2888: d = 0; break;
  case 0x288C: d = 0; break;
  case 0x2890:
    d  = (((uint32)(YAMSTATE->tctl[0])) & 0x7) << 8;
    d |= (((uint32)(YAMSTATE->tim[0])) & 0xFF) << 0;
    break;
  case 0x2894:
    d  = (((uint32)(YAMSTATE->tctl[1])) & 0x7) << 8;
    d |= (((uint32)(YAMSTATE->tim[1])) & 0xFF) << 0;
    break;
  case 0x2898:
    d  = (((uint32)(YAMSTATE->tctl[2])) & 0x7) << 8;
    d |= (((uint32)(YAMSTATE->tim[2])) & 0xFF) << 0;
    break;
  case 0x289C: d = YAMSTATE->scieb & 0x07FF; break;
  case 0x28A0: d = YAMSTATE->scipd & 0x07FF; break;
  case 0x28A4: d = 0; break;
  case 0x28A8: d = YAMSTATE->scilv0 & 0xFF; break;
  case 0x28AC: d = YAMSTATE->scilv1 & 0xFF; break;
  case 0x28B0: d = YAMSTATE->scilv2 & 0xFF; break;
  case 0x28B4: d = YAMSTATE->mcieb & 0x07FF; break;
  case 0x28B8: d = YAMSTATE->mcipd & 0x07FF; break;
  case 0x28BC: d = 0; break;
  case 0x2C00: d = 0; break;
  case 0x2D00: d = YAMSTATE->intreq & 7; break;
  case 0x2D04: d = 0; break;
  case 0x2E00: d = YAMSTATE->rtc >> 16; break;
  case 0x2E04: d = YAMSTATE->rtc; break;
  }
  return d & mask;
}

void EMU_CALL yam_aica_store_reg(void *state, uint32 a, uint32 d, uint32 mask, uint8 *breakcpu) {
  a &= 0xFFFC;
  d &= 0xFFFF & mask;
  if(a <  0x2000) { chan_aica_store_reg(YAMSTATE, a>>7, a&0x7C, d, mask); return; }
  if(a >= 0x3000) { dsp_aica_store_reg(YAMSTATE, a, d, mask); return; }
  if(a <  0x2048) {
    if(mask & 0x00FF) { YAMSTATE->efpan[(a - 0x2000) / 4] = d & 0x1F; }
    if(mask & 0xFF00) { YAMSTATE->efsdl[(a - 0x2000) / 4] = (d >> 8) & 0x0F; }
    return;
  }
  switch(a) {
  case 0x2800: // MasterVolume
    yam_flush(YAMSTATE);
    if(mask & 0x00FF) {
      YAMSTATE->mvol   = d & 0xF;
    }
    if(mask & 0xFF00) {
      YAMSTATE->mono   = (d >> 15) & 1;
    }
    break;
  case 0x2804: // RingBufferAddress
    { uint32 oldrbp = YAMSTATE->rbp;
      uint8 oldrbl = YAMSTATE->rbl;
      if(mask & 0x00FF) {
        YAMSTATE->rbp >>= 11;
        YAMSTATE->rbp &= 0xF00;
        YAMSTATE->rbp |= d & 0x0FF;
        YAMSTATE->rbp <<= 11;
      }
      if(mask & 0xFF00) {
        YAMSTATE->rbp >>= 11;
        YAMSTATE->rbp &= 0x0FF;
        YAMSTATE->rbp |= d & 0xF00;
        YAMSTATE->rbp <<= 11;
        YAMSTATE->rbl = (d >> 13) & 3;
      }
      if((oldrbp != YAMSTATE->rbp) || (oldrbl != YAMSTATE->rbl)) {
        uint32 newrbp = YAMSTATE->rbp;
        uint8 newrbl = YAMSTATE->rbl;
        YAMSTATE->rbp = oldrbp;
        YAMSTATE->rbl = oldrbl;
        yam_flush(YAMSTATE);
#ifdef ENABLE_DYNAREC
        YAMSTATE->dsp_dyna_valid = 0;
#endif
        YAMSTATE->rbp = newrbp;
        YAMSTATE->rbl = newrbl;
      }
    }
    break;
  case 0x2808: // MIDIInput, unimplemented
    break;
  case 0x280C: // ChnInfoReq
    if(mask & 0x00FF) {
      // MIDI output buffer, unimplemented
    }
    if(mask & 0xFF00) {
      YAMSTATE->mslc  = (d >> 8) & 0x3F;
      YAMSTATE->afsel = (d >> 14) & 1;
    }
    break;
  case 0x2810: break; // PlayStatus - writing probably has no effect.
  case 0x2814: break; // PlayPos - writing probably has no effect.
  case 0x2880: // misc.
    if(mask & 0x00FF) {
      YAMSTATE->mrwinh = d & 0xF;
    }
    break;
  case 0x2884: break; // misc.
  case 0x2888: break; // misc.
  case 0x288C: break; // misc.
  case 0x2890: // TimerAControl
    if(mask & 0x00FF) { YAMSTATE->tim[0] = d & 0xFF; }
    if(mask & 0xFF00) { YAMSTATE->tctl[0] = (d >> 8) & 7; }
    if(breakcpu) *breakcpu = 1;
    break;
  case 0x2894: // TimerBControl
    if(mask & 0x00FF) { YAMSTATE->tim[1] = d & 0xFF; }
    if(mask & 0xFF00) { YAMSTATE->tctl[1] = (d >> 8) & 7; }
    if(breakcpu) *breakcpu = 1;
    break;
  case 0x2898: // TimerCControl
    if(mask & 0x00FF) { YAMSTATE->tim[2] = d & 0xFF; }
    if(mask & 0xFF00) { YAMSTATE->tctl[2] = (d >> 8) & 7; }
    if(breakcpu) *breakcpu = 1;
    break;
  case 0x289C: // SCIEB
    YAMSTATE->scieb = (((YAMSTATE->scieb) & (~mask)) | (d & mask)) & 0x7FF;
    if(breakcpu) *breakcpu = 1;
    break;
  case 0x28A0: // SCIPD
    YAMSTATE->scipd = (((YAMSTATE->scipd) & (~mask)) | (d & mask)) & 0x7FF;
    if(breakcpu) *breakcpu = 1;
    break;
  case 0x28A4: // SCIRE
    YAMSTATE->scipd &= ~(d & mask);
    if(breakcpu) *breakcpu = 1;
    break;
  case 0x28A8: // SCILV0
    if(mask & 0x00FF) { YAMSTATE->scilv0 = d; }
    break;
  case 0x28AC: // SCILV1
    if(mask & 0x00FF) { YAMSTATE->scilv1 = d; }
    break;
  case 0x28B0: // SCILV2
    if(mask & 0x00FF) { YAMSTATE->scilv2 = d; }
    break;
  case 0x28B4: // MCIEB
    YAMSTATE->mcieb = (((YAMSTATE->mcieb) & (~mask)) | (d & mask)) & 0x7FF;
    break;
  case 0x28B8: // MCIPD
    YAMSTATE->mcipd = (((YAMSTATE->mcipd) & (~mask)) | (d & mask)) & 0x7FF;
    break;
  case 0x28BC: // MCIRE
    YAMSTATE->mcipd &= ~(d & mask);
    break;
  case 0x2C00: // ARMReset
    break;
  case 0x2D00: // INTRequest
    break;
  case 0x2D04: // INTClear
    // running through the recompute will clear the previous interrupt
    // as well as enabling the next one, if there is a next one
    sci_recompute(YAMSTATE);
    if(breakcpu) *breakcpu = 1;
    break;
  case 0x2E00: // RTCHi
    break;
  case 0x2E04: // RTCLo
    break;
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Generate random data
//
#if 0
static uint32 yamrand16(struct YAM_STATE *state) {
  state->randseed = 1103515245 * state->randseed + 12345;
  return state->randseed >> 16;
}
#endif

/////////////////////////////////////////////////////////////////////////////
//
// (original) Envelope-related calculations
//

//
// Adjust actual rate to get effective rate
//
static uint32 env_adjustrate(struct YAM_CHAN *chan, uint32 rate) {
  sint32 effrate = rate * 2;
  if(chan->krs < 0xF) {
    effrate += (chan->fns >> 9) & 1;
    effrate += chan->krs * 2;
    effrate = (effrate - 8) + (chan->oct ^ 8);
  }
  // Clipping is important because of the table lookups
  if(effrate <= 0) return 0;
  if(effrate >= 0x3C) return 0x3C;
  return effrate;
}

//
// Determine whether a step is going to occur here
//
static int env_needstep(uint32 effrate, uint32 odometer) {
  uint32 shift;
  uint32 pattern;
  uint32 bitplace;
  if(effrate <= 0x01) return 0;
  if(effrate >= 0x30) return ((odometer & 1) == 0);
  shift = 12 - ((effrate - 1) >> 2);
  pattern = (effrate - 1) & 3;
  if(odometer & ((1<<shift)-1)) return 0;
  bitplace = (odometer >> shift) & 7;
  return (0xFFFDDDD5 >> (pattern * 8 + bitplace)) & 1;
  // 11010101 0x01 each bit is 4096 samples
  // 11011101 0x02
  // 11111101 0x03
  // 11111111 0x04
}

/////////////////////////////////////////////////////////////////////////////
//
// Read next sample
//
static sint32 AICA_UpdateSlot(struct YAM_STATE *state, struct YAM_CHAN *chan)
{
  sint32 sample, fpart;
  int cur_sample;       //current sample
  int nxt_sample;       //next sample
  int step=chan->step;
  uint32 addr1,addr2,addr_select;                                   // current and next sample addresses
  uint32 *addr[2]      = {&addr1, &addr2};                          // used for linear interpolation
  uint32 *slot_addr[2] = {&(chan->cur_addr), &(chan->nxt_addr)};    //

  if(chan->ssctl!=0)  //no FM or noise yet
    return 0;

  if(chan->plfos!=0) {
    step=step*PLFO_Step(&(chan->PLFO));
    step>>=SHIFT;
  }

  if(chan->pcms == 0) {
    addr1=(chan->cur_addr>>(SHIFT-1))&state->ram_mask&~1;
    addr2=(chan->nxt_addr>>(SHIFT-1))&state->ram_mask&~1;
  }
  else {
    addr1=chan->cur_addr>>SHIFT;
    addr2=chan->nxt_addr>>SHIFT;
  }

  if(state->version == 1 && (chan->mdl!=0 || chan->mdxsl!=0 || chan->mdysl!=0))
  {
    sint32 smp=(state->ringbuf[(state->bufptr-64+chan->mdxsl)&(32*RINGMAX-1)]+state->ringbuf[(state->bufptr-64+chan->mdysl)&(32*RINGMAX-1)])/2;

    smp<<=0xA; // associate cycle with 1024
    smp>>=0x1A-chan->mdl; // ex. for MDL=0xF, sample range corresponds to +/- 64 pi (32=2^5 cycles) so shift by 11 (16-5 == 0x1A-0xF)
    if(chan->pcms == 0) smp<<=1;

    addr1+=smp; addr2+=smp;
  }

  if(chan->pcms == 1)  // 8-bit signed
  {
    sint8 *p1=(signed char *) (((uint8*)state->ram_ptr)+(((chan->sampleaddr+addr1)^state->mem_byte_address_xor)&state->ram_mask));
    sint8 *p2=(signed char *) (((uint8*)state->ram_ptr)+(((chan->sampleaddr+addr2)^state->mem_byte_address_xor)&state->ram_mask));
    cur_sample = (p1[0] << 8) ^ chan->sampler_invert;
    nxt_sample = (p2[0] << 8) ^ chan->sampler_invert;
  }
  else if (chan->pcms == 0)  //16 bit signed
  {
    sint16 *p1=(signed short *) (((uint8*)state->ram_ptr)+(((chan->sampleaddr+addr1)^state->mem_word_address_xor)&state->ram_mask&~1));
    sint16 *p2=(signed short *) (((uint8*)state->ram_ptr)+(((chan->sampleaddr+addr2)^state->mem_word_address_xor)&state->ram_mask&~1));
    cur_sample = p1[0] ^ chan->sampler_invert;
    nxt_sample = p2[0] ^ chan->sampler_invert;
  }
  else  // 4-bit ADPCM
  {
    uint32 adbase = chan->adbase;
    uint8 *base;
    uint32 steps_to_go = addr2, curstep = chan->curstep;

    if(adbase) {
      base = ((uint8*)state->ram_ptr);
      cur_sample = chan->cur_sample; // may already contains current decoded sample 

      // seek to the interpolation sample
      while (curstep < steps_to_go) {
        int shift1, delta1;
        shift1 = 4*((curstep&1));
        delta1 = (base[adbase]>>shift1)&0xf;
        DecodeADPCM(&(chan->cur_sample),delta1,&(chan->cur_quant));
        curstep++;
        if (!(curstep & 1)) {
          adbase = (adbase + 1) & state->ram_mask;
        }
        if (curstep == addr1)
          cur_sample = chan->cur_sample;
      }
      nxt_sample = chan->cur_sample;

      chan->adbase = adbase;
      chan->curstep = curstep;
    }
    else {
      cur_sample = nxt_sample = 0;
    }
  }

  fpart = chan->cur_addr & ((1<<SHIFT)-1);
  sample=cur_sample*((1<<SHIFT)-fpart)+nxt_sample*fpart;
  sample>>=SHIFT;

  chan->prv_addr=chan->cur_addr;
  if(chan->Backwards)
    chan->cur_addr-=step;
  else
    chan->cur_addr+=step;
  chan->nxt_addr=chan->cur_addr+(1<<SHIFT);

  addr1=chan->cur_addr>>SHIFT;
  addr2=chan->nxt_addr>>SHIFT;

  if(addr1>=chan->loopstart && !(chan->Backwards)) {
    if(chan->link && chan->EG.state==ATTACK)
      chan->EG.state = DECAY1;
  }

  for (addr_select=0;addr_select<2;addr_select++)
  {
    sint32 rem_addr;
    switch(chan->sampler_looptype)
    {
    case 0:  //no loop
      if(*addr[addr_select]>=chan->loopstart && *addr[addr_select]>=chan->loopend) {
        keyoff(chan, 0);
      }
      break;
    case 1: //normal loop
      if(*addr[addr_select]>=chan->loopend) {
        rem_addr = *slot_addr[addr_select] - (chan->loopend<<SHIFT);
        *slot_addr[addr_select]=(chan->loopstart<<SHIFT) + rem_addr;
        if(chan->pcms>=2) {
          // restore the state @ LSA - the sampler will naturally walk to (LSA + remainder)
          chan->adbase = (chan->sampleaddr+(chan->loopstart/2))&state->ram_mask;
          chan->curstep = chan->loopstart;
          if (chan->pcms == 2) {
            chan->cur_sample = chan->cur_lpsample;
            chan->cur_quant = chan->cur_lpquant;
          }
        //printf("Looping: slot_addr %x LSA %x LEA %x step %x base %x\n", slot->cur_addr>>SHIFT, LSA(slot), LEA(slot), slot->curstep, slot->adbase);
        }
      }
      break;
    case 2:  //reverse loop
      if((*addr[addr_select]>=chan->loopstart) && !(chan->Backwards)) {
        rem_addr = *slot_addr[addr_select] - (chan->loopstart<<SHIFT);
        *slot_addr[addr_select]=(chan->loopend<<SHIFT) - rem_addr;
        chan->Backwards = 1;
      }
      else if((*addr[addr_select]<chan->loopstart || (*slot_addr[addr_select]&0x80000000)) && chan->Backwards) {
        rem_addr = (chan->loopstart<<SHIFT) - *slot_addr[addr_select];
        *slot_addr[addr_select]=(chan->loopend<<SHIFT) - rem_addr;
      }
      break;
    case 3: //ping-pong
      if(*addr[addr_select]>=chan->loopend) //reached end, reverse till start
      {
        rem_addr = *slot_addr[addr_select] - (chan->loopend<<SHIFT); 
        *slot_addr[addr_select]=(chan->loopend<<SHIFT) - rem_addr;
        chan->Backwards = 1;
      }
      else if((*addr[addr_select]<chan->loopstart || (*slot_addr[addr_select]&0x80000000)) && chan->Backwards)//reached start or negative
      {
        rem_addr = (chan->loopstart<<SHIFT) - *slot_addr[addr_select];
        *slot_addr[addr_select]=(chan->loopstart<<SHIFT) + rem_addr;
        chan->Backwards = 0;
      }
      break;
    }
  }

  if(chan->alfos!=0) {
    sample=sample*ALFO_Step(&(chan->ALFO));
    sample>>=SHIFT;
  }

  if(chan->EG.state==ATTACK)
    sample=(sample*EG_Update(chan))>>SHIFT;
  else
    sample=(sample*EG_TABLE[EG_Update(chan)>>(SHIFT-10)])>>SHIFT;

  if(state->version == 1 && !chan->stwinh)
  {
    unsigned short Enc = ((chan->tl)<<0x0)|(0x7<<0xd);
    state->ringbuf[state->bufptr] = (sample * LPANTABLE_SCSP[Enc])>>(SHIFT+1);
  }

  /*if(chan->mslc) {
    AICA->udata.data[0x14/2] = addr1;
    if (!(AFSEL(AICA))) {
      UINT16 res;

      AICA->udata.data[0x10/2] |= slot->EG.state<<13;

      res = 0x3FF - (slot->EG.volume>>EG_SHIFT);

      res *= 959;
      res /= 1024;

      if (res > 959) res = 959;

      AICA->udata.data[0x10/2] = res;

      //AICA->udata.data[0x10/2] |= 0x3FF - (slot->EG.volume>>EG_SHIFT);
    }
  }*/

  return sample;
}

/////////////////////////////////////////////////////////////////////////////
//
// Generate samples
// Samples are returned in 20-bit format
// Returns the number of samples actually generated
//
static uint32 generate_samples(
  struct YAM_STATE *state,
  struct YAM_CHAN *chan,
  sint32 *buf,
  uint32 odometer,
  uint32 samples
) {
  uint32 g;
  uint32 bufptrsave = state->bufptr;

//gfreq[samples]++;

//printf("generate_samples(%08X,%08X,%u)\n",chan,buf,samples);

  for(g = 0; g < samples; g++) {
//buf[g]=g*100;continue;
    //
    // If the amp envelope is inactive, quit
    //
    if(chan->EG.volume <= 0) {
      break;
    }
    //
    // If we must generate a sample, generate it
    //
    if(buf) {
      // Apply filter, if we want it
      sint32 s = AICA_UpdateSlot(state, chan);
      if(!(chan->lpoff)) {
        uint32 fv = chan->lpflevel;
        uint32 qv = chan->q & 0x1F;
        sint32 f = (((fv & 0xFF) | 0x100) << 4) >> ((fv >> 8) ^ 0x1F);
        sint32 q = qtable[qv];
        s = f * s + (0x2000 - f + q) * (chan->lpp1) - q * (chan->lpp2);
        s >>= 13;
        chan->lpp2 = chan->lpp1;
        chan->lpp1 = s;
      }
      // Write output
      buf[g] = s;
    }
    state->bufptr = (state->bufptr + 32) & (32*RINGMAX-1);
    //
    // Now we need to advance the channel state machine, regardless of
    // whether we're generating output or not
    //
    //
    // Advance filter envelope
    //
    { uint32 effectiverate = env_adjustrate(chan, chan->fr[chan->lpfstate]);
      if(env_needstep(effectiverate, odometer)) {
        uint32 d = envdecayvalue[effectiverate][odometer&3];
        uint32 target = chan->flv[chan->lpfstate+1];
        if(chan->lpflevel < target) {
          uint32 maxd = target - chan->lpflevel;
          if(d > maxd) { d = maxd; }
          chan->lpflevel += d;
        } else if(chan->lpflevel > target) {
          uint32 maxd = chan->lpflevel - target;
          if(d > maxd) { d = maxd; }
          chan->lpflevel -= d;
        } else {
          if(chan->lpfstate < 3) { chan->lpfstate++; }
        }
      }
    }
    // Advance our temporary odometer copy
    odometer++;
    // Done with this sample!
  }
  state->bufptr = bufptrsave;
  return g;
}

/////////////////////////////////////////////////////////////////////////////
//
// Render a single channel and add it to the given outputs
//
// directout or fxout may be NULL
//
static void render_and_add_channel(
  struct YAM_STATE *state,
  struct YAM_CHAN *chan,
  sint32 *directout,
  sint32 *fxout,
  uint32 odometer,
  uint32 samples
) {
  uint32 i;
  sint32 localbuf[RENDERMAX];
  uint32 rendersamples;

  // Channel does nothing if attenuation >= 0x3C0
  if(chan->EG.volume <= 0) { return; }

  if(!chan->disdl) { directout = NULL; }
  if(!chan->dsplevel) { fxout = NULL; }

  // Generate samples
  rendersamples = generate_samples(
    state,
    chan,
    (directout || fxout || (state->version == 1 && !chan->stwinh)) ? localbuf : NULL,
    odometer,
    samples
  );

  // Add to output
  if(directout) {
    sint32 vol_l, vol_r;
    convert_stereo_send_level(
      chan->tl,
      chan->disdl,
      (state->mono) ? 0 : (chan->dipan),
      &vol_l, &vol_r,
      state->version
    );
    for(i = 0; i < rendersamples; i++) {
      directout[0] += (localbuf[i]*vol_l) >> SHIFT;
      directout[1] += (localbuf[i]*vol_r) >> SHIFT;
      directout += 2;
    }
  }
  if(fxout) {
    sint32 vol_l, vol_r;
    convert_stereo_send_level(
      chan->tl,
      chan->dsplevel,
      0,
      &vol_l, &vol_r,
      state->version
    );
    for(i = 0; i < rendersamples; i++) {
      fxout[0] += (localbuf[i]*vol_l) >> (SHIFT-2);
      fxout += 16;
    }
  }

}

/////////////////////////////////////////////////////////////////////////////
//
// Floating-point conversion
//
static uint32 __fastcall float16_to_int24(uint32 f) {
  uint32 exponent = (f >> 11) & 0xF;
  sint32 result;
  result = (f & 0x8000) << 16; // take the sign in bit 31
  result >>= 1; // duplicate the sign in bit 30
  if(exponent >= 12) { exponent = 11; } // cap exponent to 11 for denormals
  else { result ^= 0x40000000; } // reverse bit 30 for normals
  result |= (f & 0x7FF) << 19; // set bits 29-0 to the mantissa
  result >>= exponent + 8; // shift right by the exponent + 8
  return result;
}

static uint32 __fastcall int24_to_float16(uint32 i) {
  uint32 exponent = 0;
  uint32 sign = i & 0x00800000;
  if(sign) { i = ~i; }
  i &= 0x7FFFFF;
  if(i < 0x020000) { exponent += (6<<11); i <<= 6; }
  if(i < 0x100000) { exponent += (3<<11); i <<= 3; }
  if(i < 0x400000) { exponent += (1<<11); i <<= 1; }
  if(i < 0x400000) { exponent += (1<<11); i <<= 1; }
  if(i < 0x400000) { exponent += (1<<11); }
  i >>= 11;
  i &= 0x7FF;
  i |= exponent;
  if(sign) { i ^= 0x87FF; }
  return i;
}

/////////////////////////////////////////////////////////////////////////////

#define SINT32ATOFFSET(a,b) (*((sint32*)(((uint8*)(a))+(b))))

/////////////////////////////////////////////////////////////////////////////
//
// Execute one sample on the effects DSP
//
static void __fastcall dsp_sample_interpret(struct YAM_STATE *state) {
  const struct MPRO *mpro = state->mpro;
  uint32 i;
  // Pre-compute ringbuffer size mask
  uint32 rbmask = (1 << ((state->rbl)+13)) - 1;
  //
  // For 128 steps:
  //
  for(i = 0; i < 128; i++, mpro++) {
    sint32 b, x, y, shifted;
    //
    // Proper skip for "empty" instructions
    //
    if((mpro->__kisxzbon) & 0x80) {
      x = state->temp[(state->mdec_ct)&0x7F];
      state->xzbchoice[XZBCHOICE_ACC] =
        (sint32)((((sint64)x) * ((sint64)(state->yychoice[YYCHOICE_FRC_REG]))) >> 12) + x;
      continue;
    }
    state->xzbchoice[XZBCHOICE_TEMP] = state->temp[((mpro->t_0rrrrrrr)+(state->mdec_ct))&0x7F];
    state->yychoice[YYCHOICE_COEF] = state->coef[mpro->c_0rrrrrrr];
    //
    // Input read
    //
    state->xzbchoice[XZBCHOICE_INPUTS] = state->inputs[mpro->i_00rrrrrr];
    //
    // Input write
    //
    state->inputs[mpro->i_0T0wwwww] = state->mem_in_data[i & 3];
    //
    // B selection
    //
    b = SINT32ATOFFSET(state->xzbchoice, (mpro->__kisxzbon) & 0x0C);
    b ^= ((sint32)(mpro->negb));
    b -= ((sint32)(mpro->negb));
    //
    // X selection
    //
    x = SINT32ATOFFSET(state->xzbchoice, (mpro->__kisxzbon) & 0x10);
    //
    // Y selection
    //
    y = SINT32ATOFFSET(state->yychoice, (mpro->m_wrAFyyYh) & 0x0C);
    //
    // Y latch
    //
    if(mpro->m_wrAFyyYh & 2) {
      sint32 inputs = state->xzbchoice[XZBCHOICE_INPUTS];
      state->yychoice[YYCHOICE_Y_REG_H] = inputs >> 11;
      state->yychoice[YYCHOICE_Y_REG_L] = (inputs >> 4) & 0xFFF;
    }
    //
    // Shift of previous accumulator
    //
    shifted = state->xzbchoice[XZBCHOICE_ACC] << ((mpro->m_wrAFyyYh) & 1);
    if((mpro->__kisxzbon) & 0x20) {
      if(shifted > ( 0x7FFFFF)) { shifted = ( 0x7FFFFF); }
      if(shifted < (-0x800000)) { shifted = (-0x800000); }
    }
    //
    // Multiply and accumulate
    //
    state->xzbchoice[XZBCHOICE_ACC] = (sint32)((((sint64)x) * ((sint64)y)) >> 12) + b;
    //
    // Temp write
    //
    if(mpro->t_Twwwwwww < 0x80) {
      state->temp[((mpro->t_Twwwwwww)+(state->mdec_ct))&0x7F] = shifted;
    }
    //
    // Fractional address latch
    //
    if((mpro->m_wrAFyyYh) & 0x10) {
      if((mpro->__kisxzbon) & 0x40) {
        state->yychoice[YYCHOICE_FRC_REG] = shifted & 0xFFF;
      } else {
        state->yychoice[YYCHOICE_FRC_REG] = shifted >> 11;
      }
    }
    //
    // Memory operations
    //
    if((mpro->m_wrAFyyYh) & 0xC0) {
      sint32 tm = ((sint32)(mpro->tablemask));
      uint32 a = state->madrs[mpro->m_00aaaaaa];
      a += (state->adrs_reg) & ((sint32)(mpro->adrmask));
      a += (mpro->__kisxzbon) & 1;
      a += (state->mdec_ct) & (~tm);
      a &= (rbmask | tm) & 0xFFFF;
      a <<= 1;
      a += state->rbp;
      a &= (state->ram_mask);
      a ^= state->mem_word_address_xor;
      if(mpro->m_wrAFyyYh & 0x40) { // MRD
        sint32 memdata = *((sint16*)(((sint8*)(state->ram_ptr))+a));
        if(!(mpro->__kisxzbon & 2)) { memdata = float16_to_int24(memdata); }
        else { memdata <<= 8; }
        state->mem_in_data[(i+2)&3] = memdata;
      }
      if(mpro->m_wrAFyyYh & 0x80) { // MWT
        sint32 memdata = shifted;
        if(!(mpro->__kisxzbon & 2)) { memdata = int24_to_float16(memdata); }
        else { memdata >>= 8; }
        *((sint16*)(((sint8*)(state->ram_ptr))+a)) = memdata;
      }
    }
    //
    // Address latch
    //
    if((mpro->m_wrAFyyYh) & 0x20) {
      if((mpro->__kisxzbon) & 0x40) {
        state->adrs_reg = shifted >> 12;
      } else {
        state->adrs_reg = state->xzbchoice[XZBCHOICE_INPUTS] >> 16;
      }
      state->adrs_reg &= 0xFFF;
    }
    //
    // Effect output write
    //
    state->efreg[mpro->e_000Twwww] = shifted >> 8;
    // End of step
  }
}

/////////////////////////////////////////////////////////////////////////////
//
//
//

#define C(N) { *outp++ = ((uint8)(N)); }
#define C32(N) { *((uint32*)outp) = ((uint32)(N)); outp += 4; }
#define C32CALL(N) { *((uint32*)outp) = ((uint32)(N)) - (((uint32)(outp))+4); outp += 4; }

#define STRUCTOFS(thetype,thefield) ((uint32)(&(((struct thetype*)0)->thefield)))
#define STATEOFS(thefield) STRUCTOFS(YAM_STATE,thefield)

#ifdef ENABLE_DYNAREC
static int instruction_uses_shifted(struct MPRO *mpro) {
  // uses SHIFTED if:
  // - ADRL and INTERP
  if((mpro->m_wrAFyyYh & 0x20) != 0) {
    if((mpro->__kisxzbon & 0x40) != 0) return 1;
  }
  // - FRCL
  if((mpro->m_wrAFyyYh & 0x10) != 0) return 1;
  // - EWT
  if((mpro->e_000Twwww & 0x10) == 0) return 1;
  // - TWT
  if((mpro->t_Twwwwwww & 0x80) == 0) return 1;
  // - MWT
  if((mpro->m_wrAFyyYh & 0x80) != 0) return 1;
  // otherwise not
  return 0;
}
#endif

//
// Compile x86 code out of the current DSP program/coef/address set
// Also uses the current ringbuffer pointer and size, and ram pointer/mask/memwordxor
// So if any of those change, the compiled dynacode must be invalidated
//
#ifdef ENABLE_DYNAREC
static void dynacompile(struct YAM_STATE *state) {
  // Pre-compute ringbuffer size mask
  uint32 rbmask = (1 << ((state->rbl)+13)) - 1;

  uint8 *outp = state->dynacode;
  int i;
  char ins_uses_acc[129];
  char ins_uses_shifted[129];
  //
  // Put some slop here to avoid cache problems?
  //
  outp += DYNACODE_SLOP_SIZE;
  //
  // Figure out which instructions need what things
  //
  memset(ins_uses_acc, 0, sizeof(ins_uses_acc));
  memset(ins_uses_shifted, 0, sizeof(ins_uses_shifted));
  ins_uses_acc[128] = 1;
  ins_uses_shifted[128] = 1;
  for(i = 0; i < 128; i++) {
    struct MPRO *mpro = state->mpro + i;
    ins_uses_shifted[i] = instruction_uses_shifted(mpro);
    ins_uses_acc[i] =
      (ins_uses_shifted[i]) ||
      ((mpro->__kisxzbon & 0x0C) == 0x04);
  }

  //
  // Prefix
  //
  C(0x60)                                                 // pusha
  C(0x89) C(0xCF)                                         // mov edi, ecx
  C(0x8B) C(0xAF) C32(STATEOFS(mdec_ct))                  // mov ebp,[edi+<OFS32:mdec_ct>]
  C(0x8B) C(0xB7) C32(STATEOFS(xzbchoice[XZBCHOICE_ACC])) // mov esi,[edi+<OFS32:acc>]
  // 16 bytes
  //
  // Each instruction
  //
  for(i = 0; i < 128; i++) {
    struct MPRO *mpro = state->mpro + i;
    //
    // If we need to compute the new accumulator, do so (to EAX)
    //
    if(ins_uses_acc[i + 1]) {
      int need_tra =
        ((mpro->__kisxzbon & 0x10) == 0x00) ||
        ((mpro->__kisxzbon & 0x0C) == 0x00);
      //
      // If we will need TRA in the future, compute it in ECX
      //
      if(need_tra) {
        C(0x8D) C(0x4D) C(mpro->t_0rrrrrrr) // lea ecx,[ebp+<BYTE:TRA>]
        C(0x83) C(0xE1) C(0x7F)             // and ecx,7Fh
      }
      // 6 bytes max
      //
      // Load EAX with the Y value
      //
      switch(mpro->m_wrAFyyYh & 0x0C) {
      case 0x00: // FRC_REG
        C(0x8B) C(0x87) C32(STATEOFS(yychoice[YYCHOICE_FRC_REG])) // mov eax,[edi+yychoice0]
        break;
      case 0x04: // COEF
        { sint32 coef = state->coef[mpro->c_0rrrrrrr];
          C(0xB8) C32(coef)                                       // mov eax,<SINT32:COEF>
        }
        break;
      case 0x08: // Y_REG_H
        C(0x8B) C(0x87) C32(STATEOFS(yychoice[YYCHOICE_Y_REG_H])) // mov eax,[edi+yychoice2]
        break;
      case 0x0C: // Y_REG_L
        C(0x8B) C(0x87) C32(STATEOFS(yychoice[YYCHOICE_Y_REG_L])) // mov eax,[edi+yychoice3]
        break;
      }
      // 6 bytes max
      //
      // Multiply by the X value
      //
      if((mpro->__kisxzbon & 0x10) == 0) {
        C(0xF7) C(0xAC) C(0x8F) C32(STATEOFS(temp))             // imul dword ptr [edi+ecx*4+temp]
      } else {
        C(0xF7) C(0xAF) C32(STATEOFS(inputs[mpro->i_00rrrrrr])) // imul dword ptr [edi+<OFS32:INPUTS+4*IRA>]
      }
      C(0x0F) C(0xAC) C(0xD0) C(0x0C) // shrd eax,edx,12
      // 11 bytes max
      //
      // Add B if necessary
      //
      if((mpro->__kisxzbon & 0x08) == 0) {
        if(mpro->negb == 0) {
          if((mpro->__kisxzbon & 0x04) == 0) {
            C(0x03) C(0x84) C(0x8F) C32(STATEOFS(temp)) // add eax,[edi+ecx*4+<OFS32:temp>]
          } else {
            C(0x01) C(0xF0)                             // add eax,esi
          }
        } else {
          if((mpro->__kisxzbon & 0x04) == 0) {
            C(0x2B) C(0x84) C(0x8F) C32(STATEOFS(temp)) // sub eax,[edi+ecx*4+<OFS32:temp>]
          } else {
            C(0x29) C(0xF0)                             // sub eax,esi
          }
        }
      }
      // 7 bytes max
    }
    // 30 bytes max
    //
    // If YRL is on, latch Y register
    //
    if(mpro->m_wrAFyyYh & 2) {
      C(0x8B) C(0x97) C32(STATEOFS(inputs[mpro->i_00rrrrrr]))   // mov edx, [edi+<OFS32:INPUTS+4*IRA>]
      C(0xC1) C(0xFA) C(0x0B)                                   // sar edx,11
      C(0x89) C(0x97) C32(STATEOFS(yychoice[YYCHOICE_Y_REG_H])) // mov [edi+<OFS32:yychoice2>],edx
      C(0x8B) C(0x97) C32(STATEOFS(inputs[mpro->i_00rrrrrr]))   // mov edx, [edi+<OFS32:INPUTS+4*IRA>]
      C(0xC1) C(0xFA) C(0x04)                                   // sar edx,4
      C(0x81) C(0xE2) C32(0x00000FFF)                           // and edx,0FFFh
      C(0x89) C(0x97) C32(STATEOFS(yychoice[YYCHOICE_Y_REG_L])) // mov [edi+<OFS32:yychoice3>],edx
    }
    // 36 bytes max
    //
    // If we will be needing SHIFTED this instruction, edx will become SHIFTED:
    //
    if(ins_uses_shifted[i]) {
      if((mpro->__kisxzbon & 0x20) == 0) { // no saturate
        C(0x89) C(0xF2)                             // mov edx,esi
        C(0xC1) C(0xE2) C(8+(mpro->m_wrAFyyYh & 1)) // shl edx,<BYTE:8+sh>
        C(0xC1) C(0xFA) C(0x08)                     // sar edx,8
        // 8 bytes max
      } else { // saturate
        if((mpro->m_wrAFyyYh & 1) == 0) { // NOT shifting left
          C(0x8D) C(0x96) C32(0x00800000)         // lea edx,[esi+800000h]
          C(0xF7) C(0xC2) C32(0xFF000000)         // test edx,0FF000000h
          C(0x89) C(0xF2)                         // mov edx,esi
          // 14 bytes max
        } else { // shifting left
          C(0x8D) C(0x94) C(0x36) C32(0x00800000) // lea edx,[esi+esi+800000h]
          C(0xF7) C(0xC2) C32(0xFF000000)         // test edx,0FF000000h
          C(0x8D) C(0x14) C(0x36)                 // lea edx,[esi+esi]
          // 16 bytes max
        }
        C(0x74) C(0x09)                 // je +9bytes
        C(0xC1) C(0xFA) C(0x1F)         // sar edx,1Fh
        C(0x81) C(0xF2) C32(0x007FFFFF) // xor edx,7FFFFFh
        // 27 bytes max
      }
    }
    // 27 bytes max
    //
    // If we need the accumulator next instruction, save it
    //
    if(ins_uses_acc[i + 1]) {
      C(0x89) C(0xC6) // mov esi,eax
    }
    // 2 bytes max
    //
    // If FRCL is set, latch it
    //
    if(mpro->m_wrAFyyYh & 0x10) {
      C(0x89) C(0xD0) //mov eax,edx
      if(mpro->__kisxzbon & 0x40) { // interpolate mode
        C(0x25) C32(0x00000FFF) // and eax,0FFFh
      } else { // non-interpolate mode
        C(0xC1) C(0xF8) C(0x0B) // sar eax,11
      }
      C(0x89) C(0x87) C32(STATEOFS(yychoice[YYCHOICE_FRC_REG])) // mov [edi+<OFS32:yychoice0>],eax
    }
    // 13 bytes max
    //
    // If TWT is on, perform the temp write of SHIFTED
    //
    if((mpro->t_Twwwwwww & 0x80) == 0) {
      C(0x8D) C(0x4D) C(mpro->t_Twwwwwww)         // lea ecx,[ebp+<BYTE:TWA>]
      C(0x83) C(0xE1) C(0x7F)                     // and ecx,7Fh
      C(0x89) C(0x94) C(0x8F) C32(STATEOFS(temp)) // mov [edi+ecx*4+<OFS32:temp>],edx
    }
    // 13 bytes max
    //
    // If EWT is on, perform write of EFREG
    //
    if((mpro->e_000Twwww & 0x10) == 0) {
      C(0x89) C(0xD0)                                        // mov eax,edx
      C(0xC1) C(0xF8) C(0x08)                                // sar eax,8
      C(0x89) C(0x87) C32(STATEOFS(efreg[mpro->e_000Twwww])) // mov [edi+<OFS32:EFREG+4*EWA>],eax
    }
    // 11 bytes max
    //
    // If we'll be needing an address, compute it in EBX (a word address)
    // ODD LINES ONLY
    //
    if((i & 1) && (mpro->m_wrAFyyYh & 0xC0)) {
      uint32 madrsnx = state->madrs[mpro->m_00aaaaaa];
      if(mpro->__kisxzbon & 1) { madrsnx++; }
      madrsnx &= 0xFFFF;
      if(mpro->tablemask == 0) {
        C(0x8D) C(0x9D) C32(madrsnx)                       // lea ebx,[ebp+<DWORD:MADRS+NXADR>]
        if(mpro->adrmask != 0) {
          C(0x03) C(0x9F) C32(STATEOFS(adrs_reg))          // add ebx,[edi+<OFS32:adrs_reg>]
        }
        C(0x81) C(0xE3) C32(rbmask)                        // and ebx,<DWORD:rblmask>
        // 18 bytes max
      } else {
        C(0xBB) C32(madrsnx)                               // mov ebx,<DWORD:MADRS+NXADR masked by 0xFFFF>
        if(mpro->adrmask != 0) {
          C(0x03) C(0x9F) C32(STATEOFS(adrs_reg))          // add ebx,[edi+<OFS32:adrs_reg>]
          C(0x81) C(0xE3) C32(0x0000FFFF)                  // and ebx,0FFFFh
        }
        // 17 bytes max
      }
      C(0x81) C(0xC3) C32(state->rbp / 2)                  // add ebx,<DWORD:rbp/2>
      C(0x81) C(0xE3) C32(state->ram_mask / 2)             // and ebx,<DWORD:RAMMASK/2>
      if((state->mem_word_address_xor / 2) != 0) {
        C(0x83) C(0xF3) C(state->mem_word_address_xor / 2) // xor ebx,<BYTE:memwxor/2>
      }
    }
    // 33 bytes max ODD LINES ONLY
    //
    // If ADRL is set, latch address reg
    //
    if(mpro->m_wrAFyyYh & 0x20) {
      if(mpro->__kisxzbon & 0x40) { // interpolate mode
        C(0x89) C(0xD0)                                         // mov eax,edx
        C(0xC1) C(0xF8) C(0x0C)                                 // sar eax,12
      } else {
        C(0x8B) C(0x87) C32(STATEOFS(inputs[mpro->i_00rrrrrr])) // mov eax,[edi+<OFS32:INPUTS+4*IRA>]
        C(0xC1) C(0xF8) C(0x10)                                 // sar eax,16
      }
      C(0x25) C32(0x00000FFF)                 // and eax,0FFFh
      C(0x89) C(0x87) C32(STATEOFS(adrs_reg)) // mov [edi+<OFS32:adrs_reg>],eax
    }
    // 20 bytes max
    //
    // If MRD is set, read from ebx*2:
    // ODD LINES ONLY
    //
    if((i & 1) && (mpro->m_wrAFyyYh & 0x40)) {
      if((mpro->__kisxzbon & 0x02) == 0) { // NOFL=0
        C(0x0F) C(0xBF) C(0x8C) C(0x1B) C32(state->ram_ptr) // movsx ecx, word ptr [ebx+ebx+<DWORD:RAMPTR>]
        C(0xE8) C32CALL(float16_to_int24)                   // call float16_to_int24
        // 13 bytes max
      } else { // NOFL=1:
        C(0x0F) C(0xBF) C(0x84) C(0x1B) C32(state->ram_ptr) // movsx eax, word ptr [ebx+ebx+<DWORD:RAMPTR>]
        C(0xC1) C(0xE0) C(0x08)                             // shl eax,8
        // 11 bytes max
      }
      C(0x89) C(0x87) C32(STATEOFS(mem_in_data[(i+2)&3]))   // mov [edi+<OFS32:meminptr>],eax
      // 19 bytes max
    //
    // Or, if MWT is set, write edx to ebx*2:
    // ODD LINES ONLY
    //
    } else if((i & 1) && (mpro->m_wrAFyyYh & 0x80)) {
      if((mpro->__kisxzbon & 0x02) == 0) { // NOFL=0
        C(0x89) C(0xD1)                                     // mov ecx,edx
        C(0xE8) C32CALL(int24_to_float16)                   // call int24_to_float16
        C(0x66) C(0x89) C(0x84) C(0x1B) C32(state->ram_ptr) // mov [ebx+ebx+<DWORD:RAMPTR>],ax
        // 15 bytes max
      } else { // NOFL=1:
        C(0xC1) C(0xFA) C(0x08)                             // sar edx,8
        C(0x66) C(0x89) C(0x94) C(0x1B) C32(state->ram_ptr) // mov [ebx+ebx+<DWORD:RAMPTR>],dx
        // 11 bytes max
      }
      // 15 bytes max
    }
    // 19 bytes max ODD LINES ONLY
    //
    // If IWT is on, perform input write
    // ODD LINES ONLY
    //
    if((i&1) && ((mpro->i_0T0wwwww & 0x40) == 0)) {
      C(0x8B) C(0x97) C32(STATEOFS(mem_in_data[i&3]))         // mov edx, [edi+<OFS32:memindata>]
      C(0x89) C(0x97) C32(STATEOFS(inputs[mpro->i_0T0wwwww])) // mov [edi+<OFS32:INPUTS+4*IWA>],edx
    }
    // 12 bytes max ODD LINES ONLY
  }
  //
  // Suffix
  //
  C(0x89) C(0xB7) C32(STATEOFS(xzbchoice[XZBCHOICE_ACC])) // mov [edi+<OFS32:acc>],esi
  C(0x61)                                                 // popa
  C(0xC3)                                                 // retn
  // 8 bytes
  //
  // Set valid flag
  //
  state->dsp_dyna_valid = 1;

//{FILE*f=fopen("C:\\Corlett\\yamdyna.bin","wb");if(f){fwrite(state->dynacode,1,0x6000,f);fclose(f);}}

}
#endif

/////////////////////////////////////////////////////////////////////////////

typedef void (__fastcall *dsp_sample_t)(struct YAM_STATE *state);

/////////////////////////////////////////////////////////////////////////////
//
// Render effects by emulating the DSP
//
static void render_effects(
  struct YAM_STATE *state,
  sint32 *fxbus,
  sint32 *out,
  uint32 samples
) {
  dsp_sample_t samplefunc;
  uint32 i, j;
  sint32 efvol_l[16];
  sint32 efvol_r[16];

#ifdef ENABLE_DYNAREC
  if(state->dsp_dyna_enabled) {
    if(!(state->dsp_dyna_valid)) {
      dynacompile(state);
    }
    samplefunc = (dsp_sample_t)(((uint8*)(state->dynacode)) + DYNACODE_SLOP_SIZE);
#else
  if (0) {
#endif
  } else {
    samplefunc = dsp_sample_interpret;
  }

  //
  // Determine what the effect out levels are, for left and right
  //
  for(j = 0; j < 16; j++) {
    convert_stereo_send_level(
      0x00,
      state->efsdl[j],
      (state->mono) ? 0 : state->efpan[j],
      efvol_l + j, efvol_r + j,
      state->version
    );
  }
  //
  // For every sample:
  //
  for(i = 0; i < samples; i++, fxbus += 16, out += 2) {
    //
    // Clip and copy fxbus inputs (20-bit, pre-promote to 24-bit)
    //
    for(j = 0; j < 16; j++) {
      sint32 t = fxbus[j];
      if(t < (-0x80000)) t = (-0x80000);
      if(t > ( 0x7FFFF)) t = ( 0x7FFFF);
      state->inputs[0x20 + j] = t << 4;
    }
    //
    // Execute one DSP sample
    //
    samplefunc(state);
    // Advance MDEC_CT
    state->mdec_ct--;
    //
    // Copy outputs out of EFREG, scale accordingly, and add to output
    //
    for(j = 0; j < 16; j++) if(state->efsdl[j]) {
      sint32 ef = (sint32)((sint16)(state->efreg[j]));
      out[0] += (ef*efvol_l[j]) >> SHIFT;
      out[1] += (ef*efvol_r[j]) >> SHIFT;
    }
  }

}

/////////////////////////////////////////////////////////////////////////////
//
// Must not render more than RENDERMAX samples at a time
//
struct render_priority 
{
  sint32 channel_number;
  sint32 priority_level;
};
int __cdecl render_priority_compare(const void * a, const void * b) {
  const struct render_priority *_a = (const struct render_priority *) a;
  const struct render_priority *_b = (const struct render_priority *) b;
  return _b->priority_level - _a->priority_level;
}
static void render(struct YAM_STATE *state, uint32 odometer, uint32 samples) {
  uint32 i, j;
  struct render_priority priority_list[64];
  sint32 outbuf[2*RENDERMAX];
  sint32 fxbus[16*RENDERMAX];
  sint32 *directout;
//  sint32 *fxout;
  sint16 *buf;
  uint32 nchannels;
  uint32 bufptr_base;
  int wantreverb = 0;
  if(!samples) return;
  buf = YAMSTATE->out_buf;
  directout = (buf && (state->dry_out_enabled)) ? outbuf : NULL;
  nchannels = ((YAMSTATE->version) == 1) ? 32 : 64;

//  st=odometer;
//if(odometer>=11*44100)dumpch(YAMSTATE,YAMSTATE->chan+11);
/*
if(odometer >= 11*44100) {
static int dumped=0;
if(!dumped) {
dumped=1;
{FILE*f=fopen("C:\\Corlett\\yamdump.ram","wb");
if(f){ 
fwrite(YAMSTATE->ram_ptr,0x200000,1,f);
fclose(f);
}
}
}
}
*/

//logstep(state,odometer);

  // figure out if we want reverb or not
  if(buf && (state->dsp_emulation_enabled)) {
    for(i = 0; i < 16; i++) { if(state->efsdl[i] != 0) break; }
    wantreverb = (i < 16);
  } else {
    wantreverb = 0;
  }
  if(buf) {
    memset(outbuf, 0, 4*2*samples);
    if(wantreverb) memset(fxbus, 0, 4*16*samples);
  }
  //
  // Figure out if any channels need to be rendered before others
  //
  for(i = 0; i < nchannels; i++) {
    priority_list[i].channel_number = i;
    priority_list[i].priority_level = 0;
  }
  if (state->version == 1) {
    for(i = 0; i < nchannels; i++) {
      struct YAM_CHAN *chan = state->chan + i;
	  sint32 priority_level = priority_list[i].priority_level + 1;
      if (chan->mdxsl) priority_list[(i+chan->mdxsl)&31].priority_level = priority_level;
      if (chan->mdysl) priority_list[(i+chan->mdysl)&31].priority_level = priority_level;
    }
    qsort(&priority_list, nchannels, sizeof(*priority_list), render_priority_compare);
  }
  bufptr_base = state->bufptr;
  //
  // Render each channel
  //
  for(i = 0; i < nchannels; i++) {
    struct YAM_CHAN *chan;
    j = priority_list[i].channel_number;
    chan = state->chan + j;
    state->bufptr = bufptr_base + j;
// is 11
    render_and_add_channel(state, chan, directout,
      wantreverb ? (fxbus + chan->dspchan) : NULL,
      odometer, samples
    );
  }
  state->bufptr = (bufptr_base + (32*samples)) & (32*RINGMAX-1);
  //
  // Emulate DSP effects if desired
  //
  if(wantreverb) { render_effects(state, fxbus, outbuf, samples); }
  //
  // Scale, clip and copy output
  //
  if(buf) {
    sint32 vol_l, vol_r;
    const sint32 shift = SHIFT + 3 + state->version;
    convert_stereo_send_level(0x00, state->mvol, 0x00, &vol_l, &vol_r, 2);
    for(i = 0; i < samples; i++) {
      sint32 l = outbuf[2 * i + 0];
      sint32 r = outbuf[2 * i + 1];
      l *= vol_l; l >>= shift;
      r *= vol_l; r >>= shift;
      l = ICLIP16(l);
      r = ICLIP16(r);
      buf[2 * i + 0] = l;
      buf[2 * i + 1] = r;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Flush all pending samples into the output buffer
//
void EMU_CALL yam_flush(void *state) {
//  return;
//printf("yam_flush(%up)",YAMSTATE->out_pending);

  for(;;) {
    uint32 n = YAMSTATE->out_pending;
    if(n < 1) { break; }
    if(n > RENDERMAX) { n = RENDERMAX; }
    render(YAMSTATE, YAMSTATE->odometer - YAMSTATE->out_pending, n);
    YAMSTATE->out_pending -= n;
    if(YAMSTATE->out_buf) { YAMSTATE->out_buf += 2 * n; }
  }
}

/////////////////////////////////////////////////////////////////////////////
//
// Prepare or unprepare dynacode buffer for execution
//
void EMU_CALL yam_prepare_dynacode(void *state) {
#ifdef ENABLE_DYNAREC
#ifdef _WIN32
  DWORD i;
  VirtualProtect( &YAMSTATE->dynacode, sizeof(YAMSTATE->dynacode), PAGE_EXECUTE_READWRITE, &i );
#elif defined(HAVE_MPROTECT)
  unsigned long startaddr = &YAMSTATE->dynacode;
  unsigned long length    = sizeof(YAMSTATE->dynacode);
  int           psize     = getpagesize();
  unsigned long addr      = ( startaddr & ~(psize - 1) );
  mprotect( (char *) addr, length + startaddr - addr + psize, PROT_READ | PROT_WRITE | PROT_EXEC );
#endif
#endif
}

void EMU_CALL yam_unprepare_dynacode(void *state) {
#ifdef ENABLE_DYNAREC
#ifdef _WIN32
  DWORD i;
  VirtualProtect( &YAMSTATE->dynacode, sizeof(YAMSTATE->dynacode), PAGE_READWRITE, &i );
#elif defined(HAVE_MPROTECT)
  unsigned long startaddr = &YAMSTATE->dynacode;
  unsigned long length    = sizeof(YAMSTATE->dynacode);
  int           psize     = getpagesize();
  unsigned long addr      = ( startaddr & ~(psize - 1) );
  mprotect( (char *) addr, length + startaddr - addr + psize, PROT_READ | PROT_WRITE );
#endif
#endif
}

/////////////////////////////////////////////////////////////////////////////
