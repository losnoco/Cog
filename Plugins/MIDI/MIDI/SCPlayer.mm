//
//  SCPlayer.cpp
//  MIDI
//
//  Created by Christopher Snowhill on 9/22/25.
//

#include "SCPlayer.h"

#include <nuked_sc55/api.h>

#import <Cocoa/Cocoa.h>

#import <Accelerate/Accelerate.h>

SCPlayer::SCPlayer()
: MIDIPlayer() {
	_player[0] = NULL;
	_player[1] = NULL;
	_player[2] = NULL;
	_player[3] = NULL;

	lcd_timestamp[0] = 0;
	lcd_timestamp[1] = 0;
	lcd_timestamp[2] = 0;
	lcd_timestamp[3] = 0;

	lcd_last_timestamp[0] = 0;
	lcd_last_timestamp[1] = 0;
	lcd_last_timestamp[2] = 0;
	lcd_last_timestamp[3] = 0;

	size_t size = sc55_lcd_state_size();
	last_lcd_state[0] = new uint8_t[size];
	last_lcd_state[1] = new uint8_t[size];
	last_lcd_state[2] = new uint8_t[size];
	last_lcd_state[3] = new uint8_t[size];

	_workerQueue = [NSOperationQueue new];

	midiController = [NSClassFromString(@"VisualizationController") sharedMIDIController];
}

SCPlayer::~SCPlayer() {
	shutdown();
	delete[] last_lcd_state[0];
	delete[] last_lcd_state[1];
	delete[] last_lcd_state[2];
	delete[] last_lcd_state[3];
}

void SCPlayer::setUrl(NSURL *url) {
	_url = url;
}

void SCPlayer::dispatchMidi(const uint8_t *data, size_t length,
                            uint32_t sample_offset, unsigned port) {
	if(!length) return;
	(void)sample_offset;
	if(port > 3) port = 0;

	uint8_t sb = data[0];

	if(sb == 0xF0) {
		/* SysEx goes to every active port so the global state stays consistent
		 * across all SC-55 instances.  Matches the prior send_sysex behavior. */
		for(unsigned p = 0; p < 4; ++p) {
			if(_player[p])
				sc55_write_uart(_player[p], data, (uint32_t)length);
		}
		return;
	}
	if(sb >= 0xF1) {
		if(_player[port])
			sc55_write_uart(_player[port], data, (uint32_t)length);
		return;
	}

	if(_player[port])
		sc55_write_uart(_player[port], data, (uint32_t)length);
}

void SCPlayer::_lcd_callback(void *context, int port, const void *state, size_t size, uint64_t timestamp) {
	SCPlayer *_this = (SCPlayer *)context;
	_this->lcd_callback(port, state, size, timestamp);
}

void SCPlayer::lcd_callback(int port, const void *state, size_t size, uint64_t timestamp) {
	assert(port >= 0 && port <= 3);

	if(timestamp - lcd_last_timestamp[port] >= 5) {
		uint64_t lastTimestamp = lcd_last_timestamp[port];
		lcd_last_timestamp[port] = timestamp;
		@autoreleasepool {
			SCVisEvent *update = [[SCVisEvent alloc] initWithUrl:_url whichPort:port state:last_lcd_state[port] stateSize:size timestamp:lastTimestamp];
			[midiController postEvent:update];
		}
	}

	memcpy(last_lcd_state[port], state, size);
}

void SCPlayer::flushOnSeek() {
	[midiController flushEvents];
}

void SCPlayer::renderChunk(float *out, uint32_t count) {
	bzero(out, sizeof(float) * count * 2);
	while(count > 0) {
		uint32_t countToDo = count;
		if(countToDo > 512)
			countToDo = 512;

		for(size_t i = 0; i < 4; ++i) {
			if(!_player[i]) continue;

			NSOperation *op = [NSBlockOperation blockOperationWithBlock:^{
				sc55_render_with_lcd(_player[i], tempBuffer[i], countToDo, _lcd_callback, this);
				vDSP_vflt16(tempBuffer[i], 1, ftempBuffer[i], 1, countToDo * 2);
				float scale = 1ULL << 15;
				vDSP_vsdiv(ftempBuffer[i], 1, &scale, ftempBuffer[i], 1, countToDo * 2);
			}];
			[_workerQueue addOperation:op];
		}

		[_workerQueue waitUntilAllOperationsAreFinished];

		for(size_t i = 0; i < 4; ++i) {
			if(_player[i])
				vDSP_vadd(ftempBuffer[i], 1, out, 1, out, 1, countToDo * 2);
		}

		out += countToDo * 2;
		count -= countToDo;
	}
}

void SCPlayer::shutdown() {
	if(_player[3]) {
		sc55_free(_player[3]);
		_player[3] = NULL;
	}
	if(_player[2]) {
		sc55_free(_player[2]);
		_player[2] = NULL;
	}
	if(_player[1]) {
		sc55_free(_player[1]);
		_player[1] = NULL;
	}
	if(_player[0]) {
		sc55_free(_player[0]);
		_player[0] = NULL;
	}
	initialized = false;
}

static NSString *getRomName(NSString *baseName) {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	NSString *basePath = [[paths firstObject] stringByAppendingPathComponent:@"Cog"];
	basePath = [basePath stringByAppendingPathComponent:@"Roms"];
	basePath = [basePath stringByAppendingPathComponent:@"Nuked-SC55"];
	basePath = [basePath stringByAppendingPathComponent:baseName];
	return basePath;
}

static int loadRom(void *context, const char *name, uint8_t *buffer, uint32_t *size) {
	@autoreleasepool {
		NSString *_name = [NSString stringWithUTF8String:name];
		NSString *romName;
		if([_name isEqualToString:@"back.data"]) {
			NSBundle *bundle = [NSBundle bundleWithIdentifier:@"org.losnoco.nuked-sc55"];
			romName = [bundle pathForResource:@"back" ofType:@"data"];
		} else {
			romName = getRomName(_name);
		}
		BOOL dir = NO;
		NSFileManager *defaultManager = [NSFileManager defaultManager];
		if(![defaultManager fileExistsAtPath:romName isDirectory:&dir]) {
			return -1;
		}
		if(size) {
			NSData *fileData = [defaultManager contentsAtPath:romName];
			if(!fileData)
				return -1;
			if([fileData length] > *size) {
				*size = (uint32_t)[fileData length];
				return -1;
			}
			*size = (uint32_t)[fileData length];
			if(buffer) {
				memcpy(buffer, [fileData bytes], *size);
			}
		}
		return 0;
	}
}

bool SCPlayer::startup() {
	if(_player[0] || _player[1] || _player[2] || _player[3]) return true;

	for(size_t i = 0; i < 4; ++i) {
		if(!(port_mask & (1u << i))) continue;

		_player[i] = sc55_init((int)i, GS_RESET, loadRom, NULL);
		if(!_player[i]) {
			return false;
		}

		NSOperation *op = [NSBlockOperation blockOperationWithBlock:^{
			sc55_spin(_player[i], sc55_get_sample_rate(_player[i]) * 7);
		}];
		[_workerQueue addOperation:op];
	}

	[_workerQueue waitUntilAllOperationsAreFinished];

	initialized = true;
	return true;
}

double SCPlayer::sampleRate() {
	struct sc55_state *st = sc55_init(0, NONE, loadRom, NULL);
	if(!st) return 0;

	uint32_t r = sc55_get_sample_rate(st);

	sc55_free(st);

	return (double)r;
}
