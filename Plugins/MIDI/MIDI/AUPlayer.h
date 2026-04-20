#ifndef __AUPlayer_h__
#define __AUPlayer_h__

#include "MIDIPlayer.h"

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudio/CoreAudioTypes.h>

class AUPluginUI;

class AUPlayer : public MIDIPlayer {
	public:
	AUPlayer();

	virtual ~AUPlayer();

	void setSoundFont(const char *in);

	typedef void (*callback)(OSType uSubType, OSType uManufacturer, const char *name);
	static void enumComponents(callback cbEnum);

	void setComponent(OSType uSubType, OSType uManufacturer);
	void setPreset(NSDictionary *preset);

	protected:
	virtual bool startup();
	virtual void shutdown();
	virtual void renderChunk(float *out, uint32_t sample_count);
	virtual void dispatchMidi(const uint8_t *data, size_t length,
	                          uint32_t sample_offset, unsigned port);
	virtual uint32_t getChunkSize() const {
		return 512;
	}

	private:
	void loadSoundFont(const char *name);
	void sendEventTime(uint32_t b, uint32_t time, unsigned port);
	void sendSysexTime(const uint8_t *data, size_t size, unsigned port, uint32_t time);

	std::string sSoundFontName;

	AudioTimeStamp mTimeStamp;

	AudioUnit samplerUnit[4];

	AudioBufferList *bufferList;

	float *audioBuffer;

	OSType componentSubType, componentManufacturer;

	BOOL needsInput;

	NSDictionary *preset;
};

#endif
