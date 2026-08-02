//
//  TSPlayer.h
//  MIDI
//
//  Created by Christopher Snowhill on 8/2/26.
//

#ifndef __TSPlayer_h__
#define __TSPlayer_h__

#include "MIDIPlayer.h"

#include "tabulasonora/tone_generator.hpp"

class TSPlayer : public MIDIPlayer {
	public:
	TSPlayer();

	virtual ~TSPlayer();

	static double sampleRate();

	void setSCCore(const char *in);

	protected:
	virtual bool startup();
	virtual void shutdown();
	virtual void renderChunk(float *out, uint32_t sample_count);
	virtual void dispatchMidi(const uint8_t *data, size_t length,
							  uint32_t sample_offset, unsigned port);
	virtual uint32_t getChunkSize() const {
		return 320;
	}

	private:
	void sendEventTime(uint32_t b, uint32_t time, unsigned port);
	void sendSysexTime(const uint8_t *data, size_t size, unsigned port, uint32_t time);

	float *audioBuffer;

	std::string romPath;

	std::unique_ptr<const ts::RomImage> rom;
	ts::NoteRenderer *notes[2];
	ts::ToneGenerator *tonegen[2];
};

#endif // !__TSPlayer_h__
