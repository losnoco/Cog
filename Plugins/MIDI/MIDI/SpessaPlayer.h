#ifndef __SpessaPlayer_h__
#define __SpessaPlayer_h__

#include "MIDIPlayer.h"

#include <spessasynth_core/synth.h>

class SpessaPlayer : public MIDIPlayer {
	public:
	// zero variables
	SpessaPlayer();

	// close, unload
	virtual ~SpessaPlayer();

	// configuration
	void setSoundFont(const char* in);
	void setFileSoundFont(const char* in);
	void setEmbeddedBank(const uint8_t *embedded_bank, size_t bank_size, uint16_t bank_offset);

	void setInterpolation(SS_InterpolationType interp);

	private:
	virtual unsigned int send_event_needs_time();
	virtual void send_event(uint32_t b);
	virtual void send_sysex(const uint8_t *data, size_t size, size_t port);
	virtual void render(float *out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	virtual void send_event_time(uint32_t b, unsigned int time);
	virtual void send_sysex_time(const uint8_t *data, size_t size, size_t port, unsigned int time);

	std::vector<SS_SoundBank *> _banks;
	SS_Processor* _synth;
	uint16_t bankOffset;
	std::vector<uint8_t> embeddedBank;
	std::string sSoundFontName;
	std::string sFileSoundFontName;

	SS_InterpolationType interp;

	double playerTime;

	enum { outputMax = 128 };
	float outputLeft[outputMax];
	float outputRight[outputMax];
};

#endif
