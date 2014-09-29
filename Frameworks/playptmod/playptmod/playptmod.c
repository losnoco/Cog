/*
** - --=playptmod v1.10d - 8bitbubsy 2010-2014=-- -
** This is the native Win32 API version, no DLL needed in you
** production zip/rar whatever.
**
** Thanks to mukunda for learning me how to code a .MOD player
** some years back!
**
** Thanks to ad_/aciddose/adejr for the BLEP and LED filter
** routines.
**
** Note: There's a lot of weird behavior in the coding to
** "emulate" the weird stuff ProTracker on the Amiga does.
** If you see something fishy in the code, it's probably
** supposed to be like that. Please don't change it, you're
** literally asking for hours of hard debugging if you do.
**
** HOW DO I USE THIS FILE?
** Make a new file called main.c, and put this on top:
**
** #include <stdio.h>
**
** void * playptmod_Create(int soundFrequency);
** int playptmod_Load(void *, const char *filename);
** void playptmod_Play(void *);
** void playptmod_Render(void *, signed short *, int);
** void playptmod_Free(void *);
**
** void main(void)
** {
**		int app_running = 1;
**
**		void *p = playptmod_Create(44100);
**		playptmod_Load(p, "hello.mod");
**		playptmod_Play(p);
**
**		while (app_running)
**		{
**          signed short samples[1024];
**
**			if (someone_pressed_a_key())
**			{
**				app_running = 0;
**			}
**
**          playptmod_Render(p, samples, 512);
**          // output samples to system here
**
**			// Make sure to delay a bit here
**		}
**
**		playptmod_Free(p);
**
**		return 0;
** }
**
**
** You can also integrate it as a resource in the EXE,
** and use some Win32 API functions to copy the MOD
** to memory and get a pointer to it. Then you call
** playptmod_LoadMem instead.
**
*/

#define _USE_MATH_DEFINES // visual studio

#include "playptmod.h"
#include "blip_buf.h"

#include <stdio.h>
#include <string.h> // memcpy()
#include <stdlib.h> // malloc(), calloc(), free()
#include <math.h> // floorf(), sinf()

#define HI_NYBBLE(x) ((x) >> 4)
#define LO_NYBBLE(x) ((x) & 0x0F)
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define DENORMAL_OFFSET 1E-10f
#define PT_MIN_PERIOD 78
#define PT_MAX_PERIOD 937
#define MAX_CHANNELS 32
#define MOD_ROWS 64
#define MOD_SAMPLES 31

#ifndef true
#define true 1
#define false 0
#endif

enum
{
    FLAG_NOTE = 1,
    FLAG_SAMPLE = 2,
    FLAG_NEWSAMPLE = 4,
    TEMPFLAG_START = 1,
    TEMPFLAG_DELAY = 2,
    TEMPFLAG_NEW_SAMPLE = 4
};

enum
{
    soundBufferSize = 2048 * 4
};

typedef struct modnote
{
    unsigned char sample;
    unsigned char command;
    unsigned char param;
    short period;
} modnote_t;

typedef struct
{
    unsigned char orderCount;
    unsigned char patternCount;
    unsigned char rowCount;
    unsigned char restartPos;
    unsigned char order[128];
    unsigned char volume[MAX_CHANNELS];
    unsigned char pan[MAX_CHANNELS];
    unsigned char ticks;
    unsigned char format;
    unsigned char channelCount;
    short tempo;
    short initBPM;
    int moduleSize;
    int totalSampleSize;
} MODULE_HEADER;

typedef struct
{
    unsigned char fineTune;
    unsigned char volume;
    int iffSize;
    int loopStart;
    int loopLength;
    int length;
    int reallength;
    int tmpLoopStart;
    int offset;
    unsigned char attribute;
} MODULE_SAMPLE;

typedef struct
{
    char patternLoopRow;
    char patternLoopCounter;
    char volume;
    unsigned char sample;
    unsigned char command;
    unsigned char param;
    unsigned char flags;
    unsigned char tempFlags;
    unsigned char tempFlagsBackup;
    unsigned char fineTune;
    unsigned char tremoloPos;
    unsigned char vibratoPos;
    unsigned char tremoloControl;
    unsigned char tremoloSpeed;
    unsigned char tremoloDepth;
    unsigned char vibratoControl;
    unsigned char vibratoSpeed;
    unsigned char vibratoDepth;
    unsigned char glissandoControl;
    unsigned char glissandoSpeed;
    unsigned char invertLoopDelay;
    unsigned char invertLoopSpeed;
    unsigned char chanIndex;
    short period;
    short tempPeriod;
    int noNote;
    int invertLoopOffset;
    int offset;
    int offsetTemp;
    int offsetBugNotAdded;
} mod_channel;

typedef struct
{
    char moduleLoaded;
    char *sampleData;
    char *originalSampleData;
    MODULE_HEADER head;
    MODULE_SAMPLE samples[31];
    modnote_t *patterns[256];
    mod_channel channels[MAX_CHANNELS];
} MODULE;

typedef struct paula_filter_state
{
    float LED[4];
} Filter;

typedef struct paula_filter_coefficients
{
    float LED;
    float LEDFb;
} FilterC;

typedef struct voice_data
{
    const char *newData;
    const char *data;
    char swapSampleFlag;
    char loopFlag;
    int length;
    int loopBegin;
    int loopEnd;
    char newLoopFlag;
    int newLength;
    int newLoopBegin;
    int newLoopEnd;
    int index;
    int vol;
    int panL;
    int panR;
    int step;
    int newStep;
    float frac;
    float rate;
    int mute;
} Voice;

typedef struct
{
    unsigned long length;
    unsigned long remain;
    const unsigned char *buf;
    const unsigned char *t_buf;
} BUF;

typedef struct
{
    int numChans;
    char pattBreakBugPos;
    char pattBreakFlag;
    char pattDelayFlag;
    char forceEffectsOff;
    char tempVolume;
    unsigned char modRow;
    unsigned char modSpeed;
    unsigned short modBPM;
    unsigned char modTick;
    unsigned char modPattern;
    unsigned char modOrder;
    unsigned char tempFlags;
    unsigned char PBreakPosition;
    unsigned char PattDelayTime;
    unsigned char PattDelayTime2;
    unsigned char PosJumpAssert;
    unsigned char PBreakFlag;
    short tempPeriod;
    int tempoTimerVal;
    char moduleLoaded;
    char modulePlaying;
    char useLEDFilter;
    unsigned short soundBufferSize;
    unsigned int soundFrequency;
    char soundBuffers;
    float *frequencyTable;
    float *extendedFrequencyTable;
    unsigned char *sinusTable;
    int minPeriod;
    int maxPeriod;
    int loopCounter;
    int sampleCounter;
    int samplesPerTick;
    int vBlankTiming;
    Voice v[MAX_CHANNELS];
    Filter filter;
    FilterC filterC;
    float *mixBufferL;
    float *mixBufferR;
    blip_t blep[MAX_CHANNELS];
    blip_t blepVol[MAX_CHANNELS];
    unsigned int orderPlayed[256];
    MODULE *source;
} player;

static const unsigned char invertLoopSpeeds[16] =
{
    0x00, 0x05, 0x06, 0x07, 0x08, 0x0A, 0x0B, 0x0D, 0x0F, 0x13, 0x16, 0x1A, 0x20, 0x2B, 0x40, 0x80
};

static const short rawAmigaPeriods[606] =
{
    856,808,762,720,678,640,604,570,538,508,480,453,
    428,404,381,360,339,320,302,285,269,254,240,226,
    214,202,190,180,170,160,151,143,135,127,120,113,0,
    850,802,757,715,674,637,601,567,535,505,477,450,
    425,401,379,357,337,318,300,284,268,253,239,225,
    213,201,189,179,169,159,150,142,134,126,119,113,0,
    844,796,752,709,670,632,597,563,532,502,474,447,
    422,398,376,355,335,316,298,282,266,251,237,224,
    211,199,188,177,167,158,149,141,133,125,118,112,0,
    838,791,746,704,665,628,592,559,528,498,470,444,
    419,395,373,352,332,314,296,280,264,249,235,222,
    209,198,187,176,166,157,148,140,132,125,118,111,0,
    832,785,741,699,660,623,588,555,524,495,467,441,
    416,392,370,350,330,312,294,278,262,247,233,220,
    208,196,185,175,165,156,147,139,131,124,117,110,0,
    826,779,736,694,655,619,584,551,520,491,463,437,
    413,390,368,347,328,309,292,276,260,245,232,219,
    206,195,184,174,164,155,146,138,130,123,116,109,0,
    820,774,730,689,651,614,580,547,516,487,460,434,
    410,387,365,345,325,307,290,274,258,244,230,217,
    205,193,183,172,163,154,145,137,129,122,115,109,0,
    814,768,725,684,646,610,575,543,513,484,457,431,
    407,384,363,342,323,305,288,272,256,242,228,216,
    204,192,181,171,161,152,144,136,128,121,114,108,0,
    907,856,808,762,720,678,640,604,570,538,508,480,
    453,428,404,381,360,339,320,302,285,269,254,240,
    226,214,202,190,180,170,160,151,143,135,127,120,0,
    900,850,802,757,715,675,636,601,567,535,505,477,
    450,425,401,379,357,337,318,300,284,268,253,238,
    225,212,200,189,179,169,159,150,142,134,126,119,0,
    894,844,796,752,709,670,632,597,563,532,502,474,
    447,422,398,376,355,335,316,298,282,266,251,237,
    223,211,199,188,177,167,158,149,141,133,125,118,0,
    887,838,791,746,704,665,628,592,559,528,498,470,
    444,419,395,373,352,332,314,296,280,264,249,235,
    222,209,198,187,176,166,157,148,140,132,125,118,0,
    881,832,785,741,699,660,623,588,555,524,494,467,
    441,416,392,370,350,330,312,294,278,262,247,233,
    220,208,196,185,175,165,156,147,139,131,123,117,0,
    875,826,779,736,694,655,619,584,551,520,491,463,
    437,413,390,368,347,328,309,292,276,260,245,232,
    219,206,195,184,174,164,155,146,138,130,123,116,0,
    868,820,774,730,689,651,614,580,547,516,487,460,
    434,410,387,365,345,325,307,290,274,258,244,230,
    217,205,193,183,172,163,154,145,137,129,122,115,0,
    862,814,768,725,684,646,610,575,543,513,484,457,
    431,407,384,363,342,323,305,288,272,256,242,228,
    216,203,192,181,171,161,152,144,136,128,121,114,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

static short extendedRawPeriods[16 * 85 + 14];

static const short npertab[84] =
{
    /* Octaves 0 -> 6 */
    /* C    C#     D    D#     E     F    F#     G    G#     A    A#     B */
    0x6b0,0x650,0x5f4,0x5a0,0x54c,0x500,0x4b8,0x474,0x434,0x3f8,0x3c0,0x38a,
    0x358,0x328,0x2fa,0x2d0,0x2a6,0x280,0x25c,0x23a,0x21a,0x1fc,0x1e0,0x1c5,
    0x1ac,0x194,0x17d,0x168,0x153,0x140,0x12e,0x11d,0x10d,0x0fe,0x0f0,0x0e2,
    0x0d6,0x0ca,0x0be,0x0b4,0x0aa,0x0a0,0x097,0x08f,0x087,0x07f,0x078,0x071,
    0x06b,0x065,0x05f,0x05a,0x055,0x050,0x04b,0x047,0x043,0x03f,0x03c,0x038,
    0x035,0x032,0x02f,0x02d,0x02a,0x028,0x025,0x023,0x021,0x01f,0x01e,0x01c,
    0x01b,0x019,0x018,0x016,0x015,0x014,0x013,0x012,0x011,0x010,0x00f,0x00e
};

static const short finetune[16] =
{
    8363,8413,8463,8529,8581,8651,8723,8757,
    7895,7941,7985,8046,8107,8169,8232,8280
};

static float calcRcCoeff(float sampleRate, float cutOffFreq)
{
    if (cutOffFreq >= (sampleRate / 2.0f))
        return (1.0f);

    return (2.0f * (float)(M_PI) * cutOffFreq / sampleRate);
}

static BUF *bufopen(const unsigned char *bufToCopy, unsigned long bufferSize)
{
    BUF *b;

    b = (BUF *)malloc(sizeof (BUF));

    b->t_buf = bufToCopy;
    b->buf = bufToCopy;
    b->length = bufferSize;
    b->remain = bufferSize;

    return (b);
}

static void bufclose(BUF *_SrcBuf)
{
    if (_SrcBuf != NULL)
        free(_SrcBuf);
}

static unsigned long buftell(BUF *_SrcBuf)
{
    if (_SrcBuf->buf > _SrcBuf->t_buf)
        return (_SrcBuf->buf - _SrcBuf->t_buf);
    else
        return (_SrcBuf->t_buf - _SrcBuf->buf);
}

static void bufread(void *_DstBuf, size_t _ElementSize, size_t _Count, BUF *_SrcBuf)
{
    _Count *= _ElementSize;
    if (_Count > _SrcBuf->remain)
        _Count = _SrcBuf->remain;

    _SrcBuf->remain -= _Count;
    memcpy(_DstBuf, _SrcBuf->buf, _Count);
    _SrcBuf->buf += _Count;
}

static void bufseek(BUF *_SrcBuf, long _Offset, int _Origin)
{
    if (_SrcBuf->buf)
    {
        switch (_Origin)
        {
            case SEEK_SET: _SrcBuf->buf = _SrcBuf->t_buf + _Offset; break;
            case SEEK_CUR: _SrcBuf->buf += _Offset; break;
            case SEEK_END: _SrcBuf->buf = _SrcBuf->t_buf + _SrcBuf->length + _Offset; break;
            default: break;
        }

        _Offset = _SrcBuf->buf - _SrcBuf->t_buf;
        _SrcBuf->remain = (unsigned int)(_Offset) > _SrcBuf->length ? 0 : _SrcBuf->length - _Offset;
    }
}

static inline int periodToNote(player *p, char finetune, short period)
{
    unsigned char i;
    unsigned char maxNotes;
    short *tablePointer;

    if (p->minPeriod == PT_MIN_PERIOD)
    {
        maxNotes = 36;
        tablePointer = (short *)&rawAmigaPeriods[finetune * 37];
    }
    else
    {
        maxNotes = 84;
        tablePointer = (short *)&extendedRawPeriods[finetune * 85];
    }

    for (i = 0; i < maxNotes; ++i)
    {
        if (period >= tablePointer[i])
            break;
    }

    return i;
}

static void mixerSwapChSource(player *p, int ch, const char *src, int length, int loopStart, int loopLength, int step)
{
    p->v[ch].swapSampleFlag  = true;
    p->v[ch].newData         = src;
    p->v[ch].newLoopFlag     = loopLength > (2 * step);
    p->v[ch].newLength       = length;
    p->v[ch].newLoopBegin    = loopStart;
    p->v[ch].newLoopEnd      = loopStart + loopLength;
    p->v[ch].newStep         = step;
}

static void mixerSetChSource(player *p, int ch, const char *src, int length, int loopStart, int loopLength, int offset, int step)
{
    p->v[ch].swapSampleFlag = false;
    p->v[ch].data           = src;
    p->v[ch].index          = offset;
    p->v[ch].frac           = 0.0f;
    p->v[ch].length         = length;
    p->v[ch].loopFlag       = loopLength > (2 * step);
    p->v[ch].loopBegin      = loopStart;
    p->v[ch].loopEnd        = loopStart + loopLength;
    p->v[ch].step           = step;
    
    // Check external 9xx usage (Set Sample Offset)
    if (p->v[ch].loopFlag)
    {
        if (offset >= p->v[ch].loopEnd)
            p->v[ch].index = p->v[ch].loopBegin;
    }
    else
    {
        if (offset >= p->v[ch].length)
            p->v[ch].data = NULL;
    }
}

// adejr: these sin/cos approximations both use a 0..1
// parameter range and have 'normalized' (1/2 = 0db) coefficients
//
// the coefficients are for LERP(x, x * x, 0.224) * sqrt(2)
// max_error is minimized with 0.224 = 0.0013012886

static inline float sinApx(float x)
{
    x = x * (2.0f - x);
    return (x * 1.09742972f + x * x * 0.31678383f);
}

static inline float cosApx(float x)
{
    x = (1.0f - x) * (1.0f + x);
    return (x * 1.09742972f + x * x * 0.31678383f);
}

static void mixerSetChPan(player *p, int ch, int pan)
{
    float newpan;
    if (pan == 255) pan = 256; // 100% R pan fix for 8-bit pan
    
    newpan = pan * (1.0f / 256.0f);
    
    p->v[ch].panL = (int)(256.0f * cosApx(newpan));
    p->v[ch].panR = (int)(256.0f * sinApx(newpan));
}

static void mixerSetChVol(player *p, int ch, int vol)
{
    p->v[ch].vol = vol;
}

static void mixerCutChannels(player *p)
{
    int i;

    memset(p->v, 0, sizeof (p->v));
    for (i = 0; i < MAX_CHANNELS; i++)
    {
        ptm_blip_clear(&p->blep[i]);
        ptm_blip_clear(&p->blepVol[i]);
    }

    memset(&p->filter, 0, sizeof (p->filter));

    if (p->source)
    {
        for (i = 0; i < MAX_CHANNELS; i++)
        {
            mixerSetChVol(p, i, p->source->head.volume[i]);
            mixerSetChPan(p, i, p->source->head.pan[i]);
        }
    }
    else
    {
        for (i = 0; i < MAX_CHANNELS; i++)
        {
            mixerSetChVol(p, i, 64);
            mixerSetChPan(p, i, (i + 1) & 2 ? 160 : 96);
        }
    }
}

static void mixerSetChRate(player *p, int ch, float rate)
{
    p->v[ch].rate = rate;
}

static void outputAudio(player *p, int *target, int numSamples)
{
    int *out;
    int i;
    int j;
    int step;
    short tempSample;
    int tempVolume;
    float L;
    float R;

    memset(p->mixBufferL, 0, numSamples * sizeof (float));
    memset(p->mixBufferR, 0, numSamples * sizeof (float));

    for (i = 0; i < p->source->head.channelCount; ++i)
    {
        j = 0;

        if (p->v[i].data && p->v[i].rate)
        {
            step = p->v[i].step;
            for (j = 0; j < numSamples;)
            {
                tempSample = (p->v[i].data ? (step == 2 ? (p->v[i].data[p->v[i].index] + p->v[i].data[p->v[i].index + 1] * 0x100) : p->v[i].data[p->v[i].index] * 0x100) : 0);
                tempVolume = (p->v[i].data && !p->v[i].mute ? p->v[i].vol : 0);

                while (j < numSamples && (!p->v[i].data || p->v[i].frac >= 1.0f))
                {
                    float t_vol = 0.0f;
                    float t_smp = 0.0f;
                    signed int i_smp;

                    if (p->v[i].data)
                        p->v[i].frac -= 1.0f;

                    t_vol += ptm_blip_read_sample(&p->blepVol[i]);
                    t_smp += ptm_blip_read_sample(&p->blep[i]);

                    t_smp *= t_vol;
                    i_smp = (signed int)t_smp;

                    p->mixBufferL[j] += i_smp * p->v[i].panL;
                    p->mixBufferR[j] += i_smp * p->v[i].panR;

                    j++;
                }

                if (j >= numSamples) break;

                if (tempSample != p->blep[i].last_value)
                {
                    int delta = tempSample - p->blep[i].last_value;
                    p->blep[i].last_value = tempSample;
                    ptm_blip_add_delta(&p->blep[i], p->v[i].frac, delta);
                }

                if (tempVolume != p->blepVol[i].last_value)
                {
                    int delta = tempVolume - p->blepVol[i].last_value;
                    p->blepVol[i].last_value = tempVolume;
                    ptm_blip_add_delta(&p->blepVol[i], 0, delta);
                }

                if (p->v[i].data)
                {
                    p->v[i].index += step;
                    p->v[i].frac += p->v[i].rate;

                    if (p->v[i].loopFlag)
                    {
                        if (p->v[i].index >= p->v[i].loopEnd)
                        {
                            if (p->v[i].swapSampleFlag == true)
                            {
                                p->v[i].swapSampleFlag = false;

                                if (p->v[i].newLoopFlag == false)
                                {
                                    p->v[i].data = NULL;
                                    continue;
                                }

                                p->v[i].loopBegin    = p->v[i].newLoopBegin;
                                p->v[i].loopEnd      = p->v[i].newLoopEnd;
                                p->v[i].loopFlag     = p->v[i].newLoopFlag;
                                p->v[i].data         = p->v[i].newData;
                                p->v[i].length       = p->v[i].newLength;
                                p->v[i].frac         = 0.0f;
                                p->v[i].step         = p->v[i].newStep;
                                
                                while (p->v[i].index >= p->v[i].loopEnd)
                                    p->v[i].index = p->v[i].loopBegin + (p->v[i].index - p->v[i].loopEnd);
                            }
                            else
                            {
                                while (p->v[i].index >= p->v[i].loopEnd)
                                    p->v[i].index = p->v[i].loopBegin + (p->v[i].index - p->v[i].loopEnd);
                            }
                        }
                    }
                    else if (p->v[i].index >= p->v[i].length)
                    {
                        if (p->v[i].swapSampleFlag == true)
                        {
                            p->v[i].swapSampleFlag = false;
                            
                            if (p->v[i].newLoopFlag == false)
                            {
                                p->v[i].data = NULL;
                                continue;
                            }

                            p->v[i].loopBegin    = p->v[i].newLoopBegin;
                            p->v[i].loopEnd      = p->v[i].newLoopEnd;
                            p->v[i].loopFlag     = p->v[i].newLoopFlag;
                            p->v[i].data         = p->v[i].newData;
                            p->v[i].length       = p->v[i].newLength;
                            p->v[i].frac         = 0.0f;
                            p->v[i].step         = p->v[i].newStep;

                            while (p->v[i].index >= p->v[i].loopEnd)
                                p->v[i].index = p->v[i].loopBegin + (p->v[i].index - p->v[i].loopEnd);
                        }
                        else
                        {
                            p->v[i].data = NULL;
                        }
                    }
                }
            }
        }

        if ((j < numSamples) && (p->v[i].data == NULL))
        {
            for (; j < numSamples; ++j)
            {
                int i_smp;

                tempVolume = p->blepVol[i].last_value;
                tempSample = p->blep[i].last_value;

                tempVolume += ptm_blip_read_sample(&p->blepVol[i]);
                tempSample += ptm_blip_read_sample(&p->blep[i]);

                tempSample *= tempVolume;
                i_smp = (signed int)tempSample;

                p->mixBufferL[j] += i_smp * p->v[i].panL;
                p->mixBufferR[j] += i_smp * p->v[i].panR;
            }
        }
    }

    out = target;

    {
        float downscale;
        
        if (p->numChans <= 4)    
            downscale = 1.0f / 172.0f;
        else
            downscale = 1.0f / 208.0f;
            
        for (i = 0; i < numSamples; ++i)
        {
            L = p->mixBufferL[i];
            R = p->mixBufferR[i];

            if (p->useLEDFilter == true)
            {
                p->filter.LED[0] += (p->filterC.LED * (L - p->filter.LED[0])
                    + p->filterC.LEDFb * (p->filter.LED[0] - p->filter.LED[1]) + DENORMAL_OFFSET);
                p->filter.LED[1] += (p->filterC.LED * (p->filter.LED[0] - p->filter.LED[1]) + DENORMAL_OFFSET);
                p->filter.LED[2] += (p->filterC.LED * (R - p->filter.LED[2])
                    + p->filterC.LEDFb * (p->filter.LED[2] - p->filter.LED[3]) + DENORMAL_OFFSET);
                p->filter.LED[3] += (p->filterC.LED * (p->filter.LED[2] - p->filter.LED[3]) + DENORMAL_OFFSET);

                L = p->filter.LED[1];
                R = p->filter.LED[3];
            }

            L *= downscale;
            R *= downscale;

			L = CLAMP(L, INT_MIN, INT_MAX);
			R = CLAMP(R, INT_MIN, INT_MAX);

            if ( out )
            {
                *out++ = (int)(L);
                *out++ = (int)(R);
            }
        }
    }
}

static unsigned short bufGetWordBigEndian(BUF *in)
{
    unsigned char bytes[2];

    bufread(bytes, 1, 2, in);
    return ((bytes[0] << 8) | bytes[1]);
}

static unsigned short bufGetWordLittleEndian(BUF *in)
{
    unsigned char bytes[2];

    bufread(bytes, 1, 2, in);
    return ((bytes[1] << 8) | bytes[0]);
}

static unsigned int bufGetDwordLittleEndian(BUF *in)
{
    unsigned char bytes[4];

    bufread(bytes, 1, 4, in);
    return ((bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0]);
}

static int playptmod_LoadMTM(player *p, BUF *fmodule)
{
    int i, j, k;
    unsigned int trackCount, commentLength;
    unsigned char sampleCount;
    unsigned long tracksOffset, sequencesOffset, commentOffset;
    unsigned int totalSampleSize = 0, sampleOffset = 0;

    modnote_t *note = NULL;

    bufseek(fmodule, 24, SEEK_SET);
    trackCount = bufGetWordLittleEndian(fmodule);
    bufread(&p->source->head.patternCount, 1, 1, fmodule); p->source->head.patternCount++;
    bufread(&p->source->head.orderCount, 1, 1, fmodule); p->source->head.orderCount++;
    commentLength = bufGetWordLittleEndian(fmodule);
    bufread(&sampleCount, 1, 1, fmodule);
    bufseek(fmodule, 1, SEEK_CUR);
    bufread(&p->source->head.rowCount, 1, 1, fmodule);
    bufread(&p->source->head.channelCount, 1, 1, fmodule);

    if (!trackCount || !sampleCount || !p->source->head.rowCount || p->source->head.rowCount > 64 || !p->source->head.channelCount || p->source->head.channelCount > 32)
        return (false);

    bufread(&p->source->head.pan, 1, 32, fmodule);

    for (i = 0; i < 32; ++i)
    {
        if (p->source->head.pan[i] <= 15)
        {
            // 8bitbubsy: WTF no, just do << 4 then if (p == 255) p = 256 in mixer
            //p->source->head.pan[i] -= (p->source->head.pan[i] & 8) / 8;
            //p->source->head.pan[i] = (((int)p->source->head.pan[i]) * 255) / 14;
            
            p->source->head.pan[i] <<= 4;
            p->source->head.volume[i] = 64;
        }
        else
        {
            p->source->head.pan[i] = 128;
            p->source->head.volume[i] = 0;
        }
    }

    for (i = 0; i < sampleCount; ++i)
    {
        MODULE_SAMPLE * s = &p->source->samples[i];

        bufseek(fmodule, 22, SEEK_CUR);

        s->length = bufGetDwordLittleEndian(fmodule);
        s->loopStart = bufGetDwordLittleEndian(fmodule);
        s->loopLength = bufGetDwordLittleEndian(fmodule) - s->loopStart;
        if (s->loopLength < 2)
            s->loopLength = 2;

        bufread(&s->fineTune, 1, 1, fmodule);
        s->fineTune = s->fineTune & 0x0F;

        bufread(&s->volume, 1, 1, fmodule);
        bufread(&s->attribute, 1, 1, fmodule);

        totalSampleSize += s->length;
    }

    bufread(&p->source->head.order, 1, 128, fmodule);

    tracksOffset = fmodule->length - fmodule->remain;
    sequencesOffset = tracksOffset + 192 * trackCount;
    commentOffset = sequencesOffset + 64 * p->source->head.patternCount;

    for (i = 0; i < p->source->head.patternCount; ++i)
    {
        note = p->source->patterns[i] = (modnote_t *)calloc(1, sizeof (modnote_t) * p->source->head.rowCount * p->source->head.channelCount);
        if (!note)
        {
            for (j = 0; j < i; ++j)
            {
                if (p->source->patterns[j])
                {
                    free(p->source->patterns[j]);
                    p->source->patterns[j] = NULL;
                }
            }
            return 0;
        }

        for (j = 0; j < p->source->head.channelCount; ++j)
        {
            int trackNumber;
            bufseek(fmodule, sequencesOffset + 64 * i + 2 * j, SEEK_SET);
            trackNumber = bufGetWordLittleEndian(fmodule);
            if (trackNumber--)
            {
                bufseek(fmodule, tracksOffset + 192 * trackNumber, SEEK_SET);
                for (k = 0; k < p->source->head.rowCount; ++k)
                {
                    unsigned char buf[3];
                    bufread(buf, 1, 3, fmodule);
                    if (buf[0] || buf[1] || buf[2])
                    {
                        note[k * p->source->head.channelCount + j].period = (buf[0] / 4) ? extendedRawPeriods[buf[0] / 4] : 0;
                        note[k * p->source->head.channelCount + j].sample = ((buf[0] << 4) + (buf[1] >> 4)) & 0x3f;
                        note[k * p->source->head.channelCount + j].command = buf[1] & 0xf;
                        note[k * p->source->head.channelCount + j].param = buf[2];
                        if (note[k * p->source->head.channelCount + j].command == 0xf && note[k * p->source->head.channelCount + j].param == 0x00)
                            note[k * p->source->head.channelCount + j].command = 0;
                    }
                }
            }
        }
    }

    p->source->sampleData = (char *)malloc(totalSampleSize);
    if (!p->source->sampleData)
    {
        for (i = 0; i < 256; ++i)
        {
            if (p->source->patterns[i] != NULL)
            {
                free(p->source->patterns[i]);
                p->source->patterns[i] = NULL;
            }
        }

        return (false);
    }

    bufseek(fmodule, commentOffset + commentLength, SEEK_SET);

    for (i = 0; i < sampleCount; ++i)
    {
        MODULE_SAMPLE * s = &p->source->samples[i];
        s->offset = sampleOffset;
        bufread(&p->source->sampleData[sampleOffset], 1, s->length, fmodule);
        
        if (!(s->attribute & 1))
        {
            for (j = (int)sampleOffset; (unsigned int)j < sampleOffset + s->length; ++j)
                p->source->sampleData[(unsigned int)j] ^= 0x80;
        }
                
        sampleOffset += s->length;
    }

    p->source->originalSampleData = (char *)malloc(totalSampleSize);
    if (p->source->originalSampleData == NULL)
    {
        free(p->source->sampleData);
        p->source->sampleData = NULL;

        for (i = 0; i < 256; ++i)
        {
            if (p->source->patterns[i] != NULL)
            {
                free(p->source->patterns[i]);
                p->source->patterns[i] = NULL;
            }
        }

        return (false);
    }

    memcpy(p->source->originalSampleData, p->source->sampleData, totalSampleSize);
    p->source->head.totalSampleSize = totalSampleSize;

    p->useLEDFilter = false;
    p->moduleLoaded = true;
    
    p->minPeriod = 14;
    p->maxPeriod = 1712;

    p->source->head.format = FORMAT_MTM;
    p->source->head.initBPM = 125;

    return (true);
}

static void checkModType(MODULE_HEADER *h, player *p, const char *buf)
{
    if (!strncmp(buf, "M.K.", 4))
    {
        h->format = FORMAT_MK; // ProTracker v1.x
        p->numChans = h->channelCount = 4; 
        p->minPeriod = PT_MIN_PERIOD;
        p->maxPeriod = PT_MAX_PERIOD;
        return;
    }
    else if (!strncmp(buf, "M!K!", 4))
    {
        h->format = FORMAT_MK2; // ProTracker v2.x (if >64 patterns)
        p->numChans = h->channelCount = 4;
        p->minPeriod = PT_MIN_PERIOD;
        p->maxPeriod = PT_MAX_PERIOD;
        return;
    }
    else if (!strncmp(buf, "FLT4", 4))
    {
        h->format = FORMAT_FLT4; // StarTrekker (4 channel MODs only)
        p->numChans = h->channelCount = 4;
        p->minPeriod = PT_MIN_PERIOD;
        p->maxPeriod = PT_MAX_PERIOD;
        return;
    }
    else if (!strncmp(buf, "FLT8", 4))
    {
        h->format = FORMAT_FLT8;
        p->numChans = h->channelCount = 8;
        p->minPeriod = PT_MIN_PERIOD;
        p->maxPeriod = PT_MAX_PERIOD;
        return;
    }
    else if (!strncmp(buf + 1, "CHN", 3) && buf[0] >= '1' && buf[0] <= '9')
    {
        h->format = FORMAT_NCHN; // FastTracker II (1-9 channel MODs)
        p->numChans = h->channelCount = buf[0] - '0';
        p->minPeriod = 14;
        p->maxPeriod = 1712;
        return;
    }
    else if (!strncmp(buf + 2, "CH", 2) && buf[0] >= '1' && buf[0] <= '3' && buf[1] >= '0' && buf[1] <= '9')
    {
        h->format = FORMAT_NNCH; // FastTracker II (10-32 channel MODs)
        p->numChans = h->channelCount = (buf[0] - '0') * 10 + (buf[1] - '0');
        if (h->channelCount > 32)
        {
            h->format = FORMAT_UNKNOWN;
            h->channelCount = 4;
        }

        p->minPeriod = 14;
        p->maxPeriod = 1712;
        return;
    }
    else if (!strncmp(buf, "16CN", 4))
    {
        h->format = FORMAT_16CN;
        p->numChans = h->channelCount = 16;
        p->minPeriod = 14;
        p->maxPeriod = 1712;
        return;
    }
    else if (!strncmp(buf, "32CN", 4))
    {
        h->format = FORMAT_32CN;
        p->numChans = h->channelCount = 32;
        p->minPeriod = 14;
        p->maxPeriod = 1712;
        return;
    }
    else if (!strncmp(buf, "N.T.", 4))
    {
        h->format = FORMAT_MK; // NoiseTracker 1.0, same as ProTracker v1.x (?)
        p->numChans = h->channelCount = 4;
        p->minPeriod = PT_MIN_PERIOD;
        p->maxPeriod = PT_MAX_PERIOD;
        return;
    }

    h->format = FORMAT_UNKNOWN; // May be The Ultimate SoundTracker, 15 samples
    p->numChans = h->channelCount = 4;
    p->minPeriod = PT_MIN_PERIOD;
    p->maxPeriod = PT_MAX_PERIOD;
}

int playptmod_LoadMem(void *_p, const unsigned char *buf, unsigned long bufLength)
{
    player *p = (player *)_p;
    unsigned char bytes[4];
    char modSig[4];
    char *smpDat8;
    char tempSample[131070];
    char iffHdrFound;
    int i;
    int j;
    int pattern;
    int row;
    int channel;
    int sampleOffset;
    int mightBeSTK;
    int lateVerSTKFlag;
    int numSamples;
    int tmp;
    unsigned long tempOffset;
    modnote_t *note;
    MODULE_SAMPLE *s;
    BUF *fmodule;

    sampleOffset = 0;
    lateVerSTKFlag = false;
    mightBeSTK = false;

    p->source = (MODULE *)calloc(1, sizeof (MODULE));
    if (p->source == NULL)
        return (false);

    fmodule = bufopen(buf, bufLength);
    if (fmodule == NULL)
    {
        free(p->source);

        return (false);
    }

    if (bufLength <= 1624)
    {
        free(p->source);
        bufclose(fmodule);

        return (false);
    }

    bufread(modSig, 1, 3, fmodule);
    if (!strncmp(modSig, "MTM", 3))
    {
        i = playptmod_LoadMTM(p, fmodule);
        bufclose(fmodule);
        return i;
    }

    bufseek(fmodule, 0x0438, SEEK_SET);
    bufread(modSig, 1, 4, fmodule);

    checkModType(&p->source->head, p, modSig);
    if (p->source->head.format == FORMAT_UNKNOWN)
        mightBeSTK = true;

    bufseek(fmodule, 20, SEEK_SET);

    for (i = 0; i < MOD_SAMPLES; ++i)
    {
        s = &p->source->samples[i];

        if ((mightBeSTK == true) && (i > 14))
        {
            s->loopLength = 2;
        }
        else
        {
            bufseek(fmodule, 22, SEEK_CUR);

            s->length = bufGetWordBigEndian(fmodule) * 2;
            if (s->length > 9999)
                lateVerSTKFlag = true;

            bufread(&s->fineTune, 1, 1, fmodule);
            s->fineTune = s->fineTune & 0x0F;

            bufread(&s->volume, 1, 1, fmodule);
            if (s->volume > 64)
                s->volume = 64;

            if (mightBeSTK == true)
                s->loopStart = bufGetWordBigEndian(fmodule);
            else
                s->loopStart = bufGetWordBigEndian(fmodule) * 2;

            s->loopLength = bufGetWordBigEndian(fmodule) * 2;

            // fix for poorly converted STK->PTMOD modules.
            if (!mightBeSTK && ((s->loopStart + s->loopLength) > s->length))
            {
                if (((s->loopStart >> 1) + s->loopLength) <= s->length)
                {
                    s->loopStart >>= 1;
                }
                else
                {
                    s->loopStart  = 0;
                    s->loopLength = 2;
                }
            }

            if (s->loopLength < 2)
                s->loopLength = 2;

            if (mightBeSTK == true)
            {
                if (s->loopLength > 2)
                {
                    tmp = s->loopStart;
                    s->length -= s->loopStart;
                    s->loopStart = 0;
                    s->tmpLoopStart = tmp;
                }

                s->fineTune = 0;
            }

            s->attribute = 0;
        }
    }

    bufread(&p->source->head.orderCount, 1, 1, fmodule);
    if (p->source->head.orderCount == 0)
    {
        free(p->source);
        bufclose(fmodule);

        return (false);
    }
    
    if (p->source->head.orderCount > 128)
        p->source->head.orderCount = 128;
        
    bufread(&p->source->head.restartPos, 1, 1, fmodule);
    if ((mightBeSTK == true) && ((p->source->head.restartPos == 0)
        || (p->source->head.restartPos > 220)))
    {

        free(p->source);
        bufclose(fmodule);

        return (false);
    }

    p->source->head.initBPM = 125;

    if (mightBeSTK == true)
    {
        p->source->head.format = FORMAT_STK;

        if (p->source->head.restartPos == 120)
        {
            p->source->head.restartPos = 125;
        }
        else
        {
            if (p->source->head.restartPos > 239)
                p->source->head.restartPos = 239;

            p->source->head.initBPM = (short)(1773447 / ((240 - p->source->head.restartPos) * 122));
        }
    }

    for (i = 0; i < 128; ++i)
    {
        bufread(&p->source->head.order[i], 1, 1, fmodule);

        if (p->source->head.order[i] > p->source->head.patternCount)
            p->source->head.patternCount = p->source->head.order[i];
    }

    p->source->head.patternCount++;

    if (p->source->head.format != FORMAT_STK)
        bufseek(fmodule, 4, SEEK_CUR);

    for (pattern = 0; pattern < p->source->head.patternCount; ++pattern)
    {
        p->source->patterns[pattern] = (modnote_t *)calloc(64 * p->source->head.channelCount, sizeof (modnote_t));
        if (p->source->patterns[pattern] == NULL)
        {
            for (i = 0; i < pattern; ++i)
            {
                if (p->source->patterns[i] != NULL)
                {
                    free(p->source->patterns[i]);
                    p->source->patterns[i] = NULL;
                }
            }

            bufclose(fmodule);
            free(p->source);

            return (false);
        }
    }

    for (pattern = 0; pattern < p->source->head.patternCount; ++pattern)
    {
        note = p->source->patterns[pattern];
        if (p->source->head.format == FORMAT_FLT8)
        {
            for (row = 0; row < 64; ++row)
            {
                for (channel = 0; channel < 8; ++channel)
                {
                    unsigned char bytes[4];

                    if (channel == 0 && row > 0) bufseek(fmodule, -1024, SEEK_CUR);
                    else if (channel == 4) bufseek(fmodule, 1024 - 4 * 4, SEEK_CUR);

                    bufread(bytes, 1, 4, fmodule);

                    note->period = (LO_NYBBLE(bytes[0]) << 8) | bytes[1];
                    if (note->period != 0) // FLT8 is 113..856 only
                        note->period = CLAMP(note->period, 113, 856);

                    note->sample = (bytes[0] & 0xF0) | HI_NYBBLE(bytes[2]);
                    note->command = LO_NYBBLE(bytes[2]);
                    note->param = bytes[3];

                    note++;
                }
            }
        }
        else
        {
            for (row = 0; row < 64; ++row)
            {
                for (channel = 0; channel < p->source->head.channelCount; ++channel)
                {
                    bufread(bytes, 1, 4, fmodule);

                    note->period = (LO_NYBBLE(bytes[0]) << 8) | bytes[1];

                    if (note->period != 0)
                    {
                        if ((unsigned)(note->period - 113) > (856-113))
                        {
                            p->minPeriod = 14;
                            p->maxPeriod = 1712;
                        }
                        note->period = CLAMP(note->period, p->minPeriod, p->maxPeriod);
                    }

                    note->sample = (bytes[0] & 0xF0) | HI_NYBBLE(bytes[2]);
                    note->command = LO_NYBBLE(bytes[2]);
                    note->param = bytes[3];

                    if (mightBeSTK == true)
                    {
                        if (lateVerSTKFlag == false)
                        {
                            if (note->command == 0x01)
                            {
                                note->command = 0x00;
                            }
                            else if (note->command == 0x02)
                            {
                                if (note->param & 0xF0)
                                {
                                    note->command = 0x02;
                                    note->param >>= 4;
                                }
                                else if (note->param & 0x0F)
                                {
                                    note->command = 0x01;
                                }
                            }
                        }

                        if (note->command == 0x0D)
                        {
                            if (note->param == 0)
                                note->command = 0x0D;
                            else
                                note->command = 0x0A;
                        }
                    }

                    note++;
                }
            }
        }
    }

    tempOffset = buftell(fmodule);

    sampleOffset = 0;

    numSamples = (p->source->head.format == FORMAT_STK) ? 15 : 31;   
    for (i = 0; i < numSamples; ++i)
    {
        iffHdrFound = 0;

        s = &p->source->samples[i];
        s->offset = sampleOffset;
        
        j = (s->length + 1) / 2 + 5 + 16;
        if ( j > s->length ) j = s->length;
        
        bufread(tempSample, 1, j, fmodule);

        smpDat8 = tempSample;
        
        if (j > 5 + 16 && memcmp(smpDat8, "ADPCM", 5) == 0)
        {
            s->reallength = j;
        }
        else
        {
            s->reallength = s->length;
            bufread(tempSample + j, 1, s->length - j, fmodule);
            if (s->length > 8)
            {
                for (j = 0; j < (s->length - 8); ++j)
                {
                    if (memcmp(smpDat8, "8SVXVHDR", 8) == 0)
                        iffHdrFound = 1;
                    
                    if (iffHdrFound)
                    {
                        if (memcmp(smpDat8, "BODY", 4) == 0)
                        {
                            s->iffSize = j + 8;
                            s->length -= s->iffSize;
                            
                            // Relocate loopStart
                            if (s->loopStart >= ((int)(((j) + 8) & 0xFFFFFFFE))) // even'ify unit
                                s->loopStart -= ((int)(((j) + 8) & 0xFFFFFFFE));
                            
                            // Fix loopStart+loopLength > sampleLength
                            while ((s->loopStart + s->loopLength) > s->length)
                                s->loopLength--;
                            
                            break;
                        }
                    }
                    
                    ++smpDat8;
                }
            }
        }
        
        sampleOffset += s->length;
        p->source->head.totalSampleSize += s->length;
    }

    p->source->sampleData = (char *)malloc(p->source->head.totalSampleSize);
    if (p->source->sampleData == NULL)
    {
        bufclose(fmodule);
        for (pattern = 0; pattern < 256; ++i)
        {
            if (p->source->patterns[pattern] != NULL)
            {
                free(p->source->patterns[pattern]);
                p->source->patterns[pattern] = NULL;
            }
        }
        free(p->source);

        return (false);
    }

    bufseek(fmodule, tempOffset, SEEK_SET);

    numSamples = (p->source->head.format == FORMAT_STK) ? 15 : 31;
    for (i = 0; i < numSamples; ++i)
    {
        s = &p->source->samples[i];

        if (s->iffSize > 0)
            bufseek(fmodule, s->iffSize, SEEK_CUR);
        
        if (s->reallength < s->length)
        {
            const signed char * compressionTable = (const signed char *) tempSample + 5;
            const unsigned char * adpcmData = (const unsigned char *) tempSample + 5 + 16;
            int delta = 0;
            bufread(tempSample, 1, s->reallength, fmodule);
            for ( j = 0; j < s->length; ++j )
            {
                delta += compressionTable[ LO_NYBBLE( *adpcmData ) ];
                p->source->sampleData[s->offset + j] = delta;
                if ( ++j >= s->length ) break;
                delta += compressionTable[ HI_NYBBLE( *adpcmData ) ];
                p->source->sampleData[s->offset + j] = delta;
                ++adpcmData;
            }
        }
        else if ((mightBeSTK == true) && (s->loopLength > 2))
        {
            for (j = 0; j < s->tmpLoopStart; ++j)
                bufseek(fmodule, 1, SEEK_CUR);

            bufread(&p->source->sampleData[s->offset], 1, s->length - s->loopStart, fmodule);
        }
        else
        {
            bufread(&p->source->sampleData[s->offset], 1, s->length, fmodule);
        }
    }

    p->source->originalSampleData = (char *) malloc(p->source->head.totalSampleSize);
    if (p->source->originalSampleData == NULL)
    {
        bufclose(fmodule);
        free(p->source->sampleData);
        for (pattern = 0; pattern < 256; ++i)
        {
            if (p->source->patterns[pattern] != NULL)
            {
                free(p->source->patterns[pattern]);
                p->source->patterns[pattern] = NULL;
            }
        }
        free(p->source);

        return (false);
    }

    memcpy(p->source->originalSampleData, p->source->sampleData, p->source->head.totalSampleSize);

    bufclose(fmodule);

    p->source->head.rowCount = MOD_ROWS;
    memset(p->source->head.volume, 64, MAX_CHANNELS);
    for (i = 0; i < MAX_CHANNELS; ++i)
        p->source->head.pan[i] = ((i + 1) & 2) ? 160 : 96;

    p->useLEDFilter = false;
    p->moduleLoaded = true;

    return (true);
}

int playptmod_Load(void *_p, const char *filename)
{
    player *p = (player *)_p;
    if (!p->moduleLoaded)
    {
        int i;
        unsigned char *buffer;
        unsigned long fileSize;
        FILE *fileModule;

        fileModule = fopen(filename, "rb");
        if (fileModule == NULL)
            return (false);

        fseek(fileModule, 0, SEEK_END);
        fileSize = ftell(fileModule);
        fseek(fileModule, 0, SEEK_SET);

        buffer = (unsigned char *)malloc(fileSize);
        if (buffer == NULL)
        {
            fclose(fileModule);
            return (false);
        }

        fread(buffer, 1, fileSize, fileModule);
        fclose(fileModule);

        i = playptmod_LoadMem(_p, buffer, fileSize);

        free(buffer);

        return i;
    }

    return (false);
}

int playptmod_GetFormat(void *p)
{
    return ((player *)p)->source->head.format;
}

static void fxArpeggio(player *p, mod_channel *ch);
static void fxPortamentoSlideUp(player *p, mod_channel *ch);
static void fxPortamentoSlideDown(player *p, mod_channel *ch);
static void fxGlissando(player *p, mod_channel *ch);
static void fxVibrato(player *p, mod_channel *ch);
static void fxGlissandoVolumeSlide(player *p, mod_channel *ch);
static void fxVibratoVolumeSlide(player *p, mod_channel *ch);
static void fxTremolo(player *p, mod_channel *ch);
static void fxNotInUse(player *p, mod_channel *ch);
static void fxSampleOffset(player *p, mod_channel *ch);
static void fxSampleOffset_FT2(player *p, mod_channel *ch);
static void fxVolumeSlide(player *p, mod_channel *ch);
static void fxPositionJump(player *p, mod_channel *ch);
static void fxSetVolume(player *p, mod_channel *ch);
static void fxPatternBreak(player *p, mod_channel *ch);
static void fxExtended(player *p, mod_channel *ch);
static void fxSetTempo(player *p, mod_channel *ch);
static void efxSetLEDFilter(player *p, mod_channel *ch);
static void efxFinePortamentoSlideUp(player *p, mod_channel *ch);
static void efxFinePortamentoSlideDown(player *p, mod_channel *ch);
static void efxGlissandoControl(player *p, mod_channel *ch);
static void efxVibratoControl(player *p, mod_channel *ch);
static void efxSetFineTune(player *p, mod_channel *ch);
static void efxPatternLoop(player *p, mod_channel *ch);
static void efxTremoloControl(player *p, mod_channel *ch);
static void efxKarplusStrong(player *p, mod_channel *ch);
static void efxRetrigNote(player *p, mod_channel *ch);
static void efxFineVolumeSlideUp(player *p, mod_channel *ch);
static void efxFineVolumeSlideDown(player *p, mod_channel *ch);
static void efxNoteCut(player *p, mod_channel *ch);
static void efxNoteDelay(player *p, mod_channel *ch);
static void efxPatternDelay(player *p, mod_channel *ch);
static void efxInvertLoop(player *p, mod_channel *ch);

static void fxExtended_FT2(player *p, mod_channel *ch);
static void fxPan(player *p, mod_channel *ch);
static void efxPan(player *p, mod_channel *ch);

typedef void (*effect_routine)(player *p, mod_channel *);

static effect_routine fxRoutines[16] =
{
    fxArpeggio,
    fxPortamentoSlideUp,
    fxPortamentoSlideDown,
    fxGlissando,
    fxVibrato,
    fxGlissandoVolumeSlide,
    fxVibratoVolumeSlide,
    fxTremolo,
    fxNotInUse,
    fxSampleOffset,
    fxVolumeSlide,
    fxPositionJump,
    fxSetVolume,
    fxPatternBreak,
    fxExtended,
    fxSetTempo
};

static effect_routine fxRoutines_FT2[16] =
{
    fxArpeggio,
    fxPortamentoSlideUp,
    fxPortamentoSlideDown,
    fxGlissando,
    fxVibrato,
    fxGlissandoVolumeSlide,
    fxVibratoVolumeSlide,
    fxTremolo,
    fxPan,
    fxSampleOffset_FT2,
    fxVolumeSlide,
    fxPositionJump,
    fxSetVolume,
    fxPatternBreak,
    fxExtended_FT2,
    fxSetTempo
};

static effect_routine efxRoutines[16] =
{
    efxSetLEDFilter,
    efxFinePortamentoSlideUp,
    efxFinePortamentoSlideDown,
    efxGlissandoControl,
    efxVibratoControl,
    efxSetFineTune,
    efxPatternLoop,
    efxTremoloControl,
    efxKarplusStrong,
    efxRetrigNote,
    efxFineVolumeSlideUp,
    efxFineVolumeSlideDown,
    efxNoteCut,
    efxNoteDelay,
    efxPatternDelay,
    efxInvertLoop
};

static effect_routine efxRoutines_FT2[16] =
{
    fxNotInUse,
    efxFinePortamentoSlideUp,
    efxFinePortamentoSlideDown,
    efxGlissandoControl,
    efxVibratoControl,
    efxSetFineTune,
    efxPatternLoop,
    efxTremoloControl,
    efxPan,
    efxRetrigNote,
    efxFineVolumeSlideUp,
    efxFineVolumeSlideDown,
    efxNoteCut,
    efxNoteDelay,
    efxPatternDelay,
    fxNotInUse
};

static void processInvertLoop(player *p, mod_channel *ch)
{
    char invertLoopTemp;
    char *invertLoopData;
    MODULE_SAMPLE *s;

    if (ch->invertLoopSpeed > 0)
    {
        ch->invertLoopDelay += invertLoopSpeeds[ch->invertLoopSpeed];
        if (ch->invertLoopDelay >= 128)
        {
            ch->invertLoopDelay = 0;

            if (ch->sample != 0)
            {
                s = &p->source->samples[ch->sample - 1];
                if (s->loopLength > 2)
                {
                    ch->invertLoopOffset++;
                    if (ch->invertLoopOffset >= (s->loopStart + s->loopLength))
                        ch->invertLoopOffset = s->loopStart;

                    invertLoopData = &p->source->sampleData[s->offset + ch->invertLoopOffset];
                    invertLoopTemp = -1 - *invertLoopData;
                    *invertLoopData = invertLoopTemp;
                }
            }
        }
    }
}

static void efxSetLEDFilter(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
        p->useLEDFilter = !(ch->param & 1);
}

static void efxFinePortamentoSlideUp(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
    {
        if (p->tempPeriod > 0)
        {
            ch->period -= LO_NYBBLE(ch->param);

            if (p->minPeriod == PT_MIN_PERIOD)
            {
                if (ch->period < 113)
                    ch->period = 113;
            }
            else
            {
                if (ch->period < p->minPeriod)
                    ch->period = p->minPeriod;
           }

            p->tempPeriod = ch->period;
        }
    }
}

static void efxFinePortamentoSlideDown(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
    {
        if (p->tempPeriod > 0)
        {
            ch->period += LO_NYBBLE(ch->param);

            if (p->minPeriod == PT_MIN_PERIOD)
            {
                if (ch->period > 856)
                    ch->period = 856;
            }
            else
            {
                if (ch->period > p->maxPeriod)
                    ch->period = p->maxPeriod;
            }

            p->tempPeriod = ch->period;
        }
    }
}

static void efxGlissandoControl(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
        ch->glissandoControl = LO_NYBBLE(ch->param);
}

static void efxVibratoControl(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
        ch->vibratoControl = LO_NYBBLE(ch->param);
}

static void efxSetFineTune(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
        ch->fineTune = LO_NYBBLE(ch->param);
}

static void efxPatternLoop(player *p, mod_channel *ch)
{
    unsigned char tempParam;

    if (p->modTick == 0)
    {
        tempParam = LO_NYBBLE(ch->param);
        if (tempParam == 0)
        {
            ch->patternLoopRow = p->modRow;

            return;
        }

        if (ch->patternLoopCounter == 0)
        {
            ch->patternLoopCounter = tempParam;
        }
        else
        {
            ch->patternLoopCounter--;
            if (ch->patternLoopCounter == 0)
                return;
        }

        p->PBreakPosition = ch->patternLoopRow;
        p->PBreakFlag = true;
    }
}

static void efxTremoloControl(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
        ch->tremoloControl = LO_NYBBLE(ch->param);
}

static void efxKarplusStrong(player *p, mod_channel *ch)
{
    // WTF !
    // KarplusStrong sucks, since some MODs
    // used E8x for other effects instead.

    (void)ch;

}

static void efxRetrigNote(player *p, mod_channel *ch)
{
    unsigned char retrigTick;

    retrigTick = LO_NYBBLE(ch->param);
    if (retrigTick > 0)
    {
        if ((p->modTick % retrigTick) == 0)
            p->tempFlags |= TEMPFLAG_START;
    }
}

static void efxFineVolumeSlideUp(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
    {
        ch->volume += LO_NYBBLE(ch->param);

        if (ch->volume > 64)
            ch->volume = 64;

        p->tempVolume = ch->volume;
    }
}

static void efxFineVolumeSlideDown(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
    {
        ch->volume -= LO_NYBBLE(ch->param);

        if (ch->volume < 0)
            ch->volume = 0;

        p->tempVolume = ch->volume;
    }
}

static void efxNoteCut(player *p, mod_channel *ch)
{
    if (p->modTick == LO_NYBBLE(ch->param))
    {
        ch->volume = 0;
        p->tempVolume = 0;
    }
}

static void efxNoteDelay(player *p, mod_channel *ch)
{
    unsigned char delayTick;

    delayTick = LO_NYBBLE(ch->param);

    if (p->modTick == 0)
        ch->tempFlagsBackup = p->tempFlags;

    if (p->modTick < delayTick)
        p->tempFlags = TEMPFLAG_DELAY;
    else if (p->modTick == delayTick)
        p->tempFlags = ch->tempFlagsBackup;
}

static void efxPatternDelay(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
    {
        if (p->PattDelayTime2 == 0)
        {
            p->pattDelayFlag = true;
            p->PattDelayTime = LO_NYBBLE(ch->param) + 1;
        }
    }
}

static void efxInvertLoop(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
    {
        ch->invertLoopSpeed = LO_NYBBLE(ch->param);

        if (ch->invertLoopSpeed > 0)
            processInvertLoop(p, ch);
    }
}

static void handleGlissando(player *p, mod_channel *ch)
{
    char l;
    char m;
    char h;
    
    short *tablePointer;

    if (p->tempPeriod > 0)
    {
        if (ch->period < ch->tempPeriod)
        {
            ch->period += ch->glissandoSpeed;

            if (ch->period > ch->tempPeriod)
                ch->period = ch->tempPeriod;
        }
        else
        {
            ch->period -= ch->glissandoSpeed;

            if (ch->period < ch->tempPeriod)
                ch->period = ch->tempPeriod;
        }

        if (ch->glissandoControl != 0)
        {
            if (p->minPeriod == PT_MIN_PERIOD)
            {
                l = 0;
                h = 35;

                tablePointer = (short *)&rawAmigaPeriods[ch->fineTune * 37];
                while (h >= l)
                {
                    m = (h + l) / 2;

                    if (tablePointer[m] == ch->period)
                    {
                        p->tempPeriod = tablePointer[m];
                        break;
                    }
                    else if (tablePointer[m] > ch->period)
                    {
                        l = m + 1;
                    }
                    else
                    {
                        h = m - 1;
                    }
                }
            }
            else
            {
                l = 0;
                h = 83;

                tablePointer = (short *)&extendedRawPeriods[ch->fineTune * 85];
                while (h >= l)
                {
                    m = (h + l) / 2;

                    if (tablePointer[m] == ch->period)
                    {
                        p->tempPeriod = tablePointer[m];
                        break;
                    }
                    else if (tablePointer[m] > ch->period)
                    {
                        l = m + 1;
                    }
                    else
                    {
                        h = m - 1;
                    }
                }
            }
        }
        else
        {
            p->tempPeriod = ch->period;
        }
    }
}

static void processVibrato(player *p, mod_channel *ch)
{
    unsigned char vibratoTemp;
    int vibratoData;
    int applyVibrato;

    applyVibrato = 1;
    if ((p->minPeriod == PT_MIN_PERIOD) && (p->modTick == 0)) // PT/NT/UST/STK
        applyVibrato = 0;
    
    if (applyVibrato)
    {
        if (p->tempPeriod > 0)
        {
            vibratoTemp = ch->vibratoPos >> 2;
            vibratoTemp &= 0x1F;

            switch (ch->vibratoControl & 3)
            {
                case 0:
                    vibratoData = p->sinusTable[vibratoTemp];
                break;

                case 1:
                {
                    if (ch->vibratoPos < 128)
                        vibratoData = vibratoTemp << 3;
                    else
                        vibratoData = 255 - (vibratoTemp << 3);
                }
                break;

                default:
                    vibratoData = 255;
                break;
            }

            vibratoData = (vibratoData * ch->vibratoDepth) >> 7;

            if (ch->vibratoPos < 128)
            {
                p->tempPeriod += (short)vibratoData;
                if (p->tempPeriod > p->maxPeriod)
                    p->tempPeriod = p->maxPeriod;
            }
            else
            {
                p->tempPeriod -= (short)vibratoData;
                if (p->tempPeriod < p->minPeriod)
                    p->tempPeriod = p->minPeriod;
            }
        }
    }
     
    if (p->modTick > 0)
        ch->vibratoPos += (ch->vibratoSpeed << 2);
}

static void processTremolo(player *p, mod_channel *ch)
{
    unsigned char tremoloTemp;
    int tremoloData;
    int applyTremolo;
    
    applyTremolo = 1;
    if ((p->minPeriod == PT_MIN_PERIOD) && (p->modTick == 0)) // PT/NT/UST/STK
        applyTremolo = 0;
    
    if (applyTremolo)
    {
        if (p->tempVolume > 0)
        {
            tremoloTemp = ch->tremoloPos >> 2;
            tremoloTemp &= 0x1F;

            switch (ch->tremoloControl & 3)
            {
                case 0:
                    tremoloData = p->sinusTable[tremoloTemp];
                break;

                case 1:
                {
                    if (ch->vibratoPos < 128)
                        tremoloData = tremoloTemp << 3;
                    else
                        tremoloData = 255 - (tremoloTemp << 3);
                }
                break;

                default:
                    tremoloData = 255;
                break;
            }

            tremoloData = (tremoloData * ch->tremoloDepth) >> 6;

            if (ch->tremoloPos < 128)
            {
                p->tempVolume += (char)tremoloData;
                if (p->tempVolume > 64)
                    p->tempVolume = 64;
            }
            else
            {
                p->tempVolume -= (char)tremoloData;
                if (p->tempVolume < 0)
                    p->tempVolume = 0;
            }
        }
    }

    if (p->modTick > 0)
        ch->tremoloPos += (ch->tremoloSpeed << 2);
}

static void fxArpeggio(player *p, mod_channel *ch)
{
    char l;
    char m;
    char h;
    char noteToAdd;
    char arpeggioTick;
    unsigned char i;
    short *tablePointer;
    
    noteToAdd = 0;
    
    arpeggioTick = p->modTick % 3;
    if (arpeggioTick == 0)
    {
        p->tempPeriod = ch->period;
        return;
    }
    else if (arpeggioTick == 1)
    {
        noteToAdd = HI_NYBBLE(ch->param);
    }
    else if (arpeggioTick == 2)
    {
        noteToAdd = LO_NYBBLE(ch->param);
    }
    
    if (p->minPeriod == PT_MIN_PERIOD) // PT/NT/UST/STK
    {
        tablePointer = (short *)&rawAmigaPeriods[ch->fineTune * 37];
        for (i = 0; i < 36; ++i)
        {
            if (ch->period >= tablePointer[i])
            {
                p->tempPeriod = tablePointer[i + noteToAdd];
                break;
            }
        }
    }
    else
    {
        l = 0;
        h = 83;
        
        tablePointer = (short *)&extendedRawPeriods[ch->fineTune * 85];
        while (h >= l)
        {
            m = (h + l) / 2;
            
            if (tablePointer[m] == ch->period)
            {
                p->tempPeriod = tablePointer[m + noteToAdd];
                break;
            }
            else if (tablePointer[m] > ch->period)
            {
                l = m + 1;
            }
            else
            {
                h = m - 1;
            }
        }
    }
}

static void fxPortamentoSlideUp(player *p, mod_channel *ch)
{
    if ((p->modTick > 0) && (p->tempPeriod > 0))
    {
        ch->period -= ch->param;

        if (p->minPeriod == PT_MIN_PERIOD)
        {
            if (ch->period < 113)
                ch->period = 113;
        }
        else
        {
            if (ch->period < p->minPeriod)
                ch->period = p->minPeriod;
        }
        
        p->tempPeriod = ch->period;
    }
}

static void fxPortamentoSlideDown(player *p, mod_channel *ch)
{
    if ((p->modTick > 0) && (p->tempPeriod > 0))
    {
        ch->period += ch->param;
        
        if (p->minPeriod == PT_MIN_PERIOD)
        {
            if (ch->period > 856)
                ch->period = 856;
        }
        else
        {
            if (ch->period > p->maxPeriod)
                ch->period = p->maxPeriod;
        }

        p->tempPeriod = ch->period;
    }
}

static void fxGlissando(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
    {
        if (ch->param != 0)
            ch->glissandoSpeed = ch->param;
    }
    else
    {
        handleGlissando(p, ch);
    }
}

static void fxVibrato(player *p, mod_channel *ch)
{
    unsigned char hiNybble;
    unsigned char loNybble;

    if (p->modTick == 0)
    {
        hiNybble = HI_NYBBLE(ch->param);
        loNybble = LO_NYBBLE(ch->param);

        if (hiNybble != 0)
            ch->vibratoSpeed = hiNybble;

        if (loNybble != 0)
            ch->vibratoDepth = loNybble;
    }
    
    processVibrato(p, ch);
}

static void fxGlissandoVolumeSlide(player *p, mod_channel *ch)
{
    if (p->modTick > 0)
    {
        handleGlissando(p, ch);
        fxVolumeSlide(p, ch);
    }
}

static void fxVibratoVolumeSlide(player *p, mod_channel *ch)
{
    if (p->modTick > 0)
    {
        processVibrato(p, ch);
        fxVolumeSlide(p, ch);
    }
}

static void fxTremolo(player *p, mod_channel *ch)
{
    unsigned char hiNybble;
    unsigned char loNybble;

    if (p->modTick == 0)
    {
        hiNybble = HI_NYBBLE(ch->param);
        loNybble = LO_NYBBLE(ch->param);

        if (hiNybble > 0)
            ch->tremoloSpeed = hiNybble;

        if (loNybble > 0)
            ch->tremoloDepth = loNybble;
    }
    
    processTremolo(p, ch);
}

static void fxNotInUse(player *p, mod_channel *ch)
{
    (void)p;
    (void)ch;
}

static void fxSampleOffset(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
    {
        if (ch->param > 0)
            ch->offsetTemp = (unsigned short)(ch->param) * 256;

        ch->offset += ch->offsetTemp;
        
        if (!ch->noNote)
            ch->offsetBugNotAdded = false;
    }
}

static void fxSampleOffset_FT2(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
    {
        if (ch->param > 0)
            ch->offset = (unsigned short)(ch->param) * 256;

        ch->offsetBugNotAdded = true;
    }
}

static void fxVolumeSlide(player *p, mod_channel *ch)
{
    unsigned char hiNybble;
    unsigned char loNybble;

    if (p->modTick > 0)
    {
        hiNybble = HI_NYBBLE(ch->param);
        loNybble = LO_NYBBLE(ch->param);

        if (hiNybble == 0)
        {
            ch->volume -= loNybble;
            if (ch->volume < 0)
                ch->volume = 0;

            p->tempVolume = ch->volume;
        }
        else
        {
            ch->volume += hiNybble;
            if (ch->volume > 64)
                ch->volume = 64;

            p->tempVolume = ch->volume;
        }
    }
}

static void fxPositionJump(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
    {
        p->modOrder = ch->param - 1;
        p->PBreakPosition = 0;
        p->PosJumpAssert = true;
    }
}

static void fxSetVolume(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
    {
        if (ch->param > 64)
            ch->param = 64;

        ch->volume = ch->param;
        p->tempVolume = ch->param;
    }
}

static void fxPatternBreak(player *p, mod_channel *ch)
{
    unsigned char pos;

    if (p->modTick == 0)
    {
        pos = ((HI_NYBBLE(ch->param) * 10) + LO_NYBBLE(ch->param));

        if (pos > 63)
            p->PBreakPosition = 0;
        else
            p->PBreakPosition = pos;

        p->pattBreakBugPos = p->PBreakPosition;
        p->pattBreakFlag = true;
        p->PosJumpAssert = true;
    }
}

static void fxExtended(player *p, mod_channel *ch)
{
    efxRoutines[HI_NYBBLE(ch->param)](p, ch);
}

static void fxExtended_FT2(player *p, mod_channel *ch)
{
    efxRoutines_FT2[HI_NYBBLE(ch->param)](p, ch);
}

static void modSetSpeed(player *p, unsigned char speed)
{
    p->modSpeed = speed;
}

static void modSetTempo(player *p, unsigned short bpm)
{
    p->modBPM = bpm;
    p->samplesPerTick = p->tempoTimerVal / bpm;
}

static void fxSetTempo(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
    {
        if (ch->param > 0)
        {
            if ((ch->param < 32) || p->vBlankTiming)
                modSetSpeed(p, ch->param);
            else
                modSetTempo(p, ch->param);
        }
        else
        {
            /* Bit of a hack, will alert caller that song has restarted */
            p->modOrder = p->source->head.restartPos;
            p->PBreakPosition = 0;
            p->PosJumpAssert = true;
        }
    }
}

static void processEffects(player *p, mod_channel *ch)
{
    processInvertLoop(p, ch);

    if ((!ch->command && !ch->param) == 0)
    {
        switch (p->source->head.format)
        {
            case FORMAT_NCHN:
            case FORMAT_NNCH:
            case FORMAT_16CN:
            case FORMAT_32CN:
                fxRoutines_FT2[ch->command](p, ch);
            break;

            default:
                fxRoutines[ch->command](p, ch);
            break;
        }
    }
}

static void fxPan(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
        mixerSetChPan(p, ch->chanIndex, ch->param <= 128 ? ch->param * 2 : 128);
}

static void efxPan(player *p, mod_channel *ch)
{
    if (p->modTick == 0)
        mixerSetChPan(p, ch->chanIndex, LO_NYBBLE(ch->param) << 4);
}

void playptmod_Stop(void *_p)
{
    player *p = (player *)_p;
    int i;

    mixerCutChannels(p);

    p->modulePlaying = false;

    for (i = 0; i < p->source->head.channelCount; ++i)
    {
        p->source->channels[i].patternLoopCounter = 0;
        p->source->channels[i].glissandoControl = 0;
        p->source->channels[i].vibratoControl = 0;
        p->source->channels[i].tremoloControl = 0;
        p->source->channels[i].fineTune = 0;
        p->source->channels[i].invertLoopSpeed = 0;
        p->source->channels[i].period = 0;
        p->source->channels[i].tempPeriod = 0;
        p->source->channels[i].offsetBugNotAdded = true;
    }

    p->tempFlags = 0;
    p->pattBreakBugPos = -1;
    p->pattBreakFlag = false;
    p->pattDelayFlag = false;
    p->forceEffectsOff = false;
    p->PattDelayTime = 0;
    p->PattDelayTime2 = 0;
    p->PBreakPosition = 0;
    p->PosJumpAssert = false;
}

static void fetchPatternData(player *p, mod_channel *ch)
{
    int tempNote;
    modnote_t *note;

    note = &p->source->patterns[p->modPattern][(p->modRow * p->source->head.channelCount) + ch->chanIndex];
    if ((note->sample > 0) && (note->sample <= 32))
    {
        if (ch->sample != note->sample)
            ch->flags |= FLAG_NEWSAMPLE;

        ch->sample = note->sample;
        ch->flags |= FLAG_SAMPLE;
        ch->fineTune = p->source->samples[ch->sample - 1].fineTune;
    }

    ch->command = note->command;
    ch->param = note->param;

    if (note->period > 0)
    {
        if (ch->command == 0x0E)
        {
            if (HI_NYBBLE(ch->param) == 0x05)
                ch->fineTune = LO_NYBBLE(ch->param);
        }

        tempNote = periodToNote(p, 0, note->period);  
        if ((p->minPeriod == PT_MIN_PERIOD) && (tempNote == 36)) // PT/NT/STK/UST only
        {
            ch->noNote = true;
            mixerSetChSource(p, ch->chanIndex, NULL, 0, 0, 0, 0, 0);
        }
        else
        {
            ch->noNote = false;
            ch->tempPeriod = (p->minPeriod == PT_MIN_PERIOD) ? rawAmigaPeriods[(ch->fineTune * 37) + tempNote] : extendedRawPeriods[(ch->fineTune * 85) + tempNote];
            ch->flags |= FLAG_NOTE;
        }
    }
    else
    {
        ch->noNote = true;
    }
}

static void processChannel(player *p, mod_channel *ch)
{
    MODULE_SAMPLE *s;

    p->tempFlags = 0;

    if (p->modTick == 0)
    {
        if (p->PattDelayTime2 == 0)
            fetchPatternData(p, ch);

        if (ch->flags & FLAG_NOTE)
        {
            ch->flags &= ~FLAG_NOTE;

            if ((ch->command != 0x03) && (ch->command != 0x05))
            {
                ch->period = ch->tempPeriod;

                if (ch->sample > 0)
                    p->tempFlags |= TEMPFLAG_START;
            }

            ch->tempFlagsBackup = 0;

            if ((ch->vibratoControl & 4) == 0)
                ch->vibratoPos = 0;

            if ((ch->tremoloControl & 4) == 0)
                ch->tremoloPos = 0;
        }

        if (ch->flags & FLAG_SAMPLE)
        {
            ch->flags &= ~FLAG_SAMPLE;

            if (ch->sample > 0)
            {
                s = &p->source->samples[ch->sample - 1];

                ch->volume = s->volume;
                ch->invertLoopOffset = s->loopStart;

                if ((ch->command != 0x03) && (ch->command != 0x05))
                {
                    ch->offset = 0;
                    ch->offsetBugNotAdded = true;
                }

                if (ch->flags & FLAG_NEWSAMPLE)
                {
                    ch->flags &= ~FLAG_NEWSAMPLE;

                    if ((ch->period > 0) && ((ch->noNote ==  true)
                        || (ch->command == 0x03)
                        || (ch->command == 0x05)))
                        p->tempFlags |= TEMPFLAG_NEW_SAMPLE;
                }
            }
        }
    }

    p->tempPeriod = ch->period;
    p->tempVolume = ch->volume;

    if (p->forceEffectsOff == false)
        processEffects(p, ch);

    if (!(p->tempFlags & TEMPFLAG_DELAY))
    {
        if (p->tempFlags & TEMPFLAG_NEW_SAMPLE)
        {
            if (ch->sample > 0)
            {
                s = &p->source->samples[ch->sample - 1];

                if (s->length > 0)
                    mixerSwapChSource(p, ch->chanIndex, p->source->sampleData + s->offset, s->length, s->loopStart, s->loopLength, (s->attribute & 1) ? 2 : 1);
                else
                    mixerSetChSource(p, ch->chanIndex, NULL, 0, 0, 0, 0, 0);
            }
        }
        else if (p->tempFlags & TEMPFLAG_START)
        {
            if (ch->sample > 0)
            {
                s = &p->source->samples[ch->sample - 1];

                if (s->length > 0)
                {
                    if (ch->offset > 0)
                    {
                        mixerSetChSource(p, ch->chanIndex, p->source->sampleData + s->offset, s->length, s->loopStart, s->loopLength, ch->offset, (s->attribute & 1) ? 2 : 1);

                        if (p->minPeriod == PT_MIN_PERIOD) // PT/NT/STK/UST bug only
                        {
                            if (ch->offsetBugNotAdded == false)
                            {
                                ch->offset += ch->offsetTemp;
                                ch->offsetBugNotAdded = true;
                            }
                        }
                    }
                    else
                    {
                        mixerSetChSource(p, ch->chanIndex, p->source->sampleData + s->offset, s->length, s->loopStart, s->loopLength, 0, (s->attribute & 1) ? 2 : 1);
                    }
                }
                else
                {
                    mixerSetChSource(p, ch->chanIndex, NULL, 0, 0, 0, 0, 0);
                }
            }
        }

        mixerSetChVol(p, ch->chanIndex, p->tempVolume);

        if ((p->tempPeriod >= p->minPeriod) && (p->tempPeriod <= p->maxPeriod))
            mixerSetChRate(p, ch->chanIndex, (p->minPeriod == PT_MIN_PERIOD) ? p->frequencyTable[(int)p->tempPeriod] : p->extendedFrequencyTable[(int)p->tempPeriod]);
        else
            mixerSetChVol(p, ch->chanIndex, 0); // arp override bugfix
    }
}

static void nextPosition(player *p)
{
    p->modRow = p->PBreakPosition;

    p->PBreakPosition = 0;
    p->PosJumpAssert = false;

    p->modOrder++;
    if (p->modOrder >= p->source->head.orderCount)
        p->modOrder = (p->source->head.format == FORMAT_STK) ? 0 : p->source->head.restartPos;

    p->modPattern = p->source->head.order[p->modOrder];

    if (p->modRow == 0)
        p->loopCounter = p->orderPlayed[p->modOrder]++;
}

static void processTick(player *p)
{
    int i;

    if (p->minPeriod == PT_MIN_PERIOD) // PT/NT/STK/UST bug only
    {
        if (p->modTick == 0)
        {
            if (p->forceEffectsOff == true)
            {
                if (p->modRow != p->pattBreakBugPos)
                {
                    p->forceEffectsOff = false;
                    p->pattBreakBugPos = -1;
                }
            }
        }
    }

    for (i = 0; i < p->source->head.channelCount; ++i)
        processChannel(p, p->source->channels + i);

    if (p->minPeriod == PT_MIN_PERIOD) // PT/NT/STK/UST bug only
    {
        if (p->modTick == 0)
        {
            if ((p->pattBreakFlag == true) && (p->pattDelayFlag == true))
                p->forceEffectsOff = true;
        }
    }

    p->modTick++;
    if (p->modTick >= p->modSpeed)
    {
        p->modTick = 0;

        p->pattBreakFlag = false;
        p->pattDelayFlag = false;

        p->modRow++;

        if (p->PattDelayTime > 0)
        {
            p->PattDelayTime2 = p->PattDelayTime;
            p->PattDelayTime = 0;
        }

        if (p->PattDelayTime2 > 0)
        {
            p->PattDelayTime2--;
            if (p->PattDelayTime2 > 0)
                p->modRow--;
        }

        if (p->PBreakFlag == true)
        {
            p->PBreakFlag = false;
            p->modRow = p->PBreakPosition;
            p->PBreakPosition = 0;
        }

        if ((p->modRow == p->source->head.rowCount) || (p->PosJumpAssert == true))
            nextPosition(p);
    }
}

void playptmod_Render(void *_p, int *target, int length)
{
    player *p = (player *)_p;

    if (p->modulePlaying == true)
    {
        static const int soundBufferSamples = soundBufferSize / 4;

        while (length)
        {
            int tempSamples = CLAMP(length, 0, soundBufferSamples);
            
            if (p->sampleCounter)
            {
                tempSamples = CLAMP(tempSamples, 0, p->sampleCounter);
                if (target)
                {
                    outputAudio(p, target, tempSamples);
                    target += tempSamples * 2;
                }
                p->sampleCounter -= tempSamples;
                length -= tempSamples;
            }
            else
            {
                processTick(p);
                p->sampleCounter = p->samplesPerTick;
            }
        }
    }
}

void playptmod_Render16(void *_p, short *target, int length)
{
    player *p = (player *)_p;

	int tempBuffer[512];
	int * temp = ( target ) ? tempBuffer : 0;

    while (length)
    {
        int i, tempSamples = CLAMP(length, 0, 256);
        playptmod_Render(p, temp, tempSamples);
        length -= tempSamples;
        
        if ( target )
		for (i = 0; i < tempSamples * 2; ++i)
		{
			int s = tempBuffer[ i ] >> 8;
			s = CLAMP(s, -32768, 32767);
			target[ i ] = (short)s;
		}
        
        if ( target ) target += (tempSamples * 2);
    }
}

void *playptmod_Create(int samplingFrequency)
{
    player *p = (player *) calloc(1, sizeof(player));
    
    int i, j;

    p->tempoTimerVal = (samplingFrequency * 125) / 50;

    p->sinusTable = (unsigned char *)malloc(32);
    for (i = 0; i < 32; ++i)
        p->sinusTable[i] = (unsigned char)floorf(255.0f * sinf(((float)i * 3.141592f) / 32.0f));

    p->frequencyTable = (float *)malloc(sizeof (float) * 938);
    
    p->frequencyTable[0] = (float)samplingFrequency / 7093790.0f;
    for (i = 1; i <= 937; ++i)
        p->frequencyTable[i] = (float)samplingFrequency / (7093790.0f / (2.0f * (float)i));

    for (j = 0; j < 16; ++j)
        for (i = 0; i < 85; ++i)
            extendedRawPeriods[(j * 85) + i] = i == 84 ? 0 : npertab[i] * 8363 / finetune[j];

    for (i = 0; i < 14; ++i)
        extendedRawPeriods[16 * 85 + i] = 0;

    p->soundFrequency = samplingFrequency;

    p->extendedFrequencyTable = (float *)malloc(sizeof (float) * 1713);
    for (i = 14; i <= 1712; ++i) // 0..14 will never be looked up, junk is OK
        p->extendedFrequencyTable[i] = (float)samplingFrequency / (7093790.0f / (2.0f * (float)i));
    
    p->mixBufferL = (float *)malloc(soundBufferSize * sizeof (float));
    p->mixBufferR = (float *)malloc(soundBufferSize * sizeof (float));

    p->filterC.LED = calcRcCoeff((float)samplingFrequency, 3090.0f);
    p->filterC.LEDFb = 0.125f + 0.125f / (1.0f - p->filterC.LED);

    p->useLEDFilter = false;

    mixerCutChannels(p);

    return p;
}

void playptmod_Config(void *_p, int option, int value)
{
    player *p = (player *)_p;
    switch (option)
    {
        case PTMOD_OPTION_VSYNC_TIMING:
            p->vBlankTiming = value ? true : false;
        break;
    }
}

void playptmod_Play(void *_p, unsigned int startOrder)
{
    player *p = (player *)_p;
    int i;

    if (!p->modulePlaying && p->moduleLoaded)
    {
        mixerCutChannels(p);

        for (i = 0; i < p->source->head.channelCount; ++i)
        {
            p->source->channels[i].volume = 64;
            p->source->channels[i].chanIndex = (char)i;
            p->source->channels[i].patternLoopRow = 0;
            p->source->channels[i].patternLoopCounter = 0;
            p->source->channels[i].glissandoControl = 0;
            p->source->channels[i].vibratoControl = 0;
            p->source->channels[i].vibratoPos = 0;
            p->source->channels[i].tremoloControl = 0;
            p->source->channels[i].tremoloPos = 0;
            p->source->channels[i].fineTune = 0;
            p->source->channels[i].offsetBugNotAdded = true;
        }

        p->sampleCounter = 0;

        modSetTempo(p, p->source->head.initBPM);
        modSetSpeed(p, 6);

        p->modOrder = startOrder;
        p->modPattern = p->source->head.order[startOrder];
        p->modRow = 0;
        p->modTick = 0;
        p->tempFlags = 0;
        p->modTick = 0;

        p->PBreakPosition = 0;
        p->PosJumpAssert = false;
        p->pattBreakBugPos = -1;
        p->pattBreakFlag = false;
        p->pattDelayFlag = false;
        p->forceEffectsOff = false;
        p->PattDelayTime = 0;
        p->PattDelayTime2 = 0;
        p->PBreakFlag = false;

        memcpy(p->source->sampleData, p->source->originalSampleData, p->source->head.totalSampleSize);
        memset(p->orderPlayed, 0, sizeof(p->orderPlayed));

        p->loopCounter = 0;
        p->orderPlayed[startOrder] = 1;
        p->modulePlaying = true;
    }
}

void playptmod_Free(void *_p)
{
    player *p = (player *)_p;
    int i;

    if (p->moduleLoaded == true)
    {
        p->modulePlaying = false;

        for (i = 0; i < 256; ++i)
        {
            if (p->source->patterns[i] != NULL)
                free(p->source->patterns[i]);
        }

        free(p->source->originalSampleData);
        free(p->source->sampleData);
        free(p->source);

        p->moduleLoaded = false;
    }

    free(p->mixBufferL);
    free(p->mixBufferR);;
    free(p->sinusTable);
    free(p->frequencyTable);
    free(p->extendedFrequencyTable);
    free(p);
}

unsigned int playptmod_LoopCounter(void *_p)
{
    player *p = (player *)_p;
    return p->loopCounter;
}

void playptmod_GetInfo(void *_p, playptmod_info *i)
{
    int n, c;
    player *p = (player *)_p;
    int order = p->modOrder;
    int row = p->modRow;
    int pattern = p->modPattern;

    if ((p->modRow >= p->source->head.rowCount) || p->PosJumpAssert)
    {
        order++;
        if (order >= p->source->head.orderCount)
            order = (p->source->head.format == FORMAT_STK) ? 0 : p->source->head.restartPos;

        row = p->PBreakPosition;
        pattern = p->source->head.order[order];
    }

    i->order = order;
    i->pattern = pattern;
    i->row = row;
    i->speed = p->modSpeed;
    i->tempo = p->modBPM;

    for (c = 0, n = 0; n < p->source->head.channelCount; ++n)
    {
        if (p->v[n].data) c++;
    }

    i->channelsPlaying = c;
}

void playptmod_Mute(void *_p, int channel, int mute)
{
    player *p = (player *)_p;
    p->v[channel].mute = mute;
}

/* END OF FILE */
