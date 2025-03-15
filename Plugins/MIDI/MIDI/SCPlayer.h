#ifndef __SCPlayer_h__
#define __SCPlayer_h__

#include "MIDIPlayer.h"

#import <Cocoa/Cocoa.h>

#include "synth.h"

class SCPlayer : public MIDIPlayer {
	public:
	// zero variables
	SCPlayer();

	// close, unload
	virtual ~SCPlayer();

	protected:
	virtual unsigned int send_event_needs_time();
	virtual void send_event(uint32_t b);
	virtual void send_sysex(const uint8_t* event, size_t size, size_t port);
	virtual void render(float* out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	private:
	bool LoadCore();

	void send_command(uint32_t port, uint32_t command);

	void render_port(uint32_t port, float* out, uint32_t count);

	void reset(uint32_t port);

	void junk(uint32_t port, unsigned long count);

	bool process_create(uint32_t port);
	void process_terminate(uint32_t port);
	bool process_running(uint32_t port);
	uint32_t process_read_code(uint32_t port);
	void process_read_bytes(uint32_t port, void* buffer, uint32_t size);
	uint32_t process_read_bytes_pass(uint32_t port, void* buffer, uint32_t size);
	void process_write_code(uint32_t port, uint32_t code);
	void process_write_bytes(uint32_t port, const void* buffer, uint32_t size);

	EmuSC::ControlRom *_controlROM;
	EmuSC::PcmRom *_pcmROM;
	EmuSC::Synth *_synth[3];
};

#endif
