// Super Nintendo SPC music file emulator

// Game_Music_Emu https://bitbucket.org/mpyne/game-music-emu/
#ifndef SPC_EMU_H
#define SPC_EMU_H

#include "Fir_Resampler.h"
#include "Music_Emu.h"
#include "../higan/smp/smp.hpp"
#include "Spc_Filter.h"

class Spc_Emu : public Music_Emu {
public:
	// The Super Nintendo hardware samples at 32kHz. Other sample rates are
	// handled by resampling the 32kHz output; emulation accuracy is not affected.
	enum { native_sample_rate = 32000 };

	// SPC file header
	enum { header_size = 0x100 };
	struct header_t
	{
		char tag [35];
		byte format;
		byte version;
		byte pc [2];
		byte a, x, y, psw, sp;
		byte unused [2];
		char song [32];
		char game [32];
		char dumper [16];
		char comment [32];
		byte date [11];
		byte len_secs [3];
		byte fade_msec [4];
		char author [32]; // sometimes first char should be skipped (see official SPC spec)
		byte mute_mask;
		byte emulator;
		byte unused2 [46];
	};

	// Header for currently loaded file
	header_t const& header() const { return *(header_t const*) file_data; }

	// Prevents channels and global volumes from being phase-negated
	void disable_surround( bool disable = true );

	// Enables gaussian=0, cubic=1 or sinc=2 interpolation
	// Or negative levels for worse quality, linear=-1 or nearest=-2
	void interpolation_level( int level = 0 );

	// Enables native echo
	void enable_echo( bool enable = true );
	void mute_effects( bool mute );

	SuperFamicom::SMP const* get_smp() const;
	SuperFamicom::SMP * get_smp();

	static gme_type_t static_type()                 { return gme_spc_type; }

public:
	// deprecated
	using Music_Emu::load;
	blargg_err_t load( header_t const& h, Data_Reader& in ) // use Remaining_Reader
			{ return load_remaining_( &h, sizeof h, in ); }
	byte const* trailer() const; // use track_info()
	long trailer_size() const;

public:
	Spc_Emu();
	~Spc_Emu();
protected:
	blargg_err_t load_mem_( byte const*, long );
	blargg_err_t track_info_( track_info_t*, int track ) const;
	blargg_err_t set_sample_rate_( long );
	blargg_err_t start_track_( int );
	blargg_err_t play_( long, sample_t* );
	blargg_err_t skip_( long );
	void mute_voices_( int );
	void disable_echo_( bool disable );
	void set_tempo_( double );
	void enable_accuracy_( bool );
private:
	byte const* file_data;
	long        file_size;
	Fir_Resampler<24> resampler;
	SPC_Filter filter;
	SuperFamicom::SMP smp;

	blargg_err_t play_and_filter( long count, sample_t out [] );
};

inline void Spc_Emu::disable_surround( bool disable ) { smp.dsp.disable_surround( disable ); }
inline void Spc_Emu::interpolation_level( int level ) { smp.dsp.spc_dsp.interpolation_level( level ); }
inline void Spc_Emu::enable_echo( bool enable ) { smp.dsp.spc_dsp.enable_echo( enable ); }
inline void Spc_Emu::mute_effects( bool mute ) { enable_echo(!mute); }
inline SuperFamicom::SMP const* Spc_Emu::get_smp() const { return &smp; }
inline SuperFamicom::SMP * Spc_Emu::get_smp() { return &smp; }

#endif
