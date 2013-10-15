#ifndef __BMPlayer_h__
#define __BMPlayer_h__

#include "MIDIPlayer.h"

#include "../../../ThirdParty/BASS/bassmidi.h"

class BMPlayer : public MIDIPlayer
{
public:
	// zero variables
	BMPlayer();

	// close, unload
	virtual ~BMPlayer();

	// configuration
	void setSoundFont( const char * in );
	void setFileSoundFont( const char * in );
	void setSincInterpolation(bool enable = true);

private:
	virtual void send_event(uint32_t b);
	virtual void render(float * out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	void reset_drum_channels();

    std::vector<HSOUNDFONT> _soundFonts;
    std::string        sSoundFontName;
    std::string        sFileSoundFontName;

	HSTREAM            _stream;

	bool               bSincInterpolation;

	enum
	{
	                   mode_gm = 0,
					   mode_gm2,
	                   mode_gs,
	                   mode_xg
	}
	                   synth_mode;

	uint8_t            gs_part_to_ch[3][16];
	uint8_t            drum_channels[48];
};

#endif
