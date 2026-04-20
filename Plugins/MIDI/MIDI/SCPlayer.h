#ifndef __SCPlayer_h__
#define __SCPlayer_h__

#include "MIDIPlayer.h"

#import <Cocoa/Cocoa.h>

#import <CogAudio/CogSemaphore.h>

#import <CogAudio/VisualizationController.h>

class SCPlayer : public MIDIPlayer {
	public:
	SCPlayer();

	virtual ~SCPlayer();

	static double sampleRate();

	void setUrl(NSURL *url);

	void flushOnSeek();

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
	static void _lcd_callback(void *context, int port, const void *state, size_t size, uint64_t timestamp);
	void lcd_callback(int port, const void *state, size_t size, uint64_t timestamp);

	uint8_t *last_lcd_state[4];

	NSURL *_url;
	uint64_t lcd_timestamp[4];
	uint64_t lcd_last_timestamp[4];

	struct sc55_state *_player[4];

	NSOperationQueue *_workerQueue;

	short tempBuffer[4][512 * 2];
	float ftempBuffer[4][512 * 2];

	MIDIVisualizationController *midiController;
};

#endif
