// Simple low-pass and high-pass filter to better match sound output of a SNES

// Game_Music_Emu https://bitbucket.org/mpyne/game-music-emu/
#ifndef SPC_FILTER_H
#define SPC_FILTER_H

#include "blargg_common.h"

struct SPC_Filter {
public:

	// Filters count samples of stereo sound in place. Count must be a multiple of 2.
	typedef short sample_t;
	void run( sample_t* io, int count );

// Optional features

	// Clears filter to silence
	void clear();

	// Sets gain (volume), where gain_unit is normal. Gains greater than gain_unit
	// are fine, since output is clamped to 16-bit sample range.
	static const unsigned int gain_unit = 0x100;
	void set_gain( int gain );

	// Enables/disables filtering (when disabled, gain is still applied)
	void enable( bool b );

	// Sets amount of bass (logarithmic scale)
	static const unsigned int bass_none =  0;
	static const unsigned int bass_norm =  8; // normal amount
	static const unsigned int bass_max  = 31;
	void set_bass( int bass );

public:
	SPC_Filter();
	BLARGG_DISABLE_NOTHROW
private:
	static const unsigned int gain_bits = 8;
	int gain;
	int bass;
	bool enabled;
	struct chan_t { int p1, pp1, sum; };
	chan_t ch [2];
};

inline void SPC_Filter::enable( bool b )  { enabled = b; }

inline void SPC_Filter::set_gain( int g ) { gain = g; }

inline void SPC_Filter::set_bass( int b ) { bass = b; }

#endif
