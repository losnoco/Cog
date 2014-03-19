/*
** ST3PLAY v0.45
** =============
**
** C port of Scream Tracker 3's replayer, by 8bitbubsy (Olav Sørensen)
** using the original asm source codes by PSI (Sami Tammilehto) of Future Crew
**
** This is by no means a piece of beautiful code, nor is it meant to be...
** It's just an accurate Scream Tracker 3 replayer port for people to enjoy.
**
** Non-ST3 additions from other trackers (only handled for non ST3 S3Ms):
**
** - Mixing:
**   * 16-bit sample support
**   * Stereo sample support
**   * 2^32 sample length support
**   * Middle-C speeds beyond 65535
**   * Process the last 16 channels as PCM
**   * Process 8 octaves instead of 7
**
** - Effects:
**   * Command S9x        (sound control - only S91/S90 so far)
**   * Command S5x        (panbrello type)
**   * Command Mxx        (set channel volume)
**   * Command Nxy        (channel volume slide)
**   * Command Pxy        (panning slide)
**   * Command Txx<0x20   (tempo slide)
**   * Command Wxy        (global volume slide)
**   * Command Xxx        (7+1-bit pan) + XA4 for "surround"
**   * Command Yxy        (panbrello)
**   * Volume Command Pxx (set 4+1-bit panning)
**
** - Variables:
**   * Pan changed from 4-bit (0..15) to 8+1-bit (0..256)
**   * Memory variables for the new N/P/T/W/Y effects
**   * Panbrello counter
**   * Panbrello type
**   * Channel volume multiplier
**   * Channel surround flag
**
** - Other changes:
**   * Added tracker identification to make sure Scream Tracker 3.xx
**     modules are still played exactly like they should. :-)
**
**
** TODO:
** - Check SBx (pattern loop) differences between ST3 trackers
** - More testing on the newly implemented non-ST3 stuff
** - Make S2x (set finetune) work for non ST3 S3Ms
**
**
*/

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <math.h>

#if defined(_MSC_VER) && !defined(inline)
#define inline __forceinline
#endif

#include "lanczos_resampler.h"

#include "st3play.h"

// TRACKER ID
enum
{
    SCREAM_TRACKER  = 1,
    IMAGO_ORPHEUS   = 2,
    IMPULSE_TRACKER = 3,
    SCHISM_TRACKER  = 4,
    OPENMPT         = 5,
    BEROTRACKER     = 6,
    CREAMTRACKER    = 7
};


// STRUCTS
typedef struct chn
{
    int8_t aorgvol;
    int8_t avol;
    uint8_t channelnum;
    uint8_t achannelused;
    uint8_t aglis;
    uint8_t atremor;
    uint8_t atreon;
    uint8_t atrigcnt;
    uint8_t anotecutcnt;
    uint8_t anotedelaycnt;
    uint8_t avibtretype;
    uint8_t note;
    uint8_t ins;
    uint8_t vol;
    uint8_t cmd;
    uint8_t info;
    uint8_t lastins;
    uint8_t lastnote;
    uint8_t alastnfo;
    uint8_t alasteff;
    uint8_t alasteff1;
    int16_t apanpos;
    int16_t avibcnt;
    uint16_t astartoffset;
    uint16_t astartoffset00;
    int32_t ac2spd;
    int32_t asldspd;
    int32_t aorgspd;
    int32_t aspd;

    // NON-ST3 variables
    int8_t chanvol;
    uint8_t surround;
    uint8_t apantype;
    uint8_t nxymem;
    uint8_t pxymem;
    uint8_t txxmem;
    uint8_t wxymem;
    uint8_t yxymem;
    int16_t apancnt;
} chn_t;

typedef struct
{
    const int8_t *sampleData;
    int8_t loopEnabled;
    int8_t sixteenBit;
    int8_t stereo;
    int8_t adpcm;
    int8_t mixing;
    int8_t interpolating;
    int8_t oversampleCount;
    int32_t sampleLength;
    int32_t sampleLoopEnd;
    int32_t samplePosition;
    int32_t sampleLoopLength;
    
    int32_t lastSamplePosition;
    int8_t lastDelta;
    int8_t loopStartDelta;

    float incRate;
    float volume;
    float panningL;
    float panningR;
    float orgPanR;
} VOICE;

// VARIABLES / STATE
typedef struct
{
    int8_t volslidetype;
    int8_t patterndelay;
    int8_t patloopcount;
    uint8_t breakpat;
    uint8_t startrow;
    uint8_t musiccount;
    int16_t np_ord;
    int16_t np_row;
    int16_t np_pat;
    int16_t np_patoff;
    int16_t patloopstart;
    int16_t jumptorow;
    uint16_t patternadd;
    uint16_t instrumentadd;
    uint16_t patmusicrand;
    int32_t aspdmax;
    int32_t aspdmin;
    uint32_t np_patseg;
    chn_t chn[32];
    
    uint8_t mixingVolume;
    int32_t soundBufferSize;
    uint32_t outputFreq;
    
    VOICE voice[32];
    
    void *resampler[64];
    
    float f_outputFreq;
    float f_masterVolume;
    
    // pre-initialized variables
    int8_t samplingInterpolation;//      = 1;
    float *masterBufferL;//              = NULL;
    float *masterBufferR;//              = NULL;
    int32_t samplesLeft;//               = 0; // must be signed
    int8_t isMixing;//          = 0;
    uint32_t samplesPerFrame;// = 882;
    
    // GLOBAL VARIABLES
    int8_t ModuleLoaded;// = 0;
    int8_t MusicPaused;// = 0;
    int8_t Playing;// = 0;
    
    uint8_t *mseg;// = NULL;
    int8_t lastachannelused;
    int8_t tracker;
    int8_t oldstvib;
    int8_t fastvolslide;
    int8_t amigalimits;
    uint8_t musicmax;
    uint8_t numChannels;
    int16_t x_np_row;
    int16_t x_np_ord;
    int16_t x_np_pat;
    int16_t tempo;
    int16_t globalvol;
    int8_t stereomode;
    uint8_t mastervol;
    uint32_t mseg_len;
    
    uint32_t loopCount;
    uint8_t playedOrder[8192];
} PLAYER;

typedef void (*effect_routine)(PLAYER *, chn_t *ch);


enum { _soundBufferSize = 512 };


// TABLES


static const int8_t retrigvoladd[32] =
{
    0, -1, -2, -4, -8,-16,  0,  0,
    0,  1,  2,  4,  8, 16,  0,  0,
    0,  0,  0,  0,  0,  0, 10,  8,
    0,  0,  0,  0,  0,  0, 24, 32
};

#if 0
static const uint16_t xfinetune_amiga[16] =
{
    8363, 8413, 8463, 8529, 8581, 8651, 8723, 8757,
    7895, 7941, 7985, 8046, 8107, 8169, 8232, 8280
};
#endif

static const uint16_t notespd[12] =
{
    1712 * 16, 1616 * 16, 1524 * 16,
    1440 * 16, 1356 * 16, 1280 * 16,
    1208 * 16, 1140 * 16, 1076 * 16,
    1016 * 16,  960 * 16,  907 * 16
};

static const int16_t vibsin[64] =
{
     0x00, 0x18, 0x31, 0x4A, 0x61, 0x78, 0x8D, 0xA1,
     0xB4, 0xC5, 0xD4, 0xE0, 0xEB, 0xF4, 0xFA, 0xFD,
     0xFF, 0xFD, 0xFA, 0xF4, 0xEB, 0xE0, 0xD4, 0xC5,
     0xB4, 0xA1, 0x8D, 0x78, 0x61, 0x4A, 0x31, 0x18,
     0x00,-0x18,-0x31,-0x4A,-0x61,-0x78,-0x8D,-0xA1,
    -0xB4,-0xC5,-0xD4,-0xE0,-0xEB,-0xF4,-0xFA,-0xFD,
    -0xFF,-0xFD,-0xFA,-0xF4,-0xEB,-0xE0,-0xD4,-0xC5,
    -0xB4,-0xA1,-0x8D,-0x78,-0x61,-0x4A,-0x31,-0x18
};

static const uint8_t vibsqu[64] =
{
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const int16_t vibramp[64] =
{
       0, -248,-240,-232,-224,-216,-208,-200,
    -192, -184,-176,-168,-160,-152,-144,-136,
    -128, -120,-112,-104, -96, -88, -80, -72,
     -64,  -56, -48, -40, -32, -24, -16,  -8,
       0,    8,  16,  24,  32,  40,  48,  56,
      64,   72,  80,  88,  96, 104, 112, 120,
     128,  136, 144, 152, 160, 168, 176, 184,
     192,  200, 208, 216, 224, 232, 240, 248
};


// FUNCTION DECLARATIONS


static void setSamplesPerFrame(PLAYER *, uint32_t val);
static void setSamplingInterpolation(PLAYER *, int8_t value);
static void setStereoMode(PLAYER *, int8_t value);
static void setMasterVolume(PLAYER *, uint8_t value);
static void voiceSetSource(PLAYER *, uint8_t voiceNumber, const int8_t *sampleData,
    int32_t sampleLength, int32_t sampleLoopLength, int32_t sampleLoopEnd,
    int8_t loopEnabled, int8_t sixteenbit, int8_t stereo, int8_t adpcm);
static void voiceSetSamplePosition(PLAYER *, uint8_t voiceNumber, uint16_t value);
static void voiceSetVolume(PLAYER *, uint8_t voiceNumber, float volume);
static void voiceSetSurround(PLAYER *, uint8_t voiceNumber, int8_t surround);
static void voiceSetPanning(PLAYER *, uint8_t voiceNumber, uint16_t pan);
static void voiceSetSamplingFrequency(PLAYER *, uint8_t voiceNumber, float samplingFrequency);

static void FreeSong(PLAYER *);

static void s_ret(PLAYER *, chn_t *ch);
static void s_setgliss(PLAYER *, chn_t *ch);
static void s_setfinetune(PLAYER *, chn_t *ch);
static void s_setvibwave(PLAYER *, chn_t *ch);
static void s_settrewave(PLAYER *, chn_t *ch);
static void s_setpanwave(PLAYER *, chn_t *ch); // NON-ST3
static void s_setpanpos(PLAYER *, chn_t *ch);
static void s_sndcntrl(PLAYER *, chn_t *ch); // NON-ST3
static void s_patloop(PLAYER *, chn_t *ch);
static void s_notecut(PLAYER *, chn_t *ch);
static void s_notecutb(PLAYER *, chn_t *ch);
static void s_notedelay(PLAYER *, chn_t *ch);
static void s_notedelayb(PLAYER *, chn_t *ch);
static void s_patterdelay(PLAYER *, chn_t *ch);
static void s_setspeed(PLAYER *, chn_t *ch);
static void s_jmpto(PLAYER *, chn_t *ch);
static void s_break(PLAYER *, chn_t *ch);
static void s_volslide(PLAYER *, chn_t *ch);
static void s_slidedown(PLAYER *, chn_t *ch);
static void s_slideup(PLAYER *, chn_t *ch);
static void s_toneslide(PLAYER *, chn_t *ch);
static void s_vibrato(PLAYER *, chn_t *ch);
static void s_tremor(PLAYER *, chn_t *ch);
static void s_arp(PLAYER *, chn_t *ch);
static void s_chanvol(PLAYER *, chn_t *ch); // NON-ST3
static void s_chanvolslide(PLAYER *, chn_t *ch); // NON-ST3
static void s_vibvol(PLAYER *, chn_t *ch);
static void s_tonevol(PLAYER *, chn_t *ch);
static void s_panslide(PLAYER *, chn_t *ch);
static void s_retrig(PLAYER *, chn_t *ch);
static void s_tremolo(PLAYER *, chn_t *ch);
static void s_scommand1(PLAYER *, chn_t *ch);
static void s_scommand2(PLAYER *, chn_t *ch);
static void s_settempo(PLAYER *, chn_t *ch);
static void s_finevibrato(PLAYER *, chn_t *ch);
static void s_setgvol(PLAYER *, chn_t *ch);
static void s_globvolslide(PLAYER *, chn_t *ch); // NON-ST3
static void s_setpan(PLAYER *, chn_t *ch); // NON-ST3
static void s_panbrello(PLAYER *, chn_t *ch); // NON-ST3

static effect_routine ssoncejmp[16] =
{
    s_ret,
    s_setgliss,
    s_setfinetune,
    s_setvibwave,
    s_settrewave,
    s_setpanwave, // NON-ST3
    s_ret,
    s_ret,
    s_setpanpos,
    s_sndcntrl, // NON-ST3
    s_ret,
    s_patloop,
    s_notecut,
    s_notedelay,
    s_patterdelay,
    s_ret
};

static effect_routine ssotherjmp[16] =
{
    s_ret,
    s_ret,
    s_ret,
    s_ret,
    s_ret,
    s_ret,
    s_ret,
    s_ret,
    s_ret,
    s_ret,
    s_ret,
    s_ret,
    s_notecutb,
    s_notedelayb,
    s_ret,
    s_ret
};

static effect_routine soncejmp[27] =
{
    s_ret,
    s_setspeed,
    s_jmpto,
    s_break,
    s_volslide,
    s_slidedown,
    s_slideup,
    s_ret,
    s_ret,
    s_tremor,
    s_arp,
    s_ret,
    s_ret,
    s_chanvol, // NON-ST3
    s_chanvolslide, // NON-ST3
    s_ret,
    s_panslide, // NON-ST3
    s_retrig,
    s_ret,
    s_scommand1,
    s_settempo,
    s_ret,
    s_ret,
    s_globvolslide, // NON-ST3
    s_setpan, // NON-ST3
    s_panbrello, // NON-ST3
    s_ret
};

static effect_routine sotherjmp[27] =
{
    s_ret,
    s_ret,
    s_ret,
    s_ret,
    s_volslide,
    s_slidedown,
    s_slideup,
    s_toneslide,
    s_vibrato,
    s_tremor,
    s_arp,
    s_vibvol,
    s_tonevol,
    s_ret,
    s_chanvolslide, // NON-ST3
    s_ret,
    s_panslide, // NON-ST3
    s_retrig,
    s_tremolo,
    s_scommand2,
    s_settempo, // NON-ST3 (for tempo slides)
    s_finevibrato,
    s_setgvol,
    s_globvolslide, // NON-ST3
    s_ret,
    s_panbrello, // NON-ST3
    s_ret
};


// CODE START
void * st3play_Alloc(uint32_t outputFreq, int8_t interpolation)
{
    int i;
    
    PLAYER * p = (PLAYER *) calloc(1, sizeof(PLAYER));
    
    if ( !p )
        return 0;

    lanczos_init();
    
    for (i = 0; i < 64; ++i)
    {
        p->resampler[i] = lanczos_resampler_create();
        if ( !p->resampler[i] )
        {
            st3play_Free( p );
            return 0;
        }
    }
    
    p->soundBufferSize     = _soundBufferSize;
    p->masterBufferL       = (float *)(malloc(p->soundBufferSize * sizeof (float)));
    p->masterBufferR       = (float *)(malloc(p->soundBufferSize * sizeof (float)));
    
    if ( !p->masterBufferL || !p->masterBufferR )
    {
        st3play_Free( p );
        return 0;
    }

    p->numChannels = 32;
    p->stereomode = 1;
    p->globalvol = 64;
    p->mastervol = 48;

    p->outputFreq = outputFreq;
    p->f_outputFreq = (float)outputFreq;

    setSamplingInterpolation(p, interpolation);
    setSamplesPerFrame(p, ((outputFreq * 5UL) / 2 / 125));
        
    return p;
}

void st3play_Free(void *_p)
{
    int i;
    
    PLAYER * p = (PLAYER *)_p;

    FreeSong(p);
    
    for (i = 0; i < 64; ++i)
    {
        if ( p->resampler[i] )
            lanczos_resampler_delete( p->resampler[i] );
    }
    
    if ( p->masterBufferL )
        free( p->masterBufferL );
    if ( p->masterBufferR )
        free( p->masterBufferR );

    free(p);
}

static inline uint16_t get_le16(const void *_p)
{
    const uint8_t * p = (const uint8_t *)_p;
    return p[0] | (p[1] << 8);
}

static inline void set_le16(void *_p, uint16_t v)
{
    uint8_t * p = (uint8_t *)_p;
    p[0] = (uint8_t)v;
    p[1] = v >> 8;
}

static inline uint32_t get_le32(const void *_p)
{
    const uint8_t * p = (const uint8_t *)_p;
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

static inline void getlastnfo(PLAYER *p, chn_t *ch)
{
    (void)p;
    if (!ch->info)
        ch->info = ch->alastnfo;
}

static void setspeed(PLAYER *p, uint8_t val)
{
    if (val)
        p->musicmax = val;
}

static void settempo(PLAYER *p, uint16_t val)
{
    if (val > 32)
    {
        p->tempo = val;
        setSamplesPerFrame(p, ((p->outputFreq * 5UL) / 2) / p->tempo);
    }
}

static inline void setspd(PLAYER *p, uint8_t ch)
{
    int32_t tmpspd;

    tmpspd = p->chn[ch].aspd;

    if (tmpspd && (tmpspd < p->aspdmin))
        tmpspd = p->aspdmin;
    
    if (p->tracker == SCREAM_TRACKER)
    {
        if (tmpspd > p->aspdmax)
            tmpspd = p->aspdmax;
    }
    else
    {
        // *ABSOLUTE* max!
        if (tmpspd > 14317056)
            tmpspd = 14317056;
    }

    if (p->amigalimits)
    {
        if (p->chn[ch].aorgspd > p->aspdmax) p->chn[ch].aorgspd = p->aspdmax;
        if (p->chn[ch].aorgspd < p->aspdmin) p->chn[ch].aorgspd = p->aspdmin;

        p->chn[ch].aspd = tmpspd;
    }

    p->chn[ch].achannelused |= 0x80;

    // ST3 actually uses 14317056/per instead of 14317456 (8363*1712).
    // 14317056 is used in both the ST3 replayer and the S3M format docs
    if (tmpspd)
        voiceSetSamplingFrequency(p, ch, 14317056.0f / (float)tmpspd);
}

static inline void setvol(PLAYER *p, uint8_t ch)
{
    p->chn[ch].achannelused |= 0x80;
    voiceSetVolume(p, ch, ((float)(p->chn[ch].avol) / 63.0f) * ((float)(p->chn[ch].chanvol) / 64.0f) * ((float)(p->globalvol) / 64.0f));
}

static inline void setpan(PLAYER *p, uint8_t ch)
{
    voiceSetPanning(p, ch, p->chn[ch].apanpos);
}

static inline int16_t stnote2herz(PLAYER *p, uint8_t note)
{
    uint8_t tmpnote;
    uint8_t tmpocta;

    if (note == 254) return (0);

    tmpnote = note  & 0x0F;
    tmpocta = note >> 0x04;

    // ST3 doesn't do this, but do it for safety
    if (tmpnote > 11) tmpnote = 11;

    // Limit octaves to 8 in ST3 mode
    if ((p->tracker == SCREAM_TRACKER) && (tmpocta > 7))
        tmpocta = 7;

    return (notespd[tmpnote] >> tmpocta);
}

static inline int32_t scalec2spd(PLAYER *p, uint8_t ch, int32_t spd)
{
    spd *= 8363UL;

    if (p->tracker == SCREAM_TRACKER)
    {
        if ((spd >> 16) > p->chn[ch].ac2spd)
            return (32767);
    }

    if (p->chn[ch].ac2spd)
        spd /= p->chn[ch].ac2spd;

    if (p->tracker == SCREAM_TRACKER)
    {
        if (spd > 32767)
            return (32767);
    }

    return (spd);
}

// for Gxx with semitones slide flag
static inline int32_t roundspd(PLAYER *p, uint8_t ch, int32_t spd)
{
    int8_t octa;
    int8_t lastnote;
    int8_t newnote;
    int32_t note;
    int32_t lastspd;
    int32_t newspd;

    newspd = spd * p->chn[ch].ac2spd;

    if (p->tracker == SCREAM_TRACKER)
    {
        if ((newspd >> 16) >= 8363)
            return (spd);
    }

    newspd /= 8363;

    // find octave
    octa    = 0;
    lastspd = ((1712 * 8) + notespd[11]) >> 1;
    while (newspd < lastspd)
    {
        octa++;
        lastspd >>= 1;
    }

    // find note
    lastnote = 0;
    newnote  = 0;

    if (p->tracker == SCREAM_TRACKER)
        lastspd = 32767;
    else
        lastspd = 65535; // TODO

    while (newnote < 11)
    {
        note = (notespd[newnote] >> octa) - newspd;
        if (note < 0) note *= -1; // abs()

        if (note < lastspd)
        {
            lastspd = note;
            lastnote = newnote;
        }

        newnote++;
    }

    // get new speed from new note
    newspd = (uint32_t)(stnote2herz(p, (octa << 4) | (lastnote & 0x0F))) * 8363;

    if (p->tracker == SCREAM_TRACKER)
    {
        if ((newspd >> 16) >= p->chn[ch].ac2spd)
            return (spd);
    }

    if (p->chn[ch].ac2spd)
        newspd /= p->chn[ch].ac2spd;

    return (newspd);
}

static int16_t neworder(PLAYER *p)
{
skip:
    p->np_ord++;

    if ((p->mseg[0x60+(p->np_ord-1)]==255)||(p->np_ord>get_le16(&p->mseg[0x20]))) // end
        p->np_ord = 1;

    if (p->mseg[0x60 + (p->np_ord - 1)] == 254) // skip
        goto skip; // avoid recursive calling

    p->np_pat        = (int16_t)(p->mseg[0x60 + (p->np_ord - 1)]);
    p->np_patoff     = -1; // force reseek
    p->np_row        = p->startrow;
    p->startrow      = 0;
    p->patmusicrand  = 0;
    p->patloopstart  = -1;
    p->jumptorow     = -1;

    return (p->np_row);
}

// updates np_patseg and np_patoff
static inline void seekpat(PLAYER *p)
{
    uint8_t dat;
    int16_t i;
    int16_t j;

    if (p->np_patoff == -1) // seek must be done
    {
        p->np_patseg = (uint32_t)(get_le16(&p->mseg[p->patternadd + (p->np_pat << 1)])) << 4;
        if (p->np_patseg)
        {
            j = 2; // skip packed pat len flag

            // inc np_patoff on patbreak
            if (p->np_row)
            {
                i = p->np_row;
                while (i)
                {
                    dat = p->mseg[p->np_patseg + j++];
                    if (!dat)
                    {
                        i--;
                    }
                    else
                    {
                        // skip ch data
                        if (dat & 0x20) j += 2;
                        if (dat & 0x40) j += 1;
                        if (dat & 0x80) j += 2;
                    }
                }
            }

            p->np_patoff = j;
        }
    }
}

static inline uint8_t getnote(PLAYER *p)
{
    uint8_t dat;
    uint8_t ch;
    int16_t i;

    if (!p->np_patseg || (p->np_patseg >= p->mseg_len) || (p->np_pat >= get_le16(&p->mseg[0x24])))
        return (255);

    i = p->np_patoff;
    for (;;)
    {
        dat = p->mseg[p->np_patseg + i++];
        if (!dat) // end of row
        {
            p->np_patoff = i;
            return (255);
        }

        if (!(p->mseg[0x40 + (dat & 0x1F)] & 0x80))
        {
            ch = dat & 0x1F; // channel to trigger
            break;
        }

        // channel is off, skip data
        if (dat & 0x20) i += 2;
        if (dat & 0x40) i += 1;
        if (dat & 0x80) i += 2;
    }

    if (dat & 0x20)
    {
        p->chn[ch].note = p->mseg[p->np_patseg + i++];
        p->chn[ch].ins  = p->mseg[p->np_patseg + i++];

        if (p->chn[ch].note != 255) p->chn[ch].lastnote = p->chn[ch].note;
        if (p->chn[ch].ins)         p->chn[ch].lastins  = p->chn[ch].ins;
    }

    if (dat & 0x40)
        p->chn[ch].vol = p->mseg[p->np_patseg + i++];

    if (dat & 0x80)
    {
        p->chn[ch].cmd  = p->mseg[p->np_patseg + i++];
        p->chn[ch].info = p->mseg[p->np_patseg + i++];
    }

    p->np_patoff = i;
    return (ch);
}

static inline void doamiga(PLAYER *p, uint8_t ch)
{
    uint8_t *insdat;
    int8_t loop;
    uint32_t insoffs;
    uint32_t inslen;
    uint32_t insrepbeg;
    uint32_t insrepend;
    int8_t shift;

    if (p->chn[ch].ins)
    {
        p->chn[ch].lastins      = p->chn[ch].ins;
        p->chn[ch].astartoffset = 0;

        if (p->chn[ch].ins <= get_le16(&p->mseg[0x22])) // added for safety reasons
        {
            insdat = &p->mseg[(uint32_t)(get_le16(&p->mseg[p->instrumentadd + ((p->chn[ch].ins - 1) << 1)])) << 4];
            if (insdat[0])
            {
                if (insdat[0] == 1)
                {
                    p->chn[ch].ac2spd = get_le32(&insdat[0x20]);

                    p->chn[ch].avol = (int8_t)(insdat[0x1C]);
                    if (p->chn[ch].avol <  0) p->chn[ch].avol =  0;
                    if (p->chn[ch].avol > 63) p->chn[ch].avol = 63;

                    p->chn[ch].aorgvol = p->chn[ch].avol;
                    setvol(p, ch);

                    insoffs = (uint32_t)(((uint32_t)(insdat[0x0D])<<16)|((uint16_t)(insdat[0x0F])<<8)|insdat[0x0E])<<4;

                    if (insoffs > p->mseg_len)
                        insoffs = p->mseg_len;

                    inslen    = get_le32(&insdat[0x10]);

                    shift = 0;
                    if (insdat[0x1F] & 2) shift++;
                    if (insdat[0x1F] & 4) shift++;

                    if (insoffs + (inslen << shift) > p->mseg_len)
                        inslen = (p->mseg_len - insoffs) >> shift;

                    insrepbeg = get_le32(&insdat[0x14]);
                    insrepend = get_le32(&insdat[0x18]);

                    if (insrepbeg > inslen) insrepbeg = inslen;
                    if (insrepend > inslen) insrepend = inslen;

                    loop = 0;
                    if ((insdat[0x1F] & 1) && inslen && (insrepend > insrepbeg))
                        loop = 1;

                    voiceSetSource(p, ch, (const int8_t *)(&p->mseg[insoffs]), inslen,
                        insrepend - insrepbeg, insrepend, loop,
                        insdat[0x1F] & 4, insdat[0x1F] & 2, insdat[0x1E] == 4);
                }
                else
                {
                    p->chn[ch].lastins = 0;
                }
            }
        }
    }

    // continue only if we have an active instrument on this channel
    if (!p->chn[ch].lastins) return;

    if (p->chn[ch].cmd == ('O' - 64))
    {
        if (!p->chn[ch].info)
        {
            p->chn[ch].astartoffset = p->chn[ch].astartoffset00;
        }
        else
        {
            p->chn[ch].astartoffset   = (uint16_t)(p->chn[ch].info) << 8;
            p->chn[ch].astartoffset00 = p->chn[ch].astartoffset;
        }
    }

    if (p->chn[ch].note != 255)
    {
        if (p->chn[ch].note == 254)
        {
            p->chn[ch].aspd    = 0;
            p->chn[ch].avol    = 0;
            p->chn[ch].asldspd = 65535;

            setspd(p, ch);
            setvol(p, ch);

            // shutdown channel
            voiceSetSource(p, ch, NULL, 0, 0, 0, 0, 0, 0, 0);
            voiceSetSamplePosition(p, ch, 0);
        }
        else
        {
            p->chn[ch].lastnote = p->chn[ch].note;

            if ((p->chn[ch].cmd != ('G' - 64)) && (p->chn[ch].cmd != ('L' - 64)))
                voiceSetSamplePosition(p, ch, p->chn[ch].astartoffset);

            if (!p->chn[ch].aorgspd || ((p->chn[ch].cmd != ('G' - 64)) && (p->chn[ch].cmd != ('L' - 64))))
            {
                p->chn[ch].aspd    = scalec2spd(p, ch, stnote2herz(p, p->chn[ch].note));
                p->chn[ch].aorgspd = p->chn[ch].aspd;
                p->chn[ch].avibcnt = 0;
                p->chn[ch].apancnt = 0;

                setspd(p, ch);
            }

            p->chn[ch].asldspd = scalec2spd(p, ch, stnote2herz(p, p->chn[ch].note));
        }
    }

    if (p->chn[ch].vol != 255)
    {
        if (p->chn[ch].vol <= 64)
        {
            p->chn[ch].avol    = p->chn[ch].vol;
            p->chn[ch].aorgvol = p->chn[ch].vol;

            setvol(p, ch);

            return;
        }

        // NON-ST3
        if ((p->chn[ch].vol >= 128) && (p->chn[ch].vol <= 192))
        {
            p->chn[ch].apanpos = (p->chn[ch].vol - 128) << 2;
            setpan(p, ch);
        }
    }
}

static inline void donewnote(PLAYER *p, uint8_t ch, int8_t notedelayflag)
{
    if (notedelayflag)
    {
        p->chn[ch].achannelused = 0x81;
    }
    else
    {
        if (p->chn[ch].channelnum > p->lastachannelused)
        {
            p->lastachannelused = p->chn[ch].channelnum + 1;

            // hackish fix, fixes call_me_an_angel.s3m crash
            if (p->lastachannelused > 31) p->lastachannelused = 31;
        }

        p->chn[ch].achannelused = 0x01;

        if (p->chn[ch].cmd == ('S' - 64))
        {
            if ((p->chn[ch].info & 0xF0) == 0xD0)
                return;
        }
    }

    doamiga(p, ch);
}

static inline void donotes(PLAYER *p)
{
    uint8_t i;
    uint8_t ch;

    for (i = 0; i < 32; ++i)
    {
        p->chn[i].note = 255;
        p->chn[i].vol  = 255;
        p->chn[i].ins  = 0;
        p->chn[i].cmd  = 0;
        p->chn[i].info = 0;
    }

    seekpat(p);

    for (;;)
    {
        ch = getnote(p);
        if (ch == 255) break; // end of row/channels

        if ((p->mseg[0x40 + ch] & 0x7F) <= 15) // no adlib channel types yet
            donewnote(p, ch, 0);
    }
}

// tick 0 commands
static inline void docmd1(PLAYER *p)
{
    uint8_t i;

    for (i = 0; i < (p->lastachannelused + 1); ++i)
    {
        if (p->chn[i].achannelused)
        {
            if (p->chn[i].info)
                p->chn[i].alastnfo = p->chn[i].info;

            if (p->chn[i].cmd)
            {
                p->chn[i].achannelused |= 0x80;

                if (p->chn[i].cmd == ('D' - 64))
                {
                    // THEORY: I think this fix is related to
                    // AdLib channels...

                    // fix retrig if Dxy
                    p->chn[i].atrigcnt = 0;

                    // fix speed if tone port noncomplete
                    if (p->chn[i].aspd != p->chn[i].aorgspd)
                    {
                        p->chn[i].aspd = p->chn[i].aorgspd;
                        setspd(p, i);
                    }
                }
                else
                {
                    if (p->chn[i].cmd != ('I' - 64))
                    {
                        p->chn[i].atremor = 0;
                        p->chn[i].atreon  = 1;
                    }

                    if ((p->chn[i].cmd != ('H' - 64)) &&
                        (p->chn[i].cmd != ('U' - 64)) &&
                        (p->chn[i].cmd != ('K' - 64)) &&
                        (p->chn[i].cmd != ('R' - 64)))
                    {
                        p->chn[i].avibcnt |= 0x80;
                    }

                    // NON-ST3
                    if (p->tracker != SCREAM_TRACKER)
                    {
                        if (p->chn[i].cmd != ('Y' - 64))
                            p->chn[i].apancnt |= 0x80;
                    }
                }

                if (p->chn[i].cmd < 27)
                {
                    p->volslidetype = 0;
                    soncejmp[p->chn[i].cmd](p, &p->chn[i]);
                }
            }
            else
            {
                // NON-ST3
                if (p->tracker != SCREAM_TRACKER)
                { 
                    // recalc pans
                    setpan(p, i);
                    voiceSetSurround(p, i, p->chn[i].surround);
                }

                // fix retrig if no command
                p->chn[i].atrigcnt = 0;

                // fix speed if tone port noncomplete
                if (p->chn[i].aspd != p->chn[i].aorgspd)
                {
                    p->chn[i].aspd = p->chn[i].aorgspd;
                    setspd(p, i);
                }
            }
        }
    }
}

// tick >0 commands
static inline void docmd2(PLAYER *p)
{
    uint8_t i;

    for (i = 0; i < (p->lastachannelused + 1); ++i)
    {
        if (p->chn[i].achannelused)
        {
            if (p->chn[i].cmd)
            {
                p->chn[i].achannelused |= 0x80;

                if (p->chn[i].cmd < 27)
                {
                    p->volslidetype = 0;
                    sotherjmp[p->chn[i].cmd](p, &p->chn[i]);
                }
            }
        }
    }
}

void dorow(PLAYER *p) // periodically called from mixer
{
    int32_t offset, bit;
    
    p->patmusicrand = (uint16_t)(((uint32_t)(p->patmusicrand) * 0xCDEF) >> 16) + 0x1727;

    if (!p->musiccount)
    {
        if (p->patterndelay)
        {
            p->np_row--;
            docmd1(p);
            p->patterndelay--;
        }
        else
        {
            donotes(p);
            docmd1(p);
        }
    }
    else
    {
        docmd2(p);
    }

    p->musiccount++;
    if (p->musiccount >= p->musicmax)
    {
        p->np_row++;

        if (p->jumptorow != -1)
        {
            p->np_row = p->jumptorow;
            p->jumptorow = -1;
        }

        // np_row = 0..63, 64 = get new pat
        if ((p->np_row >= 64) || (!p->patloopcount && p->breakpat))
        {
            if (p->breakpat == 255)
            {
                p->breakpat = 0;
                p->Playing  = 0;

                return;
            }

            p->breakpat = 0;
            p->np_row = neworder(p); // if breakpat, np_row = break row
        }

        // x_ used for info retrieval
        p->x_np_ord = p->np_ord;
        p->x_np_row = p->np_row;
        p->x_np_pat = p->np_pat;
        
        if (p->np_row == 0)
        {
            offset = (p->np_ord - 1) / 8;
            bit = 1 << ((p->np_ord - 1) % 8);
            if (p->playedOrder[offset] & bit)
            {
                p->loopCount++;
                memset(p->playedOrder, 0, sizeof(p->playedOrder));
            }
            p->playedOrder[offset] |= bit;
        }

        p->musiccount = 0;
    }
}

static void loadheaderparms(PLAYER *p)
{
    uint8_t *insdat;
    uint16_t insnum;
    uint32_t i;
    uint32_t j;
    uint32_t inslen;
    uint32_t insoff;

    // set to init defaults first
    p->oldstvib = 0;
    setspeed(p, 6);
    settempo(p, 125);
    p->aspdmin = 64;
    p->aspdmax = 32767;
    p->globalvol = 64;
    p->amigalimits = 0;
    p->fastvolslide = 0;
    setStereoMode(p, 0);
    setMasterVolume(p, 48);

    p->tracker = p->mseg[0x29] >> 4;

    if (p->mseg[0x33])
    {
        if (p->mseg[0x33] & 0x80)
            setStereoMode(p, 1);

        if (p->mseg[0x33] & 0x7F)
        {
            if ((p->mseg[0x33] & 0x7F) < 16)
                setMasterVolume(p, 16);
            else
                setMasterVolume(p, p->mseg[0x33] & 0x7F);
        }
    }

    if (p->mseg[0x32])
        settempo(p, p->mseg[0x32]);

    if (p->mseg[0x31] != 255)
        setspeed(p, p->mseg[0x31]);

    if (p->mseg[0x30] != 255)
    {
        p->globalvol = p->mseg[0x30];
        if (p->globalvol > 64)
            p->globalvol = 64;
    }

    if (p->mseg[0x26] != 255)
    {
        p->amigalimits  = p->mseg[0x26] & 0x10;
        p->fastvolslide = p->mseg[0x26] & 0x40;

        if (p->amigalimits)
        {  
            p->aspdmax = 1712 * 2;
            p->aspdmin =  907 / 2;
        }
    }

    // force fastvolslide if ST3.00
    if (get_le16(&p->mseg[0x28]) == 0x1300)
        p->fastvolslide = 1;

    p->oldstvib = p->mseg[0x26] & 0x01;
    
    if (*((uint16_t *)(&p->mseg[0x2A])))
    {
        // we have unsigned samples, convert to signed

        insnum = get_le16(&p->mseg[0x22]);
        for (i = 0; i < insnum; ++i)
        {
            insdat = &p->mseg[get_le16(&p->mseg[p->instrumentadd + (i << 1)]) << 4];
            insoff = (uint32_t)(((uint32_t)(insdat[0x0D])<<16)|((uint16_t)(insdat[0x0F])<<8)|insdat[0x0E])<<4;

            if (insoff && (insdat[0] == 1))
            {
                if (insoff > p->mseg_len)
                    insoff = p->mseg_len;

                inslen = get_le32(&insdat[0x10]);
                
                if (insdat[0x1E] == 4) // modplug packed
                    continue;

                if (insdat[0x1F] & 2) inslen <<= 1; // stereo

                if (insdat[0x1F] & 4)
                {
                    // 16-bit
                    if (insoff + inslen * 2 > p->mseg_len)
                        inslen = (p->mseg_len - insoff) / 2;
                    for (j = 0; j < inslen; ++j)
                        set_le16(&p->mseg[insoff + (j << 1)], get_le16(&p->mseg[insoff + (j << 1)]) - 0x8000);
                }
                else
                {
                    // 8-bit
                    if (insoff + inslen > p->mseg_len)
                        inslen = p->mseg_len - insoff;
                    for (j = 0; j < inslen; ++j)
                        p->mseg[insoff + j] = p->mseg[insoff + j] - 0x80;
                }
            }
        }
    }
}

void st3play_PlaySong(void *_p, int16_t startOrder)
{
    PLAYER * p = (PLAYER *)_p;
    uint8_t i;
    uint8_t dat;
    int16_t pan;

    if (!p->ModuleLoaded) return;

    memset(p->voice, 0, sizeof (p->voice));

    loadheaderparms(p);

    p->np_ord = startOrder;
    neworder(p);
    p->x_np_ord = p->np_ord;

    // setup pans
    for (i = 0; i < 32; ++i)
    {
        pan = (p->mseg[0x33] & 0x80) ? ((p->mseg[0x40 + i] & 0x08) ? 192 : 64) : 128;
        if (p->mseg[0x35] == 0xFC) // non-default pannings follow
        {
            dat = p->mseg[(p->patternadd + (get_le16(&p->mseg[0x24]) << 1)) + i];
            if (dat & 0x20)
                pan = (dat & 0x0F) << 4;
        }

        if (stereomode)
            p->chn[i].apanpos = pan;
        else
            p->chn[i].apanpos = 7;
      
        voiceSetPanning(p, i, pan);
    }

    p->Playing = 1;
    setSamplesPerFrame(p, ((p->outputFreq * 5UL) / 2 / p->tempo));
    p->isMixing = 1;
    
    p->loopCount = 0;
    memset(p->playedOrder, 0, sizeof(p->playedOrder));
    p->playedOrder[startOrder / 8] = 1 << (startOrder % 8);
}

int8_t st3play_LoadModule(void *_p, const uint8_t *module, size_t size)
{
    PLAYER * p = (PLAYER *)_p;
    char SCRM[4];
    uint16_t i;

    if (p->ModuleLoaded)
        FreeSong(p);

    p->ModuleLoaded = 0;

    if (size < 0x30)
    {
        return (0);
    }

    memcpy(SCRM, module + 0x2C, 4);
    if (memcmp(SCRM, "SCRM", 4))
    {
        return (0);
    }

    if (p->mseg)
    {
        free(p->mseg);
        p->mseg = NULL;
    }

    p->mseg = (uint8_t *)(malloc(size));
    if (p->mseg == NULL)
    {
        return (0);
    }

    memcpy(p->mseg, module, size);

    p->mseg_len = (uint32_t) size;

    p->instrumentadd    = 0x60          +  p->mseg[0x20];
    p->patternadd       = p->instrumentadd + (p->mseg[0x22] << 1);

    p->musiccount       = 0;
    p->patterndelay     = 0;
    p->patloopstart     = 0;
    p->patloopcount     = 0;
    p->np_row           = 0;
    p->np_pat           = 0;
    p->x_np_row         = 0;
    p->x_np_pat         = 0;
    p->x_np_ord         = 0;
    p->startrow         = 0;
    p->np_patseg        = 0;
    p->np_patoff        = 0;
    p->breakpat         = 0;
    p->patmusicrand     = 0;
    p->volslidetype     = 0;
    p->jumptorow        = 0;

    // zero all channel vars
    memset(p->chn, 0, sizeof (p->chn));

    p->numChannels = 0;
    for (i = 0; i < 32; ++i)
    {
        if (!(p->mseg[0x40 + i] & 0x80))
            p->numChannels++;

        p->chn[i].channelnum   = (int8_t)(i);
        p->chn[i].achannelused = 0x80;
        p->chn[i].chanvol      = 0x40;
    }

    p->lastachannelused = 1;

    // count *real* amounts of orders
    i = get_le16(&p->mseg[0x20]);
    while (i)
    {
        if (p->mseg[0x60 + (i - 1)] != 255)
            break;

        i--;
    }
    set_le16(&p->mseg[0x20], i);

    p->ModuleLoaded = 1;

    return (1);
}


// EFFECTS

// non-used effects
static void s_ret(PLAYER *p, chn_t *ch)         { (void)p; (void)ch; }
static void s_setfinetune(PLAYER *p, chn_t *ch) { (void)p; (void)ch; } // this function is 100% broken in ST3
// ----------------

static void s_setgliss(PLAYER *p, chn_t *ch)
{
    (void)p;
    ch->aglis = ch->info & 0x0F;
}

static void s_setvibwave(PLAYER *p, chn_t *ch)
{
    (void)p;
    ch->avibtretype = (ch->avibtretype & 0xF0) | ((ch->info << 1) & 0x0F);
}

static void s_settrewave(PLAYER *p, chn_t *ch)
{
    (void)p;
    ch->avibtretype = ((ch->info << 5) & 0xF0) | (ch->avibtretype & 0x0F);
}

static void s_setpanwave(PLAYER *p, chn_t *ch) // NON-ST3
{
    if (p->tracker != SCREAM_TRACKER)
        ch->apantype = ch->info & 0x0F;
}

static void s_setpanpos(PLAYER *p, chn_t *ch)
{
    if (stereomode)
    {
        ch->apanpos = (ch->info & 0x0F) << 4;

        setpan(p, ch->channelnum);
    }
}

static void s_sndcntrl(PLAYER *p, chn_t *ch) // NON-ST3
{
    if (p->tracker != SCREAM_TRACKER)
    {
        if ((ch->info & 0x0F) == 0)
        {
            ch->surround = 0;
            voiceSetSurround(p, ch->channelnum, 0);
        }
        else if ((ch->info & 0x0F) == 1)
        {
            ch->surround = 1;
            voiceSetSurround(p, ch->channelnum, 1);
        }
    }
}

static void s_patloop(PLAYER *p, chn_t *ch)
{
    if (!(ch->info & 0x0F))
    {
        p->patloopstart = p->np_row;
        return;
    }

    if (!p->patloopcount)
    {
        p->patloopcount = (ch->info & 0x0F) + 1;

        if (p->patloopstart == -1)
            p->patloopstart = 0; // default loopstart
    }

    if (p->patloopcount > 1)
    {
        p->patloopcount--;
        p->jumptorow = p->patloopstart;
        p->np_patoff = -1; // force reseek
        if (p->patloopstart == 0)
        {
            int32_t offset = (p->np_ord - 1) / 8;
            int32_t bit = 1 << ((p->np_ord - 1) % 8);
            p->playedOrder[offset] &= ~bit;
        }
    }
    else
    {
        p->patloopcount = 0;
        p->patloopstart = p->np_row + 1;
    }
}

static void s_notecut(PLAYER *p, chn_t *ch)
{
    (void)p;
    ch->anotecutcnt = ch->info & 0x0F;
}

static void s_notecutb(PLAYER *p, chn_t *ch)
{
    if (ch->anotecutcnt)
    {
        ch->anotecutcnt--;
        if (!ch->anotecutcnt)
            voiceSetSamplingFrequency(p, ch->channelnum, 0); // cut note
    }
}

static void s_notedelay(PLAYER *p, chn_t *ch)
{
    (void)p;
    ch->anotedelaycnt = ch->info & 0x0F;
}

static void s_notedelayb(PLAYER *p, chn_t *ch)
{
    if (ch->anotedelaycnt)
    {
        ch->anotedelaycnt--;
        if (!ch->anotedelaycnt)
            donewnote(p, ch->channelnum, 1); // 1 = notedelay end
    }
}

static void s_patterdelay(PLAYER *p, chn_t *ch)
{
    if (p->patterndelay == 0)
        p->patterndelay = ch->info & 0x0F;
}

static void s_setspeed(PLAYER *p, chn_t *ch)
{
    setspeed(p, ch->info);
}

static void s_jmpto(PLAYER *p, chn_t *ch)
{
    if (ch->info != 255)
    {
        p->breakpat = 1;
        p->np_ord = ch->info;
    }
    else
    {
        p->breakpat = 255;
    }
}

static void s_break(PLAYER *p, chn_t *ch)
{
    p->startrow = ((ch->info >> 4) * 10) + (ch->info & 0x0F);
    p->breakpat = 1;
}

static void s_volslide(PLAYER *p, chn_t *ch)
{
    uint8_t infohi;
    uint8_t infolo;

    getlastnfo(p, ch);

    infohi = ch->info >> 0x04;
    infolo = ch->info  & 0x0F;

    if (infolo == 0x0F)
    {
        if (!infohi)
            ch->avol -= infolo;
        else if (!p->musiccount)
            ch->avol += infohi;
    }
    else if (infohi == 0x0F)
    {
        if (!infolo)
            ch->avol += infohi;
        else if (!p->musiccount)
            ch->avol -= infolo;
    }
    else if (p->fastvolslide || p->musiccount)
    {
        if (!infolo)
            ch->avol += infohi;
        else
            ch->avol -= infolo;
    }
    else
    {
        return; // illegal slide
    }

    if (ch->avol <  0) ch->avol =  0;
    if (ch->avol > 63) ch->avol = 63;

    setvol(p, ch->channelnum);

    if (p->volslidetype == 1)
        s_vibrato(p, ch);
    else if (p->volslidetype == 2)
        s_toneslide(p, ch);
}

static void s_slidedown(PLAYER *p, chn_t *ch)
{
    if (ch->aorgspd)
    {
        getlastnfo(p, ch);

        if (p->musiccount)
        {
            if (ch->info >= 0xE0) return; // no fine slides here

            ch->aspd += ((uint16_t)(ch->info) << 2);
            if (ch->aspd > 32767) ch->aspd = 32767;
        }
        else
        {
            if (ch->info <= 0xE0) return; // only fine slides here

            if (ch->info <= 0xF0)
            {
                ch->aspd += (ch->info & 0x0F);
                if (ch->aspd > 32767) ch->aspd = 32767;
            }
            else
            {
                ch->aspd += ((ch->info & 0x0F) << 2);
                if (ch->aspd > 32767) ch->aspd = 32767;
            }
        }

        ch->aorgspd = ch->aspd;
        setspd(p, ch->channelnum);
    }
}

static void s_slideup(PLAYER *p, chn_t *ch)
{
    if (ch->aorgspd)
    {
        getlastnfo(p, ch);

        if (p->musiccount)
        {
            if (ch->info >= 0xE0) return; // no fine slides here

            ch->aspd -= ((uint16_t)(ch->info) << 2);
            if (ch->aspd < 0) ch->aspd = 0;
        }
        else
        {
            if (ch->info <= 0xE0) return; // only fine slides here

            if (ch->info <= 0xF0)
            {
                ch->aspd -= (ch->info & 0x0F);
                if (ch->aspd < 0) ch->aspd = 0;
            }
            else
            {
                ch->aspd -= ((ch->info & 0x0F) << 2);
                if (ch->aspd < 0) ch->aspd = 0;
            }
        }

        ch->aorgspd = ch->aspd;
        setspd(p, ch->channelnum);
    }
}

static void s_toneslide(PLAYER *p, chn_t *ch)
{
    if (p->volslidetype == 2) // we came from a Lxy (toneslide+volslide)
    {
        ch->info = ch->alasteff1;
    }
    else
    {
        if (!ch->aorgspd)
        {
            if (!ch->asldspd)
                return;

            ch->aorgspd = ch->asldspd;
            ch->aspd    = ch->asldspd;
        }

        if (!ch->info)
            ch->info = ch->alasteff1;
        else
            ch->alasteff1 = ch->info;
   }

    if (ch->aorgspd != ch->asldspd)
    {
        if (ch->aorgspd < ch->asldspd)
        {
            ch->aorgspd += ((uint16_t)(ch->info) << 2);
            if (ch->aorgspd > ch->asldspd)
                ch->aorgspd = ch->asldspd;
        }
        else
        {
            ch->aorgspd -= ((uint16_t)(ch->info) << 2);
            if (ch->aorgspd < ch->asldspd)
                ch->aorgspd = ch->asldspd;
        }

        if (ch->aglis)
            ch->aspd = roundspd(p, ch->channelnum, ch->aorgspd);
        else
            ch->aspd = ch->aorgspd;

        setspd(p, ch->channelnum);
    }
}

static void s_vibrato(PLAYER *p, chn_t *ch)
{
    int8_t type;
    int16_t cnt;
    int32_t dat;

    if (p->volslidetype == 1) // we came from a Kxy (vibrato+volslide)
    {
        ch->info = ch->alasteff;
    }
    else
    {
        if (!ch->info)
            ch->info = ch->alasteff;

        if (!(ch->info & 0xF0))
            ch->info = (ch->alasteff & 0xF0) | (ch->info & 0x0F);

        ch->alasteff = ch->info;
    }

    if (ch->aorgspd)
    {
        cnt  = ch->avibcnt;
        type = (ch->avibtretype & 0x0E) >> 1;

        // sine
        if ((type == 0) || (type == 4))
        {
            if (type == 4)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibsin[cnt >> 1];
        }

        // ramp
        else if ((type == 1) || (type == 5))
        {
            if (type == 5)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibramp[cnt >> 1];
        }

        // square
        else if ((type == 2) || (type == 6))
        {
            if (type == 6)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibsqu[cnt >> 1];
        }

        // random
        else if ((type == 3) || (type == 7))
        {
            if (type == 7)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = (int32_t)(vibsin[cnt >> 1]);
            cnt += (p->patmusicrand & 0x1E);
        }

        // warning C4701: potentially uninitialized
        else
        {
            dat  = 0;
        }

        if (p->oldstvib)
            dat = ((dat * (ch->info & 0x0F)) >> 4) + ch->aorgspd;
        else
            dat = ((dat * (ch->info & 0x0F)) >> 5) + ch->aorgspd;

        ch->aspd = dat;
        setspd(p, ch->channelnum);

        ch->avibcnt = (cnt + ((ch->info >> 4) << 1)) & 0x7E;
    }
}

static void s_tremor(PLAYER *p, chn_t *ch)
{
    getlastnfo(p, ch);

    if (ch->atremor)
    {
        ch->atremor--;
        return;
    }

    if (ch->atreon)
    {
        ch->atreon = 0;

        ch->avol = 0;
        setvol(p, ch->channelnum);

        ch->atremor = ch->info & 0x0F;
    }
    else
    {
        ch->atreon = 1;

        ch->avol = ch->aorgvol;
        setvol(p, ch->channelnum);

        ch->atremor = ch->info >> 4;
    }
}

static void s_arp(PLAYER *p, chn_t *ch)
{
    int8_t note;
    int8_t octa;
    int8_t noteadd;
    uint8_t tick;

    getlastnfo(p, ch);

    tick    = p->musiccount % 3;
    noteadd = 0;

    if (tick == 1)
        noteadd = ch->info >> 4;
    else if (tick == 2)
        noteadd = ch->info & 0x0F;

    // check for octave overflow
    octa =  ch->lastnote & 0xF0;
    note = (ch->lastnote & 0x0F) + noteadd;
    while (note >= 12)
    {
        note -= 12;
        octa += 16;
    }

    ch->aspd = scalec2spd(p, ch->channelnum, stnote2herz(p, octa | note));
    setspd(p, ch->channelnum);
}

static void s_chanvol(PLAYER *p, chn_t *ch) // NON-ST3
{
    if (p->tracker != SCREAM_TRACKER)
    {
        if (ch->info <= 0x40)
            ch->chanvol = ch->info;
    }
}

static void s_chanvolslide(PLAYER *p, chn_t *ch) // NON-ST3
{
    uint8_t infohi;
    uint8_t infolo;

    if (p->tracker != SCREAM_TRACKER)
    {
        if (ch->info)
            ch->nxymem = ch->info;
        else
            ch->info   = ch->nxymem;

        infohi = ch->nxymem >> 0x04;
        infolo = ch->nxymem  & 0x0F;

        if (infolo == 0x0F)
        {
            if (!infohi)
                ch->chanvol -= infolo;
            else if (!p->musiccount)
                ch->chanvol += infohi;
        }
        else if (infohi == 0x0F)
        {
            if (!infolo)
                ch->chanvol += infohi;
            else if (!p->musiccount)
                ch->chanvol -= infolo;
        }
        else if (p->musiccount) // don't rely on fastvolslide flag here
        {
            if (!infolo)
                ch->chanvol += infohi;
            else
                ch->chanvol -= infolo;
        }
        else
        {
            return; // illegal slide
        }

        if (ch->chanvol <  0) ch->chanvol =  0;
        if (ch->chanvol > 64) ch->chanvol = 64;

        setvol(p, ch->channelnum);
    }
}

static void s_vibvol(PLAYER *p, chn_t *ch)
{
    p->volslidetype = 1;
    s_volslide(p, ch);
}

static void s_tonevol(PLAYER *p, chn_t *ch)
{
    p->volslidetype = 2;
    s_volslide(p, ch);
}

static void s_panslide(PLAYER *p, chn_t *ch) // NON-ST3
{
    uint8_t infohi;
    uint8_t infolo;

    if (p->tracker != SCREAM_TRACKER)
    {
        if (ch->info)
            ch->pxymem = ch->info;
        else
            ch->info = ch->pxymem;

        infohi = ch->pxymem >> 0x04;
        infolo = ch->pxymem  & 0x0F;

        if (infolo == 0x0F)
        {
            if (!infohi)
                ch->apanpos += (infolo << 2);
            else if (!p->musiccount)
                ch->apanpos -= (infohi << 2);
        }
        else if (infohi == 0x0F)
        {
            if (!infolo)
                ch->apanpos -= (infohi << 2);
            else if (!p->musiccount)
                ch->apanpos += (infolo << 2);
        }
        else if (p->musiccount) // don't rely on fastvolslide flag here
        {
            if (!infolo)
                ch->apanpos -= (infohi << 2);
            else
                ch->apanpos += (infolo << 2);
        }
        else
        {
            return; // illegal slide
        }

        if (ch->apanpos <   0) ch->apanpos =   0;
        if (ch->apanpos > 256) ch->apanpos = 256;

        setpan(p, ch->channelnum);
    }
}

static void s_retrig(PLAYER *p, chn_t *ch)
{
    uint8_t infohi;

    getlastnfo(p, ch);
    infohi = ch->info >> 0x04;

    if (!(ch->info & 0x0F) || (ch->atrigcnt < (ch->info & 0x0F)))
    {
        ch->atrigcnt++;
        return;
    }

    ch->atrigcnt = 0;

    voiceSetSamplePosition(p, ch->channelnum, 0);

    if (!retrigvoladd[16 + infohi])
        ch->avol += retrigvoladd[infohi];
    else
        ch->avol = (int8_t)(((int16_t)(ch->avol) * retrigvoladd[16 + infohi]) >> 4);

    if (ch->avol > 63) ch->avol = 63;
    if (ch->avol <  0) ch->avol =  0;

    setvol(p, ch->channelnum);

    ch->atrigcnt++; // probably a mistake? Keep it anyways.
}

static void s_tremolo(PLAYER *p, chn_t *ch)
{
    int8_t type;
    int16_t cnt;
    int16_t dat;

    getlastnfo(p, ch);

    if (!(ch->info & 0xF0))
        ch->info = (ch->alastnfo & 0xF0) | (ch->info & 0x0F);

    ch->alastnfo = ch->info;

    if (ch->aorgvol)
    {
        cnt  = ch->avibcnt;
        type = ch->avibtretype >> 5;

        // sine
        if ((type == 0) || (type == 4))
        {
            if (type == 4)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibsin[cnt >> 1];
        }

        // ramp
        else if ((type == 1) || (type == 5))
        {
            if (type == 5)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibramp[cnt >> 1];
        }

        // square
        else if ((type == 2) || (type == 6))
        {
            if (type == 6)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibsqu[cnt >> 1];
        }

        // random
        else if ((type == 3) || (type == 7))
        {
            if (type == 7)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibsin[cnt >> 1];
            cnt += (p->patmusicrand & 0x1E);
        }

        // warning C4701: potentially uninitialized
        else
        {
            dat  = 0;
        }

        dat = ((dat * (ch->info & 0x0F)) >> 7) + ch->aorgvol;
        if (dat > 63) dat = 63;
        if (dat <  0) dat =  0;

        ch->avol = (int8_t)(dat);
        setvol(p, ch->channelnum);

        ch->avibcnt = (cnt + ((ch->info >> 4) << 1)) & 0x7E;
    }
}

static void s_scommand1(PLAYER *p, chn_t *ch)
{
    getlastnfo(p, ch);
    ssoncejmp[ch->info >> 4](p, ch);
}

static void s_scommand2(PLAYER *p, chn_t *ch)
{
    getlastnfo(p, ch);
    ssotherjmp[ch->info >> 4](p, ch);
}

static void s_settempo(PLAYER *p, chn_t *ch)
{
    if (!p->musiccount && (ch->info >= 0x20))
        p->tempo = ch->info;

    // NON-ST3 tempo slide
    if (p->tracker != SCREAM_TRACKER)
    {
        if (!p->musiccount)
        {
            if (!ch->info)
                ch->info = ch->txxmem;
            else
                ch->txxmem = ch->info;
        }
        else if (p->musiccount)
        {
            if (ch->info <= 0x0F)
            {
                p->tempo -= ch->info;
                if (p->tempo < 32)
                    p->tempo = 32;
            }
            else if (ch->info <= 0x1F)
            {
                p->tempo += (ch->info - 0x10);
                if (p->tempo > 255)
                    p->tempo = 255;
            }
        }
    }
    // ------------------

    settempo(p, p->tempo);
}

static void s_finevibrato(PLAYER *p, chn_t *ch)
{
    int8_t type;
    int16_t cnt;
    int32_t dat;

    if (!ch->info)
        ch->info = ch->alasteff;

    if (!(ch->info & 0xF0))
        ch->info = (ch->alasteff & 0xF0) | (ch->info & 0x0F);

    ch->alasteff = ch->info;

    if (ch->aorgspd)
    {
        cnt  =  ch->avibcnt;
        type = (ch->avibtretype & 0x0E) >> 1;

        // sine
        if ((type == 0) || (type == 4))
        {
            if (type == 4)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibsin[cnt >> 1];
        }

        // ramp
        else if ((type == 1) || (type == 5))
        {
            if (type == 5)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibramp[cnt >> 1];
        }

        // square
        else if ((type == 2) || (type == 6))
        {
            if (type == 6)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibsqu[cnt >> 1];
        }

        // random
        else if ((type == 3) || (type == 7))
        {
            if (type == 7)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibsin[cnt >> 1];
            cnt += (p->patmusicrand & 0x1E);
        }

        // warning C4701: potentially uninitialized
        else
        {
            dat  = 0;
        }

        if (p->oldstvib)
            dat = ((dat * (ch->info & 0x0F)) >> 6) + ch->aorgspd;
        else
            dat = ((dat * (ch->info & 0x0F)) >> 7) + ch->aorgspd;

        ch->aspd = dat;
        setspd(p, ch->channelnum);

        ch->avibcnt = (cnt + ((ch->info >> 4) << 1)) & 0x7E;
    }
}

static void s_setgvol(PLAYER *p, chn_t *ch)
{
    if (ch->info <= 64)
        p->globalvol = ch->info;
}

static void s_globvolslide(PLAYER *p, chn_t *ch) // NON-ST3
{
    uint8_t i;
    uint8_t infohi;
    uint8_t infolo;

    if (p->tracker != SCREAM_TRACKER)
    {
        if (ch->info)
            ch->wxymem = ch->info;
        else
            ch->info = ch->wxymem;

        infohi = ch->wxymem >> 0x04;
        infolo = ch->wxymem  & 0x0F;

        if (infolo == 0x0F)
        {
            if (!infohi)
                p->globalvol -= infolo;
            else if (!p->musiccount)
                p->globalvol += infohi;
        }
        else if (infohi == 0x0F)
        {
            if (!infolo)
                p->globalvol += infohi;
            else if (!p->musiccount)
                p->globalvol -= infolo;
        }
        else if (p->musiccount) // don't rely on fastvolslide flag here
        {
            if (!infolo)
                p->globalvol += infohi;
            else
                p->globalvol -= infolo;
        }
        else
        {
            return; // illegal slide
        }

        if (p->globalvol <  0) p->globalvol =  0;
        if (p->globalvol > 64) p->globalvol = 64;

        // update all channels now
        for (i = 0; i < (p->lastachannelused + 1); ++i) setvol(p, i);
    }
}

static void s_setpan(PLAYER *p, chn_t *ch) // NON-ST3
{
    // This one should work even in MONO mode
    // for newer trackers that exports as ST3

    // Yes, I decided to comment this if check.
    //if (tracker != SCREAM_TRACKER)
    {
        if (ch->info <= 0x80)
        {
            ch->apanpos = (int16_t)(ch->info) << 1;
            setpan(p, ch->channelnum);
        }
        else if (ch->info == 0xA4) // surround
        {
            ch->surround = 1;
            voiceSetSurround(p, ch->channelnum, 1);
        }
    }
}

static void s_panbrello(PLAYER *p, chn_t *ch) // NON-ST3
{
    int8_t type;
    int16_t cnt;
    int16_t dat;

    if (p->tracker != SCREAM_TRACKER)
    {
        if (!p->musiccount)
        {
            if (!ch->info)
                ch->info = ch->alasteff;
            else
                ch->yxymem = ch->info;

            if (!(ch->info & 0xF0))
                ch->info = (ch->yxymem & 0xF0) | (ch->info & 0x0F);

            if (!(ch->info & 0x0F))
                ch->info = (ch->info & 0xF0) | (ch->yxymem & 0x0F);
        }

        cnt  = ch->apancnt;
        type = ch->apantype;

        // sine
        if ((type == 0) || (type == 4))
        {
            if (type == 4)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibsin[cnt >> 1];
        }

        // ramp
        else if ((type == 1) || (type == 5))
        {
            if (type == 5)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibramp[cnt >> 1];
        }

        // square
        else if ((type == 2) || (type == 6))
        {
            if (type == 6)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibsqu[cnt >> 1];
        }

        // random
        else if ((type == 3) || (type == 7))
        {
            if (type == 7)
            {
                cnt &= 0x7F;
            }
            else
            {
                if (cnt & 0x80) cnt = 0;
            }

            dat = vibsin[cnt >> 1];
            cnt += (p->patmusicrand & 0x1E);
        }

        // warning C4701: potentially uninitialized
        else
        {
            dat  = 0;
        }

        dat = ((dat * (ch->info & 0x0F)) >> 4) + ch->apanpos;

        if (dat <   0) dat =   0;
        if (dat > 256) dat = 256;

        voiceSetPanning(p, ch->channelnum, dat);

        ch->apancnt = (cnt + ((ch->info >> 6) << 1)) & 0x7E;
    }
}

void setSamplesPerFrame(PLAYER *p, uint32_t val)
{
    p->samplesPerFrame = val;
}

void setSamplingInterpolation(PLAYER *p, int8_t value)
{
    p->samplingInterpolation = value;
}

void setStereoMode(PLAYER *p, int8_t value)
{
    p->stereomode = value;
}

void setMasterVolume(PLAYER *p, uint8_t value)
{
    p->mastervol = value;
    p->f_masterVolume = (float)(value) / 127.0f;
}

void voiceSetSource(PLAYER *p, uint8_t voiceNumber, const int8_t *sampleData,
    int32_t sampleLength, int32_t sampleLoopLength, int32_t sampleLoopEnd,
    int8_t loopEnabled, int8_t sixteenbit, int8_t stereo, int8_t adpcm)
{
    p->voice[voiceNumber].sampleData       = sampleData;
    p->voice[voiceNumber].sampleLength     = sampleLength;
    p->voice[voiceNumber].sampleLoopEnd    = sampleLoopEnd;
    p->voice[voiceNumber].sampleLoopLength = sampleLoopLength;
    p->voice[voiceNumber].loopEnabled      = loopEnabled;
    if (adpcm)
    {
        p->voice[voiceNumber].sixteenBit   = 0;
        p->voice[voiceNumber].stereo       = 0;
        p->voice[voiceNumber].lastDelta    = 0;
        p->voice[voiceNumber].lastSamplePosition = 0;
    }
    else
    {
        p->voice[voiceNumber].sixteenBit   = sixteenbit;
        p->voice[voiceNumber].stereo       = stereo;
    }
    p->voice[voiceNumber].adpcm            = adpcm;
    p->voice[voiceNumber].mixing           = 1;
    p->voice[voiceNumber].interpolating    = 1;
    p->voice[voiceNumber].oversampleCount  = 0;

    if (p->voice[voiceNumber].samplePosition >= p->voice[voiceNumber].sampleLength)
        p->voice[voiceNumber].samplePosition = 0;
    
    lanczos_resampler_clear( p->resampler[voiceNumber] );
    lanczos_resampler_clear( p->resampler[voiceNumber+32] );
}

void voiceSetSamplePosition(PLAYER *p, uint8_t voiceNumber, uint16_t value)
{
    p->voice[voiceNumber].samplePosition = value;
    p->voice[voiceNumber].mixing         = 1;
    p->voice[voiceNumber].interpolating  = 1;
    p->voice[voiceNumber].oversampleCount  = 0;

    if (p->voice[voiceNumber].loopEnabled)
    {
        while (p->voice[voiceNumber].samplePosition >= p->voice[voiceNumber].sampleLoopEnd)
            p->voice[voiceNumber].samplePosition -= p->voice[voiceNumber].sampleLoopLength;
    }
    else if (p->voice[voiceNumber].samplePosition >= p->voice[voiceNumber].sampleLength)
    {
        p->voice[voiceNumber].mixing         = 0;
        p->voice[voiceNumber].interpolating  = 0;
        p->voice[voiceNumber].samplePosition = 0;
    }
    
    lanczos_resampler_clear( p->resampler[voiceNumber] );
    lanczos_resampler_clear( p->resampler[voiceNumber+32] );
}

void voiceSetVolume(PLAYER *p, uint8_t voiceNumber, float volume)
{
    p->voice[voiceNumber].volume = volume;
}

void voiceSetSurround(PLAYER *p, uint8_t voiceNumber, int8_t surround)
{
    if (surround)
        p->voice[voiceNumber].panningR = -p->voice[voiceNumber].orgPanR;
    else
        p->voice[voiceNumber].panningR =  p->voice[voiceNumber].orgPanR;
}

void voiceSetPanning(PLAYER *p, uint8_t voiceNumber, uint16_t pan)
{
    float pf;

    pf = (float)(pan) / 256.0f;

    p->voice[voiceNumber].panningL = 1.0f - pf;
    p->voice[voiceNumber].panningR = pf;
    p->voice[voiceNumber].orgPanR  = pf;
}

void voiceSetSamplingFrequency(PLAYER *p, uint8_t voiceNumber, float samplingFrequency)
{
    p->voice[voiceNumber].incRate = samplingFrequency / p->f_outputFreq;
}

static inline void mix8b(PLAYER *p, uint8_t ch, uint32_t samples)
{
    const int8_t *sampleData;
    int8_t loopEnabled;
    int32_t sampleLength;
    int32_t sampleLoopEnd;
    int32_t sampleLoopLength;
    int32_t samplePosition;
    int32_t samplingInterpolation;
    int32_t interpolating;
    int32_t oversampleCount;
    uint32_t j;
    float volume;
    float sample;
    float panningL;
    float panningR;
    void *resampler;

    sampleLength     = p->voice[ch].sampleLength;
    sampleLoopLength = p->voice[ch].sampleLoopLength;
    sampleLoopEnd    = p->voice[ch].sampleLoopEnd;
    loopEnabled      = p->voice[ch].loopEnabled;
    volume           = p->voice[ch].volume;
    panningL         = p->voice[ch].panningL;
    panningR         = p->voice[ch].panningR;
    interpolating    = p->voice[ch].interpolating;
    oversampleCount  = p->voice[ch].oversampleCount;

    sampleData = p->voice[ch].sampleData;
    samplingInterpolation = p->samplingInterpolation ? 1 : 32;
    
    resampler = p->resampler[ch];
    
    lanczos_resampler_set_rate( resampler, p->voice[ch].incRate * (float)samplingInterpolation );
    
    for (j = 0; (j < samples) && sampleData; ++j)
    {
        samplePosition = p->voice[ch].samplePosition;

        while (interpolating && lanczos_resampler_get_free_count(resampler))
        {
            for (; oversampleCount < samplingInterpolation && lanczos_resampler_get_free_count(resampler); ++oversampleCount)
                lanczos_resampler_write_sample(resampler, sampleData[samplePosition] * 256);
            
            if (oversampleCount < samplingInterpolation)
                break;

            samplePosition++;
            oversampleCount = 0;
            
            if (loopEnabled)
            {
                if (samplePosition >= sampleLoopEnd)
                    samplePosition -= sampleLoopLength;
            }
            else
            {
                if (samplePosition >= sampleLength)
                {
                    samplePosition = 0;
                    interpolating = 0;
                }
            }
        }
        
        p->voice[ch].samplePosition  = samplePosition;
        p->voice[ch].interpolating   = (int8_t)interpolating;
        p->voice[ch].oversampleCount = (int8_t)oversampleCount;

        if ( !lanczos_resampler_ready(resampler) )
        {
            p->voice[ch].mixing = 0;
            break;
        }
        
        sample = lanczos_resampler_get_sample(resampler);
        lanczos_resampler_remove_sample(resampler);
        
        sample *= volume;

        p->masterBufferL[j] += (sample * panningL);
        p->masterBufferR[j] += (sample * panningR);
    }
}

static inline void mix8bstereo(PLAYER *p, uint8_t ch, uint32_t samples)
{
    const int8_t *sampleData;
    int8_t loopEnabled;
    int32_t sampleLength;
    int32_t sampleLoopEnd;
    int32_t sampleLoopLength;
    int32_t samplePosition;
    int32_t samplingInterpolation;
    int32_t interpolating;
    int32_t oversampleCount;
    uint32_t j;
    float volume;
    float sampleL;
    float sampleR;
    float panningL;
    float panningR;
    void *resampler[2];

    sampleLength     = p->voice[ch].sampleLength;
    sampleLoopLength = p->voice[ch].sampleLoopLength;
    sampleLoopEnd    = p->voice[ch].sampleLoopEnd;
    loopEnabled      = p->voice[ch].loopEnabled;
    volume           = p->voice[ch].volume;
    panningL         = p->voice[ch].panningL;
    panningR         = p->voice[ch].panningR;
    interpolating    = p->voice[ch].interpolating;
    oversampleCount  = p->voice[ch].oversampleCount;

    sampleData = p->voice[ch].sampleData;
    samplingInterpolation = p->samplingInterpolation ? 1 : 32;
    
    resampler[0] = p->resampler[ch];
    resampler[1] = p->resampler[ch+32];
    
    lanczos_resampler_set_rate(resampler[0], p->voice[ch].incRate * (float)samplingInterpolation);
    lanczos_resampler_set_rate(resampler[1], p->voice[ch].incRate * (float)samplingInterpolation);

    for (j = 0; (j < samples) && sampleData; ++j)
    {
        samplePosition = p->voice[ch].samplePosition;
        
        while (interpolating && lanczos_resampler_get_free_count(resampler[0]))
        {
            for (; oversampleCount < samplingInterpolation && lanczos_resampler_get_free_count(resampler[0]); ++oversampleCount)
            {
                lanczos_resampler_write_sample(resampler[0], sampleData[samplePosition] * 256);
                lanczos_resampler_write_sample(resampler[1], sampleData[sampleLength + samplePosition] * 256);
            }
            
            if (oversampleCount < samplingInterpolation)
                break;
            
            samplePosition++;
            oversampleCount = 0;
            
            if (loopEnabled)
            {
                if (samplePosition >= sampleLoopEnd)
                    samplePosition -= sampleLoopLength;
            }
            else
            {
                if (samplePosition >= sampleLength)
                {
                    samplePosition = 0;
                    interpolating = 0;
                }
            }
        }
        
        p->voice[ch].samplePosition  = samplePosition;
        p->voice[ch].interpolating   = (int8_t)interpolating;
        p->voice[ch].oversampleCount = (int8_t)oversampleCount;
        
        if ( !lanczos_resampler_ready(resampler[0]) )
        {
            p->voice[ch].mixing = 0;
            break;
        }
        
        sampleL = lanczos_resampler_get_sample(resampler[0]);
        sampleR = lanczos_resampler_get_sample(resampler[1]);
        lanczos_resampler_remove_sample(resampler[0]);
        lanczos_resampler_remove_sample(resampler[1]);

        sampleL *= volume;
        sampleR *= volume;

        p->masterBufferL[j] += (sampleL * panningL);
        p->masterBufferR[j] += (sampleR * panningR);
    }
}

static inline void mix16b(PLAYER *p, uint8_t ch, uint32_t samples)
{
    const int16_t *sampleData;
    int8_t loopEnabled;
    int32_t sampleLength;
    int32_t sampleLoopEnd;
    int32_t sampleLoopLength;
    int32_t samplePosition;
    int32_t samplingInterpolation;
    int32_t interpolating;
    int32_t oversampleCount;
    uint32_t j;
    float volume;
    float sample;
    float panningL;
    float panningR;
    void *resampler;
    
    sampleLength     = p->voice[ch].sampleLength;
    sampleLoopLength = p->voice[ch].sampleLoopLength;
    sampleLoopEnd    = p->voice[ch].sampleLoopEnd;
    loopEnabled      = p->voice[ch].loopEnabled;
    volume           = p->voice[ch].volume;
    panningL         = p->voice[ch].panningL;
    panningR         = p->voice[ch].panningR;
    interpolating    = p->voice[ch].interpolating;
    oversampleCount  = p->voice[ch].oversampleCount;
    
    sampleData = (const int16_t*)(p->voice[ch].sampleData);
    samplingInterpolation = p->samplingInterpolation ? 1 : 32;
    
    resampler = p->resampler[ch];
    
    lanczos_resampler_set_rate( resampler, p->voice[ch].incRate * (float)samplingInterpolation );
    
    for (j = 0; (j < samples) && sampleData; ++j)
    {
        samplePosition = p->voice[ch].samplePosition;
        
        while (interpolating && lanczos_resampler_get_free_count(resampler))
        {
            for (; oversampleCount < samplingInterpolation && lanczos_resampler_get_free_count(resampler); ++oversampleCount)
                lanczos_resampler_write_sample(resampler, get_le16(&sampleData[samplePosition]));
            
            if (oversampleCount < samplingInterpolation)
                break;
            
            samplePosition++;
            oversampleCount = 0;
            
            if (loopEnabled)
            {
                if (samplePosition >= sampleLoopEnd)
                    samplePosition -= sampleLoopLength;
            }
            else
            {
                if (samplePosition >= sampleLength)
                {
                    samplePosition = 0;
                    interpolating = 0;
                }
            }
        }
        
        p->voice[ch].samplePosition  = samplePosition;
        p->voice[ch].interpolating   = (int8_t)interpolating;
        p->voice[ch].oversampleCount = (int8_t)oversampleCount;
        
        if ( !lanczos_resampler_ready(resampler) )
        {
            p->voice[ch].mixing = 0;
            break;
        }
        
        sample = lanczos_resampler_get_sample(resampler);
        lanczos_resampler_remove_sample(resampler);
        
        sample *= volume;
        
        p->masterBufferL[j] += (sample * panningL);
        p->masterBufferR[j] += (sample * panningR);
    }
}

static inline void mix16bstereo(PLAYER *p, uint8_t ch, uint32_t samples)
{
    const int16_t *sampleData;
    int8_t loopEnabled;
    int32_t sampleLength;
    int32_t sampleLoopEnd;
    int32_t sampleLoopLength;
    int32_t samplePosition;
    int32_t samplingInterpolation;
    int32_t interpolating;
    int32_t oversampleCount;
    uint32_t j;
    float volume;
    float sampleL;
    float sampleR;
    float panningL;
    float panningR;
    void *resampler[2];
    
    sampleLength     = p->voice[ch].sampleLength;
    sampleLoopLength = p->voice[ch].sampleLoopLength;
    sampleLoopEnd    = p->voice[ch].sampleLoopEnd;
    loopEnabled      = p->voice[ch].loopEnabled;
    volume           = p->voice[ch].volume;
    panningL         = p->voice[ch].panningL;
    panningR         = p->voice[ch].panningR;
    interpolating    = p->voice[ch].interpolating;
    oversampleCount  = p->voice[ch].oversampleCount;
    
    sampleData = (const int16_t*)(p->voice[ch].sampleData);
    samplingInterpolation = p->samplingInterpolation ? 1 : 32;
    
    resampler[0] = p->resampler[ch];
    resampler[1] = p->resampler[ch+32];
    
    lanczos_resampler_set_rate(resampler[0], p->voice[ch].incRate * (float)samplingInterpolation);
    lanczos_resampler_set_rate(resampler[1], p->voice[ch].incRate * (float)samplingInterpolation);
    
    for (j = 0; (j < samples) && sampleData; ++j)
    {
        samplePosition = p->voice[ch].samplePosition;
        
        while (interpolating && lanczos_resampler_get_free_count(resampler[0]))
        {
            for (; oversampleCount < samplingInterpolation && lanczos_resampler_get_free_count(resampler[0]); ++oversampleCount)
            {
                lanczos_resampler_write_sample(resampler[0], get_le16(&sampleData[samplePosition]));
                lanczos_resampler_write_sample(resampler[1], get_le16(&sampleData[sampleLength + samplePosition]));
            }
            
            if (oversampleCount < samplingInterpolation)
                break;
            
            samplePosition++;
            oversampleCount = 0;
            
            if (loopEnabled)
            {
                if (samplePosition >= sampleLoopEnd)
                    samplePosition -= sampleLoopLength;
            }
            else
            {
                if (samplePosition >= sampleLength)
                {
                    samplePosition = 0;
                    interpolating = 0;
                }
            }
        }
        
        p->voice[ch].samplePosition  = samplePosition;
        p->voice[ch].interpolating   = (int8_t)interpolating;
        p->voice[ch].oversampleCount = (int8_t)oversampleCount;
        
        if ( !lanczos_resampler_ready(resampler[0]) )
        {
            p->voice[ch].mixing = 0;
            break;
        }
        
        sampleL = lanczos_resampler_get_sample(resampler[0]);
        sampleR = lanczos_resampler_get_sample(resampler[1]);
        lanczos_resampler_remove_sample(resampler[0]);
        lanczos_resampler_remove_sample(resampler[1]);
        
        sampleL *= volume;
        sampleR *= volume;
        
        p->masterBufferL[j] += (sampleL * panningL);
        p->masterBufferR[j] += (sampleR * panningR);
    }
}

static inline int8_t get_adpcm_sample(const int8_t *sampleDictionary, const uint8_t *sampleData, int32_t samplePosition, int8_t *lastDelta)
{
    uint8_t byte = sampleData[samplePosition / 2];
    byte = (samplePosition & 1) ? byte >> 4 : byte & 15;
    return *lastDelta += sampleDictionary[byte];
}

static inline void mixadpcm(PLAYER *p, uint8_t ch, uint32_t samples)
{
    const int8_t *sampleDictionary;
    const uint8_t *sampleData;
    int8_t loopEnabled;
    int32_t sampleLength;
    int32_t sampleLoopEnd;
    int32_t sampleLoopLength;
    int32_t samplePosition;
    int32_t samplingInterpolation;
    int8_t lastDelta;
    int32_t interpolating;
    int32_t oversampleCount;
    uint32_t j;
    float volume;
    float sample;
    float panningL;
    float panningR;
    void *resampler;
    
    sampleLength     = p->voice[ch].sampleLength;
    sampleLoopLength = p->voice[ch].sampleLoopLength;
    sampleLoopEnd    = p->voice[ch].sampleLoopEnd;
    loopEnabled      = p->voice[ch].loopEnabled;
    volume           = p->voice[ch].volume;
    panningL         = p->voice[ch].panningL;
    panningR         = p->voice[ch].panningR;
    interpolating    = p->voice[ch].interpolating;
    oversampleCount  = p->voice[ch].oversampleCount;
    lastDelta        = p->voice[ch].lastDelta;
    
    sampleDictionary = p->voice[ch].sampleData;
    sampleData = (uint8_t*)sampleDictionary + 16;
    samplingInterpolation = p->samplingInterpolation ? 1 : 32;
    
    while (p->voice[ch].lastSamplePosition < p->voice[ch].samplePosition)
    {
        get_adpcm_sample(sampleDictionary, sampleData, p->voice[ch].lastSamplePosition, &lastDelta);
        p->voice[ch].lastSamplePosition++;
        if (p->voice[ch].lastSamplePosition == sampleLoopEnd - sampleLoopLength)
            p->voice[ch].loopStartDelta = lastDelta;
    }
    
    resampler = p->resampler[ch];
    
    lanczos_resampler_set_rate( resampler, p->voice[ch].incRate * (float)samplingInterpolation );
    
    for (j = 0; (j < samples) && sampleData; ++j)
    {
        samplePosition = p->voice[ch].samplePosition;
        
        while (interpolating && lanczos_resampler_get_free_count(resampler))
        {
            int8_t nextDelta = lastDelta;
            int16_t sample = get_adpcm_sample(sampleDictionary, sampleData, samplePosition, &nextDelta) * 256;
            
            for (; oversampleCount < samplingInterpolation && lanczos_resampler_get_free_count(resampler); ++oversampleCount)
                lanczos_resampler_write_sample(resampler, sample);
            
            if (oversampleCount < samplingInterpolation)
                break;
            
            lastDelta = nextDelta;
            
            samplePosition++;
            oversampleCount = 0;
            
            if (loopEnabled)
            {
                if (samplePosition == sampleLoopEnd - sampleLoopLength)
                    p->voice[ch].loopStartDelta = lastDelta;
                
                if (samplePosition >= sampleLoopEnd)
                {
                    samplePosition -= sampleLoopLength;
                    lastDelta = p->voice[ch].loopStartDelta;
                }
            }
            else
            {
                if (samplePosition >= sampleLength)
                {
                    samplePosition = 0;
                    interpolating = 0;
                }
            }
        }
        
        p->voice[ch].samplePosition  = samplePosition;
        p->voice[ch].lastSamplePosition = samplePosition;
        p->voice[ch].interpolating   = (int8_t)interpolating;
        p->voice[ch].oversampleCount = (int8_t)oversampleCount;
        p->voice[ch].lastDelta       = lastDelta;
        
        if ( !lanczos_resampler_ready(resampler) )
        {
            p->voice[ch].mixing = 0;
            break;
        }
        
        sample = lanczos_resampler_get_sample(resampler);
        lanczos_resampler_remove_sample(resampler);
        
        sample *= volume;
        
        p->masterBufferL[j] += (sample * panningL);
        p->masterBufferR[j] += (sample * panningR);
    }
}

void mixSampleBlock(PLAYER *p, int32_t *outputStream, uint32_t sampleBlockLength)
{
    int32_t *streamPointer;
    uint8_t i;
    uint32_t j;

    float outL;
    float outR;

    streamPointer = outputStream;

    memset(p->masterBufferL, 0, sampleBlockLength * sizeof (float));
    memset(p->masterBufferR, 0, sampleBlockLength * sizeof (float));

    for (i = 0; i < 32; ++i)
    {
        if (p->voice[i].incRate && p->voice[i].mixing)
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
                else if (p->voice[i].adpcm)
                    mixadpcm(p, i, sampleBlockLength);
                else
                    mix8b(p, i, sampleBlockLength);
            }
        }
    }

    for (j = 0; j < sampleBlockLength; ++j)
    {
        outL = p->masterBufferL[j] * p->f_masterVolume;
        outR = p->masterBufferR[j] * p->f_masterVolume;

        if      (outL > INT_MAX) outL = INT_MAX;
        else if (outL < INT_MIN) outL = INT_MIN;
        if      (outR > INT_MAX) outR = INT_MAX;
        else if (outR < INT_MIN) outR = INT_MIN;

        *streamPointer++ = (int32_t)(outL);
        *streamPointer++ = (int32_t)(outR);
    }
}

void st3play_RenderFixed(void *_p, int32_t *buffer, int32_t count)
{
    PLAYER * p = (PLAYER *)_p;
    int32_t samplesTodo;
    int32_t *outputStream;
    
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

                    outputStream   += (samplesTodo << 1);
                }
                
                p->samplesLeft -= samplesTodo;
                count -= samplesTodo;
            }
            else
            {
                if (p->Playing)
                    dorow(p);

                p->samplesLeft = p->samplesPerFrame;
            }
        }
    }
}

void st3play_Render16(void *_p, int16_t *buffer, int32_t count)
{
    int32_t bufferFixed[1024];
    int32_t samplesTodo;
    int32_t i, sample;
    int16_t *outputStream;
    
    outputStream = buffer;
    
    while (count)
    {
        samplesTodo = (count > 512) ? 512 : count;
        st3play_RenderFixed(_p, bufferFixed, samplesTodo);
        count -= samplesTodo;
        samplesTodo <<= 1;
        for (i = 0; i < samplesTodo; ++i)
        {
            sample = bufferFixed[i] >> 8;
            if (sample > 32767) sample = 32767;
            else if (sample < -32768) sample = -32768;
            outputStream[i] = (int16_t)sample;
        }
        outputStream += samplesTodo;
    }
}

void FreeSong(PLAYER *p)
{
    p->Playing          = 0;

    memset(p->voice, 0, sizeof (p->voice));

    if (p->mseg)
    {
        free(p->mseg);
        p->mseg = NULL;
    }

    p->ModuleLoaded = 0;
}

int32_t st3play_GetLoopCount(void *_p)
{
    PLAYER * p = (PLAYER *)_p;
    return p->loopCount;
}

void st3play_GetInfo(void *_p, st3_info *info)
{
    int32_t i, channels_playing;
    PLAYER * p = (PLAYER *)_p;
    info->order = p->x_np_ord - 1;
    info->pattern = p->x_np_pat;
    info->row = p->x_np_row;
    info->speed = p->musicmax;
    info->tempo = p->tempo;
    channels_playing = 0;
    if (p->isMixing)
    {
        for (i = 0; i < 32; ++i)
        {
            if (p->voice[i].mixing)
                ++channels_playing;
        }
    }
    info->channels_playing = (int8_t)channels_playing;
}

// EOF
