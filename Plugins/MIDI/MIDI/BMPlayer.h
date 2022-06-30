#ifndef __BMPlayer_h__
#define __BMPlayer_h__

#include "MIDIPlayer.h"

#include <bassmidi.h>

typedef struct sflist_presets sflist_presets;

class BMPlayer : public MIDIPlayer {
	public:
	// zero variables
	BMPlayer();

	// close, unload
	virtual ~BMPlayer();

	// configuration
	void setSoundFont(const char* in);
	void setFileSoundFont(const char* in);
	void setSincInterpolation(bool enable = true);

	private:
	virtual void send_event(uint32_t b);
	virtual void send_sysex(const uint8_t* data, size_t size, size_t port);
	virtual void render(float* out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	void reset_parameters();

	std::vector<HSOUNDFONT> _soundFonts;
	sflist_presets* _presetList;
	std::string sSoundFontName;
	std::string sFileSoundFontName;

	HSTREAM _stream;

	bool bSincInterpolation;
};

#endif
