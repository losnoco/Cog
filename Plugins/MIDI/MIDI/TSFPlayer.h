#ifndef __TSFPlayer_h__
#define __TSFPlayer_h__

#include "MIDIPlayer.h"

#include <bassmidi.h>

typedef struct sflist_presets sflist_presets;

class TSFPlayer : public MIDIPlayer {
	public:
	// zero variables
	TSFPlayer();

	// close, unload
	virtual ~TSFPlayer();

	// configuration
	void setSoundFont(const char* in);
	void setFileSoundFont(const char* in);

	private:
	virtual void send_event(uint32_t b);
	virtual void send_sysex(const uint8_t* data, size_t size, size_t port);
	virtual void render(float* out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	void reset_parameters();

	struct tsf* _synth;
	std::string sSoundFontName;
	std::string sFileSoundFontName;
};

#endif
