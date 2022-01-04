// Game_Music_Emu https://bitbucket.org/mpyne/game-music-emu/

// Super Nintendo SFM music file emulator

#ifndef SPC_SFM_H
#define SPC_SFM_H

#include "Fir_Resampler.h"
#include "Music_Emu.h"
#include "../higan/smp/smp.hpp"
#include "Spc_Filter.h"

#include "Bml_Parser.h"

class Sfm_Emu : public Music_Emu {
public:
	// Minimum allowed file size
	enum { sfm_min_file_size = 8 + 65536 + 128 };
	
	// The Super Nintendo hardware samples at 32kHz. Other sample rates are
	// handled by resampling the 32kHz output; emulation accuracy is not affected.
	enum { native_sample_rate = 32000 };
    	
	// This will serialize the current state of the emulator into a new SFM file
	blargg_err_t serialize( std::vector<uint8_t> & out );
	
	// Disables annoying pseudo-surround effect some music uses
	void disable_surround( bool disable = true );
	
	// Enables gaussian=0, cubic=1 or sinc=2 interpolation
	// Or sets worse quality, linear=-1, nearest=-2
	void interpolation_level( int level = 0 );
	
	// Enables native echo
	void enable_echo(bool enable = true);
	void mute_effects(bool mute);
	
	SuperFamicom::SMP const* get_smp() const;
	SuperFamicom::SMP * get_smp();
	
	static gme_type_t static_type()                 { return gme_sfm_type; }
	
public:
	Sfm_Emu();
	~Sfm_Emu();
	
protected:
	blargg_err_t load_mem_( byte const [], long );
	blargg_err_t track_info_( track_info_t*, int track ) const;
	blargg_err_t set_track_info_( const track_info_t*, int track );
	blargg_err_t set_sample_rate_( long );
	blargg_err_t start_track_( int );
	blargg_err_t play_( long, sample_t [] );
	blargg_err_t skip_( long );
	void mute_voices_( int );
	void set_tempo_( double );
	void enable_accuracy_( bool );
	byte const* file_data;
	long        file_size;

private:
	Fir_Resampler<24> resampler;
	SPC_Filter filter;
	SuperFamicom::SMP smp;
	
	Bml_Parser metadata;
	
	blargg_err_t play_and_filter( long count, sample_t out [] );
};

inline void Sfm_Emu::disable_surround( bool disable ) { smp.dsp.disable_surround( disable ); }
inline void Sfm_Emu::interpolation_level( int level ) { smp.dsp.spc_dsp.interpolation_level( level ); }
inline void Sfm_Emu::enable_echo(bool enable) { smp.dsp.spc_dsp.enable_echo(enable); }
inline void Sfm_Emu::mute_effects(bool mute) { enable_echo(!mute); }
inline SuperFamicom::SMP const* Sfm_Emu::get_smp() const { return &smp; }
inline SuperFamicom::SMP * Sfm_Emu::get_smp() { return &smp; }

#endif // SPC_SFM_H
