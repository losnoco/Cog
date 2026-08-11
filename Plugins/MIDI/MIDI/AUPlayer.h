#ifndef __AUPlayer_h__
#define __AUPlayer_h__

#include "MIDIPlayer.h"

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudio/CoreAudioTypes.h>

class AUPluginUI;

/* An Audio Unit instrument as a MIDIPlayer.
 *
 * A MIDI file may address up to four ports, and a port is a whole second set
 * of sixteen channels.  There is no port-select message in a MIDI stream: the
 * port rides on each message, as the cable number, and an audio unit that
 * accepts more than one cable can serve every port from one instance.  Only
 * units that say they cannot -- virtualMIDICableCount of 1, which is every
 * v2 unit including Apple's DLS synth -- need an instance per port, with
 * their outputs summed.  See AUPlayer.mm for which API carries the cable. */
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
	/* MIDIPlayer's port_mask is four bits wide. */
	static const unsigned max_ports = 4;

	/* One audio unit, and the two ways of reaching it.
	 *
	 * `unit` is the v2 handle, which is what renders and what carries the
	 * sampler properties that have no v3 spelling -- the sound bank URL above
	 * all.  `auUnit` is the same instance seen through the v3 API, which is
	 * the only place the cable count and the scheduling blocks live.  Both
	 * come from one AVAudioUnit, which owns the instance and must outlive it.
	 *
	 * Initialised in class rather than by a memset in the constructor: the
	 * Objective-C members are ARC-managed, so this struct has a constructor of
	 * its own and clearing it with memset would write over references ARC
	 * believes it owns. */
	struct Instance {
		AVAudioUnit *node = nil;
		AudioUnit unit = NULL;
		AUAudioUnit *auUnit = nil;

		/* Fetched before the unit is initialized, as the API asks.  Either may
		 * be nil, and everything falls back to MusicDeviceMIDIEvent. */
		AUScheduleMIDIEventBlock scheduleEvent = nil;
		AUMIDIEventListBlock scheduleEventList = nil;

		bool needsInput = false;
	};

	bool openInstance(const AudioComponentDescription &cd, Instance &into);
	bool configureInstance(Instance &instance);
	static void closeInstance(Instance &instance);

	void loadSoundFont(const char *name);

	/* Named and shaped as in TSPlayer, which dispatches the same way. */
	void sendEventTime(uint32_t b, uint32_t time, unsigned port);
	void sendSysexTime(const uint8_t *data, size_t size, unsigned port, uint32_t time);

	/* Where the port actually becomes a cable. */
	void sendToPort(unsigned port, const uint8_t *data, size_t length, uint32_t sample_offset);
	void sendToCable(Instance &instance, uint8_t cable, const uint8_t *data, size_t length,
	                 uint32_t sample_offset);

	std::string sSoundFontName;

	AudioTimeStamp mTimeStamp;

	Instance instances[max_ports];

	/* How many of `instances` are open, and how many ports each one answers
	 * for.  `instanceCount * cablesPerInstance` covers every port in use. */
	unsigned instanceCount;
	unsigned cablesPerInstance;

	AudioBufferList *bufferList;

	float *audioBuffer;

	OSType componentSubType, componentManufacturer;

	NSDictionary *preset;
};

#endif
