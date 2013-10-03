#ifndef GBA_H
#define GBA_H

#ifdef EMU_COMPILE
#include "../common/Types.h"

#include "Sound.h"

#include "../apu/Gb_Apu.h"
#include "../apu/Multi_Buffer.h"
#else
#include <HighlyAdvanced/Types.h>

#include <HighlyAdvanced/Sound.h>

#include <HighlyAdvanced/Gb_Apu.h>
#include <HighlyAdvanced/Multi_Buffer.h>
#endif

#define SAVE_GAME_VERSION_1 1
#define SAVE_GAME_VERSION_2 2
#define SAVE_GAME_VERSION_3 3
#define SAVE_GAME_VERSION_4 4
#define SAVE_GAME_VERSION_5 5
#define SAVE_GAME_VERSION_6 6
#define SAVE_GAME_VERSION_7 7
#define SAVE_GAME_VERSION_8 8
#define SAVE_GAME_VERSION_9 9
#define SAVE_GAME_VERSION_10 10
#define SAVE_GAME_VERSION  SAVE_GAME_VERSION_10

typedef struct {
  u8 *address;
  u32 mask;
} memoryMap;

typedef union {
  struct {
#ifdef WORDS_BIGENDIAN
    u8 B3;
    u8 B2;
    u8 B1;
    u8 B0;
#else
    u8 B0;
    u8 B1;
    u8 B2;
    u8 B3;
#endif
  } B;
  struct {
#ifdef WORDS_BIGENDIAN
    u16 W1;
    u16 W0;
#else
    u16 W0;
    u16 W1;
#endif
  } W;
#ifdef WORDS_BIGENDIAN
  volatile u32 I;
#else
	u32 I;
#endif
} reg_pair;

struct GBASystem;

class Gba_Pcm {
public:
    void init(GBASystem *);
    void apply_control( int idx );
    void update( int dac );
    void end_frame( GBA::blip_time_t );

private:
    GBASystem* gba;
    GBA::Blip_Buffer* output;
    GBA::blip_time_t last_time;
    int last_amp;
    int shift;
};

class Gba_Pcm_Fifo {
public:
    int     which;
    Gba_Pcm pcm;

    void init(GBASystem *);
    void write_control( int data );
    void write_fifo( int data );
    void timer_overflowed( int which_timer );

    // public only so save state routines can access it
    GBASystem* gba;
    int  readIndex;
    int  count;
    int  writeIndex;
    u8   fifo [32];
    int  dac;
private:

    int  timer;
    bool enabled;
};

// Callback class, passed the audio data from the emulator
struct GBASoundOut
{
    virtual ~GBASoundOut() { }
    // Receives signed 16-bit stereo audio and a byte count
    virtual void write(const void * samples, unsigned long bytes) = 0;
};

struct GBASystem
{
    reg_pair reg[45];
    memoryMap map[256];
    bool ioReadable[0x400];

    bool N_FLAG;
    bool C_FLAG;
    bool Z_FLAG;
    bool V_FLAG;
    bool armState;
    bool armIrqEnable;
    u32 armNextPC;
    int armMode;
    u32 stop;
    int saveType;
    bool useBios;
    bool skipBios;
    int frameSkip;
    bool speedup;
    bool synchronize;
    bool cpuDisableSfx;
    bool cpuIsMultiBoot;
    bool parseDebug;
    int layerSettings;
    int layerEnable;
    bool speedHack;
    int cpuSaveType;
    bool cheatsEnabled;
    bool mirroringEnable;
    bool skipSaveGameBattery;
    bool skipSaveGameCheats;

    u8 *bios;
    u8 *rom;
    u8 *internalRAM;
    u8 *workRAM;
    u8 *paletteRAM;
    u8 *vram;
    u8 *oam;
    u8 *ioMem;

    int customBackdropColor;

    u16 DISPCNT;
    u16 DISPSTAT;
    u16 VCOUNT;
    u16 BG0CNT;
    u16 BG1CNT;
    u16 BG2CNT;
    u16 BG3CNT;
    u16 BG0HOFS;
    u16 BG0VOFS;
    u16 BG1HOFS;
    u16 BG1VOFS;
    u16 BG2HOFS;
    u16 BG2VOFS;
    u16 BG3HOFS;
    u16 BG3VOFS;
    u16 BG2PA;
    u16 BG2PB;
    u16 BG2PC;
    u16 BG2PD;
    u16 BG2X_L;
    u16 BG2X_H;
    u16 BG2Y_L;
    u16 BG2Y_H;
    u16 BG3PA;
    u16 BG3PB;
    u16 BG3PC;
    u16 BG3PD;
    u16 BG3X_L;
    u16 BG3X_H;
    u16 BG3Y_L;
    u16 BG3Y_H;
    u16 WIN0H;
    u16 WIN1H;
    u16 WIN0V;
    u16 WIN1V;
    u16 WININ;
    u16 WINOUT;
    u16 MOSAIC;
    u16 BLDMOD;
    u16 COLEV;
    u16 COLY;
    u16 DM0SAD_L;
    u16 DM0SAD_H;
    u16 DM0DAD_L;
    u16 DM0DAD_H;
    u16 DM0CNT_L;
    u16 DM0CNT_H;
    u16 DM1SAD_L;
    u16 DM1SAD_H;
    u16 DM1DAD_L;
    u16 DM1DAD_H;
    u16 DM1CNT_L;
    u16 DM1CNT_H;
    u16 DM2SAD_L;
    u16 DM2SAD_H;
    u16 DM2DAD_L;
    u16 DM2DAD_H;
    u16 DM2CNT_L;
    u16 DM2CNT_H;
    u16 DM3SAD_L;
    u16 DM3SAD_H;
    u16 DM3DAD_L;
    u16 DM3DAD_H;
    u16 DM3CNT_L;
    u16 DM3CNT_H;
    u16 TM0D;
    u16 TM0CNT;
    u16 TM1D;
    u16 TM1CNT;
    u16 TM2D;
    u16 TM2CNT;
    u16 TM3D;
    u16 TM3CNT;
    u16 P1;
    u16 IE;
    u16 IF;
    u16 IME;

    int gfxBG2Changed;
    int gfxBG3Changed;
    int eepromInUse;

    int emulating;

    int SWITicks;
    int IRQTicks;

    u32 mastercode;
    int layerEnableDelay;
    bool busPrefetch;
    bool busPrefetchEnable;
    u32 busPrefetchCount;
    int cpuDmaTicksToUpdate;
    int cpuDmaCount;
    bool cpuDmaHack;
    u32 cpuDmaLast;
    int dummyAddress;

    bool cpuBreakLoop;
    int cpuNextEvent;

    int clockTicks;

    int gbaSaveType;
    bool intState;
    bool stopState;
    bool holdState;
    int holdType;
    bool cpuSramEnabled;
    bool cpuFlashEnabled;
    bool cpuEEPROMEnabled;
    bool cpuEEPROMSensorEnabled;

    u32 cpuPrefetch[2];

    int cpuTotalTicks;

    int lcdTicks;
    u8 timerOnOffDelay;
    u16 timer0Value;
    bool timer0On;
    int timer0Ticks;
    int timer0Reload;
    int timer0ClockReload;
    u16 timer1Value;
    bool timer1On;
    int timer1Ticks;
    int timer1Reload;
    int timer1ClockReload;
    u16 timer2Value;
    bool timer2On;
    int timer2Ticks;
    int timer2Reload;
    int timer2ClockReload;
    u16 timer3Value;
    bool timer3On;
    int timer3Ticks;
    int timer3Reload;
    int timer3ClockReload;
    u32 dma0Source;
    u32 dma0Dest;
    u32 dma1Source;
    u32 dma1Dest;
    u32 dma2Source;
    u32 dma2Dest;
    u32 dma3Source;
    u32 dma3Dest;

    bool fxOn;
    bool windowOn;
    int frameCount;

    char buffer[1024];

    u32 lastTime;
    int count;

    int capture;
    int capturePrevious;
    int captureNumber;

    u8 memoryWait[16];
    u8 memoryWait32[16];
    u8 memoryWaitSeq[16];
    u8 memoryWaitSeq32[16];

    u8 biosProtected[4];

    #ifdef WORDS_BIGENDIAN
    bool cpuBiosSwapped;
    #endif

    int romSize;

    u8 cpuBitsSet[256];
    u8 cpuLowestBitSet[256];

    // Sound settings
    bool soundDeclicking;
    bool soundInterpolation; // 1 if PCM should have low-pass filtering
    float soundFiltering;    // 0.0 = none, 1.0 = max
    long  soundSampleRate;

    u16   soundFinalWave [1600];
    bool  soundPaused;

    enum { SOUND_CLOCK_TICKS_ = 167772 }; // 1/100 second

    int   SOUND_CLOCK_TICKS;
    int   soundTicks;

    float soundVolume;
    int soundEnableFlag;
    float soundFiltering_;
    float soundVolume_;

    GBASoundOut    * output;

    Gba_Pcm_Fifo        pcm [2];
    GBA::Gb_Apu*        gb_apu;
    GBA::Stereo_Buffer* stereo_buffer;

    GBA::Blip_Synth<GBA::blip_best_quality,1> pcm_synth [3]; // 32 kHz, 16 kHz, 8 kHz

    GBASystem();
};

extern void CPUCleanUp(GBASystem *);
extern void CPUUpdateRender(GBASystem *);
extern void CPUUpdateRenderBuffers(GBASystem *, bool);
extern int CPULoadRom(GBASystem *, const void *, u32);
extern void doMirroring(GBASystem *, bool);
extern void CPUUpdateRegister(GBASystem *, u32, u16);
extern void applyTimer (GBASystem *);
extern void CPUInit(GBASystem *);
extern void CPUReset(GBASystem *);
extern void CPULoop(GBASystem *, int);
extern void CPUCheckDMA(GBASystem *, int,int);

#define R13_IRQ  18
#define R14_IRQ  19
#define SPSR_IRQ 20
#define R13_USR  26
#define R14_USR  27
#define R13_SVC  28
#define R14_SVC  29
#define SPSR_SVC 30
#define R13_ABT  31
#define R14_ABT  32
#define SPSR_ABT 33
#define R13_UND  34
#define R14_UND  35
#define SPSR_UND 36
#define R8_FIQ   37
#define R9_FIQ   38
#define R10_FIQ  39
#define R11_FIQ  40
#define R12_FIQ  41
#define R13_FIQ  42
#define R14_FIQ  43
#define SPSR_FIQ 44

#ifdef EMU_COMPILE
#include "Globals.h"
#else
#include <HighlyAdvanced/Globals.h>
#endif

#endif // GBA_H
