/*
 *  Copyright (C) 2002-2009  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef _MSC_VER
#define INLINE __forceinline
#define DB_FASTCALL __fastcall
#else
#define INLINE inline
#define DB_FASTCALL __attribute__((fastcall))
#endif

typedef         double		Real64;
/* The internal types */

#ifdef HAVE_STDINT_H
#include <stdint.h>

typedef uint8_t         	Bit8u;
typedef int8_t         		Bit8s;
typedef uint16_t        	Bit16u;
typedef int16_t     		Bit16s;
typedef uint32_t    		Bit32u;
typedef int32_t     		Bit32s;
typedef uint64_t        	Bit64u;
typedef int64_t         	Bit64s;
#else
typedef  unsigned char		Bit8u;
typedef    signed char		Bit8s;
typedef unsigned short		Bit16u;
typedef   signed short		Bit16s;
typedef  unsigned long		Bit32u;
typedef    signed long		Bit32s;
#ifdef _MSC_VER
typedef unsigned __int64	Bit64u;
typedef   signed __int64	Bit64s;
#else
typedef unsigned long long  Bit64u;
typedef   signed long long  Bit64s;
#endif
#endif

typedef unsigned int		Bitu;
typedef signed int			Bits;

//#include "dosbox.h"

//Use 8 handlers based on a small logatirmic wavetabe and an exponential table for volume
#define WAVE_HANDLER	10
//Use a logarithmic wavetable with an exponential table for volume
#define WAVE_TABLELOG	11
//Use a linear wavetable with a multiply table for volume
#define WAVE_TABLEMUL	12

//Select the type of wave generator routine
#define DBOPL_WAVE WAVE_TABLEMUL

#if (DBOPL_WAVE == WAVE_HANDLER)
typedef Bits ( DB_FASTCALL *WaveHandler) ( Bitu i, Bitu volume );
#endif

typedef Bits ( *Operator_VolumeHandler) ( struct Operator * );
typedef struct Channel* ( *Channel_SynthHandler) ( struct Channel *, struct Chip* chip, Bit32u samples, Bit32s* output );

//Different synth modes that can generate blocks of data
typedef enum {
	sm2AM,
	sm2FM,
	sm3AM,
	sm3FM,
	sm4Start,
	sm3FMFM,
	sm3AMFM,
	sm3FMAM,
	sm3AMAM,
	sm6Start,
	sm2Percussion,
	sm3Percussion
} SynthMode;

//Shifts for the values contained in chandata variable
enum {
	SHIFT_KSLBASE = 16,
	SHIFT_KEYCODE = 24
};

enum {
    MASK_KSR = 0x10,
    MASK_SUSTAIN = 0x20,
    MASK_VIBRATO = 0x40,
    MASK_TREMOLO = 0x80
};

typedef enum {
    OFF,
    RELEASE,
    SUSTAIN,
    DECAY,
    ATTACK
} Operator_State;

struct Operator {
	//Masks for operator 20 values
	Operator_VolumeHandler volHandler;

#if (DBOPL_WAVE == WAVE_HANDLER)
	WaveHandler waveHandler;	//Routine that generate a wave 
#else
	Bit16s* waveBase;
	Bit32u waveMask;
	Bit32u waveStart;
#endif
	Bit32u waveIndex;			//WAVE_BITS shifted counter of the frequency index
	Bit32u waveAdd;				//The base frequency without vibrato
	Bit32u waveCurrent;			//waveAdd + vibratao

	Bit32u chanData;			//Frequency/octave and derived data coming from whatever channel controls this
	Bit32u freqMul;				//Scale channel frequency with this, TODO maybe remove?
	Bit32u vibrato;				//Scaled up vibrato strength
	Bit32s sustainLevel;		//When stopping at sustain level stop here
	Bit32s totalLevel;			//totalLevel is added to every generated volume
	Bit32u currentLevel;		//totalLevel + tremolo
	Bit32s volume;				//The currently active volume
	
	Bit32u attackAdd;			//Timers for the different states of the envelope
	Bit32u decayAdd;
	Bit32u releaseAdd;
	Bit32u rateIndex;			//Current position of the evenlope

	Bit8u rateZero;				//Bits for the different states of the envelope having no changes
	Bit8u keyOn;				//Bitmask of different values that can generate keyon
	//Registers, also used to check for changes
	Bit8u reg20, reg40, reg60, reg80, regE0;
	//Active part of the envelope we're in
	Bit8u state;
	//0xff when tremolo is enabled
	Bit8u tremoloMask;
	//Strength of the vibrato
	Bit8u vibStrength;
	//Keep track of the calculated KSR so we can check for changes
	Bit8u ksr;
};

struct Channel {
	struct Operator op[2];
	Channel_SynthHandler synthHandler;
	Bit32u chanData;		//Frequency/octave and derived values
	Bit32s old[2];			//Old data for feedback

	Bit8u feedback;			//Feedback shift
	Bit8u regB0;			//Register values to check for changes
	Bit8u regC0;
	//This should correspond with reg104, bit 6 indicates a Percussion channel, bit 7 indicates a silent channel
	Bit8u fourMask;
	Bit8s maskLeft;		//Sign extended values for both channel's panning
	Bit8s maskRight;
};

struct Chip {
	//This is used as the base counter for vibrato and tremolo
	Bit32u lfoCounter;
	Bit32u lfoAdd;


	Bit32u noiseCounter;
	Bit32u noiseAdd;
	Bit32u noiseValue;

	//Frequency scales for the different multiplications
	Bit32u freqMul[16];
	//Rates for decay and release for rate of this chip
	Bit32u linearRates[76];
	//Best match attack rates for the rate of this chip
	Bit32u attackRates[76];

	//18 channels with 2 operators each
	struct Channel chan[18];

	Bit8u reg104;
	Bit8u reg08;
	Bit8u reg04;
	Bit8u regBD;
	Bit8u vibratoIndex;
	Bit8u tremoloIndex;
	Bit8s vibratoSign;
	Bit8u vibratoShift;
	Bit8u tremoloValue;
	Bit8u vibratoStrength;
	Bit8u tremoloStrength;
	//Mask for allowed wave forms
	Bit8u waveFormMask;
	//0 or -1 when enabled
	Bit8s opl3Active;
};

void Chip_Init(struct Chip *chip);
void Chip_Setup( struct Chip *chip, Bit32u clock, Bit32u rate );

void Chip_WriteReg( struct Chip *chip, Bit32u reg, Bit8u val );
Bit32u Chip_WriteAddr( struct Chip *chip, Bit32u port, Bit8u val );

void Chip_GenerateBlock2( struct Chip *chip, Bitu total, Bit32s* output );
void Chip_GenerateBlock3( struct Chip *chip, Bitu total, Bit32s* output );
