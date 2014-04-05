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

/*
	DOSBox implementation of a combined Yamaha YMF262 and Yamaha YM3812 emulator.
	Enabling the opl3 bit will switch the emulator to stereo opl3 output instead of regular mono opl2
	Except for the table generation it's all integer math
	Can choose different types of generators, using muls and bigger tables, try different ones for slower platforms
	The generation was based on the MAME implementation but tried to have it use less memory and be faster in general
	MAME uses much bigger envelope tables and this will be the biggest cause of it sounding different at times

	//TODO Don't delay first operator 1 sample in opl3 mode
	//TODO Maybe not use class method pointers but a regular function pointers with operator as first parameter
	//TODO Fix panning for the Percussion channels, would any opl3 player use it and actually really change it though?
	//TODO Check if having the same accuracy in all frequency multipliers sounds better or not

	//DUNNO Keyon in 4op, switch to 2op without keyoff.
*/

/* $Id$ */


#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "dbopl.h"

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

typedef struct Operator Operator;
typedef struct Channel Channel;
typedef struct Chip Chip;

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
	Bit8s mask;
	Bit8u chanActive;
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

#ifndef PI
#define PI 3.14159265358979323846
#endif

#define OPLRATE		((double)(14318180.0 / 288.0))
#define TREMOLO_TABLE 52

//Try to use most precision for frequencies
//Else try to keep different waves in synch
//#define WAVE_PRECISION        1
#ifndef WAVE_PRECISION
//Wave bits available in the top of the 32bit range
//Original adlib uses 10.10, we use 10.22
#define WAVE_BITS      10
#else
//Need some extra bits at the top to have room for octaves and frequency multiplier
//We support to 8 times lower rate
//128 * 15 * 8 = 15350, 2^13.9, so need 14 bits
#define WAVE_BITS      14
#endif
#define WAVE_SH		( 32 - WAVE_BITS )
#define WAVE_MASK	( ( 1 << WAVE_SH ) - 1 )

//Use the same accuracy as the waves
#define LFO_SH ( WAVE_SH - 10 )
//LFO is controlled by our tremolo 256 sample limit
#define LFO_MAX ( 256 << ( LFO_SH ) )

	
//Maximum amount of attenuation bits
//Envelope goes to 511, 9 bits
#if (DBOPL_WAVE == WAVE_TABLEMUL )
//Uses the value directly
#define ENV_BITS	( 9 )
#else
//Add 3 bits here for more accuracy and would have to be shifted up either way
#define ENV_BITS	( 9 )
#endif
//Limits of the envelope with those bits and when the envelope goes silent
#define ENV_MIN		0
#define ENV_EXTRA	( ENV_BITS - 9 )
#define ENV_MAX		( 511 << ENV_EXTRA )
#define ENV_LIMIT	( ( 12 * 256) >> ( 3 - ENV_EXTRA ) )
#define ENV_SILENT( _X_ ) ( (_X_) >= ENV_LIMIT )

//Attack/decay/release rate counter shift
#define RATE_SH		24
#define RATE_MASK	( ( 1 << RATE_SH ) - 1 )
//Has to fit within 16bit lookuptable
#define MUL_SH		16

//Check some ranges
#if ENV_EXTRA > 3
#error Too many envelope bits
#endif


//How much to substract from the base value for the final attenuation
static const Bit8u KslCreateTable[16] = {
	//0 will always be be lower than 7 * 8
	64, 32, 24, 19, 
	16, 12, 11, 10, 
	 8,  6,  5,  4,
	 3,  2,  1,  0,
};

#define M(_X_) ((Bit8u)( (_X_) * 2))
static const Bit8u FreqCreateTable[16] = {
	M(0.5), M(1 ), M(2 ), M(3 ), M(4 ), M(5 ), M(6 ), M(7 ),
	M(8  ), M(9 ), M(10), M(10), M(12), M(12), M(15), M(15)
};
#undef M

//We're not including the highest attack rate, that gets a special value
static const Bit8u AttackSamplesTable[13] = {
	69, 55, 46, 40,
	35, 29, 23, 20,
	19, 15, 11, 10,
	9
};
//On a real opl these values take 8 samples to reach and are based upon larger tables
static const Bit8u EnvelopeIncreaseTable[13] = {
	4,  5,  6,  7,
	8, 10, 12, 14,
	16, 20, 24, 28,
	32, 
};

#if ( DBOPL_WAVE == WAVE_HANDLER ) || ( DBOPL_WAVE == WAVE_TABLELOG )
static Bit16u ExpTable[ 256 ];
#endif

#if ( DBOPL_WAVE == WAVE_HANDLER )
//PI table used by WAVEHANDLER
static Bit16u SinTable[ 512 ];
#endif

#if ( DBOPL_WAVE > WAVE_HANDLER )
//Layout of the waveform table in 512 entry intervals
//With overlapping waves we reduce the table to half it's size

//	|    |//\\|____|WAV7|//__|/\  |____|/\/\|
//	|\\//|    |    |WAV7|    |  \/|    |    |
//	|06  |0126|17  |7   |3   |4   |4 5 |5   |

//6 is just 0 shifted and masked

static Bit16s WaveTable[ 8 * 512 ];
//Distance into WaveTable the wave starts
static const Bit16u WaveBaseTable[8] = {
	0x000, 0x200, 0x200, 0x800,
	0xa00, 0xc00, 0x100, 0x400,

};
//Mask the counter with this
static const Bit16u WaveMaskTable[8] = {
	1023, 1023, 511, 511,
	1023, 1023, 512, 1023,
};

//Where to start the counter on at keyon
static const Bit16u WaveStartTable[8] = {
	512, 0, 0, 0,
	0, 512, 512, 256,
};
#endif

#if ( DBOPL_WAVE == WAVE_TABLEMUL )
static Bit16u MulTable[ 384 ];
#endif

static Bit8u KslTable[ 8 * 16 ];
static Bit8u TremoloTable[ TREMOLO_TABLE ];
//Start of a channel behind the chip struct start
static Bit16u ChanOffsetTable[32];
//Start of an operator behind the chip struct start
static Bit16u OpOffsetTable[64];

//The lower bits are the shift of the operator vibrato value
//The highest bit is right shifted to generate -1 or 0 for negation
//So taking the highest input value of 7 this gives 3, 7, 3, 0, -3, -7, -3, 0
static const Bit8s VibratoTable[ 8 ] = {	
	1 - 0x00, 0 - 0x00, 1 - 0x00, 30 - 0x00, 
	1 - 0x80, 0 - 0x80, 1 - 0x80, 30 - 0x80 
};

//Shift strength for the ksl value determined by ksl strength
static const Bit8u KslShiftTable[4] = {
	31,1,2,0
};

//Generate a table index and table shift value using input value from a selected rate
static void EnvelopeSelect( Bit8u val, Bit8u* index, Bit8u* shift ) {
	if ( val < 13 * 4 ) {				//Rate 0 - 12
		*shift = 12 - ( val >> 2 );
		*index = val & 3;
	} else if ( val < 15 * 4 ) {		//rate 13 - 14
		*shift = 0;
		*index = val - 12 * 4;
	} else {							//rate 15 and up
		*shift = 0;
		*index = 12;
	}
}

#if ( DBOPL_WAVE == WAVE_HANDLER )
/*
	Generate the different waveforms out of the sine/exponetial table using handlers
*/
static inline Bits MakeVolume( Bitu wave, Bitu volume ) {
	Bitu total = wave + volume;
	Bitu index = total & 0xff;
	Bitu sig = ExpTable[ index ];
	Bitu exp = total >> 8;
#if 0
	//Check if we overflow the 31 shift limit
	if ( exp >= 32 ) {
		LOG_MSG( "WTF %d %d", total, exp );
	}
#endif
	return (sig >> exp);
};

static Bits DB_FASTCALL WaveForm0( Bitu i, Bitu volume ) {
	Bits neg = 0 - (( i >> 9) & 1);//Create ~0 or 0
	Bitu wave = SinTable[i & 511];
	return (MakeVolume( wave, volume ) ^ neg) - neg;
}
static Bits DB_FASTCALL WaveForm1( Bitu i, Bitu volume ) {
	Bit32u wave = SinTable[i & 511];
	wave |= ( ( (i ^ 512 ) & 512) - 1) >> ( 32 - 12 );
	return MakeVolume( wave, volume );
}
static Bits DB_FASTCALL WaveForm2( Bitu i, Bitu volume ) {
	Bitu wave = SinTable[i & 511];
	return MakeVolume( wave, volume );
}
static Bits DB_FASTCALL WaveForm3( Bitu i, Bitu volume ) {
	Bitu wave = SinTable[i & 255];
	wave |= ( ( (i ^ 256 ) & 256) - 1) >> ( 32 - 12 );
	return MakeVolume( wave, volume );
}
static Bits DB_FASTCALL WaveForm4( Bitu i, Bitu volume ) {
	//Twice as fast
	i <<= 1;
	Bits neg = 0 - (( i >> 9) & 1);//Create ~0 or 0
	Bitu wave = SinTable[i & 511];
	wave |= ( ( (i ^ 512 ) & 512) - 1) >> ( 32 - 12 );
	return (MakeVolume( wave, volume ) ^ neg) - neg;
}
static Bits DB_FASTCALL WaveForm5( Bitu i, Bitu volume ) {
	//Twice as fast
	i <<= 1;
	Bitu wave = SinTable[i & 511];
	wave |= ( ( (i ^ 512 ) & 512) - 1) >> ( 32 - 12 );
	return MakeVolume( wave, volume );
}
static Bits DB_FASTCALL WaveForm6( Bitu i, Bitu volume ) {
	Bits neg = 0 - (( i >> 9) & 1);//Create ~0 or 0
	return (MakeVolume( 0, volume ) ^ neg) - neg;
}
static Bits DB_FASTCALL WaveForm7( Bitu i, Bitu volume ) {
	//Negative is reversed here
	Bits neg = (( i >> 9) & 1) - 1;
	Bitu wave = (i << 3);
	//When negative the volume also runs backwards
	wave = ((wave ^ neg) - neg) & 4095;
	return (MakeVolume( wave, volume ) ^ neg) - neg;
}

static const WaveHandler WaveHandlerTable[8] = {
	WaveForm0, WaveForm1, WaveForm2, WaveForm3,
	WaveForm4, WaveForm5, WaveForm6, WaveForm7
};

#endif

/*
	Operator
*/

//We zero out when rate == 0
static INLINE void Operator_UpdateAttack( struct Operator *o, const struct Chip* chip ) {
	Bit8u rate = o->reg60 >> 4;
	if ( rate ) {
		Bit8u val = (rate << 2) + o->ksr;
		o->attackAdd = chip->attackRates[ val ];
		o->rateZero &= ~(1 << ATTACK);
	} else {
		o->attackAdd = 0;
		o->rateZero |= (1 << ATTACK);
	}
}

static INLINE void Operator_UpdateDecay( struct Operator *o, const struct Chip* chip ) {
	Bit8u rate = o->reg60 & 0xf;
	if ( rate ) {
		Bit8u val = (rate << 2) + o->ksr;
		o->decayAdd = chip->linearRates[ val ];
		o->rateZero &= ~(1 << DECAY);
	} else {
		o->decayAdd = 0;
		o->rateZero |= (1 << DECAY);
	}
}

static INLINE void Operator_UpdateRelease( struct Operator *o, const struct Chip* chip ) {
	Bit8u rate = o->reg80 & 0xf;
	if ( rate ) {
		Bit8u val = (rate << 2) + o->ksr;
		o->releaseAdd = chip->linearRates[ val ];
		o->rateZero &= ~(1 << RELEASE);
		if ( !(o->reg20 & MASK_SUSTAIN ) ) {
			o->rateZero &= ~( 1 << SUSTAIN );
		}	
	} else {
		o->rateZero |= (1 << RELEASE);
		o->releaseAdd = 0;
		if ( !(o->reg20 & MASK_SUSTAIN ) ) {
			o->rateZero |= ( 1 << SUSTAIN );
		}	
	}
}

static INLINE void Operator_UpdateAttenuation( struct Operator *o ) {
	Bit8u kslBase = (Bit8u)((o->chanData >> SHIFT_KSLBASE) & 0xff);
	Bit32u tl = o->reg40 & 0x3f;
	Bit8u kslShift = KslShiftTable[ o->reg40 >> 6 ];
	//Make sure the attenuation goes to the right bits
	o->totalLevel = tl << ( ENV_BITS - 7 );	//Total level goes 2 bits below max
	o->totalLevel += ( kslBase << ENV_EXTRA ) >> kslShift;
}

static void Operator_UpdateFrequency( struct Operator *o ) {
	Bit32u freq = o->chanData & (( 1 << 10 ) - 1);
	Bit32u block = (o->chanData >> 10) & 0xff;
	
#ifdef WAVE_PRECISION
	block = 7 - block;
	o->waveAdd = ( freq * o->freqMul ) >> block;
#else
	o->waveAdd = (freq << block) * o->freqMul;
#endif
	if ( o->reg20 & MASK_VIBRATO ) {
		o->vibStrength = (Bit8u)(freq >> 7);
#ifdef WAVE_PRECISION
		o->vibrato = ( o->vibStrength * o->freqMul ) >> block;
#else
		o->vibrato = ( o->vibStrength << block ) * o->freqMul;
#endif
	} else {
		o->vibStrength = 0;
		o->vibrato = 0;
	}
}

static void Operator_UpdateRates( struct Operator *o, const struct Chip* chip ) {
	//Mame seems to reverse this where enabling ksr actually lowers
	//the rate, but pdf manuals says otherwise?
	Bit8u newKsr = (Bit8u)((o->chanData >> SHIFT_KEYCODE) & 0xff);
	if ( !( o->reg20 & MASK_KSR ) ) {
		newKsr >>= 2;
	}
	if ( o->ksr == newKsr )
		return;
	o->ksr = newKsr;
	Operator_UpdateAttack( o, chip );
	Operator_UpdateDecay( o, chip );
	Operator_UpdateRelease( o, chip );
}

static INLINE Bit32s Operator_RateForward( struct Operator *o, Bit32u add ) {
	Bit32s ret;
	o->rateIndex += add;
	ret = o->rateIndex >> RATE_SH;
	o->rateIndex = o->rateIndex & RATE_MASK;
	return ret;
}

static INLINE void Operator_SetState( struct Operator *o, Bit8u s );

static Bits Operator_Volume_Attack( struct Operator *o ) {
	Bit32s vol = o->volume;
	Bit32s change;
    change = Operator_RateForward( o, o->attackAdd );
    if ( !change )
        return vol;
    vol += ( (~vol) * change ) >> 3;
    if ( vol < ENV_MIN ) {
        o->volume = ENV_MIN;
        o->rateIndex = 0;
        Operator_SetState( o, DECAY );
        return ENV_MIN;
    }
	o->volume = vol;
	return vol;
}

static Bits Operator_Volume_Decay( struct Operator *o ) {
	Bit32s vol = o->volume;
    vol += Operator_RateForward( o, o->decayAdd );
    if ( vol >= o->sustainLevel ) {
        //Check if we didn't overshoot max attenuation, then just go off
        if ( vol >= ENV_MAX ) {
            o->volume = ENV_MAX;
            Operator_SetState( o, OFF );
            return ENV_MAX;
        }
        //Continue as sustain
        o->rateIndex = 0;
        Operator_SetState( o, SUSTAIN );
    }
	o->volume = vol;
	return vol;
}

static Bits Operator_Volume_Sustain( struct Operator *o ) {
	Bit32s vol = o->volume;
    if ( o->reg20 & MASK_SUSTAIN ) {
        return vol;
    }
    //In sustain phase, but not sustaining, do regular release
    vol += Operator_RateForward( o, o->releaseAdd );;
    if ( vol >= ENV_MAX ) {
        o->volume = ENV_MAX;
        Operator_SetState( o, OFF );
        return ENV_MAX;
    }
	o->volume = vol;
	return vol;
}

static Bits Operator_Volume_Release( struct Operator *o ) {
	Bit32s vol = o->volume;
    vol += Operator_RateForward( o, o->releaseAdd );;
    if ( vol >= ENV_MAX ) {
        o->volume = ENV_MAX;
        Operator_SetState( o, OFF );
        return ENV_MAX;
    }
	o->volume = vol;
	return vol;
}

static Bits Operator_Volume_Off( struct Operator *o ) {
    (void)o;
	return ENV_MAX;
}

static const Operator_VolumeHandler VolumeHandlerTable[5] = {
	&Operator_Volume_Off,
	&Operator_Volume_Release,
	&Operator_Volume_Sustain,
	&Operator_Volume_Decay,
	&Operator_Volume_Attack
};

static INLINE void Operator_SetState( struct Operator *o, Bit8u s ) {
	o->state = s;
	o->volHandler = VolumeHandlerTable[ s ];
}

static INLINE Bitu Operator_ForwardVolume(struct Operator *o) {
	return o->currentLevel + (o->volHandler)(o);
}


static INLINE Bitu Operator_ForwardWave(struct Operator *o) {
	o->waveIndex += o->waveCurrent;
	return o->waveIndex >> WAVE_SH;
}

static void Operator_Write20( struct Operator *o, const struct Chip* chip, Bit8u val ) {
	Bit8u change = (o->reg20 ^ val );
	if ( !change ) 
		return;
	o->reg20 = val;
	//Shift the tremolo bit over the entire register, saved a branch, YES!
	o->tremoloMask = (Bit8s)(val) >> 7;
	o->tremoloMask &= ~(( 1 << ENV_EXTRA ) -1);
	//Update specific features based on changes
	if ( change & MASK_KSR ) {
		Operator_UpdateRates( o, chip );
	}
	//With sustain enable the volume doesn't change
	if ( o->reg20 & MASK_SUSTAIN || ( !o->releaseAdd ) ) {
		o->rateZero |= ( 1 << SUSTAIN );
	} else {
		o->rateZero &= ~( 1 << SUSTAIN );
	}
	//Frequency multiplier or vibrato changed
	if ( change & (0xf | MASK_VIBRATO) ) {
		o->freqMul = chip->freqMul[ val & 0xf ];
		Operator_UpdateFrequency(o);
	}
}

static void Operator_Write40( struct Operator *o, const struct Chip* chip, Bit8u val ) {
    (void)chip;
	if (!(o->reg40 ^ val ))
		return;
	o->reg40 = val;
	Operator_UpdateAttenuation( o );
}

static void Operator_Write60( struct Operator *o, const struct Chip* chip, Bit8u val ) {
	Bit8u change = o->reg60 ^ val;
	o->reg60 = val;
	if ( change & 0x0f ) {
		Operator_UpdateDecay( o, chip );
	}
	if ( change & 0xf0 ) {
		Operator_UpdateAttack( o, chip );
	}
}

static void Operator_Write80( struct Operator *o, const struct Chip* chip, Bit8u val ) {
	Bit8u change = (o->reg80 ^ val );
	Bit8u sustain;
	if ( !change ) 
		return;
	o->reg80 = val;
	sustain = val >> 4;
	//Turn 0xf into 0x1f
	sustain |= ( sustain + 1) & 0x10;
	o->sustainLevel = sustain << ( ENV_BITS - 5 );
	if ( change & 0x0f ) {
		Operator_UpdateRelease( o, chip );
	}
}

static void Operator_WriteE0( struct Operator *o, const struct Chip* chip, Bit8u val ) {
	Bit8u waveForm;
	if ( !(o->regE0 ^ val) )
		return;
	//in opl3 mode you can always selet 7 waveforms regardless of waveformselect
	waveForm = val & ( ( 0x3 & chip->waveFormMask ) | (0x7 & chip->opl3Active ) );
	o->regE0 = val;
#if ( DBOPL_WAVE == WAVE_HANDLER )
	o->waveHandler = WaveHandlerTable[ waveForm ];
#else
	o->waveBase = WaveTable + WaveBaseTable[ waveForm ];
	o->waveStart = WaveStartTable[ waveForm ] << WAVE_SH;
	o->waveMask = WaveMaskTable[ waveForm ];
#endif
}

static INLINE unsigned char Operator_Silent(struct Operator *o) {
	if ( !ENV_SILENT( o->totalLevel + o->volume ) )
		return 0;
	if ( !(o->rateZero & ( 1 << o->state ) ) )
		return 0;
	return 1;
}

static INLINE void Operator_Prepare( struct Operator *o, const struct Chip* chip )  {
	o->currentLevel = o->totalLevel + (chip->tremoloValue & o->tremoloMask);
	o->waveCurrent = o->waveAdd;
	if ( o->vibStrength >> chip->vibratoShift ) {
		Bit32s add = o->vibrato >> chip->vibratoShift;
		//Sign extend over the shift value
		Bit32s neg = chip->vibratoSign;
		//Negate the add with -1 or 0
		add = ( add ^ neg ) - neg;
		o->waveCurrent += add;
	}
}

static void Operator_KeyOn( struct Operator *o, Bit8u mask ) {
	if ( !o->keyOn ) {
		//Restart the frequency generator
#if ( DBOPL_WAVE > WAVE_HANDLER )
		o->waveIndex = o->waveStart;
#else
		o->waveIndex = 0;
#endif
		o->rateIndex = 0;
		Operator_SetState( o, ATTACK );
	}
	o->keyOn |= mask;
}

static void Operator_KeyOff( struct Operator *o, Bit8u mask ) {
	o->keyOn &= ~mask;
	if ( !o->keyOn ) {
		if ( o->state != OFF ) {
			Operator_SetState( o, RELEASE );
		}
	}
}

static INLINE Bits Operator_GetWave( struct Operator *o, Bitu index, Bitu vol ) {
#if ( DBOPL_WAVE == WAVE_HANDLER )
	return o->waveHandler( index, vol << ( 3 - ENV_EXTRA ) );
#elif ( DBOPL_WAVE == WAVE_TABLEMUL )
	return (o->waveBase[ index & o->waveMask ] * MulTable[ vol >> ENV_EXTRA ]) >> MUL_SH;
#elif ( DBOPL_WAVE == WAVE_TABLELOG )
	Bit32s wave = o->waveBase[ index & o->waveMask ];
	Bit32u total = ( wave & 0x7fff ) + vol << ( 3 - ENV_EXTRA );
	Bit32s sig = ExpTable[ total & 0xff ];
	Bit32u exp = total >> 8;
	Bit32s neg = wave >> 16;
	return ((sig ^ neg) - neg) >> exp;
#else
#error "No valid wave routine"
#endif
}

static Bits INLINE Operator_GetSample( struct Operator *o, Bits modulation ) {
	Bitu vol = Operator_ForwardVolume(o);
	if ( ENV_SILENT( vol ) ) {
		//Simply forward the wave
		o->waveIndex += o->waveCurrent;
		return 0;
	} else {
		Bitu index = Operator_ForwardWave(o);
		index += modulation;
		return Operator_GetWave( o, index, vol );
	}
}

static void Operator_Init(struct Operator *o) {
	o->chanData = 0;
	o->freqMul = 0;
	o->waveIndex = 0;
	o->waveAdd = 0;
	o->waveCurrent = 0;
	o->keyOn = 0;
	o->ksr = 0;
	o->reg20 = 0;
	o->reg40 = 0;
	o->reg60 = 0;
	o->reg80 = 0;
	o->regE0 = 0;
	Operator_SetState( o, OFF );
	o->rateZero = (1 << OFF);
	o->sustainLevel = ENV_MAX;
	o->currentLevel = ENV_MAX;
	o->totalLevel = ENV_MAX;
	o->volume = ENV_MAX;
}

/*
	Channel
*/

static INLINE struct Operator* Channel_Op( struct Channel *c, Bitu index ) {
    return &( ( c + (index >> 1) )->op[ index & 1 ]);
}

static struct Channel* Channel_Block_sm2AM( struct Channel *c, struct Chip* chip, Bit32u samples, Bit32s* output ) {
	Bitu i;

	if ( Operator_Silent( Channel_Op(c, 0) ) && Operator_Silent( Channel_Op(c, 1) ) ) {
        c->old[0] = c->old[1] = 0;
		c->chanActive = 0;
        return (c + 1);
    }
	c->chanActive = 1;

	//Init the operators with the the current vibrato and tremolo values
    Operator_Prepare( Channel_Op( c, 0 ), chip );
    Operator_Prepare( Channel_Op( c, 1 ), chip );

	for ( i = 0; i < samples; i++ ) {
		//Do unsigned shift so we can shift out all bits but still stay in 10 bit range otherwise
		Bit32s mod = (Bit32u)((c->old[0] + c->old[1])) >> c->feedback;
        Bit32s sample;
        Bit32s out0;
		c->old[0] = c->old[1];
		c->old[1] = Operator_GetSample( Channel_Op( c, 0 ), mod );
		out0 = c->old[0];
        sample = out0 + Operator_GetSample( Channel_Op( c, 1), 0 );
        output[ i ] += sample & c->mask;
	}

    return ( c + 1 );
}

static struct Channel* Channel_Block_sm2FM( struct Channel *c, struct Chip* chip, Bit32u samples, Bit32s* output ) {
	Bitu i;

    if ( Operator_Silent( Channel_Op(c, 1) ) ) {
        c->old[0] = c->old[1] = 0;
		c->chanActive = 0;
        return (c + 1);
    }
	c->chanActive = 1;
	
	//Init the operators with the the current vibrato and tremolo values
	Operator_Prepare( Channel_Op( c, 0 ), chip );
	Operator_Prepare( Channel_Op( c, 1 ), chip );

	for ( i = 0; i < samples; i++ ) {
		//Do unsigned shift so we can shift out all bits but still stay in 10 bit range otherwise
		Bit32s mod = (Bit32u)((c->old[0] + c->old[1])) >> c->feedback;
		Bit32s sample;
        Bit32s out0;
		c->old[0] = c->old[1];
		c->old[1] = Operator_GetSample( Channel_Op( c, 0 ), mod );
		out0 = c->old[0];
        sample = Operator_GetSample( Channel_Op( c, 1 ), out0 );
        output[ i ] += sample & c->mask;
	}

    return ( c + 1 );
}

static struct Channel* Channel_Block_sm3AM( struct Channel *c, struct Chip* chip, Bit32u samples, Bit32s* output ) {
	Bitu i;

	if ( Operator_Silent( Channel_Op( c, 0 ) ) && Operator_Silent( Channel_Op( c, 1 ) ) ) {
        c->old[0] = c->old[1] = 0;
		c->chanActive = 0;
        return (c + 1);
    }
	c->chanActive = 1;

	//Init the operators with the the current vibrato and tremolo values
    Operator_Prepare( Channel_Op( c, 0 ), chip );
    Operator_Prepare( Channel_Op( c, 1 ), chip );

	for ( i = 0; i < samples; i++ ) {
		//Do unsigned shift so we can shift out all bits but still stay in 10 bit range otherwise
		Bit32s mod = (Bit32u)((c->old[0] + c->old[1])) >> c->feedback;
		Bit32s sample;
        Bit32s out0;
		c->old[0] = c->old[1];
		c->old[1] = Operator_GetSample( Channel_Op( c, 0 ), mod );
		out0 = c->old[0];
        sample = out0 + Operator_GetSample( Channel_Op( c, 1 ), 0 );
        output[ i * 2 + 0 ] += sample & c->maskLeft & c->mask;
        output[ i * 2 + 1 ] += sample & c->maskRight & c->mask;
	}

    return ( c + 1 );
}

static struct Channel* Channel_Block_sm3FM( struct Channel *c, struct Chip* chip, Bit32u samples, Bit32s* output ) {
	Bitu i;

	if ( Operator_Silent( Channel_Op( c, 1 ) ) ) {
        c->old[0] = c->old[1] = 0;
		c->chanActive = 0;
        return (c + 1);
    }
	c->chanActive = 1;

	//Init the operators with the the current vibrato and tremolo values
    Operator_Prepare( Channel_Op( c, 0 ), chip );
    Operator_Prepare( Channel_Op( c, 1 ), chip );

	for ( i = 0; i < samples; i++ ) {
		//Do unsigned shift so we can shift out all bits but still stay in 10 bit range otherwise
		Bit32s mod = (Bit32u)((c->old[0] + c->old[1])) >> c->feedback;
		Bit32s sample;
        Bit32s out0;
		c->old[0] = c->old[1];
		c->old[1] = Operator_GetSample( Channel_Op( c, 0 ), mod );
		out0 = c->old[0];
        sample = Operator_GetSample( Channel_Op( c, 1 ), out0 );
        output[ i * 2 + 0 ] += sample & c->maskLeft & c->mask;
        output[ i * 2 + 1 ] += sample & c->maskRight & c->mask;
	}

    return ( c + 1 );
}

static struct Channel* Channel_Block_sm3FMFM( struct Channel *c, struct Chip* chip, Bit32u samples, Bit32s* output ) {
	Bitu i;

    if ( Operator_Silent( Channel_Op( c, 3 ) ) ) {
        c->old[0] = c->old[1] = 0;
		c->chanActive = 0;
        return (c + 2);
    }
	c->chanActive = 1;

	//Init the operators with the the current vibrato and tremolo values
    Operator_Prepare( Channel_Op( c, 0 ), chip );
    Operator_Prepare( Channel_Op( c, 1 ), chip );
    Operator_Prepare( Channel_Op( c, 2 ), chip );
    Operator_Prepare( Channel_Op( c, 3 ), chip );

	for ( i = 0; i < samples; i++ ) {
		//Do unsigned shift so we can shift out all bits but still stay in 10 bit range otherwise
		Bit32s mod = (Bit32u)((c->old[0] + c->old[1])) >> c->feedback;
		Bit32s sample;
        Bit32s out0;
        Bit32s next;
		c->old[0] = c->old[1];
		c->old[1] = Operator_GetSample( Channel_Op( c, 0 ), mod );
		out0 = c->old[0];
        next = Operator_GetSample( Channel_Op( c, 1 ), out0 );
        next = Operator_GetSample( Channel_Op( c, 2 ), next );
        sample = Operator_GetSample( Channel_Op( c, 3 ), next );
        output[ i * 2 + 0 ] += sample & c->maskLeft & c->mask;
        output[ i * 2 + 1 ] += sample & c->maskRight & c->mask;
	}

    return( c + 2 );
}

static struct Channel* Channel_Block_sm3AMFM( struct Channel *c, struct Chip* chip, Bit32u samples, Bit32s* output ) {
	Bitu i;

    if ( Operator_Silent( Channel_Op( c, 0 ) ) && Operator_Silent( Channel_Op( c, 3 ) ) ) {
        c->old[0] = c->old[1] = 0;
		c->chanActive = 0;
        return (c + 2);
    }
	c->chanActive = 1;
    
	//Init the operators with the the current vibrato and tremolo values
    Operator_Prepare( Channel_Op( c, 0 ), chip );
    Operator_Prepare( Channel_Op( c, 1 ), chip );
    Operator_Prepare( Channel_Op( c, 2 ), chip );
    Operator_Prepare( Channel_Op( c, 3 ), chip );
    
	for ( i = 0; i < samples; i++ ) {
		//Do unsigned shift so we can shift out all bits but still stay in 10 bit range otherwise
		Bit32s mod = (Bit32u)((c->old[0] + c->old[1])) >> c->feedback;
		Bit32s sample;
        Bit32s out0;
        Bit32s next;
		c->old[0] = c->old[1];
		c->old[1] = Operator_GetSample( Channel_Op( c, 0 ), mod );
		out0 = c->old[0];
        sample = out0;
        next = Operator_GetSample( Channel_Op( c, 1 ), 0 );
        next = Operator_GetSample( Channel_Op( c, 2 ), next );
        sample += Operator_GetSample( Channel_Op( c, 3 ), next );
        output[ i * 2 + 0 ] += sample & c->maskLeft & c->mask;
        output[ i * 2 + 1 ] += sample & c->maskRight & c->mask;
	}

    return( c + 2 );
}

static struct Channel* Channel_Block_sm3FMAM( struct Channel *c, struct Chip* chip, Bit32u samples, Bit32s* output ) {
	Bitu i;

    if ( Operator_Silent( Channel_Op( c, 1) ) && Operator_Silent( Channel_Op( c, 3 ) ) ) {
        c->old[0] = c->old[1] = 0;
		c->chanActive = 0;
        return (c + 2);
    }
	c->chanActive = 1;
    
	//Init the operators with the the current vibrato and tremolo values
    Operator_Prepare( Channel_Op( c, 0 ), chip );
    Operator_Prepare( Channel_Op( c, 1 ), chip );
    Operator_Prepare( Channel_Op( c, 2 ), chip );
    Operator_Prepare( Channel_Op( c, 3 ), chip );
    
	for ( i = 0; i < samples; i++ ) {
		//Do unsigned shift so we can shift out all bits but still stay in 10 bit range otherwise
		Bit32s mod = (Bit32u)((c->old[0] + c->old[1])) >> c->feedback;
		Bit32s sample;
        Bit32s out0;
        Bit32s next;
		c->old[0] = c->old[1];
		c->old[1] = Operator_GetSample( Channel_Op( c, 0 ), mod );
		out0 = c->old[0];
        sample = Operator_GetSample( Channel_Op( c, 1 ), out0 );
        next = Operator_GetSample( Channel_Op( c, 2 ), 0 );
        sample += Operator_GetSample( Channel_Op( c, 3 ), next );
        output[ i * 2 + 0 ] += sample & c->maskLeft & c->mask;
        output[ i * 2 + 1 ] += sample & c->maskRight & c->mask;
	}

    return( c + 2 );
}

static struct Channel* Channel_Block_sm3AMAM( struct Channel *c, struct Chip* chip, Bit32u samples, Bit32s* output ) {
	Bitu i;

    if ( Operator_Silent( Channel_Op( c, 0 ) ) && Operator_Silent( Channel_Op( c, 2 ) )  && Operator_Silent( Channel_Op( c, 3 ) ) ) {
        c->old[0] = c->old[1] = 0;
		c->chanActive = 0;
        return (c + 2);
    }
	c->chanActive = 1;
    
	//Init the operators with the the current vibrato and tremolo values
    Operator_Prepare( Channel_Op( c, 0 ), chip );
    Operator_Prepare( Channel_Op( c, 1 ), chip );
    Operator_Prepare( Channel_Op( c, 2 ), chip );
    Operator_Prepare( Channel_Op( c, 3 ), chip );
    
	for ( i = 0; i < samples; i++ ) {
		//Do unsigned shift so we can shift out all bits but still stay in 10 bit range otherwise
		Bit32s mod = (Bit32u)((c->old[0] + c->old[1])) >> c->feedback;
		Bit32s sample;
        Bit32s out0;
		Bits next;
		c->old[0] = c->old[1];
		c->old[1] = Operator_GetSample( Channel_Op( c, 0 ), mod );
		out0 = c->old[0];
        sample = out0;
        next = Operator_GetSample( Channel_Op( c, 1 ), 0 );
        sample += Operator_GetSample( Channel_Op( c, 2 ), next );
        sample += Operator_GetSample( Channel_Op( c, 3 ), 0 );
        output[ i * 2 + 0 ] += sample & c->maskLeft & c->mask;
        output[ i * 2 + 1 ] += sample & c->maskRight & c->mask;
	}

    return( c + 2 );
}

static INLINE Bit32u Chip_ForwardNoise(struct Chip *chip);

static Bit32s Channel_GeneratePercussion( struct Channel *chan, struct Chip* chip ) {
	//BassDrum
	Bit32s mod = (Bit32u)((chan->old[0] + chan->old[1])) >> chan->feedback;
    Bit32s sample;
    Bit32u noiseBit;
    Bit32u c2, c5;
    Bit32u phaseBit;
    Bit32u hhVol;
    Bit32u sdVol;
    Bit32u tcVol;

	chan->chanActive = 0;

	if ( !Operator_Silent( Channel_Op(chan, 1) ) )
		chan->chanActive++;
	if ( !Operator_Silent( Channel_Op(chan, 4) ) )
		chan->chanActive++;
    
	chan->old[0] = chan->old[1];
	chan->old[1] = Operator_GetSample( Channel_Op( chan, 0 ), mod );
    
	//When bassdrum is in AM mode first operator is ignored
	if ( chan->regC0 & 1 ) {
		mod = 0;
	} else {
		mod = chan->old[0];
	}
	sample = Operator_GetSample( Channel_Op( chan, 1 ), mod );
    
    
	//Precalculate stuff used by other outputs
	noiseBit = Chip_ForwardNoise( chip ) & 0x1;
	c2 = Operator_ForwardWave( Channel_Op( chan, 2 ) );
	c5 = Operator_ForwardWave( Channel_Op( chan, 5 ) );
	phaseBit = (((c2 & 0x88) ^ ((c2<<5) & 0x80)) | ((c5 ^ (c5<<2)) & 0x20)) ? 0x02 : 0x00;
    
	//Hi-Hat
	hhVol = Operator_ForwardVolume( Channel_Op( chan, 2 ) );
	if ( !ENV_SILENT( hhVol ) ) {
		Bit32u hhIndex = (phaseBit<<8) | (0x34 << ( phaseBit ^ (noiseBit << 1 )));
		sample += Operator_GetWave( Channel_Op( chan, 2 ), hhIndex, hhVol );
		chan->chanActive++;
	}
	//Snare Drum
	sdVol = Operator_ForwardVolume( Channel_Op( chan, 3 ) );
	if ( !ENV_SILENT( sdVol ) ) {
		Bit32u sdIndex = ( 0x100 + (c2 & 0x100) ) ^ ( noiseBit << 8 );
		sample += Operator_GetWave( Channel_Op( chan, 3 ), sdIndex, sdVol );
		chan->chanActive++;
	}
	//Tom-tom
	sample += Operator_GetSample( Channel_Op( chan, 4 ), 0 );
    
	//Top-Cymbal
	tcVol = Operator_ForwardVolume( Channel_Op( chan, 5 ) );
	if ( !ENV_SILENT( tcVol ) ) {
		Bit32u tcIndex = (1 + phaseBit) << 8;
		sample += Operator_GetWave( Channel_Op( chan, 5 ), tcIndex, tcVol );
		chan->chanActive++;
	}
	sample <<= 1;
    return sample;
}

static struct Channel* Channel_Block_sm2Percussion( struct Channel *c, struct Chip* chip, Bit32u samples, Bit32s* output ) {
	Bitu i;

	//Init the operators with the the current vibrato and tremolo values
    Operator_Prepare( Channel_Op( c, 0 ), chip );
    Operator_Prepare( Channel_Op( c, 1 ), chip );
    Operator_Prepare( Channel_Op( c, 2 ), chip );
    Operator_Prepare( Channel_Op( c, 3 ), chip );
    Operator_Prepare( Channel_Op( c, 4 ), chip );
    Operator_Prepare( Channel_Op( c, 5 ), chip );

	for ( i = 0; i < samples; i++ ) {
        output[i] += Channel_GeneratePercussion( c, chip ) & c->mask;
	}

    return( c + 3 );
}

static struct Channel* Channel_Block_sm3Percussion( struct Channel *c, struct Chip* chip, Bit32u samples, Bit32s* output ) {
	Bitu i;

	//Init the operators with the the current vibrato and tremolo values
    Operator_Prepare( Channel_Op( c, 0 ), chip );
    Operator_Prepare( Channel_Op( c, 1 ), chip );
    Operator_Prepare( Channel_Op( c, 2 ), chip );
    Operator_Prepare( Channel_Op( c, 3 ), chip );
    Operator_Prepare( Channel_Op( c, 4 ), chip );
    Operator_Prepare( Channel_Op( c, 5 ), chip );
    
	for ( i = 0; i < samples; i++ ) {
		Bit32s sample = Channel_GeneratePercussion( c, chip ) & c->mask;
        output[i * 2    ] += sample;
		output[i * 2 + 1] += sample;
	}
    
    return( c + 3 );
}

static void Channel_Init(struct Channel *c) {
	c->old[0] = c->old[1] = 0;
	c->chanData = 0;
	c->regB0 = 0;
	c->regC0 = 0;
	c->maskLeft = -1;
	c->maskRight = -1;
	c->mask = -1;
	c->feedback = 31;
	c->fourMask = 0;
	c->chanActive = 0;
	c->synthHandler = &Channel_Block_sm2FM;
    Operator_Init(&c->op[0]);
    Operator_Init(&c->op[1]);
}

static void Channel_SetChanData( struct Channel *c, const struct Chip* chip, Bit32u data ) {
	Bit32u change = c->chanData ^ data;
	c->chanData = data;
	Channel_Op( c, 0 )->chanData = data;
	Channel_Op( c, 1 )->chanData = data;
	//Since a frequency update triggered this, always update frequency
    Operator_UpdateFrequency( Channel_Op( c, 0 ) );
    Operator_UpdateFrequency( Channel_Op( c, 1 ) );
	if ( change & ( 0xff << SHIFT_KSLBASE ) ) {
        Operator_UpdateAttenuation( Channel_Op( c, 0 ) );
        Operator_UpdateAttenuation( Channel_Op( c, 1 ) );
	}
	if ( change & ( 0xff << SHIFT_KEYCODE ) ) {
        Operator_UpdateRates( Channel_Op( c, 0 ), chip );
        Operator_UpdateRates( Channel_Op( c, 1 ), chip );
	}
}

static void Channel_UpdateFrequency( struct Channel *c, const struct Chip* chip, Bit8u fourOp ) {
	//Extrace the frequency bits
	Bit32u data = c->chanData & 0xffff;
	Bit32u kslBase = KslTable[ data >> 6 ];
	Bit32u keyCode = ( data & 0x1c00) >> 9;
	if ( chip->reg08 & 0x40 ) {
		keyCode |= ( data & 0x100)>>8;	/* notesel == 1 */
	} else {
		keyCode |= ( data & 0x200)>>9;	/* notesel == 0 */
	}
	//Add the keycode and ksl into the highest bits of chanData
	data |= (keyCode << SHIFT_KEYCODE) | ( kslBase << SHIFT_KSLBASE );
    Channel_SetChanData( c + 0, chip, data );
	if ( fourOp & 0x3f ) {
        Channel_SetChanData( c + 1, chip, data );
	}
}

static void Channel_WriteA0( struct Channel *c, const struct Chip* chip, Bit8u val ) {
	Bit8u fourOp = chip->reg104 & chip->opl3Active & c->fourMask;
	//Don't handle writes to silent fourop channels
    Bit32u change;
	if ( fourOp > 0x80 )
		return;
	change = (c->chanData ^ val ) & 0xff;
	if ( change ) {
		c->chanData ^= change;
		Channel_UpdateFrequency( c, chip, fourOp );
	}
}

static void Channel_WriteB0( struct Channel *c, const struct Chip* chip, Bit8u val ) {
	Bit8u fourOp = chip->reg104 & chip->opl3Active & c->fourMask;
	Bitu change;
	//Don't handle writes to silent fourop channels
	if ( fourOp > 0x80 )
		return;
	change = ( c->chanData ^ ( val << 8 ) ) & 0x1f00;
	if ( change ) {
		c->chanData ^= change;
		Channel_UpdateFrequency( c, chip, fourOp );
	}
	//Check for a change in the keyon/off state
	if ( !((val ^ c->regB0) & 0x20))
		return;
	c->regB0 = val;
	if ( val & 0x20 ) {
        Operator_KeyOn( Channel_Op( c, 0 ), 0x1 );
        Operator_KeyOn( Channel_Op( c, 1 ), 0x1 );
		if ( fourOp & 0x3f ) {
            Operator_KeyOn( Channel_Op( c, 2 ), 0x1 );
            Operator_KeyOn( Channel_Op( c, 3 ), 0x1 );
		}
	} else {
        Operator_KeyOff( Channel_Op( c, 0 ), 0x1 );
        Operator_KeyOff( Channel_Op( c, 1 ), 0x1 );
		if ( fourOp & 0x3f ) {
            Operator_KeyOff( Channel_Op( c, 2 ), 0x1 );
            Operator_KeyOff( Channel_Op( c, 3 ), 0x1 );
		}
	}
}

static void Channel_WriteC0( struct Channel *c, const struct Chip* chip, Bit8u val ) {
	Bit8u change = val ^ c->regC0;
	if ( !change )
		return;
	c->regC0 = val;
	c->feedback = ( val >> 1 ) & 7;
	if ( c->feedback ) {
		//We shift the input to the right 10 bit wave index value
		c->feedback = 9 - c->feedback;
	} else {
		c->feedback = 31;
	}
	//Select the new synth mode
	if ( chip->opl3Active ) {
		//4-op mode enabled for this channel
		if ( (chip->reg104 & c->fourMask) & 0x3f ) {
			struct Channel* chan0, *chan1;
			Bit8u synth;
			//Check if it's the 2nd channel in a 4-op
			if ( !(c->fourMask & 0x80 ) ) {
				chan0 = c;
				chan1 = c + 1;
			} else {
				chan0 = c - 1;
				chan1 = c;
			}

			synth = ( (chan0->regC0 & 1) << 0 )| (( chan1->regC0 & 1) << 1 );
			switch ( synth ) {
			case 0:
				chan0->synthHandler = &Channel_Block_sm3FMFM;
				break;
			case 1:
				chan0->synthHandler = &Channel_Block_sm3AMFM;
				break;
			case 2:
				chan0->synthHandler = &Channel_Block_sm3FMAM;
				break;
			case 3:
				chan0->synthHandler = &Channel_Block_sm3AMAM;
				break;
			}
		//Disable updating percussion channels
		} else if ((c->fourMask & 0x40) && ( chip->regBD & 0x20) ) {

		//Regular dual op, am or fm
		} else if ( val & 1 ) {
			c->synthHandler = &Channel_Block_sm3AM;
		} else {
			c->synthHandler = &Channel_Block_sm3FM;
		}
		c->maskLeft = ( val & 0x10 ) ? -1 : 0;
		c->maskRight = ( val & 0x20 ) ? -1 : 0;
	//opl2 active
	} else { 
		//Disable updating percussion channels
		if ( (c->fourMask & 0x40) && ( chip->regBD & 0x20 ) ) {

		//Regular dual op, am or fm
		} else if ( val & 1 ) {
			c->synthHandler = &Channel_Block_sm2AM;
		} else {
			c->synthHandler = &Channel_Block_sm2FM;
		}
	}
}

static void Channel_ResetC0( struct Channel *c, const struct Chip* chip ) {
	Bit8u val = c->regC0;
	c->regC0 ^= 0xff;
	Channel_WriteC0( c, chip, val );
}

/*
	Chip
*/

Bitu Chip_GetSize()
{
    return sizeof(struct Chip);
}

static void InitTables( void );

void Chip_Init(void *_chip) {
    Bit32u i;
    struct Chip *chip = (struct Chip *)_chip;
    InitTables();
	chip->reg08 = 0;
	chip->reg04 = 0;
	chip->regBD = 0;
	chip->reg104 = 0;
	chip->opl3Active = 0;
    for (i = 0; i < 18; ++i)
        Channel_Init( &chip->chan[i] );
}

static INLINE Bit32u Chip_ForwardNoise(struct Chip *chip) {
	Bitu count;
	chip->noiseCounter += chip->noiseAdd;
	count = chip->noiseCounter >> LFO_SH;
	chip->noiseCounter &= WAVE_MASK;
	for ( ; count > 0; --count ) {
		//Noise calculation from mame
		chip->noiseValue ^= ( 0x800302 ) & ( 0 - (chip->noiseValue & 1 ) );
		chip->noiseValue >>= 1;
	}
	return chip->noiseValue;
}

static Bit32u Chip_ForwardLFO( struct Chip *chip, Bit32u samples ) {
    Bit32u todo, count;
    
	//Current vibrato value, runs 4x slower than tremolo
	chip->vibratoSign = ( VibratoTable[ chip->vibratoIndex >> 2] ) >> 7;
	chip->vibratoShift = ( VibratoTable[ chip->vibratoIndex >> 2] & 7) + chip->vibratoStrength;
	chip->tremoloValue = TremoloTable[ chip->tremoloIndex ] >> chip->tremoloStrength;

	//Check hom many samples there can be done before the value changes
	todo = LFO_MAX - chip->lfoCounter;
	count = (todo + chip->lfoAdd - 1) / chip->lfoAdd;
	if ( count > samples ) {
		count = samples;
		chip->lfoCounter += count * chip->lfoAdd;
	} else {
		chip->lfoCounter += count * chip->lfoAdd;
		chip->lfoCounter &= (LFO_MAX - 1);
		//Maximum of 7 vibrato value * 4
		chip->vibratoIndex = ( chip->vibratoIndex + 1 ) & 31;
		//Clip tremolo to the the table size
		if ( chip->tremoloIndex + 1 < TREMOLO_TABLE  )
			++chip->tremoloIndex;
		else
			chip->tremoloIndex = 0;
	}
	return count;
}

static void Chip_WriteBD( struct Chip *chip, Bit8u val ) {
	Bit8u change = chip->regBD ^ val;
	if ( !change )
		return;
	chip->regBD = val;
	//TODO could do this with shift and xor?
	chip->vibratoStrength = (val & 0x40) ? 0x00 : 0x01;
	chip->tremoloStrength = (val & 0x80) ? 0x00 : 0x02;
	if ( val & 0x20 ) {
		//Drum was just enabled, make sure channel 6 has the right synth
		if ( change & 0x20 ) {
			if ( chip->opl3Active ) {
				chip->chan[6].synthHandler = &Channel_Block_sm3Percussion;
			} else {
				chip->chan[6].synthHandler = &Channel_Block_sm2Percussion;
			}
		}
		//Bass Drum
		if ( val & 0x10 ) {
			Operator_KeyOn( &chip->chan[6].op[0], 0x2 );
            Operator_KeyOn( &chip->chan[6].op[1], 0x2 );
		} else {
            Operator_KeyOff( &chip->chan[6].op[0], 0x2 );
            Operator_KeyOff( &chip->chan[6].op[1], 0x2 );
		}
		//Hi-Hat
		if ( val & 0x1 ) {
            Operator_KeyOn( &chip->chan[7].op[0], 0x2 );
		} else {
            Operator_KeyOff( &chip->chan[7].op[0], 0x2 );
		}
		//Snare
		if ( val & 0x8 ) {
            Operator_KeyOn( &chip->chan[7].op[1], 0x2 );
		} else {
            Operator_KeyOff( &chip->chan[7].op[1], 0x2 );
		}
		//Tom-Tom
		if ( val & 0x4 ) {
            Operator_KeyOn( &chip->chan[8].op[0], 0x2 );
		} else {
            Operator_KeyOff( &chip->chan[8].op[0], 0x2 );
		}
		//Top Cymbal
		if ( val & 0x2 ) {
            Operator_KeyOn( &chip->chan[8].op[1], 0x2 );
		} else {
            Operator_KeyOff( &chip->chan[8].op[1], 0x2 );
		}
	//Toggle keyoffs when we turn off the percussion
	} else if ( change & 0x20 ) {
		//Trigger a reset to setup the original synth handler
        Channel_ResetC0( &chip->chan[6], chip );
        Operator_KeyOff( &chip->chan[6].op[0], 0x2 );
        Operator_KeyOff( &chip->chan[6].op[1], 0x2 );
        Operator_KeyOff( &chip->chan[7].op[0], 0x2 );
        Operator_KeyOff( &chip->chan[7].op[1], 0x2 );
        Operator_KeyOff( &chip->chan[8].op[0], 0x2 );
        Operator_KeyOff( &chip->chan[8].op[1], 0x2 );
	}
}


#define REGOP( _FUNC_ )															\
	index = ( ( reg >> 3) & 0x20 ) | ( reg & 0x1f );								\
	if ( OpOffsetTable[ index ] ) {													\
		struct Operator* regOp = (struct Operator*)( ((char *)chip ) + OpOffsetTable[ index ] );	\
		Operator_##_FUNC_( regOp, chip, val );										\
	}

#define REGCHAN( _FUNC_ )																\
	index = ( ( reg >> 4) & 0x10 ) | ( reg & 0xf );										\
	if ( ChanOffsetTable[ index ] ) {													\
		struct Channel* regChan = (struct Channel*)( ((char *)chip ) + ChanOffsetTable[ index ] );	\
		Channel_##_FUNC_( regChan, chip, val );											\
	}

void Chip_WriteReg( void *_chip, Bit32u reg, Bit8u val ) {
	Bitu index;
    struct Chip *chip = (struct Chip *)_chip;
	switch ( (reg & 0xf0) >> 4 ) {
	case 0x00 >> 4:
		if ( reg == 0x01 ) {
			chip->waveFormMask = ( val & 0x20 ) ? 0x7 : 0x0;
		} else if ( reg == 0x104 ) {
			//Only detect changes in lowest 6 bits
			if ( !((chip->reg104 ^ val) & 0x3f) )
				return;
			//Always keep the highest bit enabled, for checking > 0x80
			chip->reg104 = 0x80 | ( val & 0x3f );
		} else if ( reg == 0x105 ) {
			int i;
			//MAME says the real opl3 doesn't reset anything on opl3 disable/enable till the next write in another register
			if ( !((chip->opl3Active ^ val) & 1 ) )
				return;
			chip->opl3Active = ( val & 1 ) ? 0xff : 0;
			//Update the 0xc0 register for all channels to signal the switch to mono/stereo handlers
			for ( i = 0; i < 18; i++ ) {
                Channel_ResetC0( &chip->chan[i], chip );
			}
		} else if ( reg == 0x08 ) {
			chip->reg08 = val;
		}
	case 0x10 >> 4:
		break;
	case 0x20 >> 4:
	case 0x30 >> 4:
		REGOP( Write20 );
		break;
	case 0x40 >> 4:
	case 0x50 >> 4:
		REGOP( Write40 );
		break;
	case 0x60 >> 4:
	case 0x70 >> 4:
		REGOP( Write60 );
		break;
	case 0x80 >> 4:
	case 0x90 >> 4:
		REGOP( Write80 );
		break;
	case 0xa0 >> 4:
		REGCHAN( WriteA0 );
		break;
	case 0xb0 >> 4:
		if ( reg == 0xbd ) {
			Chip_WriteBD( chip, val );
		} else {
			REGCHAN( WriteB0 );
		}
		break;
	case 0xc0 >> 4:
		REGCHAN( WriteC0 );
	case 0xd0 >> 4:
		break;
	case 0xe0 >> 4:
	case 0xf0 >> 4:
		REGOP( WriteE0 );
		break;
	}
}

Bit32u Chip_WriteAddr( void *_chip, Bit32u port, Bit8u val ) {
    struct Chip *chip = (struct Chip *)_chip;
	switch ( port & 3 ) {
	case 0:
		return val;
	case 2:
		if ( chip->opl3Active || (val == 0x05) )
			return 0x100 | val;
		else 
			return val;
	}
	return 0;
}

static void Chip_GenerateBlock2( struct Chip *chip, Bitu total, Bit32s* output ) {
	while ( total > 0 ) {
        struct Channel* ch;
        int count;
		Bit32u samples = Chip_ForwardLFO( chip, total );
		Bitu i;
		for ( i = 0; i < samples; i++ ) {
			output[i] = 0;
		}
		count = 0;
		for( ch = chip->chan; ch < chip->chan + 9; ) {
			count++;
			ch = (ch->synthHandler)( ch, chip, samples, output );
		}
		total -= samples;
		output += samples;
	}
}

static void Chip_GenerateBlock3( struct Chip *chip, Bitu total, Bit32s* output  ) {
	while ( total > 0 ) {
        struct Channel* ch;
        int count;
		Bit32u samples = Chip_ForwardLFO( chip, total );
		Bitu i;
		for ( i = 0; i < samples; i++ ) {
			output[i * 2 + 0 ] = 0;
			output[i * 2 + 1 ] = 0;
		}
		count = 0;
		for( ch = chip->chan; ch < chip->chan + 18; ) {
			count++;
			ch = (ch->synthHandler)( ch, chip, samples, output );
		}
		total -= samples;
		output += samples * 2;
	}
}

void Chip_GenerateBlock_Mono( void *_chip, Bitu total, Bit32s* output ) {
    struct Chip *chip = (struct Chip *)_chip;
    if (chip->opl3Active) {
        while ( total > 0 ) {
            Bit32s temp[512];
            Bitu todo = ( total > 256 ) ? 256 : total, i;
            Chip_GenerateBlock3( chip, todo, temp );
            total -= todo;
            todo *= 2;
            for ( i = 0; i < todo; i += 2 ) {
                *output++ = (temp[i] + temp[i + 1]) >> 1;
            }
        }
    } else
        Chip_GenerateBlock2( chip, total, output );
}

void Chip_GenerateBlock_Stereo( void *_chip, Bitu total, Bit32s* output) {
    struct Chip *chip = (struct Chip *)_chip;
    if (!chip->opl3Active) {
        Chip_GenerateBlock2( chip, total, output );
        while ( total-- ) {
            output[total * 2 + 1] = output[total * 2] = output[total];
        }
    } else
        Chip_GenerateBlock3( chip, total, output );
}

Bitu Chip_GetActiveChannels( void *_chip ) {
	struct Chip *chip = (struct Chip *)_chip;
	int totalChannels = (chip->opl3Active) ? 18 : 9;
	int i;
	Bitu active = 0;
	for ( i = 0; i < totalChannels; ++i )
		active += chip->chan[i].chanActive;
	return active;
}

void Chip_Mute( void *_chip, Bit8u channel, Bit8u mute ) {
	struct Chip *chip = (struct Chip *)_chip;
	if (channel >= 18)
		return;
	chip->chan[channel].mask = mute ? 0 : -1;
}

void Chip_Setup( void *_chip, Bit32u clock, Bit32u rate ) {
    struct Chip *chip = (struct Chip *)_chip;
	double original = (double)clock / 288.0;
	double scale = original / (double)rate;
#ifdef WAVE_PRECISION
	double freqScale;
#else
	Bit32u freqScale;
#endif
	int i;
	Bit8u j;

	if (fabs(scale - 1.0) < 0.00001)
		scale = 1.0;

	//Noise counter is run at the same precision as general waves
	chip->noiseAdd = (Bit32u)( 0.5 + scale * ( 1 << LFO_SH ) );
	chip->noiseCounter = 0;
	chip->noiseValue = 1; //Make sure it triggers the noise xor the first time
	//The low frequency oscillation counter
	//Every time his overflows vibrato and tremoloindex are increased
	chip->lfoAdd = (Bit32u)( 0.5 + scale * ( 1 << LFO_SH ) );
	chip->lfoCounter = 0;
	chip->vibratoIndex = 0;
	chip->tremoloIndex = 0;

	//With higher octave this gets shifted up
	//-1 since the freqCreateTable = *2
#ifdef WAVE_PRECISION
	freqScale = ( 1 << 7 ) * scale * ( 1 << ( WAVE_SH - 1 - 10));
	for ( i = 0; i < 16; i++ ) {
		//Use rounding with 0.5
		chip->freqMul[i] = (Bit32u)( 0.5 + freqScale * FreqCreateTable[ i ] );
	}
#else
	freqScale = (Bit32u)( 0.5 + scale * ( 1 << ( WAVE_SH - 1 - 10)));
	for ( i = 0; i < 16; i++ ) {
		chip->freqMul[i] = freqScale * FreqCreateTable[ i ];
	}
#endif

	//-3 since the real envelope takes 8 steps to reach the single value we supply
	for ( j = 0; j < 76; j++ ) {
		Bit8u index, shift;
		EnvelopeSelect( j, &index, &shift );
		chip->linearRates[j] = (Bit32u)( scale * (EnvelopeIncreaseTable[ index ] << ( RATE_SH + ENV_EXTRA - shift - 3 )));
	}
	//Generate the best matching attack rate
	for ( j = 0; j < 62; j++ ) {
		Bit8u index, shift;
		Bit32s original, guessAdd, bestAdd;
		Bit32u bestDiff, passes;
		EnvelopeSelect( j, &index, &shift );
		//Original amount of samples the attack would take
		original = (Bit32u)( (AttackSamplesTable[ index ] << shift) / scale);
		 
		guessAdd = (Bit32u)( scale * (EnvelopeIncreaseTable[ index ] << ( RATE_SH - shift - 3 )));
		bestAdd = guessAdd;
		bestDiff = 1 << 30;
		for( passes = 0; passes < 16; passes ++ ) {
			Bit32s volume = ENV_MAX;
			Bit32s samples = 0;
			Bit32u count = 0;
			Bit32s diff;
			Bit32u lDiff;
			while ( volume > 0 && samples < original * 2 ) {
				Bit32s change;
				count += guessAdd;
				change = count >> RATE_SH;
				count &= RATE_MASK;
				if ( change ) { 
					volume += ( ~volume * change ) >> 3;
				}
				samples++;

			}
			diff = original - samples;
			lDiff = labs( diff );
			//Init last on first pass
			if ( lDiff < bestDiff ) {
				bestDiff = lDiff;
				bestAdd = guessAdd;
				if ( !bestDiff )
					break;
			}
			//Below our target
			if ( diff < 0 ) {
				//Better than the last time
				Bit32s mul = ((original - diff) << 12) / original;
				guessAdd = ((guessAdd * mul) >> 12);
				guessAdd++;
			} else if ( diff > 0 ) {
				Bit32s mul = ((original - diff) << 12) / original;
				guessAdd = (guessAdd * mul) >> 12;
				guessAdd--;
			}
		}
		chip->attackRates[j] = bestAdd;
	}
	for ( j = 62; j < 76; j++ ) {
		//This should provide instant volume maximizing
		chip->attackRates[j] = 8 << RATE_SH;
	}
	//Setup the channels with the correct four op flags
	//Channels are accessed through a table so they appear linear here
	chip->chan[ 0].fourMask = 0x00 | ( 1 << 0 );
	chip->chan[ 1].fourMask = 0x80 | ( 1 << 0 );
	chip->chan[ 2].fourMask = 0x00 | ( 1 << 1 );
	chip->chan[ 3].fourMask = 0x80 | ( 1 << 1 );
	chip->chan[ 4].fourMask = 0x00 | ( 1 << 2 );
	chip->chan[ 5].fourMask = 0x80 | ( 1 << 2 );

	chip->chan[ 9].fourMask = 0x00 | ( 1 << 3 );
	chip->chan[10].fourMask = 0x80 | ( 1 << 3 );
	chip->chan[11].fourMask = 0x00 | ( 1 << 4 );
	chip->chan[12].fourMask = 0x80 | ( 1 << 4 );
	chip->chan[13].fourMask = 0x00 | ( 1 << 5 );
	chip->chan[14].fourMask = 0x80 | ( 1 << 5 );

	//mark the percussion channels
	chip->chan[ 6].fourMask = 0x40;
	chip->chan[ 7].fourMask = 0x40;
	chip->chan[ 8].fourMask = 0x40;

	//Clear Everything in opl3 mode
	Chip_WriteReg( chip, 0x105, 0x1 );
	for ( i = 0; i < 512; i++ ) {
		if ( i == 0x105 )
			continue;
		Chip_WriteReg( chip, i, 0xff );
		Chip_WriteReg( chip, i, 0x0 );
	}
	Chip_WriteReg( chip, 0x105, 0x0 );
	//Clear everything in opl2 mode
	for ( i = 0; i < 256; i++ ) {
		Chip_WriteReg( chip, i, 0xff );
		Chip_WriteReg( chip, i, 0x0 );
	}
}

static unsigned char doneTables = 0;
static void InitTables( void ) {
	int i, oct;
	Bit8u j;
	Bitu k;
	if ( doneTables )
		return;
#if ( DBOPL_WAVE == WAVE_HANDLER ) || ( DBOPL_WAVE == WAVE_TABLELOG )
	//Exponential volume table, same as the real adlib
	for ( int i = 0; i < 256; i++ ) {
		//Save them in reverse
		ExpTable[i] = (int)( 0.5 + ( pow(2.0, ( 255 - i) * ( 1.0 /256 ) )-1) * 1024 );
		ExpTable[i] += 1024; //or remove the -1 oh well :)
		//Preshift to the left once so the final volume can shift to the right
		ExpTable[i] *= 2;
	}
#endif
#if ( DBOPL_WAVE == WAVE_HANDLER )
	//Add 0.5 for the trunc rounding of the integer cast
	//Do a PI sinetable instead of the original 0.5 PI
	for ( int i = 0; i < 512; i++ ) {
		SinTable[i] = (Bit16s)( 0.5 - log10( sin( (i + 0.5) * (PI / 512.0) ) ) / log10(2.0)*256 );
	}
#endif
#if ( DBOPL_WAVE == WAVE_TABLEMUL )
	//Multiplication based tables
	for ( i = 0; i < 384; i++ ) {
		int s = i * 8;
		//TODO maybe keep some of the precision errors of the original table?
		double val = ( 0.5 + ( pow(2.0, -1.0 + ( 255 - s) * ( 1.0 /256 ) )) * ( 1 << MUL_SH ));
		MulTable[i] = (Bit16u)(val);
	}

	//Sine Wave Base
	for ( i = 0; i < 512; i++ ) {
		WaveTable[ 0x0200 + i ] = (Bit16s)(sin( (i + 0.5) * (PI / 512.0) ) * 4084);
		WaveTable[ 0x0000 + i ] = -WaveTable[ 0x200 + i ];
	}
	//Exponential wave
	for ( i = 0; i < 256; i++ ) {
		WaveTable[ 0x700 + i ] = (Bit16s)( 0.5 + ( pow(2.0, -1.0 + ( 255 - i * 8) * ( 1.0 /256 ) ) ) * 4085 );
		WaveTable[ 0x6ff - i ] = -WaveTable[ 0x700 + i ];
	}
#endif
#if ( DBOPL_WAVE == WAVE_TABLELOG )
	//Sine Wave Base
	for ( i = 0; i < 512; i++ ) {
		WaveTable[ 0x0200 + i ] = (Bit16s)( 0.5 - log10( sin( (i + 0.5) * (PI / 512.0) ) ) / log10(2.0)*256 );
		WaveTable[ 0x0000 + i ] = ((Bit16s)0x8000) | WaveTable[ 0x200 + i];
	}
	//Exponential wave
	for ( i = 0; i < 256; i++ ) {
		WaveTable[ 0x700 + i ] = i * 8;
		WaveTable[ 0x6ff - i ] = ((Bit16s)0x8000) | i * 8;
	} 
#endif

	//	|    |//\\|____|WAV7|//__|/\  |____|/\/\|
	//	|\\//|    |    |WAV7|    |  \/|    |    |
	//	|06  |0126|27  |7   |3   |4   |4 5 |5   |

#if (( DBOPL_WAVE == WAVE_TABLELOG ) || ( DBOPL_WAVE == WAVE_TABLEMUL ))
	for ( i = 0; i < 256; i++ ) {
		//Fill silence gaps
		WaveTable[ 0x400 + i ] = WaveTable[0];
		WaveTable[ 0x500 + i ] = WaveTable[0];
		WaveTable[ 0x900 + i ] = WaveTable[0];
		WaveTable[ 0xc00 + i ] = WaveTable[0];
		WaveTable[ 0xd00 + i ] = WaveTable[0];
		//Replicate sines in other pieces
		WaveTable[ 0x800 + i ] = WaveTable[ 0x200 + i ];
		//double speed sines
		WaveTable[ 0xa00 + i ] = WaveTable[ 0x200 + i * 2 ];
		WaveTable[ 0xb00 + i ] = WaveTable[ 0x000 + i * 2 ];
		WaveTable[ 0xe00 + i ] = WaveTable[ 0x200 + i * 2 ];
		WaveTable[ 0xf00 + i ] = WaveTable[ 0x200 + i * 2 ];
	} 
#endif

	//Create the ksl table
	for ( oct = 0; oct < 8; oct++ ) {
		int base = oct * 8;
		for ( i = 0; i < 16; i++ ) {
			int val = base - KslCreateTable[i];
			if ( val < 0 )
				val = 0;
			//*4 for the final range to match attenuation range
			KslTable[ oct * 16 + i ] = (Bit8u)(val * 4);
		}
	}
	//Create the Tremolo table, just increase and decrease a triangle wave
	for ( j = 0; j < TREMOLO_TABLE / 2; j++ ) {
		Bit8u val = j << ENV_EXTRA;
		TremoloTable[j] = val;
		TremoloTable[TREMOLO_TABLE - 1 - j] = val;
	}
	//Create a table with offsets of the channels from the start of the chip
    for ( k = 0; k < 32; k++ ) {
		Bitu index = k & 0xf;
        Bitu blah;
		struct Chip *chip = 0;
		if ( index >= 9 ) {
			ChanOffsetTable[k] = 0;
			continue;
		}
		//Make sure the four op channels follow eachother
		if ( index < 6 ) {
			index = (index % 3) * 2 + ( index / 3 );
		}
		//Add back the bits for highest ones
		if ( k >= 16 )
			index += 9;
        blah = (Bitu)( (unsigned long)( &(chip->chan[ index ]) ) );
		ChanOffsetTable[k] = (Bit16u)blah;
	}
	//Same for operators
	for ( k = 0; k < 64; k++ ) {
        Bitu chNum;
        Bitu opNum;
        Bitu blah;
        struct Channel* chan = 0;
		if ( k % 8 >= 6 || ( (k / 8) % 4 == 3 ) ) {
			OpOffsetTable[k] = 0;
			continue;
		}
		chNum = (k / 8) * 3 + (k % 8) % 3;
		//Make sure we use 16 and up for the 2nd range to match the chanoffset gap
		if ( chNum >= 12 )
			chNum += 16 - 12;
		opNum = ( k % 8 ) / 3;
        blah = (Bitu)( (unsigned long) ( &(chan->op[opNum]) ) );
		OpOffsetTable[k] = (Bit16u)(ChanOffsetTable[ chNum ] + blah);
	}
#if 0
	//Stupid checks if table's are correct
	for ( k = 0; k < 18; k++ ) {
        Bit32u find = (Bit16u)( &(chip->chan[ k ]) );
		Bitu c;
		for ( c = 0; c < 32; c++ ) {
			if ( ChanOffsetTable[c] == find ) {
				find = 0;
				break;
			}
		}
		if ( find ) {
			find = find;
		}
	}
	for ( k = 0; k < 36; k++ ) {
        Bit32u find = (Bit16u)( &(chip->chan[ k / 2 ].op[k % 2]) );
		Bitu c;
		for ( c = 0; c < 64; c++ ) {
			if ( OpOffsetTable[c] == find ) {
				find = 0;
				break;
			}
		}
		if ( find ) {
			find = find;
		}
	}
#endif
    doneTables = 1;
}

/*Bit32u Handler::WriteAddr( Bit32u port, Bit8u val ) {
	return chip.WriteAddr( port, val );

}
void Handler::WriteReg( Bit32u addr, Bit8u val ) {
	chip.WriteReg( addr, val );
}

void Handler::Generate( MixerChannel* chan, Bitu samples ) {
	Bit32s buffer[ 512 * 2 ];
	if ( samples > 512 )
		samples = 512;
	if ( !chip.opl3Active ) {
		chip.GenerateBlock2( samples, buffer );
		chan->AddSamples_m32( samples, buffer );
	} else {
		chip.GenerateBlock3( samples, buffer );
		chan->AddSamples_s32( samples, buffer );
	}
}

void Handler::Init( Bitu rate ) {
	InitTables();
	chip.Setup( rate );
}*/
