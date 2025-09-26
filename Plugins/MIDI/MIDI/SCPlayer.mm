//
//  SCPlayer.cpp
//  MIDI
//
//  Created by Christopher Snowhill on 9/23/25.
//

#include "SCPlayer.h"

#include <nuked_sc55/api.h>

#import <Cocoa/Cocoa.h>

#import <Accelerate/Accelerate.h>

#import "SCVis.h"

SCPlayer::SCPlayer()
: MIDIPlayer() {
	_player[0] = NULL;
	_player[1] = NULL;
	_player[2] = NULL;

	lcd_timestamp[0] = 0;
	lcd_timestamp[1] = 0;
	lcd_timestamp[2] = 0;

	lcd_last_timestamp[0] = 0;
	lcd_last_timestamp[1] = 0;
	lcd_last_timestamp[2] = 0;

	size_t size = sc55_lcd_state_size();
	last_lcd_state[0] = new uint8_t[size];
	last_lcd_state[1] = new uint8_t[size];
	last_lcd_state[2] = new uint8_t[size];
	
	_workerQueue = [[NSOperationQueue alloc] init];
}

SCPlayer::~SCPlayer() {
	shutdown();
}

void SCPlayer::setUrl(NSURL *url) {
	_url = url;
}

void SCPlayer::send_event(uint32_t b) {
	uint8_t event[3];
	event[0] = static_cast<uint8_t>(b);
	event[1] = static_cast<uint8_t>(b >> 8);
	event[2] = static_cast<uint8_t>(b >> 16);
	unsigned port = (b >> 24) & 0x7F;
	if(port > 2) port = 0;
	if(_player[port]) {
		const unsigned channel = (b & 0x0F) + port * 16;
		const unsigned command = b & 0xF0;
		const unsigned event_length = (command >= 0xF8 && command <= 0xFF) ? 1 : ((command == 0xC0 || command == 0xD0) ? 2 : 3);
		sc55_write_uart(_player[port], event, event_length);
	}
}

void SCPlayer::send_sysex(const uint8_t *data, size_t size, size_t port) {
	if(_player[0])
		sc55_write_uart(_player[0], data, (uint32_t)size);
	if(_player[1])
		sc55_write_uart(_player[1], data, (uint32_t)size);
	if(_player[2])
		sc55_write_uart(_player[2], data, (uint32_t)size);
}

// These are called from threads and post messages which are handled on the same thread
void SCPlayer::_lcd_callback(void *context, int port, const void *state, size_t size, uint64_t timestamp) {
	SCPlayer *_this = (SCPlayer *)context;
	_this->lcd_callback(port, state, size, timestamp);
}

void SCPlayer::lcd_callback(int port, const void *state, size_t size, uint64_t timestamp) {
	assert(port >= 0 && port <= 2);
	
	if(timestamp - lcd_last_timestamp[port] >= 5) {
		uint64_t lastTimestamp = lcd_last_timestamp[port];
		lcd_last_timestamp[port] = timestamp;
		@autoreleasepool {
			SCVisUpdate *update = [[SCVisUpdate alloc] initWithFile:_url whichScreen:port state:last_lcd_state[port] stateSize:size timestamp:lastTimestamp];
			[SCVisUpdate post:update];
		}
	}
	
	memcpy(last_lcd_state[port], state, size);
}

void SCPlayer::render(float *out, unsigned long count) {
	bzero(out, sizeof(float) * count * 2);
	while(count > 0) {
		unsigned long countToDo = count;
		if(countToDo > 512)
			countToDo = 512;

		for(size_t i = 0; i < 3; ++i) {
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

		for(size_t i = 0; i < 3; ++i) {
			if(_player[i])
				vDSP_vadd(ftempBuffer[i], 1, out, 1, out, 1, countToDo * 2);
		}

		out += countToDo * 2;
		count -= countToDo;
	}
}

void SCPlayer::shutdown() {
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
				*size = (uint32_t) [fileData length];
				return -1;
			}
			*size = (uint32_t) [fileData length];
			if(buffer) {
				memcpy(buffer, [fileData bytes], *size);
			}
		}
		return 0;
	}
}

bool SCPlayer::startup() {
	if(_player[0] || _player[1] || _player[2]) return true;

	for(size_t i = 0; i < 3; ++i) {
		if(!(port_mask & (1 << i))) continue;

		_player[i] = sc55_init(i, GS_RESET, loadRom, NULL);
		if(!_player[i]) {
			return false;
		}

		NSOperation *op = [NSBlockOperation blockOperationWithBlock:^{
			sc55_spin(_player[i], sc55_get_sample_rate(_player[i]) * 7);
		}];
		[_workerQueue addOperation:op];
	}

	[_workerQueue waitUntilAllOperationsAreFinished];

	return true;
}

uint32_t SCPlayer::sampleRate() {
	struct sc55_state *st = sc55_init(0, NONE, loadRom, NULL);
	if(!st) return 0;

	uint32_t r = sc55_get_sample_rate(st);

	sc55_free(st);

	return r;
}
