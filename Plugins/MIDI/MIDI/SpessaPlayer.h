#ifndef __SpessaPlayer_h__
#define __SpessaPlayer_h__

#include "MIDIPlayer.h"

#include <spessasynth_core/synth.h>

class SpessaPlayer : public MIDIPlayer {
	public:
	SpessaPlayer();

	virtual ~SpessaPlayer();

	void setSoundFont(const char *in);
	void setFileBankOffset(uint16_t bank_offset);
	void setFileSoundFont(const char *in);

	void setInterpolation(SS_InterpolationType interp);

	protected:
	virtual SS_Processor *getProcessor() {
		return _synth;
	}
	virtual uint32_t getChunkSize() const {
		return 128; /* SS_MAX_SOUND_CHUNK */
	}

	/* Volume is already applied to the processor by SS_Sequencer; no-op
	 * external scaling. */
	virtual void handleMasterVolume(float value) {
	}

	virtual bool startup();
	virtual void shutdown();
	virtual void renderChunk(float *out, uint32_t sample_count);

	private:
	std::vector<SS_SoundBank *> _banks;
	SS_Processor *_synth;
	uint16_t fileBankOffset;
	std::string sSoundFontName;
	std::string sFileSoundFontName;

	SS_InterpolationType interp;
};

#endif
