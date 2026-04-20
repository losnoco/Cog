#ifndef __MIDIPlayer_h__
#define __MIDIPlayer_h__

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

#include <spessasynth_core/midi.h>
#include <spessasynth_core/sequencer.h>
#include <spessasynth_core/synth.h>

extern const uint8_t syx_reset_gm[];
extern const uint8_t syx_reset_gm2[];
extern const uint8_t syx_reset_gs[];
extern const uint8_t syx_reset_xg[];

bool syx_equal(const uint8_t *a, const uint8_t *b);
bool syx_is_reset(const uint8_t *data);
bool syx_is_gs(const uint8_t *data, size_t size);

class MIDIPlayer {
	public:
	enum {
		loop_mode_enable = 1 << 0,
		loop_mode_force = 1 << 1
	};

	typedef enum {
		filter_default = 0,
		filter_gm,
		filter_gm2,
		filter_sc55,
		filter_sc88,
		filter_sc88pro,
		filter_sc8850,
		filter_xg
	} filter_mode;

	MIDIPlayer();

	virtual ~MIDIPlayer();

	void setSampleRate(double rate);
	void setLoopMode(unsigned int mode);
	void setLoopCount(unsigned int jumpCount);
	void setFilterMode(filter_mode m, bool disable_reverb_chorus);

	/* Non-owning SS_MIDIFile; must remain valid for the player's lifetime. */
	bool Load(SS_MIDIFile *midi_file, unsigned subsong, unsigned loop_mode, double fade_seconds);
	unsigned long Play(float *out, unsigned long count);
	void Seek(unsigned long sample);
	unsigned long Tell() const;

	bool GetLastError(std::string &p_out);

	protected:
	virtual bool startup() {
		return false;
	}
	virtual void shutdown() {
	}

	/* Render `sample_count` stereo-interleaved frames into `out`.  For
	 * callback-mode backends, any MIDI commands for this chunk have
	 * already been delivered via dispatchMidi.  For processor-mode
	 * backends (SpessaPlayer), ss_sequencer_tick has also just advanced
	 * the processor and the events have been queued internally. */
	virtual void renderChunk(float *out, uint32_t sample_count) {
	}

	/* Deliver a MIDI command at a sample offset within the next renderChunk.
	 * `data` begins with the status byte (0x80-0xFF).  `port` is 0-based
	 * (derived from 0xF5 port-select SysEx emitted by SS_Sequencer).  The
	 * port-select message itself is not passed through. */
	virtual void dispatchMidi(const uint8_t *data, size_t length,
	                          uint32_t sample_offset, unsigned port) {
	}

	/* Processor-mode backends (SpessaPlayer) override this to bind
	 * SS_Sequencer to their SS_Processor directly, bypassing callbacks. */
	virtual SS_Processor *getProcessor() {
		return nullptr;
	}

	/* Tick/render quantum in frames.  Defaults to SS_MAX_SOUND_CHUNK (128). */
	virtual uint32_t getChunkSize() const {
		return 128;
	}

	/* Called when the sequencer changes master volume (fade).  Default
	 * stores the scalar for Play() to apply during output mixdown.  Processor-
	 * mode backends may override to no-op (SS_Sequencer already applies the
	 * value directly to the processor). */
	virtual void handleMasterVolume(float value) {
		master_volume = value;
	}

	virtual bool get_last_error(std::string &p_out) {
		return false;
	}

	double dSampleRate;
	unsigned port_mask;
	filter_mode mode;
	bool reverb_chorus_disabled;
	bool initialized;
	float master_volume;

	SS_MIDIFile *midi_file;
	SS_Sequencer *sequencer;

	/* Track-window within the midi_file for format-2 subsong playback. */
	double subsong_start_seconds;
	double subsong_end_seconds;
	double duration_seconds;
	double fade_seconds;

	unsigned loop_mode_flags;
	unsigned loop_count;

	long samples_rendered;
	long samples_total;

	/* Callback-mode event queue filled by ss_sequencer_tick. */
	struct PendingEvent {
		std::vector<uint8_t> data;
		double timestamp;
	};
	std::vector<PendingEvent> pending_events;

	void sysex_reset(size_t port, uint32_t sample_offset);

	private:
	static void midi_command_cb(void *ctx, const uint8_t *data, size_t length, double timestamp);
	static void master_volume_cb(void *ctx, float value);

	bool buildSequencer();
	void teardownSequencer();

	void dispatchFilterReset(size_t port, uint32_t sample_offset);
	void inject(std::vector<uint8_t> bytes, double timestamp);
	void queueMidi(const uint8_t *data, size_t len, double ts);
};

#endif
