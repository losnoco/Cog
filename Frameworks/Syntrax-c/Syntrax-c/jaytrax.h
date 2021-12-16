#ifndef JAYTRAX_H
#define JAYTRAX_H

#define WANTEDOVERLAP (15)    //wanted declick overlap length(in samples). Will be smaller than a song tick.
#define MIXBUF_LEN    (512)   //temporary mixing buffer length
#define SAMPSPOOLSIZE (0xFFF) //buffer for unrolling samples

enum INTERP_LIST {
    ITP_NONE,
    ITP_NEAREST,
    ITP_LINEAR,
    ITP_QUADRATIC,
    ITP_CUBIC,
    //ITP_BLEP,
    INTERP_COUNT
};

enum SE_BUFTYPE {
    BUF_MAINL,
    BUF_MAINR,
    BUF_ECHOL,
    BUF_ECHOR,
    MIXBUF_NR
};

enum SE_PLAYMODE {
    SE_PM_SONG = 0,
    SE_PM_PATTERN
};

//in case of changing any of the below
//please change jxs loader to account for changes
#define SE_ORDERS_SUBSONG (256)
#define SE_ROWS_PAT (64)
#define SE_EFF_INST (4)
#define SE_WAVES_INST (16)
#define SE_SAMPS_WAVE (256)
#define SE_ARPS_SONG (16)
#define SE_STEPS_ARP (16)
#define SE_NAMELEN (32)

#define SE_NROFCHANS (16)           // number of chans replayer can take
#define SE_NROFFINETUNESTEPS (16)   // number of finetune scales
#define SE_NROFEFFECTS (18)         // number of available wave effects

typedef struct JT1Order JT1Order;
struct JT1Order {
    int16_t patnr;      // welk pattern spelen...
    int16_t patlen;     // 0/16/32/48
};

typedef struct JT1Row JT1Row;
struct JT1Row {
    uint8_t srcnote;
    uint8_t dstnote;
    uint8_t inst;
    int8_t  param;
    uint8_t script;
};

typedef struct JT1Subsong JT1Subsong;
struct JT1Subsong {
    uint8_t  mute[SE_NROFCHANS];   // which channels are muted? (1=muted)
    int32_t  songspd;    // delay tussen de pattern-stepjes
    int32_t  groove;     // groove value... 0=nothing, 1 = swing, 2=shuffle
    int32_t  songpos;    // waar start song? (welke maat?)
    int32_t  songstep;   // welke patternpos offset? (1/64 van maat)
    int32_t  endpos;     // waar stopt song? (welke maat?)
    int32_t  endstep;    // welke patternpos offset? (1/64 van maat)
    int32_t  looppos;    // waar looped song? (welke maat?)
    int32_t  loopstep;   // welke patternpos offset? (1/64 van maat)
    int16_t  songloop;   // if true, the song loops inbetween looppos and endpos
    char     name[SE_NAMELEN];   // name of subsong
    int16_t  nrofchans;  //nr of channels used
    uint16_t delaytime; // the delaytime (for the echo effect)
    uint8_t  delayamount[SE_NROFCHANS]; // amount per channel for the echo-effect
    int16_t  amplification; //extra amplification factor (20 to 1000)
    JT1Order orders[SE_NROFCHANS][SE_ORDERS_SUBSONG];
};

typedef struct JT1Effect JT1Effect;
struct JT1Effect {
    int32_t dsteffect;
    int32_t srceffect1;
    int32_t srceffect2;
    int32_t osceffect;
    int32_t effectvar1;
    int32_t effectvar2;
    int32_t effectspd;
    int32_t oscspd;
    int32_t effecttype;
    int8_t  oscflg;
    int8_t  reseteffect;
};

// inst is the structure which has the entire instrument definition.
typedef struct JT1Inst JT1Inst;
struct JT1Inst {
    int16_t   mugiversion;
    char      instname[SE_NAMELEN];
    int16_t   waveform;
    int16_t   wavelength;
    int16_t   mastervol;
    int16_t   amwave;
    int16_t   amspd;
    int16_t   amlooppoint;
    int16_t   finetune;
    int16_t   fmwave;
    int16_t   fmspd;
    int16_t   fmlooppoint;
    int16_t   fmdelay;
    int16_t   arpeggio;
    int8_t    resetwave[SE_WAVES_INST];
    int16_t   panwave;  
    int16_t   panspd;
    int16_t   panlooppoint;
    JT1Effect fx[SE_EFF_INST];
    char      samplename[SE_NAMELEN];
    //ugly. Move samples into their own spot
    int16_t   sharing;    // sample sharing! sharing contains instr nr of shared sanpledata (0=no sharing)
    int16_t   loopflg;    //does the sample loop or play one/shot? (0=1shot)
    int16_t   bidirecflg; // does the sample loop birdirectional? (0=no)
    int32_t   startpoint;
    int32_t   looppoint;
    int32_t   endpoint;
    uint8_t   hasSampData;     // pointer naar de sample (mag 0 zijn)
    int32_t   samplelength;      // length of sample
    int16_t   waves[SE_WAVES_INST * SE_SAMPS_WAVE];
};

typedef struct JT1Song JT1Song;
struct JT1Song {
    int16_t      mugiversion;//version of mugician this song was saved with
    int32_t      nrofpats;   //aantal patterns beschikbaar
    int32_t      nrofsongs;  //aantal beschikbare subsongs
    int32_t      nrofinst;   //aantal gebruikte instruments
    
    JT1Subsong** subsongs;
    JT1Row*      patterns;
    char**       patNames;
    JT1Inst**    instruments;
    uint8_t**    samples;
    int8_t       arpTable[SE_ARPS_SONG * SE_STEPS_ARP];
    
};

//---------------------internal structs

// Chanfx is an internal structure which keeps track of the current effect parameters per active channel
typedef struct JT1VoiceEffect JT1VoiceEffect;
struct JT1VoiceEffect {
    int    fxcnt1;
    int    fxcnt2;
    int    osccnt;
    double a0;
    double b1;
    double b2;
    double y1;
    double y2;
    int    Vhp;
    int    Vbp;
    int    Vlp;
};

// chandat is an internal structure which keeps track of the current instruemnts current variables per active channel
typedef struct JT1Voice JT1Voice;
struct JT1Voice {
    int32_t  songpos;
    int32_t  patpos;
    int32_t  instrument;
    int32_t  volcnt;
    int32_t  pancnt;
    int32_t  arpcnt;
    int32_t  curnote;
    int32_t  curfreq;
    int32_t  curvol;
    int32_t  curpan;
    int32_t  bendadd;    // for the pitchbend
    int32_t  destfreq;   // ...
    int32_t  bendspd;    // ...
    int32_t  bendtonote;
    int32_t  freqcnt;
    int32_t  freqdel;
    uint8_t* sampledata;
    int32_t  looppoint;
    int32_t  endpoint;
    uint8_t  loopflg;
    uint8_t  bidirecflg;
    int32_t  synthPos;
    int32_t  samplepos;
    
    //immediate render vars
    int16_t*       wavePtr;
    int32_t        waveLength;
    int32_t        freqOffset;
    int16_t        gainMainL;
    int16_t        gainMainR;
    int16_t        gainEchoL;
    int16_t        gainEchoR;
    
    JT1VoiceEffect fx[SE_WAVES_INST];
    int16_t        waves[SE_WAVES_INST * SE_SAMPS_WAVE];
};

typedef struct Interpolator Interpolator;
struct Interpolator {
    uint8_t id;
    int16_t numTaps;
    int32_t (*fItp) (int16_t* buf, int32_t pos, int32_t sizeMask);
    char    name[32];
};

typedef struct JT1Player JT1Player;
struct JT1Player {
    JT1Song*      song;
    JT1Subsong*   subsong;
    JT1Voice      voices[SE_NROFCHANS];
    int32_t       subsongNr;
    int16_t       timeCnt;      // Samplecounter which stores the njumber of samples before the next songparams are calculated (is reinited with timeSpd)
    int16_t       timeSpd;      // Sample accurate counter which indicates every how many samples the song should progress 1 tick. Is dependant on rendering frequency and BPM
    uint8_t       playFlg;      // 0 if playback is stopped, 1 if song is being played
    uint8_t       pauseFlg;     // 0 if playback is not paused, 1 if playback is paused
    int32_t       playSpeed;    // Actual delay in between notes
    int32_t       patternDelay; // Current delay in between notes (resets with playSpeed)
    int32_t       playPosition; // Current position in song (coarse)
    int32_t       playStep;     // Current position in song (fine)
    int32_t       masterVolume; // Mastervolume of the replayer (256=max - 0=min)
    int16_t       leftDelayBuffer[65536];   // buffer to simulate an echo on the left stereo channel
    int16_t       rightDelayBuffer[65536];  // buffer to simulate an echo on the right stereo channel
    int16_t       overlapBuffer[WANTEDOVERLAP*2]; // Buffer which stores overlap between waveforms to avoid clicks
    int16_t       overlapCnt;   // Used to store how much overlap we have already rendered
    uint16_t      delayCnt;       // Internal counter used for delay
    int32_t       tempBuf[MIXBUF_LEN * MIXBUF_NR];
    Interpolator* itp;

    int32_t       playMode;           // in which mode is the replayer? Song or patternmode?
    int32_t       currentPattern;     // Which pattern are we currently playing (In pattern play mode)
    int32_t       patternLength;      // Current length of a pattern (in pattern play mode)
    int32_t       patternOffset;      // Current play offset in the pattern (used for display)

    int32_t       loopCnt;      // If song is meant to loop, the number of times the song has looped
};

//---------------------API

#ifdef __cplusplus
extern "C" {
#endif

int        jaytrax_loadSong(JT1Player* SELF, JT1Song* sng);
void       jaytrax_changeSubsong(JT1Player* SELF, int subsongnr);
void       jaytrax_stopSong(JT1Player* SELF);
void       jaytrax_pauseSong(JT1Player* SELF);
void       jaytrax_continueSong(JT1Player* SELF);
void       jaytrax_setInterpolation(JT1Player* SELF, uint8_t id);
JT1Player* jaytrax_init(void);
void       jaytrax_free(JT1Player* SELF);
void       jaytrax_renderChunk(JT1Player* SELF, int16_t* renderbuf, int32_t nrofsamples, int32_t frequency);
int32_t    jaytrax_getLength(JT1Player* SELF, int subsongnr, int loopCnt, int frequency);

#ifdef __cplusplus
}
#endif
#endif
