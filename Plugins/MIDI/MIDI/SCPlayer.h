#ifndef __SCPlayer_h__
#define __SCPlayer_h__

#include "MIDIPlayer.h"

#import <Cocoa/Cocoa.h>

#import <CogAudio/CogSemaphore.h>

class SCPlayer : public MIDIPlayer {
	public:
	// zero variables
	SCPlayer();

	// close, unload
	virtual ~SCPlayer();
	
	static uint32_t sampleRate();

	void setUrl(NSURL *url);

	protected:
	virtual void send_event(uint32_t b);
	virtual void send_sysex(const uint8_t* event, size_t size, size_t port);
	virtual void render(float* out, unsigned long count);

	virtual void shutdown();
	virtual bool startup();

	private:
	static void _lcd_callback(void *context, int port, const void *state, size_t size, uint64_t timestamp);
	void lcd_callback(int port, const void *state, size_t size, uint64_t timestamp);

	uint8_t *last_lcd_state[3];

	NSURL *_url;
	uint64_t lcd_timestamp[3];
	uint64_t lcd_last_timestamp[3];

	struct sc55_state *_player[3];

	NSOperationQueue *_workerQueue;

	short tempBuffer[3][512 * 2];
	float ftempBuffer[3][512 * 2];
};

#endif
