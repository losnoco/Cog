#ifndef __MSPlayer_h__
#define __MSPlayer_h__

#include "MIDIPlayer.h"

#include "interface.h"

class midisynth;

class MSPlayer : public MIDIPlayer {
	public:
	MSPlayer();

	virtual ~MSPlayer();

	void set_synth(unsigned int synth);
	void set_bank(unsigned int bank);
	void set_extp(unsigned int extp);

	typedef void (*enum_callback)(unsigned int synth, unsigned int bank, const char *name);

	void enum_synthesizers(enum_callback callback);

	protected:
	virtual bool startup();
	virtual void shutdown();
	virtual void renderChunk(float *out, uint32_t sample_count);
	virtual void dispatchMidi(const uint8_t *data, size_t length,
	                          uint32_t sample_offset, unsigned port);
	virtual uint32_t getChunkSize() const {
		return 256;
	}

	private:
	unsigned int synth_id;
	unsigned int bank_id;
	unsigned int extp;
	midisynth *synth;
};

#endif
