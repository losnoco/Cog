#ifndef SOUND_H
#define SOUND_H

// Sound emulation setup/options and GBA sound emulation

#ifdef EMU_COMPILE
#include "../common/Types.h"
#else
#include <HighlyAdvanced/Types.h>
#endif

struct GBASystem;

struct GBASoundOut;

// Initializes sound and returns true if successful. Sets sound quality to
// current value in soundQuality global.
bool soundInit(GBASystem *, GBASoundOut *);

// sets the Sound throttle
void soundSetThrottle(GBASystem *, unsigned short throttle);

// Manages sound volume, where 1.0 is normal
void soundSetVolume( GBASystem *, float );
float soundGetVolume(GBASystem *);

// Manages muting bitmask. The bits control the following channels:
// 0x001 Pulse 1
// 0x002 Pulse 2
// 0x004 Wave
// 0x008 Noise
// 0x100 PCM 1
// 0x200 PCM 2
void soundSetEnable( GBASystem *, int mask );
int  soundGetEnable( GBASystem * );

// Pauses/resumes system sound output
void soundPause( GBASystem * );
void soundResume( GBASystem * );

// Cleans up sound. Afterwards, soundInit() can be called again.
void soundShutdown( GBASystem * );

//// GBA sound options

long soundGetSampleRate( GBASystem * );
void soundSetSampleRate( GBASystem *, long sampleRate );



//// GBA sound emulation

// GBA sound registers
#define SGCNT0_H 0x82
#define FIFOA_L 0xa0
#define FIFOA_H 0xa2
#define FIFOB_L 0xa4
#define FIFOB_H 0xa6

// Resets emulated sound hardware
void soundReset(GBASystem *);

// Emulates write to sound hardware
void soundEvent( GBASystem *, u32 addr, u8  data );
void soundEvent( GBASystem *, u32 addr, u16 data ); // TODO: error-prone to overload like this

// Notifies emulator that a timer has overflowed
void soundTimerOverflow( GBASystem *, int which );

// Notifies emulator that SOUND_CLOCK_TICKS clocks have passed
void psoundTickfn(GBASystem *);

namespace GBA { class Multi_Buffer; }

void flush_samples(GBASystem *, GBA::Multi_Buffer * buffer);

#endif // SOUND_H
