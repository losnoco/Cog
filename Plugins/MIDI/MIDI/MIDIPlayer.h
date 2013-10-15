#ifndef __MIDIPlayer_h__
#define __MIDIPlayer_h__

#include <midi_processing/midi_container.h>

class MIDIPlayer
{
public:
	enum
	{
		loop_mode_enable = 1 << 0,
		loop_mode_force  = 1 << 1
	};

	// zero variables
	MIDIPlayer();

	// close, unload
	virtual ~MIDIPlayer() {};

	// setup
	void setSampleRate(unsigned long rate);

	bool Load(const midi_container & midi_file, unsigned subsong, unsigned loop_mode, unsigned clean_flags);
	unsigned long Play(float * out, unsigned long count);
	void Seek(unsigned long sample);

protected:
	virtual void send_event(uint32_t b) {}
	virtual void render(float * out, unsigned long count) {}

	virtual void shutdown() {};
	virtual bool startup() {return false;}

	unsigned long      uSampleRate;
	system_exclusive_table mSysexMap;

private:
	unsigned long      uSamplesRemaining;

	unsigned           uLoopMode;

	std::vector<midi_stream_event> mStream;

	unsigned long      uStreamPosition;
	unsigned long      uTimeCurrent;
	unsigned long      uTimeEnd;

	unsigned long      uStreamLoopStart;
	unsigned long      uTimeLoopStart;
    unsigned long      uStreamEnd;
};

#endif

