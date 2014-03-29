/*
 ** FT2PLAY v0.35
 ** =============
 **
 ** C port of FastTracker II's replayer, by 8bitbubsy (Olav Sørensen)
 ** using the original pascal+asm source codes by Mr.H (Fredrik Huss) of Triton
 **
 ** This is by no means a piece of beautiful code, nor is it meant to be...
 ** It's just an accurate FastTracker II replayer port for people to enjoy.
 **
 **
 ** (extreme) non-FT2 extensions:
 ** - Max 127 channels (was 32)
 ** - Any amount-of-channels number (FT2 supports *even* numbers only)
 ** - Max 256 instruments (was 128)
 ** - Max 32 samples per instrument (was 16)
 ** - Max 1024 rows per pattern (was 256)
 ** - Stereo samples
 **
 ** These additions shouldn't break FT2 accuracy, unless the XM is malicious.
 **
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#include "resampler.h"

#include "ft2play.h"

#if defined(_MSC_VER) && !defined(inline)
#define inline __forceinline
#endif

#define USE_VOL_RAMP

enum
{
    IS_Vol            = 1,
    IS_Period         = 2,
    IS_NyTon          = 4
};


// *** STRUCTS *** (remember 1-byte alignment for header/loader structs)

#ifdef _MSC_VER
#pragma pack(push)
#pragma pack(1)
#endif
typedef struct SongHeaderTyp_t
{
    int32_t HeaderSize;
    uint16_t Len;
    uint16_t RepS;
    uint16_t AntChn;
    uint16_t AntPtn;
    uint16_t AntInstrs;
    uint16_t Flags;
    uint16_t DefTempo;
    uint16_t DefSpeed;
    uint8_t SongTab[256];
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
SongHeaderTyp;

typedef struct SampleHeaderTyp_t
{
    int32_t Len;
    int32_t RepS;
    int32_t RepL;
    uint8_t vol;
    int8_t Fine;
    uint8_t Typ;
    uint8_t Pan;
    int8_t RelTon;
    uint8_t Junk;
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
SampleHeaderTyp;

typedef struct InstrHeaderTyp_t
{
    int32_t InstrSize;
    uint8_t Typ;
    uint16_t AntSamp;
    int32_t SampleSize;
    uint8_t TA[96];
    int16_t EnvVP[12][2];
    int16_t EnvPP[12][2];
    uint8_t EnvVPAnt;
    uint8_t EnvPPAnt;
    uint8_t EnvVSust;
    uint8_t EnvVRepS;
    uint8_t EnvVRepE;
    uint8_t EnvPSust;
    uint8_t EnvPRepS;
    uint8_t EnvPRepE;
    uint8_t EnvVTyp;
    uint8_t EnvPTyp;
    uint8_t VibTyp;
    uint8_t VibSweep;
    uint8_t VibDepth;
    uint8_t VibRate;
    uint16_t FadeOut;
    
    SampleHeaderTyp Samp[32];
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
InstrHeaderTyp;

typedef struct PatternHeaderTyp_t
{
    int32_t PatternHeaderSize;
    uint8_t Typ;
    uint16_t PattLen;
    uint16_t DataLen;
}
#ifdef __GNUC__
__attribute__ ((packed))
#endif
PatternHeaderTyp;
#ifdef _MSC_VER
#pragma pack(pop)
#endif

typedef struct SongTyp_t
{
    uint16_t Len;
    uint16_t RepS;
    uint8_t AntChn;
    uint16_t AntPtn;
    uint16_t AntInstrs;
    int16_t SongPos;
    int16_t PattNr;
    int16_t PattPos;
    int16_t PattLen;
    uint16_t Speed;
    uint16_t Tempo;
    uint16_t InitSpeed;
    uint16_t InitTempo;
    int16_t GlobVol; // must be signed
    uint16_t Timer;
    uint8_t PattDelTime;
    uint8_t PattDelTime2;
    uint8_t PBreakFlag;
    uint8_t PBreakPos;
    uint8_t PosJumpFlag;
    uint8_t SongTab[256];
    uint16_t Ver;
    uint16_t startOrder;
} SongTyp;

typedef struct SampleTyp_t
{
    int32_t Len;
    int32_t RepS;
    int32_t RepL;
    uint8_t Vol;
    int8_t Fine;
    uint8_t Typ;
    uint8_t Pan;
    int8_t RelTon;
    uint8_t Junk;
    int8_t *Pek;
} SampleTyp;

typedef struct InstrTyp_t
{
    uint32_t SampleSize;
    uint8_t TA[96];
    int16_t EnvVP[12][2];
    int16_t EnvPP[12][2];
    uint8_t EnvVPAnt;
    uint8_t EnvPPAnt;
    uint8_t EnvVSust;
    uint8_t EnvVRepS;
    uint8_t EnvVRepE;
    uint8_t EnvPSust;
    uint8_t EnvPRepS;
    uint8_t EnvPRepE;
    uint8_t EnvVTyp;
    uint8_t EnvPTyp;
    uint8_t VibTyp;
    uint8_t VibSweep;
    uint8_t VibDepth;
    uint8_t VibRate;
    uint16_t FadeOut;
    
    SampleTyp Samp[32];
    
    uint16_t AntSamp;
} InstrTyp;

typedef struct StmTyp_t
{
    int8_t OutVol;       // must be signed
    int8_t RealVol;      // must be signed
    int8_t RelTonNr;     // must be signed
    int8_t FineTune;     // must be signed
    int16_t OutPan;      // must be signed
    int16_t RealPeriod;  // must be signed
    int32_t FadeOutAmp;  // must be signed
    int16_t EnvVIPValue; // must be signed
    int16_t EnvPIPValue; // must be signed
    uint8_t OldVol;
    uint8_t OldPan;
    uint16_t OutPeriod;
    uint8_t FinalPan;
    uint16_t FinalPeriod;
    uint8_t EnvSustainActive;
    uint16_t SmpStartPos;
    uint16_t InstrNr;
    uint16_t TonTyp;
    uint8_t EffTyp;
    uint8_t Eff;
    uint8_t SmpOffset;
    uint16_t WantPeriod;
    uint8_t WaveCtrl;
    uint8_t Status;
    uint8_t PortaDir;
    uint8_t GlissFunk;
    uint16_t PortaSpeed;
    uint8_t VibPos;
    uint8_t TremPos;
    uint8_t VibSpeed;
    uint8_t VibDepth;
    uint8_t TremSpeed;
    uint8_t TremDepth;
    uint8_t PattPos;
    uint8_t LoopCnt;
    uint8_t VolSlideSpeed;
    uint8_t FVolSlideUpSpeed;
    uint8_t FVolSlideDownSpeed;
    uint8_t FPortaUpSpeed;
    uint8_t FPortaDownSpeed;
    uint8_t EPortaUpSpeed;
    uint8_t EPortaDownSpeed;
    uint8_t PortaUpSpeed;
    uint8_t PortaDownSpeed;
    uint8_t RetrigSpeed;
    uint8_t RetrigCnt;
    uint8_t RetrigVol;
    uint8_t VolKolVol;
    uint8_t TonNr;
    uint16_t FadeOutSpeed;
    uint16_t EnvVCnt;
    uint8_t EnvVPos;
    uint16_t EnvVAmp;
    uint16_t EnvPCnt;
    uint8_t EnvPPos;
    uint16_t EnvPAmp;
    uint8_t EVibPos;
    uint16_t EVibAmp;
    uint16_t EVibSweep;
    uint8_t TremorSave;
    uint8_t TremorPos;
    uint8_t GlobVolSlideSpeed;
    uint8_t PanningSlideSpeed;
    uint8_t Nr;
    
    SampleTyp InstrOfs; // read only
    InstrTyp InstrSeg;  // read only
    
    float FinalVol;
} StmTyp;

typedef struct TonTyp_t
{
    uint8_t Ton;
    uint8_t Instr;
    uint8_t Vol;
    uint8_t EffTyp;
    uint8_t Eff;
} TonTyp;

typedef struct
{
    int8_t loopEnabled;
    int8_t sixteenBit;
    int8_t stereo;
    int8_t busy;
    const int8_t *sampleData;
    int8_t loopBidi;
    int8_t loopDir;
    int32_t sampleLength;
    int32_t sampleLoopEnd;
    int32_t samplePosition;
    int32_t sampleLoopLength;
    int8_t interpolating;
    
    float incRate;
    float frac;
    float volumeL;
    float volumeR;
    
#ifdef USE_VOL_RAMP
    float targetVolL;
    float targetVolR;
    float volDeltaL;
    float volDeltaR;
    int8_t rampTerminates;
#endif
} VOICE;

typedef struct
{
    uint8_t *_ptr;
    uintptr_t _cnt;
    uint8_t *_base;
    uintptr_t _bufsiz;
    int32_t _eof;
} MEM;

typedef struct
{
    int8_t  *VibSineTab;
    uint16_t PattLens[256];
    int16_t *Note2Period;
    int16_t *linearPeriods;
    int16_t *amigaPeriods;
    uint32_t *LogTab;
    int8_t   LinearFrqTab;
    int32_t soundBufferSize;
    uint32_t outputFreq;
    
    TonTyp   *NilPatternLine;
    TonTyp   *Patt[256];
    StmTyp    Stm[127];
    SongTyp   Song;
    InstrTyp *Instr[255 + 1];
#ifdef USE_VOL_RAMP
    VOICE voice[127*2];
    
    void *resampler[127*2*2];
#else
    VOICE voice[127];
    
    void *resampler[127*2];
#endif
    
    float *PanningTab;
    float f_outputFreq;
    
#ifdef USE_VOL_RAMP
    float f_samplesPerFrame;
    float f_samplesPerFrameSharp;
#endif
    
    // pre-initialized variables
    int8_t samplingInterpolation;//      = 1;
#ifdef USE_VOL_RAMP
    int8_t rampStyle;
#endif
    float *masterBufferL;//              = NULL;
    float *masterBufferR;//              = NULL;
    int32_t samplesLeft;//               = 0; // must be signed
    int8_t isMixing;//                   = 0;
    uint32_t samplesPerFrame;// = 882;
    
    // globally accessed
    int8_t ModuleLoaded;// = 0;
    int8_t Playing;//      = 0;
    uint8_t numChannels;// = 127;
    
    uint8_t muted[16];
    
    uint32_t loopCount;
    uint8_t playedOrder[8192];
} PLAYER;

enum { _soundBufferSize = 512 };

// FUNCTION DECLARATIONS

static MEM *mopen(const uint8_t *src, uintptr_t length);
static void mclose(MEM *buf);
//static intptr_t mtell(MEM *buf);
static size_t mread(void *buffer, size_t size, size_t count, MEM *buf);
//static size_t mwrite(const void *buffer, size_t size, size_t count, MEM *buf);
static int32_t meof(MEM *buf);
static void mseek(MEM *buf, intptr_t offset, int32_t whence);
static void setSamplesPerFrame(PLAYER *, uint32_t val);
static void voiceSetSource(PLAYER *, uint8_t i, const int8_t *sampleData,
                           int32_t sampleLength,  int32_t sampleLoopLength,
                           int32_t sampleLoopEnd, int8_t loopEnabled,
                           int8_t sixteenbit, int8_t stereo);
static void voiceSetSamplePosition(PLAYER *, uint8_t i, uint16_t value);
static void voiceSetVolume(PLAYER *, uint8_t i, float vol, uint8_t pan, uint8_t sharp);
static void voiceSetSamplingFrequency(PLAYER *, uint8_t i, float samplingFrequency);


// TABLES AND VARIABLES


static const uint16_t AmigaFinePeriod[12 * 8] =
{
    907,900,894,887,881,875,868,862,856,850,844,838,
    832,826,820,814,808,802,796,791,785,779,774,768,
    762,757,752,746,741,736,730,725,720,715,709,704,
    699,694,689,684,678,675,670,665,660,655,651,646,
    640,636,632,628,623,619,614,610,604,601,597,592,
    588,584,580,575,570,567,563,559,555,551,547,543,
    538,535,532,528,524,520,516,513,508,505,502,498,
    494,491,487,484,480,477,474,470,467,463,460,457
};

// This table'a data is so small that generating it makes no sense
static const uint8_t VibTab[32] =
{
    0,  24,  49, 74, 97,120,141,161,
    180,197,212,224,235,244,250,253,
    255,253,250,244,235,224,212,197,
    180,161,141,120, 97, 74, 49, 24
};

// CODE START

static inline void RetrigVolume(StmTyp *ch)
{
    ch->RealVol = ch->OldVol;
    ch->OutVol  = ch->OldVol;
    ch->OutPan  = ch->OldPan;
    ch->Status |= IS_Vol;
}

static void RetrigEnvelopeVibrato(StmTyp *ch)
{
    if (!(ch->WaveCtrl & 0x04)) ch->VibPos  = 0;
    if (!(ch->WaveCtrl & 0x40)) ch->TremPos = 0;
    
    ch->RetrigCnt = 0;
    ch->TremorPos = 0;
    
    ch->EnvSustainActive = 1;
    
    if (ch->InstrSeg.EnvVTyp & 1)
    {
        ch->EnvVCnt = 0xFFFF;
        ch->EnvVPos = 0;
    }
    
    if (ch->InstrSeg.EnvPTyp & 1)
    {
        ch->EnvPCnt = 0xFFFF;
        ch->EnvPPos = 0;
    }
    
    // FT2 doesn't check if fadeout is more than 32768
    ch->FadeOutSpeed = (int32_t)(ch->InstrSeg.FadeOut) << 1;
    ch->FadeOutAmp   = 65536;
    
    if (ch->InstrSeg.VibDepth != 0)
    {
        ch->EVibPos = 0;
        
        if (ch->InstrSeg.VibSweep != 0)
        {
            ch->EVibAmp   = 0;
            ch->EVibSweep = ((uint16_t)(ch->InstrSeg.VibDepth) << 8) / ch->InstrSeg.VibSweep;
        }
        else
        {
            ch->EVibAmp   = (uint16_t)(ch->InstrSeg.VibDepth) << 8;
            ch->EVibSweep = 0;
        }
    }
}

static void KeyOff(StmTyp *ch)
{
    ch->EnvSustainActive = 0;
    
    if (!(ch->InstrSeg.EnvPTyp & 1)) // yes, FT2 does this (!)
    {
        if (ch->EnvPCnt >= ch->InstrSeg.EnvPP[ch->EnvPPos][0])
            ch->EnvPCnt  = ch->InstrSeg.EnvPP[ch->EnvPPos][0] - 1;
    }
    
    if (ch->InstrSeg.EnvVTyp & 1)
    {
        if (ch->EnvVCnt >= ch->InstrSeg.EnvVP[ch->EnvVPos][0])
            ch->EnvVCnt  = ch->InstrSeg.EnvVP[ch->EnvVPos][0] - 1;
    }
    else
    {
        ch->RealVol = 0;
        ch->OutVol  = 0;
        ch->Status |= IS_Vol;
    }
}

static inline uint32_t GetFrequenceValue(PLAYER *p, uint16_t period)
{
    uint16_t index;
    
    if (!period) return (0);
    
    if (p->LinearFrqTab)
    {
        index = (12 * 192 * 4) - period;
        return (p->LogTab[index % 768] >> ((14 - (index / 768)) & 0x1F));
    }
    else
    {
        return ((1712 * 8363) / period);
    }
}

static void StartTone(PLAYER *p, uint8_t Ton, uint8_t EffTyp, uint8_t Eff, StmTyp *ch)
{
    SampleTyp *s;
    
    uint16_t tmpTon;
    uint8_t samp;
    
    // if we came from Rxy (retrig), we didn't check note (Ton) yet
    if (Ton == 97)
    {
        KeyOff(ch);
        return;
    }
    
    if (Ton == 0)
    {
        Ton = ch->TonNr;
        if (Ton == 0) return; // if still no note, return.
    }
    // ------------------------------------------------------------
    
    if (p->Instr[ch->InstrNr] != NULL)
        ch->InstrSeg = *p->Instr[ch->InstrNr];
    else
        ch->InstrSeg = *p->Instr[0]; // placeholder for invalid samples
    
    ch->TonNr    = Ton;
    samp         = ch->InstrSeg.TA[Ton - 1]; // & 0x0F;
    s            = &ch->InstrSeg.Samp[samp];
    ch->InstrOfs = *s;
    ch->RelTonNr = s->RelTon;
    
    Ton += ch->RelTonNr;
    if (Ton >= (12 * 10)) return;
    
    ch->OldVol = s->Vol;
    
    // FT2 doesn't do this, but we don't want to blow our eardrums
    // on malicious XMs...
    if (ch->OldVol > 64) ch->OldVol = 64;
    
    ch->OldPan = s->Pan;
    
    if ((EffTyp == 0x0E) && ((Eff & 0xF0) == 0x50))
        ch->FineTune = (int8_t)((Eff & 0x0F) << 4) - 128;
    else
        ch->FineTune = s->Fine;
    
    if (Ton > 0)
    {
        tmpTon = (((Ton - 1) & 0x00FF) << 4) + (((ch->FineTune / 8) + 16) & 0x00FF);
        
        if (tmpTon < ((12 * 10 * 16) + 16))
        {
            ch->RealPeriod = p->Note2Period[tmpTon];
            ch->OutPeriod  = ch->RealPeriod;
        }
    }
    
    ch->Status |= (IS_Period | IS_Vol | IS_NyTon);
    
    if (EffTyp == 9)
    {
        if (Eff != 0)
            ch->SmpOffset = ch->Eff;
        
        ch->SmpStartPos = (uint16_t)(ch->SmpOffset) << 8;
    }
    else
    {
        ch->SmpStartPos = 0;
    }
}

static void MultiRetrig(PLAYER *p, StmTyp *ch)
{
    uint8_t cnt;
    int16_t vol;
    int8_t cmd;
    
    cnt = ch->RetrigCnt + 1;
    if (cnt < ch->RetrigSpeed)
    {
        ch->RetrigCnt = cnt;
        return;
    }
    
    ch->RetrigCnt = 0;
    
    vol = ch->RealVol;
    cmd = ch->RetrigVol;
    
    if      (cmd == 0x01) vol  -= 1;
    else if (cmd == 0x02) vol  -= 2;
    else if (cmd == 0x03) vol  -= 4;
    else if (cmd == 0x04) vol  -= 8;
    else if (cmd == 0x05) vol  -= 16;
    else if (cmd == 0x06) vol   = (vol >> 1) + (vol >> 2) + (vol >> 3);
    else if (cmd == 0x07) vol >>= 1;
    // 0x08 is not handled, ignore it
    else if (cmd == 0x09) vol  += 1;
    else if (cmd == 0x0A) vol  += 2;
    else if (cmd == 0x0B) vol  += 4;
    else if (cmd == 0x0C) vol  += 8;
    else if (cmd == 0x0D) vol  += 16;
    else if (cmd == 0x0E) vol   = (vol >> 1) + vol;
    else if (cmd == 0x0F) vol  += vol;
    
    if      (vol <  0) vol =  0;
    else if (vol > 64) vol = 64;
    
    ch->RealVol = (int8_t)(vol);
    ch->OutVol  = (int8_t)(vol);
    
    if ((ch->VolKolVol >= 0x10) && (ch->VolKolVol <= 0x50))
    {
        ch->RealVol = ch->VolKolVol - 16;
        ch->OutVol  = ch->RealVol;
    }
    else if ((ch->VolKolVol >= 0xC0) && (ch->VolKolVol <= 0xCF))
    {
        ch->OutPan = (ch->VolKolVol & 0x0F) << 4;
    }
    
    StartTone(p, 0, 0, 0, ch);
}

static inline void GetNewNote(PLAYER *pl, StmTyp *ch, TonTyp *p)
{
    // TODO: Rewrite this, eliminate gotos and labels!
    //  ---> This way to GOTO Palace
    
    int8_t tmp;
    int8_t envUpdate;
    uint8_t tmpEff;
    uint8_t tmpEffHi;
    int16_t newEnvPos;
    int16_t envPos;
    uint16_t portaTmp;
    uint16_t i;
    uint16_t inst;
    
    ch->VolKolVol = p->Vol;
    tmp = ch->Eff;
    if (ch->EffTyp == 0) goto FrqTest2;
    if (ch->EffTyp == 4) goto FrqTest1;
    if (ch->EffTyp != 6) goto NoFrqReset;
FrqTest1:
    if ((p->EffTyp == 4) || (p->EffTyp == 6)) goto NoFrqReset;
    tmp = 1;
FrqTest2:
    if (tmp == 0) goto NoFrqReset;
    //FrqReset:
    ch->OutPeriod = ch->RealPeriod;
    ch->Status |= IS_Period;
NoFrqReset:
    ch->EffTyp = p->EffTyp;
    ch->Eff = p->Eff;
    ch->TonTyp = (p->Instr << 8) | p->Ton;
    
    // *** Nytt instrument ***
    inst = p->Instr;
    if (inst ==   0) goto NoNewInstr;
    if (inst <= 256) goto NewInstrOK;
    inst = 0;
    goto NoNewInstr;
NewInstrOK:
    ch->InstrNr = p->Instr;
NoNewInstr:
    
    // *** Kolla special effekter ***
    if (!((p->EffTyp == 0x0E) && ((p->Eff & 0xF0) == 0xD0))) goto NoNoteDelay;
    if ((p->Eff & 0x0F) == 0) goto SpecEffSlut;
    return;
NoNoteDelay:
    
    if (!((p->EffTyp == 0x0E) && ((p->Eff & 0xF0) == 0x90))) goto NoNoteRetrig;
    if ((p->Eff & 0x0F) == 0) goto ForceSetPeriod;
NoNoteRetrig:
SpecEffSlut:
    
    // *** Kolla tone portamento ***
    if ((ch->VolKolVol & 0xF0) == 0xF0)       goto V_SetTonePorta;
    if ((p->EffTyp == 3) || (p->EffTyp == 5)) goto SetTonePorta;
    if (p->EffTyp == 0x14)                    goto KeyOffCmd;
NoKeyOffCmd:
    goto SetPeriod;
DonePeriod:
    
    if (inst == 0) goto CheckEffects;
    RetrigVolume(ch);
    RetrigEnvelopeVibrato(ch);
    goto CheckEffects;
    
    // *** Ny ton ***
SetPeriod:
    if (p->Ton == 0) goto DonePeriod;
ForceSetPeriod:
    if (p->Ton != 97) goto NoKeyOff;
DoKeyOff:
    KeyOff(ch);
    
    if (inst == 0) goto CheckEffects;
    RetrigVolume(ch);
    goto CheckEffects;
    
NoKeyOff:
    StartTone(pl, p->Ton, p->EffTyp, p->Eff, ch);
    goto DonePeriod;
    
    // *** Key-off cmd ***
KeyOffCmd:
    if (p->Eff == 0) goto DoKeyOff;
    goto NoKeyOffCmd;
    
    // *** Toneporta ***
SetTonePorta:
    if ((p->EffTyp == 5) || (p->Eff == 0)) goto NoPortaSpeed;
    ch->PortaSpeed = (int16_t)(p->Eff) << 2;
NoPortaSpeed:
    goto FixTonePorta;
    
V_SetTonePorta:
    if ((ch->VolKolVol & 0x0F) == 0) goto V_NoPortaSpeed;
    ch->PortaSpeed = (int16_t)(ch->VolKolVol & 0x0F) << 6;
V_NoPortaSpeed:
    goto FixTonePorta;
FixTonePorta:
    if (p->Ton == 0) goto NoPortaFrq;
    if (p->Ton == 97) goto DoKeyOff;
    portaTmp = ((((p->Ton - 1) + ch->RelTonNr) & 0x00FF) << 4) + (((ch->FineTune / 8) + 16) & 0x00FF);
    if (portaTmp >= ((12 * 10 * 16) + 16)) goto NoPortaFrq;
    ch->WantPeriod = pl->Note2Period[portaTmp];
    if (ch->WantPeriod == ch->RealPeriod) goto NoPorta;
    if (ch->WantPeriod  < ch->RealPeriod) goto PortaUp;
    ch->PortaDir = 1;
    goto NoPortaFrq;
PortaUp:
    ch->PortaDir = 2;
    goto NoPortaFrq;
NoPorta:
    ch->PortaDir = 0;
NoPortaFrq:
    goto DonePeriod;
CheckEffects:
    
    
    // *** VOLUME COLUMN EFFECTS (TICK 0) ***
    
    
    // set volume
    if ((ch->VolKolVol >= 0x10) && (ch->VolKolVol <= 0x50))
    {
        ch->RealVol = ch->VolKolVol - 0x10;
        ch->OutVol  = ch->RealVol;
        ch->Status |= IS_Vol;
    }
    
    // fine volume slide down
    else if ((ch->VolKolVol & 0xF0) == 0x80)
    {
        ch->RealVol -= (ch->VolKolVol & 0x0F);
        if (ch->RealVol < 0) ch->RealVol = 0;
        
        ch->OutVol  = ch->RealVol;
        ch->Status |= IS_Vol;
    }
    
    // fine volume slide up
    else if ((ch->VolKolVol & 0xF0) == 0x90)
    {
        ch->RealVol += (ch->VolKolVol & 0x0F);
        if (ch->RealVol > 64) ch->RealVol = 64;
        
        ch->OutVol  = ch->RealVol;
        ch->Status |= IS_Vol;
    }
    
    // set vibrato speed
    else if ((ch->VolKolVol & 0xF0) == 0xA0)
        ch->VibSpeed = (ch->VolKolVol & 0x0F) << 2;
    
    // set panning
    else if ((ch->VolKolVol & 0xF0) == 0xC0)
    {
        ch->OutPan  = (ch->VolKolVol & 0x0F) << 4;
        ch->Status |= IS_Vol;
    }
    
    
    // *** MAIN EFFECTS (TICK 0) ***
    
    
    if ((ch->EffTyp == 0) && (ch->Eff == 0)) return;
    
    // 8xx - set panning
    if (ch->EffTyp == 8)
    {
        ch->OutPan  = ch->Eff;
        ch->Status |= IS_Vol;
    }
    
    // Bxx - position jump
    else if (ch->EffTyp == 11)
    {
        pl->Song.SongPos     = ch->Eff;
        pl->Song.SongPos    -= 1;
        
        pl->Song.PBreakPos   = 0;
        pl->Song.PosJumpFlag = 1;
    }
    
    // Cxx - set volume
    else if (ch->EffTyp == 12)
    {
        ch->RealVol = ch->Eff;
        if (ch->RealVol > 64) ch->RealVol = 64;
        
        ch->OutVol  = ch->RealVol;
        ch->Status |= IS_Vol;
    }
    
    // Dxx - pattern break
    else if (ch->EffTyp == 13)
    {
        pl->Song.PosJumpFlag = 1;
        
        tmpEff = ((ch->Eff >> 4) * 10) + (ch->Eff & 0x0F);
        if (tmpEff <= 63)
            pl->Song.PBreakPos = tmpEff;
        else
            pl->Song.PBreakPos = 0;
    }
    
    // Exx - E effects
    else if (ch->EffTyp == 14)
    {
        // E1x - fine period slide up
        if ((ch->Eff & 0xF0) == 0x10)
        {
            tmpEff = ch->Eff & 0x0F;
            if (tmpEff == 0) tmpEff = ch->FPortaUpSpeed;
            ch->FPortaUpSpeed = tmpEff;
            
            ch->RealPeriod -= ((int16_t)(tmpEff) << 2);
            if (ch->RealPeriod < 1) ch->RealPeriod = 1;
            
            ch->OutPeriod = ch->RealPeriod;
            ch->Status   |= IS_Period;
        }
        
        // E2x - fine period slide down
        else if ((ch->Eff & 0xF0) == 0x20)
        {
            tmpEff = ch->Eff & 0x0F;
            if (tmpEff == 0) tmpEff = ch->FPortaDownSpeed;
            ch->FPortaDownSpeed = tmpEff;
            
            ch->RealPeriod += ((int16_t)(tmpEff) << 2);
            if (ch->RealPeriod > (32000 - 1)) ch->RealPeriod = 32000 - 1;
            
            ch->OutPeriod = ch->RealPeriod;
            ch->Status   |= IS_Period;
        }
        
        // E3x - set glissando type
        else if ((ch->Eff & 0xF0) == 0x30) ch->GlissFunk = ch->Eff & 0x0F;
        
        // E4x - set vibrato waveform
        else if ((ch->Eff & 0xF0) == 0x40) ch->WaveCtrl = (ch->WaveCtrl & 0xF0) | (ch->Eff & 0x0F);
        
        // E5x (set finetune) is handled in StartTone();
        
        // E6x - pattern loop
        else if ((ch->Eff & 0xF0) == 0x60)
        {
            if (ch->Eff == 0x60) // E60, empty param
            {
                ch->PattPos = pl->Song.PattPos & 0x00FF;
            }
            else
            {
                if (ch->LoopCnt == 0)
                {
                    ch->LoopCnt     = ch->Eff & 0x0F;
                    pl->Song.PBreakPos  = ch->PattPos;
                    pl->Song.PBreakFlag = 1;
                }
                else
                {
                    ch->LoopCnt--;
                    if (ch->LoopCnt != 0)
                    {
                        pl->Song.PBreakPos  = ch->PattPos;
                        pl->Song.PBreakFlag = 1;
                    }
                }
                if (pl->Song.PBreakFlag && pl->Song.PBreakPos == 0)
                {
                    int32_t offset = pl->Song.SongPos / 8;
                    int32_t bit = 1 << (pl->Song.SongPos % 8);
                    pl->playedOrder[offset] &= ~bit;
                }
            }
        }
        
        // E7x - set tremolo waveform
        else if ((ch->Eff & 0xF0) == 0x70) ch->WaveCtrl = ((ch->Eff & 0x0F) << 4) | (ch->WaveCtrl & 0x0F);
        
        // EAx - fine volume slide up
        else if ((ch->Eff & 0xF0) == 0xA0)
        {
            tmpEff = ch->Eff & 0x0F;
            if (tmpEff == 0) tmpEff = ch->FVolSlideUpSpeed;
            ch->FVolSlideUpSpeed = tmpEff;
            
            ch->RealVol += tmpEff;
            if (ch->RealVol > 64) ch->RealVol = 64;
            
            ch->OutVol  = ch->RealVol;
            ch->Status |= IS_Vol;
        }
        
        // EBx - fine volume slide down
        else if ((ch->Eff & 0xF0) == 0xB0)
        {
            tmpEff = ch->Eff & 0x0F;
            if (tmpEff == 0) tmpEff = ch->FVolSlideDownSpeed;
            ch->FVolSlideDownSpeed = tmpEff;
            
            ch->RealVol -= tmpEff;
            if (ch->RealVol < 0) ch->RealVol = 0;
            
            ch->OutVol = ch->RealVol;
            ch->Status |= IS_Vol;
        }
        
        // ECx - note cut
        else if ((ch->Eff & 0xF0) == 0xC0)
        {
            if (ch->Eff == 0xC0) // empty param
            {
                ch->RealVol = 0;
                ch->OutVol  = 0;
                ch->Status |= IS_Vol;
            }
        }
        
        // EEx - pattern delay
        else if ((ch->Eff & 0xF0) == 0xE0)
        {
            if (pl->Song.PattDelTime2 == 0)
                pl->Song.PattDelTime = (ch->Eff & 0x0F) + 1;
        }
    }
    
    // Fxx - set speed/tempo
    else if (ch->EffTyp == 15)
    {
        if (ch->Eff >= 32)
        {
            pl->Song.Speed = ch->Eff;
            setSamplesPerFrame(pl, (pl->outputFreq * 5UL) / 2 / pl->Song.Speed);
        }
        else
        {
            // F00 makes sense for stopping the song in tracker,
            // but in a replayer let's make the song start over instead.
            if (ch->Eff == 0)
            {
                memset(pl->voice, 0, sizeof (pl->voice));

                pl->Song.PattPos     = 0;
                pl->Song.PBreakPos   = 0;
                pl->Song.PosJumpFlag = 0;
                pl->Song.SongPos     = pl->Song.startOrder;
                pl->Song.PattNr      = pl->Song.SongTab[pl->Song.SongPos];
                pl->Song.PattLen     = pl->PattLens[pl->Song.PattNr];
                pl->Song.Timer       = 1;
                pl->Song.Speed       = pl->Song.InitSpeed;
                pl->Song.Tempo       = pl->Song.InitTempo;
                pl->Song.GlobVol     = 64;
            }
            else
            {
                pl->Song.Tempo = ch->Eff;
                pl->Song.Timer = ch->Eff;
            }
        }
    }
    
    // Gxx - set global volume
    else if (ch->EffTyp == 16)
    {
        pl->Song.GlobVol = ch->Eff;
        if (pl->Song.GlobVol > 64) pl->Song.GlobVol = 64;
        
        for (i = 0; i < pl->Song.AntChn; ++i) pl->Stm[i].Status |= IS_Vol;
    }
    
    // Lxx - set vol and pan envelope position
    else if (ch->EffTyp == 21)
    {
        // *** VOLUME ENVELOPE ***
        if (ch->InstrSeg.EnvVTyp & 1)
        {
            ch->EnvVCnt = ch->Eff - 1;
            envPos      = 0;
            envUpdate   = 1;
            newEnvPos   = ch->Eff;
            
            if (ch->InstrSeg.EnvVPAnt > 1)
            {
                envPos++;
                for (i = 0; i < ch->InstrSeg.EnvVPAnt; ++i)
                {
                    if (newEnvPos < ch->InstrSeg.EnvVP[envPos][0])
                    {
                        envPos--;
                        newEnvPos -= ch->InstrSeg.EnvVP[envPos][0];
                        
                        if (newEnvPos == 0)
                        {
                            envUpdate = 0;
                            break;
                        }
                        
                        if (ch->InstrSeg.EnvVP[envPos + 1][0] <= ch->InstrSeg.EnvVP[envPos + 0][0])
                        {
                            envUpdate = 1;
                            break;
                        }
                        
                        ch->EnvVIPValue  = ch->InstrSeg.EnvVP[envPos + 1][1];
                        ch->EnvVIPValue -= ch->InstrSeg.EnvVP[envPos + 0][1];
                        ch->EnvVIPValue  = (ch->EnvVIPValue & 0x00FF) << 8;
                        
                        ch->EnvVIPValue/=(ch->InstrSeg.EnvVP[envPos+1][0]-ch->InstrSeg.EnvVP[envPos][0]);
                        ch->EnvVAmp=(ch->EnvVIPValue*(newEnvPos-1))+((ch->InstrSeg.EnvVP[envPos][1] & 0x00FF)<<8);
                        
                        envPos++;
                        
                        envUpdate = 0;
                        break;
                    }
                    
                    envPos++;
                }
                
                if (envUpdate) envPos--;
            }
            
            if (envUpdate)
            {
                ch->EnvVIPValue = 0;
                ch->EnvVAmp     = (ch->InstrSeg.EnvVP[envPos][1] & 0x00FF) << 8;
            }
            
            if (envPos >= ch->InstrSeg.EnvVPAnt) envPos = (int16_t)(ch->InstrSeg.EnvVPAnt) - 1;
            ch->EnvVPos = (envPos < 0) ? 0 : (uint8_t)(envPos);
        }
        
        // *** PANNING ENVELOPE ***
        if (ch->InstrSeg.EnvVTyp & 2) // probably an FT2 bug
        {
            ch->EnvPCnt = ch->Eff - 1;
            envPos      = 0;
            envUpdate   = 1;
            newEnvPos   = ch->Eff;
            
            if (ch->InstrSeg.EnvPPAnt > 1)
            {
                envPos++;
                for (i = 0; i < ch->InstrSeg.EnvPPAnt; ++i)
                {
                    if (newEnvPos < ch->InstrSeg.EnvPP[envPos][0])
                    {
                        envPos--;
                        newEnvPos -= ch->InstrSeg.EnvPP[envPos][0];
                        
                        if (newEnvPos == 0)
                        {
                            envUpdate = 0;
                            break;
                        }
                        
                        if (ch->InstrSeg.EnvPP[envPos + 1][0] <= ch->InstrSeg.EnvPP[envPos + 0][0])
                        {
                            envUpdate = 1;
                            break;
                        }
                        
                        ch->EnvPIPValue  = ch->InstrSeg.EnvPP[envPos + 1][1];
                        ch->EnvPIPValue -= ch->InstrSeg.EnvPP[envPos + 0][1];
                        ch->EnvPIPValue  = (ch->EnvPIPValue & 0x00FF) << 8;
                        
                        ch->EnvPIPValue/=(ch->InstrSeg.EnvPP[envPos+1][0]-ch->InstrSeg.EnvPP[envPos][0]);
                        ch->EnvPAmp=(ch->EnvPIPValue*(newEnvPos-1))+((ch->InstrSeg.EnvPP[envPos][1]&0x00FF)<<8);
                        
                        envPos++;
                        
                        envUpdate = 0;
                        break;
                    }
                    
                    envPos++;
                }
                
                if (envUpdate) envPos--;
            }
            
            if (envUpdate)
            {
                ch->EnvPIPValue = 0;
                ch->EnvPAmp     = (ch->InstrSeg.EnvPP[envPos][1] & 0x00FF) << 8;
            }
            
            if (envPos >= ch->InstrSeg.EnvPPAnt) envPos = (int16_t)(ch->InstrSeg.EnvPPAnt) - 1;
            ch->EnvPPos = (envPos < 0) ? 0 : (uint8_t)(envPos);
        }
    }
    
    // Rxy - note multi retrigger
    else if (ch->EffTyp == 27)
    {
        tmpEff = ch->Eff & 0x0F;
        if (tmpEff == 0) tmpEff = ch->RetrigSpeed;
        ch->RetrigSpeed = tmpEff;
        
        tmpEffHi = ch->Eff >> 4;
        if (tmpEffHi == 0) tmpEffHi = ch->RetrigVol;
        ch->RetrigVol = tmpEffHi;
        
        if (ch->VolKolVol == 0) MultiRetrig(pl, ch);
    }
    
    // X1x - extra fine period slide up
    else if ((ch->EffTyp == 33) && ((ch->Eff & 0xF0) == 0x10))
    {
        tmpEff = ch->Eff & 0x0F;
        if (tmpEff == 0) tmpEff = ch->EPortaUpSpeed;
        ch->EPortaUpSpeed = tmpEff;
        
        ch->RealPeriod -= tmpEff;
        if (ch->RealPeriod < 1) ch->RealPeriod = 1;
        
        ch->OutPeriod = ch->RealPeriod;
        ch->Status   |= IS_Period;
    }
    
    // X2x - extra fine period slide down
    else if ((ch->EffTyp == 33) && ((ch->Eff & 0xF0) == 0x20))
    {
        tmpEff = ch->Eff & 0x0F;
        if (tmpEff == 0) tmpEff = ch->EPortaDownSpeed;
        ch->EPortaDownSpeed = tmpEff;
        
        ch->RealPeriod += tmpEff;
        if (ch->RealPeriod > (32000 - 1)) ch->RealPeriod = 32000 - 1;
        
        ch->OutPeriod = ch->RealPeriod;
        ch->Status   |= IS_Period;
    }
}

static void FixaEnvelopeVibrato(PLAYER *p, StmTyp *ch)
{
    uint16_t envVal;
    uint8_t envPos;
    int8_t envInterpolateFlag;
    int8_t envDidInterpolate;
    uint8_t autoVibTmp;
    
    ch->Status |= IS_Vol;
    
    // *** FADEOUT ***
    if (ch->EnvSustainActive == 0)
    {
        ch->FadeOutAmp -= ch->FadeOutSpeed;
        if (ch->FadeOutAmp < 0)
        {
            ch->FadeOutAmp   = 0;
            ch->FadeOutSpeed = 0;
        }
    }
    
    // *** VOLUME ENVELOPE ***
    envInterpolateFlag = 1;
    envDidInterpolate  = 0;
    envVal             = 0;
    
    if (ch->InstrSeg.EnvVTyp & 1)
    {
        envPos = ch->EnvVPos;
        
        ch->EnvVCnt++;
        if (ch->EnvVCnt == ch->InstrSeg.EnvVP[envPos][0])
        {
            ch->EnvVAmp = (ch->InstrSeg.EnvVP[envPos][1] & 0x00FF) << 8;
            
            envPos++;
            if (ch->InstrSeg.EnvVTyp & 4)
            {
                envPos--;
                if (envPos == ch->InstrSeg.EnvVRepE)
                {
                    if (!(ch->InstrSeg.EnvVTyp&2)||(envPos!=ch->InstrSeg.EnvVSust)||ch->EnvSustainActive)
                    {
                        envPos      = ch->InstrSeg.EnvVRepS;
                        ch->EnvVCnt = ch->InstrSeg.EnvVP[envPos][0];
                        ch->EnvVAmp = (ch->InstrSeg.EnvVP[envPos][1] & 0x00FF) << 8;
                    }
                }
                envPos++;
            }
            
            ch->EnvVIPValue = 0;
            
            if (envPos < ch->InstrSeg.EnvVPAnt)
            {
                if ((ch->InstrSeg.EnvVTyp & 2) && ch->EnvSustainActive)
                {
                    envPos--;
                    if (envPos == ch->InstrSeg.EnvVSust)
                        envInterpolateFlag = 0;
                    else
                        envPos++;
                }
                
                if (envInterpolateFlag)
                {
                    ch->EnvVPos = envPos;
                    if (ch->InstrSeg.EnvVP[envPos - 0][0] > ch->InstrSeg.EnvVP[envPos - 1][0])
                    {
                        ch->EnvVIPValue  = ch->InstrSeg.EnvVP[envPos - 0][1];
                        ch->EnvVIPValue -= ch->InstrSeg.EnvVP[envPos - 1][1];
                        ch->EnvVIPValue  = (ch->EnvVIPValue & 0x00FF) << 8;
                        ch->EnvVIPValue /= (ch->InstrSeg.EnvVP[envPos][0] - ch->InstrSeg.EnvVP[envPos - 1][0]);
                        
                        envVal = ch->EnvVAmp;
                        envDidInterpolate = 1;
                    }
                }
            }
        }
        
        if (!envDidInterpolate)
        {
            ch->EnvVAmp += ch->EnvVIPValue;
            
            envVal = ch->EnvVAmp;
            if ((envVal & 0xFF00) > 0x4000)
            {
                ch->EnvVIPValue = 0;
                envVal = ((envVal & 0xFF00) > 0x8000) ? 0x0000 : 0x4000;
            }
        }
        
        ch->FinalVol  = (float)(ch->OutVol)     / 64.0f;
        ch->FinalVol *= (float)(ch->FadeOutAmp) / 65536.0f;
        ch->FinalVol *= (float)(envVal >> 8)    / 64.0f;
        ch->FinalVol *= (float)(p->Song.GlobVol)   / 64.0f;
    }
    else
    {
        ch->FinalVol  = (float)(ch->OutVol)     / 64.0f;
        ch->FinalVol *= (float)(ch->FadeOutAmp) / 65536.0f;
        ch->FinalVol *= (float)(p->Song.GlobVol)   / 64.0f;
    }
    
    // *** PANNING ENVELOPE ***
    envInterpolateFlag = 1;
    envDidInterpolate  = 0;
    envVal             = 0;
    
    if (ch->InstrSeg.EnvPTyp & 1)
    {
        envPos = ch->EnvPPos;
        
        ch->EnvPCnt++;
        if (ch->EnvPCnt == ch->InstrSeg.EnvPP[envPos][0])
        {
            ch->EnvPAmp = (ch->InstrSeg.EnvPP[envPos][1] & 0x00FF) << 8;
            
            envPos++;
            if (ch->InstrSeg.EnvPTyp & 4)
            {
                envPos--;
                if (envPos == ch->InstrSeg.EnvPRepE)
                {
                    if (!(ch->InstrSeg.EnvPTyp&2)||(envPos!=ch->InstrSeg.EnvPSust)||ch->EnvSustainActive)
                    {
                        envPos      = ch->InstrSeg.EnvPRepS;
                        ch->EnvPCnt = ch->InstrSeg.EnvPP[envPos][0];
                        ch->EnvPAmp = (ch->InstrSeg.EnvPP[envPos][1] & 0x00FF) << 8;
                    }
                }
                envPos++;
            }
            
            ch->EnvPIPValue = 0;
            
            if (envPos < ch->InstrSeg.EnvPPAnt)
            {
                if ((ch->InstrSeg.EnvPTyp & 2) && ch->EnvSustainActive)
                {
                    envPos--;
                    if (envPos == ch->InstrSeg.EnvPSust)
                        envInterpolateFlag = 0;
                    else
                        envPos++;
                }
                
                if (envInterpolateFlag)
                {
                    ch->EnvPPos = envPos;
                    if (ch->InstrSeg.EnvPP[envPos - 0][0] > ch->InstrSeg.EnvPP[envPos - 1][0])
                    {
                        ch->EnvPIPValue  = ch->InstrSeg.EnvPP[envPos - 0][1];
                        ch->EnvPIPValue -= ch->InstrSeg.EnvPP[envPos - 1][1];
                        ch->EnvPIPValue  = (ch->EnvPIPValue & 0x00FF) << 8;
                        ch->EnvPIPValue /= (ch->InstrSeg.EnvPP[envPos][0] - ch->InstrSeg.EnvPP[envPos - 1][0]);
                        
                        envVal = ch->EnvPAmp;
                        envDidInterpolate = 1;
                    }
                }
            }
        }
        
        if (!envDidInterpolate)
        {
            ch->EnvPAmp += ch->EnvPIPValue;
            
            envVal = ch->EnvPAmp;
            if ((envVal & 0xFF00) > 0x4000)
            {
                ch->EnvPIPValue = 0;
                envVal = ((envVal & 0xFF00) > 0x8000) ? 0x0000 : 0x4000;
            }
        }
        
        ch->FinalPan  = (uint8_t)(ch->OutPan);
        ch->FinalPan += (uint8_t)((((envVal >> 8) - 32) * (128 - abs(ch->OutPan - 128)) / 32));
    }
    else
    {
        ch->FinalPan = (uint8_t)(ch->OutPan);
    }
    
    // *** AUTO VIBRATO ***
    if (ch->InstrSeg.VibDepth != 0)
    {
        if (ch->EVibSweep != 0)
        {
            if (ch->EnvSustainActive)
            {
                ch->EVibAmp += ch->EVibSweep;
                if ((ch->EVibAmp >> 8) > ch->InstrSeg.VibDepth)
                {
                    ch->EVibAmp   = (uint16_t)(ch->InstrSeg.VibDepth) << 8;
                    ch->EVibSweep = 0;
                }
            }
        }
        
        autoVibTmp = ch->EVibPos;
        
        if      (ch->InstrSeg.VibTyp == 1) autoVibTmp = (autoVibTmp > 127) ? 192 : 64;
        else if (ch->InstrSeg.VibTyp == 2) autoVibTmp = (((((autoVibTmp >> 1) & 0x00FF) + 64) & 127) - 64) ^ -1;
        else if (ch->InstrSeg.VibTyp == 3) autoVibTmp = (((((autoVibTmp >> 1) & 0x00FF) + 64) & 127) - 64);
        
        ch->FinalPeriod = ((p->VibSineTab[autoVibTmp] * ch->EVibAmp) >> 14) + ch->OutPeriod;
        if (ch->FinalPeriod > (32000 - 1)) ch->FinalPeriod = 0; // Yes, FT2 zeroes it out
        
        ch->Status  |= IS_Period;
        ch->EVibPos += ch->InstrSeg.VibRate;
    }
    else
    {
        ch->FinalPeriod = ch->OutPeriod;
    }
}

static inline void GetNextPos(PLAYER *p)
{
    if (p->Song.Timer == 1)
    {
        p->Song.PattPos++;
        
        if (p->Song.PattDelTime != 0)
        {
            p->Song.PattDelTime2 = p->Song.PattDelTime;
            p->Song.PattDelTime  = 0;
        }
        
        if (p->Song.PattDelTime2 != 0)
        {
            p->Song.PattDelTime2--;
            if (p->Song.PattDelTime2 != 0) p->Song.PattPos--;
        }
        
        if (p->Song.PBreakFlag)
        {
            p->Song.PBreakFlag = 0;
            p->Song.PattPos    = p->Song.PBreakPos;
        }
        
        if ((p->Song.PattPos >= p->Song.PattLen) || p->Song.PosJumpFlag)
        {
            p->Song.PattPos     = p->Song.PBreakPos;
            p->Song.PBreakPos   = 0;
            p->Song.PosJumpFlag = 0;
            
            p->Song.SongPos++;
            if (p->Song.SongPos >= p->Song.Len) p->Song.SongPos = p->Song.RepS;
            
            p->Song.PattNr  = p->Song.SongTab[p->Song.SongPos];
            p->Song.PattLen = p->PattLens[p->Song.PattNr];
        }
        
        if (p->Song.PattPos == 0)
        {
            int32_t offset = p->Song.SongPos / 8;
            int32_t bit = 1 << (p->Song.SongPos % 8);
            if (p->playedOrder[offset] & bit)
            {
                p->loopCount++;
                memset(p->playedOrder, 0, sizeof(p->playedOrder));
            }
            p->playedOrder[offset] |= bit;
        }
    }
}

static int16_t RelocateTon(PLAYER *p, int16_t inPeriod, int8_t addNote, StmTyp *ch)
{
    int8_t i;
    int8_t fineTune;
    
    int32_t outPeriod; // is 32-bit for testing bit 17, for carry (adc/sbb)
    int16_t oldPeriod;
    int16_t addPeriod;
    
    oldPeriod = 0;
    addPeriod = (8 * 12 * 16) * 2;
    fineTune  = ((ch->FineTune / 8) + 16) * 2;
    
    for (i = 0; i < 8; ++i)
    {
        outPeriod = (((oldPeriod + addPeriod) >> 1) & 0xFFE0) + fineTune;
        if (outPeriod < fineTune) outPeriod += (1 << 8);
        
        if (((outPeriod - 16) >> 1) < ((12 * 10 * 16) + 16)) // non-FT2 security fix
        {
            if (inPeriod >= p->Note2Period[(outPeriod - 16) >> 1]) // 16-bit look-up, shift it down
            {
                outPeriod -= fineTune;
                if (outPeriod & 0x00010000) outPeriod = (outPeriod - (1 << 8)) & 0x0000FFE0;
                addPeriod = (int16_t)(outPeriod);
            }
            else
            {
                outPeriod -= fineTune;
                if (outPeriod & 0x00010000) outPeriod = (outPeriod - (1 << 8)) & 0x0000FFE0;
                oldPeriod = (int16_t)(outPeriod);
            }
        }
    }
    
    outPeriod = oldPeriod + fineTune;
    if (outPeriod < fineTune) outPeriod += (1 << 8);
    outPeriod += ((int16_t)(addNote) << 5);
    
    if (outPeriod >= ((((8 * 12 * 16) + 15) * 2) - 1)) outPeriod = ((8 * 12 * 16) + 15) * 2;
    return (p->Note2Period[outPeriod >> 1]); // 16-bit look-up, shift it down
}

static void TonePorta(PLAYER *p, StmTyp *ch)
{
    if (ch->PortaDir != 0)
    {
        if (ch->PortaDir > 1)
        {
            ch->RealPeriod -= ch->PortaSpeed;
            if (ch->RealPeriod <= ch->WantPeriod)
            {
                ch->PortaDir   = 1;
                ch->RealPeriod = ch->WantPeriod;
            }
        }
        else
        {
            ch->RealPeriod += ch->PortaSpeed;
            if (ch->RealPeriod >= ch->WantPeriod)
            {
                ch->PortaDir   = 1;
                ch->RealPeriod = ch->WantPeriod;
            }
        }
        
        if (ch->GlissFunk) // semi-tone slide flag
            ch->OutPeriod = RelocateTon(p, ch->RealPeriod, 0, ch);
        else
            ch->OutPeriod = ch->RealPeriod;
        
        ch->Status |= IS_Period;
    }
}

static void Volume(StmTyp *ch) // actually volume slide
{
    uint8_t tmpEff;
    
    tmpEff = ch->Eff;
    if (tmpEff == 0) tmpEff = ch->VolSlideSpeed;
    ch->VolSlideSpeed = tmpEff;
    
    if (!(tmpEff & 0xF0))
    {
        ch->RealVol -= tmpEff;
        if (ch->RealVol < 0) ch->RealVol = 0;
    }
    else
    {
        ch->RealVol += (tmpEff >> 4);
        if (ch->RealVol > 64) ch->RealVol = 64;
    }
    
    ch->OutVol  = ch->RealVol;
    ch->Status |= IS_Vol;
}

static void Vibrato2(StmTyp *ch)
{
    uint8_t tmpVibPos;
    int8_t tmpVibTyp;
    
    tmpVibPos = (ch->VibPos >> 2) & 0x1F;
    tmpVibTyp =  ch->WaveCtrl     & 0x03;
    
    if (tmpVibTyp == 0)
    {
        tmpVibPos = VibTab[tmpVibPos];
    }
    else if (tmpVibTyp == 1)
    {
        tmpVibPos <<= 3; // (0..31) * 8
        if (ch->VibPos >= 128) tmpVibPos ^= -1;
    }
    else
    {
        tmpVibPos = 255;
    }
    
    tmpVibPos = ((uint16_t)(tmpVibPos) * ch->VibDepth) >> 5;
    
    if (ch->VibPos >= 128) ch->OutPeriod = ch->RealPeriod - tmpVibPos;
    else                   ch->OutPeriod = ch->RealPeriod + tmpVibPos;
    
    ch->Status |= IS_Period;
    ch->VibPos += ch->VibSpeed;
}

static void Vibrato(StmTyp *ch)
{
    if (ch->Eff != 0)
    {
        if (ch->Eff & 0x0F) ch->VibDepth =  ch->Eff & 0x0F;
        if (ch->Eff & 0xF0) ch->VibSpeed = (ch->Eff & 0xF0) >> 2; // speed*4
    }
    
    Vibrato2(ch);
}

static inline void DoEffects(PLAYER *p, StmTyp *ch)
{
    uint8_t tmpEff;
    uint16_t i;
    
    uint8_t tremorData;
    uint8_t tremorSign;
    
    // *** VOLUME COLUMN EFFECTS (TICKS >0) ***
    
    // volume slide down
    if ((ch->VolKolVol & 0xF0) == 0x60)
    {
        ch->RealVol -= (ch->VolKolVol & 0x0F);
        if (ch->RealVol < 0) ch->RealVol = 0;
        
        ch->OutVol  = ch->RealVol;
        ch->Status |= IS_Vol;
    }
    
    // volume slide up
    else if ((ch->VolKolVol & 0xF0) == 0x70)
    {
        ch->RealVol += (ch->VolKolVol & 0x0F);
        if (ch->RealVol > 64) ch->RealVol = 64;
        
        ch->OutVol  = ch->RealVol;
        ch->Status |= IS_Vol;
    }
    
    // vibrato (+ set vibrato depth)
    else if ((ch->VolKolVol & 0xF0) == 0xB0)
    {
        if (ch->VolKolVol != 0xB0)
            ch->VibDepth = ch->VolKolVol & 0x0F;
        
        Vibrato2(ch);
    }
    
    // pan slide left
    else if ((ch->VolKolVol & 0xF0) == 0xD0)
    {
        ch->OutPan -= (ch->VolKolVol & 0x0F);
        if (ch->OutPan < 0) ch->OutPan = 0;
        
        ch->Status |= IS_Vol;
    }
    
    // pan slide right
    else if ((ch->VolKolVol & 0xF0) == 0xE0)
    {
        ch->OutPan += (ch->VolKolVol & 0x0F);
        if (ch->OutPan > 255) ch->OutPan = 255;
        
        ch->Status |= IS_Vol;
    }
    
    // tone porta
    else if ((ch->VolKolVol & 0xF0) == 0xF0) TonePorta(p, ch);
    
    // *** MAIN EFFECTS (TICKS >0) ***
    
    if (((ch->Eff == 0) && (ch->EffTyp == 0)) || (ch->EffTyp >= 36)) return;
    
    // 0xy - Arpeggio
    if (ch->EffTyp == 0)
    {
        int8_t note;
        uint16_t tick;
        
        tick = p->Song.Timer;
        note = 0;
        
        // FT2 "out of boundary" arp LUT simulation
        if      (tick  > 16) tick  = 2;
        else if (tick == 15) tick  = 0;
        else                 tick %= 3;
        
        // this simulation doesn't work properly for >=128 tick arps.
        // but you'd need to hexedit the initial speed to get >31
        
        if (tick == 0)
        {
            ch->OutPeriod = ch->RealPeriod;
        }
        else
        {
            if      (tick == 1) note = ch->Eff >> 4;
            else if (tick == 2) note = ch->Eff & 0x0F;
            
            ch->OutPeriod = RelocateTon(p, ch->RealPeriod, note, ch);
        }
        
        ch->Status |= IS_Period;
    }
    
    // 1xx - period slide up
    else if (ch->EffTyp == 1)
    {
        tmpEff = ch->Eff;
        if (tmpEff == 0) tmpEff = ch->PortaUpSpeed;
        ch->PortaUpSpeed = tmpEff;
        
        ch->RealPeriod -= ((int16_t)(tmpEff) << 2);
        if (ch->RealPeriod < 1) ch->RealPeriod = 1;
        
        ch->OutPeriod = ch->RealPeriod;
        ch->Status   |= IS_Period;
    }
    
    // 2xx - period slide up
    else if (ch->EffTyp == 2)
    {
        tmpEff = ch->Eff;
        if (tmpEff == 0) tmpEff = ch->PortaUpSpeed;
        ch->PortaUpSpeed = tmpEff;
        
        ch->RealPeriod += ((int16_t)(tmpEff) << 2);
        if (ch->RealPeriod > (32000 - 1)) ch->RealPeriod = 32000 - 1;
        
        ch->OutPeriod = ch->RealPeriod;
        ch->Status   |= IS_Period;
    }
    
    // 3xx - tone portamento
    else if (ch->EffTyp == 3) TonePorta(p, ch);
    
    // 4xy - vibrato
    else if (ch->EffTyp == 4) Vibrato(ch);
    
    // 5xy - tone portamento + volume slide
    else if (ch->EffTyp == 5)
    {
        TonePorta(p, ch);
        Volume(ch);
    }
    
    // 6xy - vibrato + volume slide
    else if (ch->EffTyp == 6)
    {
        Vibrato2(ch);
        Volume(ch);
    }
    
    // 7xy - tremolo
    else if (ch->EffTyp == 7)
    {
        uint8_t tmpTremPos;
        int8_t tmpTremTyp;
        
        tmpEff = ch->Eff;
        if (tmpEff != 0)
        {
            if (tmpEff & 0x0F) ch->TremDepth =  tmpEff & 0x0F;
            if (tmpEff & 0xF0) ch->TremSpeed = (tmpEff & 0xF0) >> 2; // speed*4
        }
        
        tmpTremPos = (ch->TremPos  >> 2) & 0x1F;
        tmpTremTyp = (ch->WaveCtrl >> 4) & 0x03;
        
        if (tmpTremTyp == 0)
        {
            tmpTremPos = VibTab[tmpTremPos];
        }
        else if (tmpTremTyp == 1)
        {
            tmpTremPos <<= 3; // (0..31) * 8
            if (ch->VibPos >= 128) tmpTremPos ^= -1; // VibPos indeed, FT2 bug
        }
        else
        {
            tmpTremPos = 255;
        }
        
        tmpTremPos = (uint8_t)(((uint16_t)(tmpTremPos) * ch->TremDepth) >> 6);
        
        if (ch->TremPos >= 128)
        {
            ch->OutVol = ch->RealVol - tmpTremPos;
            if (ch->OutVol < 0) ch->OutVol = 0;
        }
        else
        {
            ch->OutVol = ch->RealVol + tmpTremPos;
            if (ch->OutVol > 64) ch->OutVol = 64;
        }
        
        ch->TremPos += ch->TremSpeed;
        
        ch->Status |= IS_Vol;
    }
    
    // Axy - volume slide
    else if (ch->EffTyp == 10) Volume(ch); // actually volume slide
    
    // Exy - E effects
    else if (ch->EffTyp == 14)
    {
        // E9x - note retrigger
        if ((ch->Eff & 0xF0) == 0x90)
        {
            if (ch->Eff != 0x90) // E90 is handled in GetNewNote();
            {
                if (((p->Song.Tempo - p->Song.Timer) % (ch->Eff & 0x0F)) == 0)
                {
                    StartTone(p, 0, 0, 0, ch);
                    RetrigEnvelopeVibrato(ch);
                }
            }
        }
        
        // ECx - note cut
        else if ((ch->Eff & 0xF0) == 0xC0)
        {
            if (((p->Song.Tempo - p->Song.Timer) & 0x00FF) == (ch->Eff & 0x0F))
            {
                ch->OutVol  = 0;
                ch->RealVol = 0;
                ch->Status |= IS_Vol;
            }
        }
        
        // EDx - note delay
        else if ((ch->Eff & 0xF0) == 0xD0)
        {
            if (((p->Song.Tempo - p->Song.Timer) & 0x00FF) == (ch->Eff & 0x0F))
            {
                StartTone(p, ch->TonTyp & 0x00FF, 0, 0, ch);
                
                if (ch->TonTyp & 0xFF00)
                    RetrigVolume(ch);
                
                RetrigEnvelopeVibrato(ch);
                
                if ((ch->VolKolVol >= 0x10) && (ch->VolKolVol <= 0x50))
                {
                    ch->OutVol  = ch->VolKolVol - 16;
                    ch->RealVol = ch->OutVol;
                }
                else if ((ch->VolKolVol >= 0xC0) && (ch->VolKolVol <= 0xCF))
                {
                    ch->OutPan = (ch->VolKolVol << 4) & 0x00FF;
                }
            }
        }
    }
    
    // Hxy - global volume slide
    else if (ch->EffTyp == 17)
    {
        tmpEff = ch->Eff;
        if (tmpEff == 0) tmpEff = ch->GlobVolSlideSpeed;
        ch->GlobVolSlideSpeed = tmpEff;
        
        if (!(tmpEff & 0xF0))
        {
            p->Song.GlobVol -= tmpEff;
            if (p->Song.GlobVol < 0) p->Song.GlobVol = 0;
        }
        else
        {
            p->Song.GlobVol += (tmpEff >> 4);
            if (p->Song.GlobVol > 64) p->Song.GlobVol = 64;
        }
        
        for (i = 0; i < p->Song.AntChn; ++i) p->Stm[i].Status |= IS_Vol;
    }
    
    // Kxx - key off
    else if (ch->EffTyp == 20)
    {
        if (((p->Song.Tempo - p->Song.Timer) & 31) == (ch->Eff & 0x0F))
            KeyOff(ch);
    }
    
    // Pxy - panning slide
    else if (ch->EffTyp == 25)
    {
        tmpEff = ch->Eff;
        if (tmpEff == 0) tmpEff = ch->PanningSlideSpeed;
        ch->PanningSlideSpeed = tmpEff;
        
        if (!(ch->Eff & 0xF0))
        {
            ch->OutPan += (ch->Eff >> 4);
            if (ch->OutPan > 255) ch->OutPan = 255;
        }
        else
        {
            ch->OutPan -= (ch->Eff & 0x0F);
            if (ch->OutPan < 0) ch->OutPan = 0;
        }
        
        ch->Status |= IS_Vol;
    }
    
    // Rxy - multi note retrig
    else if (ch->EffTyp == 27) MultiRetrig(p, ch);
    
    // Txy - tremor
    else if (ch->EffTyp == 29)
    {
        tmpEff = ch->Eff;
        if (tmpEff == 0) tmpEff = ch->TremorSave;
        ch->TremorSave = tmpEff;
        
        tremorSign = ch->TremorPos & 0x80;
        tremorData = ch->TremorPos & 0x7F;
        
        tremorData--;
        if (tremorData & 0x80)
        {
            if (tremorSign == 0x80)
            {
                tremorSign = 0x00;
                tremorData = tmpEff & 0x0F;
            }
            else
            {
                tremorSign = 0x80;
                tremorData = tmpEff >> 4;
            }
        }
        
        ch->TremorPos = tremorData | tremorSign;
        
        ch->OutVol  = tremorSign ? ch->RealVol : 0;
        ch->Status |= IS_Vol;
    }
}

static void MainPlayer(PLAYER *p) // periodically called from mixer
{
    StmTyp *ch;
    SampleTyp s;
    
    uint8_t i;
    int8_t tickzero;
    
    if (p->Playing)
    {
        tickzero = 0;
        
        p->Song.Timer--;
        if (p->Song.Timer == 0)
        {
            p->Song.Timer = p->Song.Tempo;
            tickzero   = 1;
        }
        
        if (tickzero)
        {
            if (p->Song.PattDelTime2 == 0)
            {
                for (i = 0; i < p->Song.AntChn; ++i)
                {
                    if (p->Patt[p->Song.PattNr] != NULL)
                        GetNewNote(p, &p->Stm[i], &p->Patt[p->Song.PattNr][(p->Song.PattPos * 127) + i]);
                    else
                        GetNewNote(p, &p->Stm[i], &p->NilPatternLine[(p->Song.PattPos * 127) + i]);
                    
                    FixaEnvelopeVibrato(p, &p->Stm[i]);
                }
            }
            else
            {
                for (i = 0; i < p->Song.AntChn; ++i)
                {
                    DoEffects(p, &p->Stm[i]);
                    FixaEnvelopeVibrato(p, &p->Stm[i]);
                }
            }
        }
        else
        {
            for (i = 0; i < p->Song.AntChn; ++i)
            {
                DoEffects(p, &p->Stm[i]);
                FixaEnvelopeVibrato(p, &p->Stm[i]);
            }
        }
        
        GetNextPos(p);
    }
    else
    {
        for (i = 0; i < p->Song.AntChn; ++i)
            FixaEnvelopeVibrato(p, &p->Stm[i]);
    }
    
    // update mixer
    for (i = 0; i < p->Song.AntChn; ++i)
    {
        ch = &p->Stm[i];

#ifdef USE_VOL_RAMP
        if ((ch->Status & (IS_Vol | (p->rampStyle > 0 ? IS_NyTon : 0))) == IS_Vol)
#else
        if (ch->Status & IS_Vol)
#endif
            voiceSetVolume(p, ch->Nr, ch->FinalVol, ch->FinalPan, 0);
        
        if (ch->Status & IS_Period)
            voiceSetSamplingFrequency(p, ch->Nr, (float)(GetFrequenceValue(p, ch->FinalPeriod)));
        
        if (ch->Status & IS_NyTon)
        {
#ifdef USE_VOL_RAMP
            if (p->rampStyle > 0)
            {
                p->voice[ch->Nr + 127] = p->voice[ch->Nr];
                voiceSetVolume(p, ch->Nr, ch->FinalVol, ch->FinalPan, 1);
                voiceSetVolume(p, ch->Nr + 127, 0, ch->FinalPan, 1);
                resampler_dup_inplace(p->resampler[ch->Nr + 127], p->resampler[ch->Nr]);
                resampler_dup_inplace(p->resampler[ch->Nr + 127 + 254], p->resampler[ch->Nr + 254]);
            }
#endif
            
            s = ch->InstrOfs;
            
            voiceSetSamplePosition(p, ch->Nr, ch->SmpStartPos);
            voiceSetSource(p, ch->Nr, s.Pek, s.Len, s.RepL, s.RepS + s.RepL, s.Typ & 3, s.Typ & 16, s.Typ & 32);
        }
        
        ch->Status = 0;
    }
}

static void StopVoices(PLAYER *p)
{
    uint8_t a;
    
    memset(p->voice, 0, sizeof (p->voice));
    
    for (a = 0; a < 127; ++a)
    {
        p->Stm[a].Nr       = a;
        p->Stm[a].TonTyp   = 0;
        p->Stm[a].RelTonNr = 0;
        p->Stm[a].InstrNr  = 0;
        p->Stm[a].InstrSeg = *p->Instr[0];
        p->Stm[a].Status   = IS_Vol;
        p->Stm[a].RealVol  = 0;
        p->Stm[a].OutVol   = 0;
        p->Stm[a].OldVol   = 0;
        p->Stm[a].OldPan   = 128;
        p->Stm[a].OutPan   = 128;
        p->Stm[a].VibDepth = 0;
        p->Stm[a].FinalVol = 0.0f;
        p->Stm[a].FinalPan = 128;
    }
}

static void SetPos(PLAYER *p, int16_t SongPos, int16_t PattPos)
{
    if (SongPos != -1)
    {
        p->Song.SongPos = SongPos;
        if ((p->Song.Len > 0) && (p->Song.SongPos >= p->Song.Len))
            p->Song.SongPos = p->Song.Len - 1;
        
        p->Song.PattNr  = p->Song.SongTab[SongPos];
        p->Song.PattLen = p->PattLens[p->Song.PattNr];
    }
    
    if (PattPos != -1)
    {
        p->Song.PattPos = PattPos;
        if (p->Song.PattPos >= p->Song.PattLen)
            p->Song.PattPos = p->Song.PattLen - 1;
    }
    
    p->Song.Timer = 1;
}

static void FreeInstr(PLAYER *pl, uint16_t ins)
{
    uint8_t i;
    InstrTyp *p;
    
    p = pl->Instr[ins];
    if (p == NULL) return;
    
    for (i = 0; i < 32; ++i)
    {
        if (p->Samp[i].Pek) free(p->Samp[i].Pek);
        p->Samp[i].Pek = NULL;
    }
    
    free(pl->Instr[ins]);
    pl->Instr[ins] = NULL;
}

static void FreeMusic(PLAYER *p)
{
    uint16_t a;
    
    for (a = 1; a < (255 + 1); ++a)
        FreeInstr(p, a);
    
    for (a = 0; a < 256; ++a)
    {
        if (p->Patt[a]) free(p->Patt[a]);
        
        p->Patt[a]     = NULL;
        p->PattLens[a] = 64;
    }
    
    memset(&p->Song, 0, sizeof (p->Song));
    
    p->Song.Len     = 1;
    p->Song.Tempo   = 6;
    p->Song.Speed   = 125;
    p->Song.Timer   = 1;
    p->Song.AntChn  = 127;
    p->LinearFrqTab = 1;
    
    StopVoices(p);
    SetPos(p, 0, 0);
}

static void Delta2Samp(int8_t *p, uint32_t len, uint8_t typ)
{
    uint32_t i;
    
    int16_t *p16;
    int16_t news16;
    int16_t olds16L;
    int16_t olds16R;
    
    int8_t *p8;
    int8_t news8;
    int8_t olds8L;
    int8_t olds8R;
    
    if (typ & 16) len >>= 1; // 16-bit
    if (typ & 32) len >>= 1; // stereo
    
    if (typ & 32)
    {
        if (typ & 16)
        {
            p16    = (int16_t *)(p);
            olds16L = 0;
            olds16R = 0;
            
            for (i = 0; i < len; ++i)
            {
                news16  = p16[i] + olds16L;
                p16[i]  = news16;
                olds16L = news16;
                
                news16       = p16[len + i] + olds16R;
                p16[len + i] = news16;
                olds16R      = news16;
            }
        }
        else
        {
            p8    = (int8_t *)(p);
            olds8L = 0;
            olds8R = 0;
            
            for (i = 0; i < len; ++i)
            {
                news8  = p8[i] + olds8L;
                p8[i]  = news8;
                olds8L = news8;
                
                news8       = p8[len + i] + olds8R;
                p8[len + i] = news8;
                olds8R      = news8;
            }
        }
    }
    else
    {
        if (typ & 16)
        {
            p16    = (int16_t *)(p);
            olds16L = 0;
            
            for (i = 0; i < len; ++i)
            {
                news16 = p16[i] + olds16L;
                p16[i] = news16;
                olds16L = news16;
            }
        }
        else
        {
            p8    = (int8_t *)(p);
            olds8L = 0;
            
            for (i = 0; i < len; ++i)
            {
                news8 = p8[i] + olds8L;
                p8[i] = news8;
                olds8L = news8;
            }
        }
    }
}


static void FreeAllInstr(PLAYER *p)
{
    uint16_t i;
    for (i = 1; i < (255 + 1); ++i) FreeInstr(p, i);
}

static int8_t AllocateInstr(PLAYER *pl, uint16_t i)
{
    InstrTyp *p;
    uint8_t j;
    
    if (pl->Instr[i] == NULL)
    {
        p = (InstrTyp *)(calloc(1, sizeof (InstrTyp)));
        if (p == NULL) return (0);
        
        for (j = 0; j < 32; ++j)
        {
            p->Samp[j].Pan = 128;
            p->Samp[j].Vol = 64;
        }
        
        pl->Instr[i] = p;
        
        return (1);
    }
    
    return (0);
}

static int8_t LoadInstrHeader(PLAYER *p, MEM *buf, uint16_t i)
{
    InstrHeaderTyp ih;
    uint8_t j;
    
    memset(&ih, 0, sizeof (ih));
    
    mread(&ih.InstrSize, 4, 1, buf);
    
    if (ih.InstrSize == 0) ih.InstrSize = 263; // default size
    ih.InstrSize -= (4 + 22); // skip instrsize and name
    
    mseek(buf, 22, SEEK_CUR); // skip name
    
    if (ih.InstrSize > 217)
    {
        mread(&ih.Typ, 217, 1, buf);
        mseek(buf, ih.InstrSize - 217, SEEK_CUR);
    }
    else
    {
        mread(&ih.Typ, ih.InstrSize, 1, buf);
    }
    
    if ((ih.SampleSize == 0) || (ih.SampleSize > 40)) ih.SampleSize = 40; // default
    ih.SampleSize -= (22); // skip name
    
    if (ih.AntSamp > 32) return (0);
    
    if (ih.AntSamp > 0)
    {
        if (AllocateInstr(p, i) == 0) return (0);
        
        memcpy(p->Instr[i]->TA, ih.TA, 212);
        p->Instr[i]->AntSamp = ih.AntSamp;
        
        for (j = 0; j < ih.AntSamp; ++j)
        {
            mread(&ih.Samp[j], 18, 1, buf);
            mseek(buf, 22, SEEK_CUR); // skip junk + name
            memcpy(&p->Instr[i]->Samp[j], &ih.Samp[j], 18);

            // non-FT2 fix: Force loop flags off if loop length is 0
            if (p->Instr[i]->Samp[j].RepL == 0)
                p->Instr[i]->Samp[j].Typ &= 0xFC;
        }
    }
    
    return (1);
}

static void CheckSampleRepeat(PLAYER *p, uint16_t ins, uint8_t smp)
{
    SampleTyp *s;
    
    if (p->Instr[ins])
    {
        s = &p->Instr[ins]->Samp[smp];
        
        if (s->RepS < 0)                  s->RepS = 0;
        if (s->RepL < 0)                  s->RepL = 0;
        if (s->RepS > s->Len)             s->RepS = s->Len;
        if ((s->RepS + s->RepL) > s->Len) s->RepL = s->Len - s->RepS;
    }
}

static inline int8_t get_adpcm_sample(const int8_t *sampleDictionary, const uint8_t *sampleData, int32_t samplePosition, int8_t *lastDelta)
{
    uint8_t byte = sampleData[samplePosition / 2];
    byte = (samplePosition & 1) ? byte >> 4 : byte & 15;
    return *lastDelta += sampleDictionary[byte];
}

static void Adpcm2Samp(uint8_t * sample, uint32_t length)
{
    const int8_t *sampleDictionary;
    const uint8_t *sampleData;

    uint32_t samplePosition;
    int8_t lastDelta;
    
    uint8_t * sampleDataOut = (uint8_t *) malloc(length);
    if (!sampleDataOut)
        return;
    
    sampleDictionary = (const int8_t *)sample;
    sampleData = (uint8_t*)sampleDictionary + 16;
    
    samplePosition = 0;
    lastDelta = 0;
    
    while (samplePosition < length)
    {
        sampleDataOut[samplePosition] = get_adpcm_sample(sampleDictionary, sampleData, samplePosition, &lastDelta);
        samplePosition++;
    }

    memcpy(sample, sampleDataOut, length);
}

static int8_t LoadInstrSample(PLAYER *p, MEM *buf, uint16_t i)
{
    uint16_t j;
    int32_t l;
    
    if (p->Instr[i])
    {
        for (j = 1; j <= p->Instr[i]->AntSamp; ++j)
        {
            int adpcm = 0;
            p->Instr[i]->Samp[j - 1].Pek = NULL;

            l = p->Instr[i]->Samp[j - 1].Len;
            if (p->Instr[i]->Samp[j - 1].Junk == 0xAD &&
                !(p->Instr[i]->Samp[j - 1].Typ & (16|32)))
                adpcm = (((l + 1) / 2) + 16);
            if (l > 0)
            {
                p->Instr[i]->Samp[j - 1].Pek = (int8_t *)(malloc(l));
                if (p->Instr[i]->Samp[j - 1].Pek == NULL)
                {
                    for (j = i; j <= p->Song.AntInstrs; ++j) FreeInstr(p, j);
                    return (0);
                }
                
                mread(p->Instr[i]->Samp[j - 1].Pek, adpcm ? adpcm : l, 1, buf);
                if (!adpcm)
                    Delta2Samp(p->Instr[i]->Samp[j - 1].Pek, l, p->Instr[i]->Samp[j - 1].Typ);
                else
                    Adpcm2Samp(p->Instr[i]->Samp[j - 1].Pek, l);
            }
            
            CheckSampleRepeat(p, i, (uint8_t)(j) - 1);
        }
    }
    
    return (1);
}

static void UnpackPatt(PLAYER *p, TonTyp *patdata, uint16_t length, uint16_t packlen, uint8_t *packdata)
{
    uint32_t patofs;
    
    uint16_t i;
    uint16_t packindex;
    
    uint8_t j;
    uint8_t packnote;
    
    packindex = 0;
    for (i = 0; i < length; ++i)
    {
        for (j = 0; j < p->Song.AntChn; ++j)
        {
            if (packindex >= packlen) return;
            
            patofs   = (i * 127) + j;
            packnote = packdata[packindex++];
            
            if (packnote & 0x80)
            {
                if (packnote & 0x01) patdata[patofs].Ton    = packdata[packindex++];
                if (packnote & 0x02) patdata[patofs].Instr  = packdata[packindex++];
                if (packnote & 0x04) patdata[patofs].Vol    = packdata[packindex++];
                if (packnote & 0x08) patdata[patofs].EffTyp = packdata[packindex++];
                if (packnote & 0x10) patdata[patofs].Eff    = packdata[packindex++];
            }
            else
            {
                patdata[patofs].Ton    = packnote;
                patdata[patofs].Instr  = packdata[packindex++];
                patdata[patofs].Vol    = packdata[packindex++];
                patdata[patofs].EffTyp = packdata[packindex++];
                patdata[patofs].Eff    = packdata[packindex++];
            }
        }
    }
}

static int8_t PatternEmpty(PLAYER *p, uint16_t nr)
{
    uint32_t patofs;
    uint16_t i;
    uint8_t j;
    
    if (p->Patt[nr] == NULL)
    {
        return (1);
    }
    else
    {
        for (i = 0; i < p->PattLens[nr]; ++i)
        {
            for (j = 0; j < p->Song.AntChn; ++j)
            {
                patofs = (i * 127) + j;
                
                if (p->Patt[nr][patofs].Ton)    return (0);
                if (p->Patt[nr][patofs].Instr)  return (0);
                if (p->Patt[nr][patofs].Vol)    return (0);
                if (p->Patt[nr][patofs].EffTyp) return (0);
                if (p->Patt[nr][patofs].Eff)    return (0);
            }
        }
    }
    
    return (1);
}

static int8_t LoadPatterns(PLAYER *p, MEM *buf)
{
    PatternHeaderTyp ph;
    uint8_t *patttmp;
    
    uint16_t i;
    uint8_t tmpLen;
    
    for (i = 0; i < p->Song.AntPtn; ++i)
    {
        mread(&ph.PatternHeaderSize, 4, 1, buf);
        mread(&ph.Typ, 1, 1, buf);
        
        ph.PattLen = 0;
        if (p->Song.Ver == 0x0102)
        {
            mread(&tmpLen, 1, 1, buf);
            ph.PattLen = (uint16_t)(tmpLen) + 1; // +1 in v1.02
        }
        else
        {
            mread(&ph.PattLen, 2, 1, buf);
        }
        
        mread(&ph.DataLen, 2, 1, buf);
        
        if (p->Song.Ver == 0x0102)
            mseek(buf, ph.PatternHeaderSize - 8, SEEK_CUR);
        else
            mseek(buf, ph.PatternHeaderSize - 9, SEEK_CUR);
        
        if (meof(buf))
        {
            mclose(buf);
            return (0);
        }
        
        p->PattLens[i] = ph.PattLen;
        if (ph.DataLen > 0)
        {
            p->Patt[i] = (TonTyp *)(calloc(sizeof (TonTyp), ph.PattLen * 127));
            if (p->Patt[i] == NULL)
            {
                mclose(buf);
                return (0);
            }
            
            patttmp = (uint8_t *)(malloc(ph.DataLen));
            if (patttmp == NULL)
            {
                mclose(buf);
                return (0);
            }
            
            mread(patttmp, ph.DataLen, 1, buf);
            UnpackPatt(p, p->Patt[i], ph.PattLen, ph.DataLen, patttmp);
            free(patttmp);
        }
        
        if (PatternEmpty(p, i))
        {
            if (p->Patt[i]) free(p->Patt[i]);
            
            p->Patt[i]     = NULL;
            p->PattLens[i] = 64;
        }
    }
    
    return (1);
}

static void UpDateInstrs(PLAYER *p)
{
    uint16_t i;
    uint8_t j;
    
    for (i = 0; i < (255 + 1); ++i)
    {
        if (p->Instr[i])
        {
            for (j = 0; j < 32; ++j)
            {
                CheckSampleRepeat(p, i, j);
                
                if (p->Instr[i]->Samp[j].Pek == NULL)
                {
                    p->Instr[i]->Samp[j].Len  = 0;
                    p->Instr[i]->Samp[j].RepS = 0;
                    p->Instr[i]->Samp[j].RepL = 0;
                }
            }
        }
    }
}

static void ft2play_FreeSong(void *_p)
{
    PLAYER * p = (PLAYER *)_p;
    
    p->Playing          = 0;
    
    memset(p->voice, 0, sizeof (p->voice));
    
    FreeMusic(p);
    
    p->ModuleLoaded = 0;
}

int8_t ft2play_LoadModule(void *_p, const uint8_t *buffer, size_t size)
{
    PLAYER *p = (PLAYER *)_p;
    MEM *buf;
    SongHeaderTyp h;
    char Sig[17];
    uint16_t Ver;
    
    uint16_t i;
    
    if (p->ModuleLoaded)
        ft2play_FreeSong(p);
    
    buf = mopen(buffer, size);
    if (buf == NULL) return (0);
    
    p->ModuleLoaded = 0;
    
    AllocateInstr(p, 0); // instr0 = placeholder for invalid ins
    p->Instr[0]->Samp[0].Vol = 0; // mute invalid instruments
    
    FreeMusic(p);
    p->LinearFrqTab = 0;
    
    // start loading
    
    mread(Sig, 1, 17, buf);
    if ((memcmp(Sig, "Extended Module: ", 17) != 0))
    {
        mclose(buf);
        return (0);
    }
    
    mseek(buf, 58, SEEK_SET);
    if (meof(buf))
    {
        mclose(buf);
        return (0);
    }
    
    mread(&Ver, 2, 1, buf);
    if ((Ver < 0x0102) || (Ver > 0x104))
    {
        mclose(buf);
        return (0);
    }
    
    mread(&h, sizeof (h), 1, buf);
    if (meof(buf))
    {
        mclose(buf);
        return (0);
    }
    
    if ((h.AntChn < 1) || (h.AntChn > 127) || (h.AntPtn > 256))
    {
        mclose(buf);
        return (0);
    }
    
    mseek(buf, 60 + h.HeaderSize, SEEK_SET);
    if (meof(buf))
    {
        mclose(buf);
        return (0);
    }
    
    p->Song.Len       = h.Len;
    p->Song.RepS      = h.RepS;
    p->Song.AntChn    = (uint8_t)(h.AntChn);
    p->Song.InitSpeed = h.DefSpeed ? h.DefSpeed : 125;
    p->Song.Speed     = p->Song.InitSpeed;
    p->Song.InitTempo = h.DefTempo ? h.DefTempo : 6;
    p->Song.Tempo     = p->Song.InitTempo;
    p->Song.AntInstrs = h.AntInstrs;
    p->Song.AntPtn    = h.AntPtn;
    p->Song.Ver       = Ver;
    p->LinearFrqTab   = h.Flags & 1;
    
    memcpy(p->Song.SongTab, h.SongTab, 256);
    
    if (p->Song.Ver < 0x0104)
    {
        for (i = 1; i <= h.AntInstrs; ++i)
        {
            if (LoadInstrHeader(p, buf, i) == 0)
            {
                FreeAllInstr(p);
                mclose(buf);
                return (0);
            }
        }
        
        if (LoadPatterns(p, buf) == 0)
        {
            FreeAllInstr(p);
            mclose(buf);
            return (0);
        }
        
        for (i = 1; i <= h.AntInstrs; ++i)
        {
            if (LoadInstrSample(p, buf, i) == 0)
            {
                FreeAllInstr(p);
                mclose(buf);
                return (0);
            }
        }
    }
    else
    {
        if (LoadPatterns(p, buf) == 0)
        {
            mclose(buf);
            return (0);
        }
        
        for (i = 1; i <= h.AntInstrs; ++i)
        {
            if (LoadInstrHeader(p, buf, i) == 0)
            {
                FreeInstr(p, i);
                mclose(buf);
                buf = NULL;
                break;
            }
            
            if (LoadInstrSample(p, buf, i) == 0)
            {
                mclose(buf);
                buf = NULL;
                break;
            }
        }
    }
    
    if (p->LinearFrqTab)
        p->Note2Period = p->linearPeriods;
    else
        p->Note2Period = p->amigaPeriods;
    
    mclose(buf);
    
    if (p->Song.RepS > p->Song.Len) p->Song.RepS = 0;
    
    StopVoices(p);
    SetPos(p, 0, 0);
    
    UpDateInstrs(p);
    
    p->ModuleLoaded = 1;
    
    return (1);
}

void voiceSetSource(PLAYER *p, uint8_t i, const int8_t *sampleData,
                    int32_t sampleLength,  int32_t sampleLoopLength,
                    int32_t sampleLoopEnd, int8_t loopEnabled,
                    int8_t sixteenbit, int8_t stereo)
{
    if (sixteenbit)
    {
        sampleLength     >>= 1;
        sampleLoopEnd    >>= 1;
        sampleLoopLength >>= 1;
    }
    
    if (stereo)
    {
        sampleLength     >>= 1;
        sampleLoopEnd    >>= 1;
        sampleLoopLength >>= 1;
    }
    
    p->voice[i].sampleData       = sampleData;
    p->voice[i].sampleLength     = sampleLength;
    p->voice[i].sampleLoopEnd    = sampleLoopEnd;
    p->voice[i].sampleLoopLength = sampleLoopLength;
    p->voice[i].loopBidi         = loopEnabled & 2;
    p->voice[i].loopEnabled      = loopEnabled;
    p->voice[i].sixteenBit       = sixteenbit;
    p->voice[i].loopDir          = 0;
    p->voice[i].stereo           = stereo;
    p->voice[i].interpolating    = 1;
#ifdef USE_VOL_RAMP
    p->voice[i].rampTerminates   = 0;
#endif
    
    resampler_clear(p->resampler[i]);
#ifdef USE_VOL_RAMP
    resampler_clear(p->resampler[i+254]);
#else
    resampler_clear(p->resampler[i+127]);
#endif
}

void voiceSetSamplePosition(PLAYER *p, uint8_t i, uint16_t value)
{
    p->voice[i].samplePosition   = value;
    if (p->voice[i].samplePosition >= p->voice[i].sampleLength)
    {
        p->voice[i].samplePosition = 0;
        p->voice[i].sampleData     = NULL;
    }
    
    p->voice[i].interpolating    = 1;
    
    resampler_clear(p->resampler[i]);
#ifdef USE_VOL_RAMP
    resampler_clear(p->resampler[i+254]);
#else
    resampler_clear(p->resampler[i+127]);
#endif
}

void voiceSetVolume(PLAYER *p, uint8_t i, float vol, uint8_t pan, uint8_t sharp)
{
#ifdef USE_VOL_RAMP
    if (p->rampStyle > 0 && !sharp)
    {
        if ((p->voice[i].volumeL == 0 && p->voice[i].volumeR == 0) || vol == 0)
            sharp = 3;
    }
    if (p->rampStyle > 1 || (p->rampStyle > 0 && sharp))
    {
        const float rampRate = sharp ? p->f_samplesPerFrameSharp : p->f_samplesPerFrame;
        if (sharp)
        {
            if (vol)
            {
                p->voice[i].volumeL = 0.0f;
                p->voice[i].volumeR = 0.0f;
            }
            else if (sharp != 3)
                p->voice[i].rampTerminates = 1;
        }
        
        p->voice[i].targetVolL = vol * p->PanningTab[256 - pan];
        p->voice[i].targetVolR = vol * p->PanningTab[      pan];
        p->voice[i].volDeltaL  = (p->voice[i].targetVolL - p->voice[i].volumeL) * rampRate;
        p->voice[i].volDeltaR  = (p->voice[i].targetVolR - p->voice[i].volumeR) * rampRate;
    }
    else
    {
        p->voice[i].volumeL = vol * p->PanningTab[256 - pan];
        p->voice[i].volumeR = vol * p->PanningTab[      pan];
        p->voice[i].targetVolL = p->voice[i].volumeL;
        p->voice[i].targetVolR = p->voice[i].volumeR;
        p->voice[i].volDeltaL = 0;
        p->voice[i].volDeltaR = 0;
    }
#else
    p->voice[i].volumeL = vol * p->PanningTab[256 - pan];
    p->voice[i].volumeR = vol * p->PanningTab[      pan];
#endif
}

void voiceSetSamplingFrequency(PLAYER *p, uint8_t i, float samplingFrequency)
{
    p->voice[i].incRate = samplingFrequency / p->f_outputFreq;
}

static inline void mix8b(PLAYER *p, uint32_t ch, uint32_t samples)
{
    uint32_t j;
    
    const int8_t *sampleData;
    
    float sample;
    float sampleL;
    float sampleR;
    
    int32_t sampleLength;
    int32_t sampleLoopEnd;
    int32_t sampleLoopLength;
    int32_t sampleLoopBegin;
    int32_t samplePosition;
    
    int8_t loopEnabled;
    int8_t loopBidi;
    int8_t loopDir;
    
    int32_t interpolating;
    
#ifdef USE_VOL_RAMP
    int32_t rampStyle = p->rampStyle;
#endif
    
    void * resampler;
    
    sampleLength     = p->voice[ch].sampleLength;
    sampleLoopLength = p->voice[ch].sampleLoopLength;
    sampleLoopEnd    = p->voice[ch].sampleLoopEnd;
    sampleLoopBegin  = sampleLoopEnd - sampleLoopLength;
    loopEnabled      = p->voice[ch].loopEnabled;
    loopBidi         = p->voice[ch].loopBidi;
    loopDir          = p->voice[ch].loopDir;
    interpolating    = p->voice[ch].interpolating;
    
    sampleData = p->voice[ch].sampleData;
    
    resampler = p->resampler[ch];
    
    resampler_set_rate(resampler, p->voice[ch].incRate );
    
    for (j = 0; (j < samples) && (p->voice[ch].sampleData != NULL); ++j)
    {
        p->voice[ch].busy = 1;
        samplePosition = p->voice[ch].samplePosition;
        
        while (interpolating && resampler_get_free_count(resampler))
        {
            resampler_write_sample(resampler, sampleData[samplePosition] * 256);
            
            if (loopDir == 1)
            {
                if (--samplePosition < 0)
                    samplePosition = 0;
            }
            else
            {
                ++samplePosition;
            }
            
            if (loopEnabled)
            {
                if (loopBidi)
                {
                    if (loopDir == 1)
                    {
                        if (samplePosition <= sampleLoopBegin)
                        {
                            samplePosition += (sampleLoopBegin - samplePosition) - 1;
                            loopDir = 0;
                        }
                    }
                    else
                    {
                        if (samplePosition >= sampleLoopEnd)
                        {
                            samplePosition -= (samplePosition - sampleLoopEnd) + 1;
                            loopDir = 1;
                        }
                    }
                }
                else
                {
                    if (samplePosition >= sampleLoopEnd)
                        samplePosition -= sampleLoopLength;
                }
            }
            else if (samplePosition >= sampleLength)
            {
                interpolating = 0;
                break;
            }
        }
        
        p->voice[ch].samplePosition  = samplePosition;
        p->voice[ch].loopDir         = loopDir;
        p->voice[ch].interpolating   = (int8_t)interpolating;
        
        if ( !resampler_ready(resampler) )
        {
            p->voice[ch].sampleData     = NULL;
            p->voice[ch].samplePosition = 0;
            p->voice[ch].busy           = 0;
            break;
        }
        
        sample = resampler_get_sample(resampler);
        resampler_remove_sample(resampler);
        
        sampleL = (sample * p->voice[ch].volumeL);
        sampleR = (sample * p->voice[ch].volumeR);
        
#ifdef USE_VOL_RAMP
        if (rampStyle > 0)
        {
            p->voice[ch].volumeL += p->voice[ch].volDeltaL;
            p->voice[ch].volumeR += p->voice[ch].volDeltaR;
        
            if (p->voice[ch].volDeltaL >= 0.0f)
            {
                if (p->voice[ch].volumeL > p->voice[ch].targetVolL)
                    p->voice[ch].volumeL = p->voice[ch].targetVolL;
            }
            else
            {
                if (p->voice[ch].volumeL < p->voice[ch].targetVolL)
                    p->voice[ch].volumeL = p->voice[ch].targetVolL;
            }
        
            if (p->voice[ch].volDeltaR >= 0.0f)
            {
            
                if (p->voice[ch].volumeR > p->voice[ch].targetVolR)
                    p->voice[ch].volumeR = p->voice[ch].targetVolR;
            }
            else
            {
                if (p->voice[ch].volumeR < p->voice[ch].targetVolR)
                    p->voice[ch].volumeR = p->voice[ch].targetVolR;
            }
        
            if (p->voice[ch].rampTerminates && !p->voice[ch].volumeL && !p->voice[ch].volumeR)
            {
                p->voice[ch].sampleData     = NULL;
                p->voice[ch].samplePosition = 0;
                p->voice[ch].busy           = 0;
            }
        }
#endif
        
        p->masterBufferL[j] += sampleL;
        p->masterBufferR[j] += sampleR;
    }
}

static inline void mix8bstereo(PLAYER *p, uint32_t ch, uint32_t samples)
{
    uint32_t j;
    
    const int8_t *sampleData;
    
    float sampleL;
    float sampleR;
    
    int32_t sampleLength;
    int32_t sampleLoopEnd;
    int32_t sampleLoopLength;
    int32_t sampleLoopBegin;
    int32_t samplePosition;
    
    int8_t loopEnabled;
    int8_t loopBidi;
    int8_t loopDir;
    
    int32_t interpolating;

#ifdef USE_VOL_RAMP
    int32_t rampStyle = p->rampStyle;
#endif
    
    void * resampler[2];
    
    sampleLength     = p->voice[ch].sampleLength;
    sampleLoopLength = p->voice[ch].sampleLoopLength;
    sampleLoopEnd    = p->voice[ch].sampleLoopEnd;
    sampleLoopBegin  = sampleLoopEnd - sampleLoopLength;
    loopEnabled      = p->voice[ch].loopEnabled;
    loopBidi         = p->voice[ch].loopBidi;
    loopDir          = p->voice[ch].loopDir;
    interpolating    = p->voice[ch].interpolating;
    
    sampleData = p->voice[ch].sampleData;
    
    resampler[0] = p->resampler[ch];
#ifdef USE_VOL_RAMP
    resampler[1] = p->resampler[ch+254];
#else
    resampler[1] = p->resampler[ch+127];
#endif
    
    resampler_set_rate(resampler[0], p->voice[ch].incRate );
    resampler_set_rate(resampler[1], p->voice[ch].incRate );
    
    for (j = 0; (j < samples) && (p->voice[ch].sampleData != NULL); ++j)
    {
        p->voice[ch].busy = 1;
        samplePosition = p->voice[ch].samplePosition;
        
        while (interpolating && resampler_get_free_count(resampler[0]))
        {
            resampler_write_sample(resampler[0], sampleData[samplePosition] * 256);
            resampler_write_sample(resampler[1], sampleData[sampleLength + samplePosition] * 256);
            
            if (loopDir == 1)
            {
                if (--samplePosition < 0)
                    samplePosition = 0;
            }
            else
            {
                ++samplePosition;
            }
            
            if (loopEnabled)
            {
                if (loopBidi)
                {
                    if (loopDir == 1)
                    {
                        if (samplePosition <= sampleLoopBegin)
                        {
                            samplePosition += (sampleLoopBegin - samplePosition) - 1;
                            loopDir = 0;
                        }
                    }
                    else
                    {
                        if (samplePosition >= sampleLoopEnd)
                        {
                            samplePosition -= (samplePosition - sampleLoopEnd) + 1;
                            loopDir = 1;
                        }
                    }
                }
                else
                {
                    if (samplePosition >= sampleLoopEnd)
                        samplePosition -= sampleLoopLength;
                }
            }
            else if (samplePosition >= sampleLength)
            {
                interpolating = 0;
                break;
            }
        }
        
        p->voice[ch].samplePosition  = samplePosition;
        p->voice[ch].loopDir         = loopDir;
        p->voice[ch].interpolating   = (int8_t)interpolating;
        
        if ( !resampler_ready(resampler[0]) )
        {
            p->voice[ch].sampleData     = NULL;
            p->voice[ch].samplePosition = 0;
            p->voice[ch].busy           = 0;
            break;
        }
        
        sampleL = resampler_get_sample(resampler[0]);
        sampleR = resampler_get_sample(resampler[1]);
        resampler_remove_sample(resampler[0]);
        resampler_remove_sample(resampler[1]);
        
        sampleL = (sampleL * p->voice[ch].volumeL);
        sampleR = (sampleR * p->voice[ch].volumeR);
        
#ifdef USE_VOL_RAMP
        if (rampStyle > 0)
        {
            p->voice[ch].volumeL += p->voice[ch].volDeltaL;
            p->voice[ch].volumeR += p->voice[ch].volDeltaR;
            
            if (p->voice[ch].volDeltaL >= 0.0f)
            {
                if (p->voice[ch].volumeL > p->voice[ch].targetVolL)
                    p->voice[ch].volumeL = p->voice[ch].targetVolL;
            }
            else
            {
                if (p->voice[ch].volumeL < p->voice[ch].targetVolL)
                    p->voice[ch].volumeL = p->voice[ch].targetVolL;
            }
            
            if (p->voice[ch].volDeltaR >= 0.0f)
            {
                
                if (p->voice[ch].volumeR > p->voice[ch].targetVolR)
                    p->voice[ch].volumeR = p->voice[ch].targetVolR;
            }
            else
            {
                if (p->voice[ch].volumeR < p->voice[ch].targetVolR)
                    p->voice[ch].volumeR = p->voice[ch].targetVolR;
            }
            
            if (p->voice[ch].rampTerminates && !p->voice[ch].volumeL && !p->voice[ch].volumeR)
            {
                p->voice[ch].sampleData     = NULL;
                p->voice[ch].samplePosition = 0;
                p->voice[ch].busy           = 0;
            }
        }
#endif
        
        p->masterBufferL[j] += sampleL;
        p->masterBufferR[j] += sampleR;
    }
}

static inline void mix16b(PLAYER *p, uint32_t ch, uint32_t samples)
{
    uint32_t j;
    
    const int16_t *sampleData;
    
    float sample;
    float sampleL;
    float sampleR;
    
    int32_t sampleLength;
    int32_t sampleLoopEnd;
    int32_t sampleLoopLength;
    int32_t sampleLoopBegin;
    int32_t samplePosition;
    
    int8_t loopEnabled;
    int8_t loopBidi;
    int8_t loopDir;
    
    int32_t interpolating;

#ifdef USE_VOL_RAMP
    int32_t rampStyle = p->rampStyle;
#endif
    
    void * resampler;
    
    sampleLength     = p->voice[ch].sampleLength;
    sampleLoopLength = p->voice[ch].sampleLoopLength;
    sampleLoopEnd    = p->voice[ch].sampleLoopEnd;
    sampleLoopBegin  = sampleLoopEnd - sampleLoopLength;
    loopEnabled      = p->voice[ch].loopEnabled;
    loopBidi         = p->voice[ch].loopBidi;
    loopDir          = p->voice[ch].loopDir;
    interpolating    = p->voice[ch].interpolating;
    
    sampleData = (const int16_t *)(p->voice[ch].sampleData);
    
    resampler = p->resampler[ch];
    
    resampler_set_rate(resampler, p->voice[ch].incRate );
    
    for (j = 0; (j < samples) && (p->voice[ch].sampleData != NULL); ++j)
    {
        p->voice[ch].busy = 1;
        samplePosition = p->voice[ch].samplePosition;
        
        while (interpolating && resampler_get_free_count(resampler))
        {
            resampler_write_sample(resampler, sampleData[samplePosition]);
            
            if (loopDir == 1)
            {
                if (--samplePosition < 0)
                    samplePosition = 0;
            }
            else
            {
                ++samplePosition;
            }
            
            if (loopEnabled)
            {
                if (loopBidi)
                {
                    if (loopDir == 1)
                    {
                        if (samplePosition <= sampleLoopBegin)
                        {
                            samplePosition += (sampleLoopBegin - samplePosition) - 1;
                            loopDir = 0;
                        }
                    }
                    else
                    {
                        if (samplePosition >= sampleLoopEnd)
                        {
                            samplePosition -= (samplePosition - sampleLoopEnd) + 1;
                            loopDir = 1;
                        }
                    }
                }
                else
                {
                    if (samplePosition >= sampleLoopEnd)
                        samplePosition -= sampleLoopLength;
                }
            }
            else if (samplePosition >= sampleLength)
            {
                interpolating = 0;
                break;
            }
        }
        
        p->voice[ch].samplePosition  = samplePosition;
        p->voice[ch].loopDir         = loopDir;
        p->voice[ch].interpolating   = (int8_t)interpolating;
        
        if ( !resampler_ready(resampler) )
        {
            p->voice[ch].sampleData     = NULL;
            p->voice[ch].samplePosition = 0;
            p->voice[ch].busy           = 0;
            break;
        }
        
        sample = resampler_get_sample(resampler);
        resampler_remove_sample(resampler);
        
        sampleL = (sample * p->voice[ch].volumeL);
        sampleR = (sample * p->voice[ch].volumeR);
        
#ifdef USE_VOL_RAMP
        if (rampStyle > 0)
        {
            p->voice[ch].volumeL += p->voice[ch].volDeltaL;
            p->voice[ch].volumeR += p->voice[ch].volDeltaR;
            
            if (p->voice[ch].volDeltaL >= 0.0f)
            {
                if (p->voice[ch].volumeL > p->voice[ch].targetVolL)
                    p->voice[ch].volumeL = p->voice[ch].targetVolL;
            }
            else
            {
                if (p->voice[ch].volumeL < p->voice[ch].targetVolL)
                    p->voice[ch].volumeL = p->voice[ch].targetVolL;
            }
            
            if (p->voice[ch].volDeltaR >= 0.0f)
            {
                
                if (p->voice[ch].volumeR > p->voice[ch].targetVolR)
                    p->voice[ch].volumeR = p->voice[ch].targetVolR;
            }
            else
            {
                if (p->voice[ch].volumeR < p->voice[ch].targetVolR)
                    p->voice[ch].volumeR = p->voice[ch].targetVolR;
            }
            
            if (p->voice[ch].rampTerminates && !p->voice[ch].volumeL && !p->voice[ch].volumeR)
            {
                p->voice[ch].sampleData     = NULL;
                p->voice[ch].samplePosition = 0;
                p->voice[ch].busy           = 0;
            }
        }
#endif
        
        p->masterBufferL[j] += sampleL;
        p->masterBufferR[j] += sampleR;
    }
}

static inline void mix16bstereo(PLAYER *p, uint32_t ch, uint32_t samples)
{
    uint32_t j;
    
    const int16_t *sampleData;
    
    float sampleL;
    float sampleR;
    
    int32_t sampleLength;
    int32_t sampleLoopEnd;
    int32_t sampleLoopLength;
    int32_t sampleLoopBegin;
    int32_t samplePosition;
    
    int8_t loopEnabled;
    int8_t loopBidi;
    int8_t loopDir;
    
    int32_t interpolating;

#ifdef USE_VOL_RAMP
    int32_t rampStyle = p->rampStyle;
#endif
    
    void * resampler[2];
    
    sampleLength     = p->voice[ch].sampleLength;
    sampleLoopLength = p->voice[ch].sampleLoopLength;
    sampleLoopEnd    = p->voice[ch].sampleLoopEnd;
    sampleLoopBegin  = sampleLoopEnd - sampleLoopLength;
    loopEnabled      = p->voice[ch].loopEnabled;
    loopBidi         = p->voice[ch].loopBidi;
    loopDir          = p->voice[ch].loopDir;
    interpolating    = p->voice[ch].interpolating;
    
    sampleData = (const int16_t *)(p->voice[ch].sampleData);
    
    resampler[0] = p->resampler[ch];
#ifdef USE_VOL_RAMP
    resampler[1] = p->resampler[ch+254];
#else
    resampler[1] = p->resampler[ch+127];
#endif
    
    resampler_set_rate(resampler[0], p->voice[ch].incRate );
    resampler_set_rate(resampler[1], p->voice[ch].incRate );
    
    for (j = 0; (j < samples) && (p->voice[ch].sampleData != NULL); ++j)
    {
        p->voice[ch].busy = 1;
        samplePosition = p->voice[ch].samplePosition;
        
        while (interpolating && resampler_get_free_count(resampler[0]))
        {
            resampler_write_sample(resampler[0], sampleData[samplePosition]);
            resampler_write_sample(resampler[1], sampleData[sampleLength + samplePosition]);
            
            if (loopDir == 1)
            {
                if (--samplePosition < 0)
                    samplePosition = 0;
            }
            else
            {
                ++samplePosition;
            }
            
            if (loopEnabled)
            {
                if (loopBidi)
                {
                    if (loopDir == 1)
                    {
                        if (samplePosition <= sampleLoopBegin)
                        {
                            samplePosition += (sampleLoopBegin - samplePosition) - 1;
                            loopDir = 0;
                        }
                    }
                    else
                    {
                        if (samplePosition >= sampleLoopEnd)
                        {
                            samplePosition -= (samplePosition - sampleLoopEnd) + 1;
                            loopDir = 1;
                        }
                    }
                }
                else
                {
                    if (samplePosition >= sampleLoopEnd)
                        samplePosition -= sampleLoopLength;
                }
            }
            else if (samplePosition >= sampleLength)
            {
                interpolating = 0;
                break;
            }
        }
        
        p->voice[ch].samplePosition  = samplePosition;
        p->voice[ch].loopDir         = loopDir;
        p->voice[ch].interpolating   = (int8_t)interpolating;
        
        if ( !resampler_ready(resampler[0]) )
        {
            p->voice[ch].sampleData     = NULL;
            p->voice[ch].samplePosition = 0;
            p->voice[ch].busy           = 0;
            break;
        }
        
        sampleL = resampler_get_sample(resampler[0]);
        sampleR = resampler_get_sample(resampler[1]);
        resampler_remove_sample(resampler[0]);
        resampler_remove_sample(resampler[1]);
        
        sampleL = (sampleL * p->voice[ch].volumeL);
        sampleR = (sampleR * p->voice[ch].volumeR);
        
#ifdef USE_VOL_RAMP
        if (rampStyle > 0)
        {
            p->voice[ch].volumeL += p->voice[ch].volDeltaL;
            p->voice[ch].volumeR += p->voice[ch].volDeltaR;
            
            if (p->voice[ch].volDeltaL >= 0.0f)
            {
                if (p->voice[ch].volumeL > p->voice[ch].targetVolL)
                    p->voice[ch].volumeL = p->voice[ch].targetVolL;
            }
            else
            {
                if (p->voice[ch].volumeL < p->voice[ch].targetVolL)
                    p->voice[ch].volumeL = p->voice[ch].targetVolL;
            }
            
            if (p->voice[ch].volDeltaR >= 0.0f)
            {
                
                if (p->voice[ch].volumeR > p->voice[ch].targetVolR)
                    p->voice[ch].volumeR = p->voice[ch].targetVolR;
            }
            else
            {
                if (p->voice[ch].volumeR < p->voice[ch].targetVolR)
                    p->voice[ch].volumeR = p->voice[ch].targetVolR;
            }
            
            if (p->voice[ch].rampTerminates && !p->voice[ch].volumeL && !p->voice[ch].volumeR)
            {
                p->voice[ch].sampleData     = NULL;
                p->voice[ch].samplePosition = 0;
                p->voice[ch].busy           = 0;
            }
        }
#endif
        
        p->masterBufferL[j] += sampleL;
        p->masterBufferR[j] += sampleR;
    }
}

static inline void mixChannel(PLAYER *p, uint32_t i, uint32_t sampleBlockLength)
{
    if (p->voice[i].stereo)
    {
        if (p->voice[i].sixteenBit)
            mix16bstereo(p, i, sampleBlockLength);
        else
            mix8bstereo(p, i, sampleBlockLength);
    }
    else
    {
        if (p->voice[i].sixteenBit)
            mix16b(p, i, sampleBlockLength);
        else
            mix8b(p, i, sampleBlockLength);
    }
}

static void mixSampleBlock(PLAYER *p, float *outputStream, uint32_t sampleBlockLength)
{
    float *streamPointer;
    uint32_t i;
#ifdef USE_VOL_RAMP
    int32_t rampStyle = p->rampStyle;
#endif
    
    float outL;
    float outR;
    
    streamPointer = outputStream;
    
    memset(p->masterBufferL, 0, sampleBlockLength * sizeof (float));
    memset(p->masterBufferR, 0, sampleBlockLength * sizeof (float));
    
    for (i = 0; i < p->numChannels; ++i)
    {
        if (p->muted[i / 8] & (1 << (i % 8)))
            continue;
        mixChannel(p, i, sampleBlockLength);
#ifdef USE_VOL_RAMP
        if (rampStyle > 0)
            mixChannel(p, i + 127, sampleBlockLength);
#endif
    }
    
    for (i = 0; i < sampleBlockLength; ++i)
    {
        outL = p->masterBufferL[i] * (1.0f / 3.0f);
        outR = p->masterBufferR[i] * (1.0f / 3.0f);

        *streamPointer++ = outL;
        *streamPointer++ = outR;
    }
}

void ft2play_RenderFloat(void *_p, float *buffer, int32_t count)
{
    PLAYER * p = (PLAYER *)_p;
    int32_t samplesTodo;
    float * outputStream;
    
    if (p->isMixing)
    {
        outputStream = buffer;

        while (count)
        {
            if (p->samplesLeft)
            {
                samplesTodo = (count < p->samplesLeft) ? count : p->samplesLeft;
                samplesTodo = (samplesTodo < p->soundBufferSize) ? samplesTodo : p->soundBufferSize;

                if (outputStream)
                {
                    mixSampleBlock(p, outputStream, samplesTodo);

                    outputStream  += (samplesTodo << 1);
                }

                p->samplesLeft   -= samplesTodo;
                count -= samplesTodo;
            }
            else
            {
                if (p->Playing)
                    MainPlayer(p);

                p->samplesLeft = p->samplesPerFrame;
            }
        }
    }
}

void ft2play_RenderFixed32(void *_p, int32_t *buffer, int32_t count, int8_t depth)
{
    int32_t i;
    float * fbuffer = (float *)buffer;
    float scale = (float)(1 << (depth - 1));
    float sample;
    assert(sizeof(int32_t) == sizeof(float));
    ft2play_RenderFloat(_p, fbuffer, count);
    for (i = 0; i < count * 2; ++i)
    {
        sample = fbuffer[i] * scale;
        if (sample > INT_MAX) sample = INT_MAX;
        else if (sample < INT_MIN) sample = INT_MIN;
        buffer[i] = (int32_t)sample;
    }
}

void ft2play_RenderFixed16(void *_p, int16_t *buffer, int32_t count, int8_t depth)
{
    int32_t i, SamplesTodo;
    float scale = (float)(1 << (depth - 1));
    float sample;
    float fbuffer[1024];
    while (count)
    {
        SamplesTodo = (count < 512) ? count : 512;
        ft2play_RenderFloat(_p, fbuffer, SamplesTodo);
        for (i = 0; i < SamplesTodo * 2; ++i)
        {
            sample = fbuffer[i] * scale;
            if (sample > 32767) sample = 32767;
            else if (sample < -32768) sample = -32768;
            buffer[i] = (int16_t)sample;
        }
        buffer += SamplesTodo * 2;
        count -= SamplesTodo;
    }
}

void * ft2play_Alloc(uint32_t _samplingFrequency, int8_t interpolation, int8_t ramp_style)
{
    uint8_t j;
    uint16_t i;
    int16_t noteVal;
    uint16_t noteIndex;
    
    PLAYER * p = (PLAYER *) calloc(1, sizeof(PLAYER));
    
    if ( !p )
        return NULL;
    
    p->samplesPerFrame = 882;
    p->numChannels = 127;
    
    p->outputFreq          = _samplingFrequency;
    p->f_outputFreq        = (float)(p->outputFreq);
    p->soundBufferSize     = _soundBufferSize;
    
    p->masterBufferL       = (float *)(malloc(p->soundBufferSize * sizeof (float)));
    p->masterBufferR       = (float *)(malloc(p->soundBufferSize * sizeof (float)));
    if ( !p->masterBufferL || !p->masterBufferR )
        goto error;
    
    p->samplingInterpolation = interpolation;
#ifdef USE_VOL_RAMP
    p->rampStyle             = ramp_style;
#endif
    
    resampler_init();
    
#ifdef USE_VOL_RAMP
    for ( i = 0; i < 127 * 2 * 2; ++i )
#else
    for ( i = 0; i < 127 * 2; ++i )
#endif
    {
        p->resampler[i] = resampler_create();
        if ( !p->resampler[i] )
            goto error;
        resampler_set_quality(p->resampler[i], interpolation);
    }
    
    // allocate memory for pointers
    
    p->NilPatternLine = (TonTyp *)(calloc(sizeof (TonTyp), 256 * 127));
    if (p->NilPatternLine == NULL)
        goto error;
    
    p->linearPeriods = (int16_t *)(malloc(sizeof (int16_t) * ((12 * 10 * 16) + 16)));
    if (p->linearPeriods == NULL)
        goto error;
    
    p->amigaPeriods = (int16_t *)(malloc(sizeof (int16_t) * ((12 * 10 * 16) + 16)));
    if (p->amigaPeriods == NULL)
        goto error;
    
    p->VibSineTab = (int8_t *)(malloc(256));
    if (p->VibSineTab == NULL)
        goto error;
    
    p->PanningTab = (float *)(malloc(sizeof (float) * 257));
    if (p->PanningTab == NULL)
        goto error;
    
    p->LogTab = (uint32_t *)(malloc(sizeof (uint32_t) * 768));
    if (p->LogTab == NULL)
        goto error;
    
    // generate tables
    
    for (i = 0; i < 768; ++i)
        p->LogTab[i] = (uint32_t)(floor(((256.0f * 8363.0f) * exp((float)(i) / 768.0f * logf(2.0f))) + 0.5f));
    
    for (i = 0; i < ((12 * 10 * 16) + 16); ++i)
        p->linearPeriods[i] = (((12 * 10 * 16) + 16) * 4) - (i << 2);
    
    noteIndex = 0;
    for (i = 0; i < 10; ++i)
    {
        for (j = 0; j < ((i == 9) ? (96 + 8) : 96); ++j)
        {
            noteVal = ((AmigaFinePeriod[j] << 6) + (-1 + (1 << i))) >> (i + 1);
            
            p->amigaPeriods[noteIndex++] = noteVal;
            p->amigaPeriods[noteIndex++] = noteVal;
        }
    }
    
    for (i = 0; i < (((12 * 10 * 16) + 16) / 2); ++i)
        p->amigaPeriods[(i << 1) + 1] = (p->amigaPeriods[i << 1] + p->amigaPeriods[(i << 1) + 2]) >> 1;
    
    // generate auto-vibrato table (value-exact to its original table)
    for (i = 0; i < 256; ++i)
        p->VibSineTab[i] = (int8_t)floorf((64.0f * sinf(((float)(-i) * (2.0f * 3.1415927f)) / 256.0f)) + 0.5f);
    
    // generate FT2's pan table [round(65536*sqrt(n/256)) for n = 0...256]
    for (i = 0; i < 257; ++i)
        p->PanningTab[i] = sqrtf((float)(i) / 256.0f);
    
    return p;
    
error:
    ft2play_Free( p );
    return NULL;
}

void ft2play_Free(void *_p)
{
    uint32_t i;
    
    PLAYER * p = (PLAYER *)_p;
    
    if (p->isMixing)
    {
        p->isMixing = 0;
        
        if (p->masterBufferL) free(p->masterBufferL); p->masterBufferL = NULL;
        if (p->masterBufferR) free(p->masterBufferR); p->masterBufferR = NULL;
    }

    p->Playing          = 0;
    
    ft2play_FreeSong(p);
    
    if (p->LogTab)         free(p->LogTab);         p->LogTab         = NULL;
    if (p->PanningTab)     free(p->PanningTab);     p->PanningTab     = NULL;
    if (p->VibSineTab)     free(p->VibSineTab);     p->VibSineTab     = NULL;
    if (p->amigaPeriods)   free(p->amigaPeriods);   p->amigaPeriods   = NULL;
    if (p->linearPeriods)  free(p->linearPeriods);  p->linearPeriods  = NULL;
    if (p->NilPatternLine) free(p->NilPatternLine); p->NilPatternLine = NULL;

#ifdef USE_VOL_RAMP
    for ( i = 0; i < 127 * 2 * 2; ++i )
#else
    for ( i = 0; i < 127 * 2; ++i )
#endif
    {
        if ( p->resampler[i] )
            resampler_delete( p->resampler[i] );
        p->resampler[i] = NULL;
    }
    
    free (p);
}

void ft2play_PlaySong(void *_p, int32_t startOrder)
{
    PLAYER * p = (PLAYER *)_p;
    
    if (!p->ModuleLoaded) return;
    
    StopVoices(p);
    
    p->Song.GlobVol = 64;
    p->numChannels  = p->Song.AntChn;
    p->Playing      = 1;
    
    setSamplesPerFrame(p, ((p->outputFreq * 5UL) / 2 / p->Song.Speed));
    p->isMixing = 1;
    
    SetPos(p, (int16_t)startOrder, 0);

    p->Song.startOrder = (int16_t)startOrder;
    
    p->loopCount = 0;
    memset(p->playedOrder, 0, sizeof(p->playedOrder));
    p->playedOrder[startOrder / 8] = 1 << (startOrder % 8);
}

static MEM *mopen(const uint8_t *src, uintptr_t length)
{
    MEM *b;
    
    if ((src == NULL) || (length <= 0)) return (NULL);
    
    b = (MEM *)(malloc(sizeof (MEM)));
    if (b == NULL) return (NULL);
    
    b->_base   = (uint8_t *)(src);
    b->_ptr    = (uint8_t *)(src);
    b->_cnt    = length;
    b->_bufsiz = length;
    b->_eof    = 0;
    
    return (b);
}

static void mclose(MEM *buf)
{
    if (buf != NULL)
    {
        free(buf);
        buf = NULL;
    }
}

#if 0
static intptr_t mtell(MEM *buf)
{
    return (buf->_bufsiz - buf->_cnt);
}
#endif

static size_t mread(void *buffer, size_t size, size_t count, MEM *buf)
{
    size_t wrcnt;
    intptr_t pcnt;
    
    if (buf       == NULL) return (0);
    if (buf->_ptr == NULL) return (0);
    
    wrcnt = size * count;
    if ((size == 0) || buf->_eof) return (0);
    
    pcnt = ((uint32_t)(buf->_cnt) > wrcnt) ? wrcnt : buf->_cnt;
    memcpy(buffer, buf->_ptr, pcnt);
    
    buf->_cnt -= pcnt;
    buf->_ptr += pcnt;
    
    if (buf->_cnt <= 0)
    {
        buf->_ptr = buf->_base + buf->_bufsiz;
        buf->_cnt = 0;
        buf->_eof = 1;
    }
    
    return (pcnt / size);
}

#if 0
static size_t mwrite(const void *buffer, size_t size, size_t count, MEM *buf)
{
    size_t wrcnt;
    intptr_t pcnt;
    
    if (buf       == NULL) return (0);
    if (buf->_ptr == NULL) return (0);
    
    wrcnt = size * count;
    if ((size == 0) || buf->_eof) return (0);
    
    pcnt = ((uint32_t)(buf->_cnt) > wrcnt) ? wrcnt : buf->_cnt;
    memcpy(buf->_ptr, buffer, pcnt);
    
    buf->_cnt -= pcnt;
    buf->_ptr += pcnt;
    
    if (buf->_cnt <= 0)
    {
        buf->_ptr = buf->_base + buf->_bufsiz;
        buf->_cnt = 0;
        buf->_eof = 1;
    }
    
    return (pcnt / size);
}
#endif

static int32_t meof(MEM *buf)
{
    if (buf == NULL) return (1); // XXX: Should return a different value?
    
    return (buf->_eof);
}

static void mseek(MEM *buf, intptr_t offset, int32_t whence)
{
    if (buf == NULL) return;
    
    if (buf->_base)
    {
        switch (whence)
        {
            case SEEK_SET: buf->_ptr  = buf->_base + offset;                break;
            case SEEK_CUR: buf->_ptr += offset;                             break;
            case SEEK_END: buf->_ptr  = buf->_base + buf->_bufsiz + offset; break;
            default: break;
        }
        
        buf->_eof = 0;
        if (buf->_ptr >= (buf->_base + buf->_bufsiz))
        {
            buf->_ptr = buf->_base + buf->_bufsiz;
            buf->_eof = 1;
        }
        
        buf->_cnt = (buf->_base + buf->_bufsiz) - buf->_ptr;
    }
}

static void setSamplesPerFrame(PLAYER *p, uint32_t val)
{
    p->samplesPerFrame   = val;
    
#ifdef USE_VOL_RAMP
    p->f_samplesPerFrame = 1.0f / ((float)(val) / 4.0f);
    p->f_samplesPerFrameSharp = 1.0f / (p->f_outputFreq / 1000.0f); // 1ms
#endif
}

void ft2play_Mute(void *_p, int8_t channel, int8_t mute)
{
    PLAYER * p = (PLAYER *)_p;
    int8_t mask = 1 << (channel % 8);
	if (channel > 127)
		return;
    if (mute)
        p->muted[channel / 8] |= mask;
    else
		p->muted[channel / 8] &= ~mask;
}

uint32_t ft2play_GetLoopCount(void *_p)
{
    PLAYER * p = (PLAYER *)_p;
    return p->loopCount;
}

void ft2play_GetInfo(void *_p, ft2_info *info)
{
    int32_t i, channels_playing;
    PLAYER * p = (PLAYER *)_p;
    info->order = p->Song.SongPos;
    info->pattern = p->Song.PattNr;
    info->row = p->Song.PattPos;
    info->speed = p->Song.Tempo; // Hurr
    info->tempo = p->Song.Speed;
    channels_playing = 0;
    if (p->isMixing)
    {
        for (i = 0; i < p->Song.AntChn; ++i)
        {
            if (p->voice[i].busy)
                ++channels_playing;
        }
    }
    info->channels_playing = (uint8_t)channels_playing;
}

// EOF
