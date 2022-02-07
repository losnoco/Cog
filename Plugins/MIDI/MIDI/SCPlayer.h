#ifndef __SCPlayer_h__
#define __SCPlayer_h__

#include "MIDIPlayer.h"

#include "SCCore.h"

class SCPlayer : public MIDIPlayer {
	public:
	// zero variables
	SCPlayer();

	// close, unload
	virtual ~SCPlayer();

	unsigned int get_playing_note_count();

	void set_sccore_path(const char* path);

	protected:
	virtual unsigned int send_event_needs_time();
	virtual void send_event(uint32_t b);
	virtual void send_sysex(const uint8_t* data, size_t size, size_t port);
	virtual void render(float* out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	virtual void send_event_time(uint32_t b, unsigned int time);
	virtual void send_sysex_time(const uint8_t* data, size_t size, size_t port, unsigned int time);

	private:
	unsigned int instance_id;
	bool initialized;
	SCCore* sampler;

	char* sccore_path;
};

#endif
