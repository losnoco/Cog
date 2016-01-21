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
    
    void SetLoopMode(unsigned mode);

protected:
	virtual void send_event(uint32_t b, uint32_t sample_offset) {}
	virtual void render_512(float * out) {}

	virtual void shutdown() {};
	virtual bool startup() {return false;}

	unsigned long      uSampleRate;
	system_exclusive_table mSysexMap;

private:
	void render(float * out, uint32_t count);
	
	unsigned long      uSamplesRemaining;

	unsigned           uLoopMode;

	std::vector<midi_stream_event> mStream;

	unsigned long      uStreamPosition;
	unsigned long      uTimeCurrent;
	unsigned long      uTimeEnd;

	unsigned long      uStreamLoopStart;
	unsigned long      uTimeLoopStart;
    unsigned long      uStreamEnd;
	
	unsigned int       uSamplesInBuffer;
	float              fSampleBuffer[512 * 2];
};

#endif

