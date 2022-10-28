#include "SCPlayer.h"

#include <vector>

static uint16_t getwordle(uint8_t *pData) {
	return (uint16_t)(pData[0] | (((uint16_t)pData[1]) << 8));
}

static uint32_t getdwordle(uint8_t *pData) {
	return pData[0] | (((uint32_t)pData[1]) << 8) | (((uint32_t)pData[2]) << 16) | (((uint32_t)pData[3]) << 24);
}

bool SCPlayer::LoadCore() {
	bool rval = process_create(0);
	if(rval) rval = process_create(1);
	if(rval) rval = process_create(2);

	return rval;
}

bool SCPlayer::process_create(uint32_t port) {
	bTerminating[port] = false;

	hChildStd_IN[port] = [[NSPipe alloc] init];
	hChildStd_OUT[port] = [[NSPipe alloc] init];

	NSURL *launcherUrl = [[NSBundle mainBundle] URLForResource:@"scpipe" withExtension:@""];
	NSURL *coreUrl = [[NSBundle mainBundle] URLForResource:@"IIAM" withExtension:@"bin"];

	hProcess[port] = [[NSTask alloc] init];
	[hProcess[port] setExecutableURL:launcherUrl];
	[hProcess[port] setArguments:@[[coreUrl path]]];
	[hProcess[port] setStandardInput:hChildStd_IN[port]];
	[hProcess[port] setStandardOutput:hChildStd_OUT[port]];

	NSError *error = nil;
	if(![hProcess[port] launchAndReturnError:&error] || error != nil) {
		process_terminate(port);
		return false;
	}

	uint32_t code = process_read_code(port);

	if(code != 0) {
		process_terminate(port);
		return false;
	}

	return true;
}

void SCPlayer::process_terminate(uint32_t port) {
	if(bTerminating[port]) return;

	bTerminating[port] = true;

	if(hProcess[port]) {
		process_write_code(port, 0);
		[hProcess[port] interrupt];
		[hProcess[port] waitUntilExit];
		[hProcess[port] terminate];
		hProcess[port] = nil;
	}
	if(hChildStd_IN[port]) {
		hChildStd_IN[port] = nil;
	}
	if(hChildStd_OUT[port]) {
		hChildStd_OUT[port] = nil;
	}

	bTerminating[port] = false;
}

bool SCPlayer::process_running(uint32_t port) {
	if(hProcess[port] && [hProcess[port] isRunning]) return true;
	return false;
}

uint32_t SCPlayer::process_read_bytes_pass(uint32_t port, void *out, uint32_t size) {
	NSError *error = nil;
	NSData *data = [[hChildStd_OUT[port] fileHandleForReading] readDataUpToLength:size error:&error];
	if(!data || error) return 0;
	NSUInteger bytesDone = [data length];
	memcpy(out, [data bytes], bytesDone);
	return bytesDone;
}

void SCPlayer::process_read_bytes(uint32_t port, void *out, uint32_t size) {
	if(process_running(port) && size) {
		uint8_t *ptr = (uint8_t *)out;
		uint32_t done = 0;
		while(done < size) {
			uint32_t delta = process_read_bytes_pass(port, ptr + done, size - done);
			if(delta == 0) {
				memset(out, 0xFF, size);
				break;
			}
			done += delta;
		}
	} else
		memset(out, 0xFF, size);
}

uint32_t SCPlayer::process_read_code(uint32_t port) {
	uint32_t code;
	process_read_bytes(port, &code, sizeof(code));
	return code;
}

void SCPlayer::process_write_bytes(uint32_t port, const void *in, uint32_t size) {
	if(process_running(port)) {
		if(size == 0) return;
		NSError *error = nil;
		NSData *data = [NSData dataWithBytes:in length:size];
		if(![[hChildStd_IN[port] fileHandleForWriting] writeData:data error:&error] || error) {
			process_terminate(port);
		}
	}
}

void SCPlayer::process_write_code(uint32_t port, uint32_t code) {
	process_write_bytes(port, &code, sizeof(code));
}

SCPlayer::SCPlayer()
: MIDIPlayer() {
	initialized = false;
	for(unsigned int i = 0; i < 3; ++i) {
		bTerminating[i] = false;
		hProcess[i] = nil;
		hChildStd_IN[i] = nil;
		hChildStd_OUT[i] = nil;
	}
}

SCPlayer::~SCPlayer() {
	shutdown();
}

void SCPlayer::send_event(uint32_t b) {
	uint32_t port = (b >> 24) & 0xFF;
	if(port > 2) port = 0;
	process_write_code(port, 2);
	process_write_code(port, b & 0xFFFFFF);
	if(process_read_code(port) != 0)
		process_terminate(port);
}

void SCPlayer::send_sysex(const uint8_t *event, size_t size, size_t port) {
	process_write_code(port, 3);
	process_write_code(port, (uint32_t)size);
	process_write_bytes(port, event, size);
	if(process_read_code(port) != 0)
		process_terminate(port);
	if(port == 0) {
		send_sysex(event, size, 1);
		send_sysex(event, size, 2);
	}
}

void SCPlayer::send_event_time(uint32_t b, unsigned int time) {
	uint32_t port = (b >> 24) & 0xFF;
	if(port > 2) port = 0;
	process_write_code(port, 6);
	process_write_code(port, b & 0xFFFFFF);
	process_write_code(port, time);
	if(process_read_code(port) != 0)
		process_terminate(port);
}

void SCPlayer::send_sysex_time(const uint8_t *event, size_t size, size_t port, unsigned int time) {
	process_write_code(port, 7);
	process_write_code(port, size);
	process_write_code(port, time);
	process_write_bytes(port, event, size);
	if(process_read_code(port) != 0)
		process_terminate(port);
	if(port == 0) {
		send_sysex_time(event, size, 1, time);
		send_sysex_time(event, size, 2, time);
	}
}

void SCPlayer::render_port(uint32_t port, float *out, uint32_t count) {
	process_write_code(port, 4);
	process_write_code(port, count);
	if(process_read_code(port) != 0) {
		process_terminate(port);
		memset(out, 0, count * sizeof(float) * 2);
		return;
	}
	process_read_bytes(port, out, count * sizeof(float) * 2);
}

void SCPlayer::render(float *out, unsigned long count) {
	memset(out, 0, count * sizeof(float) * 2);
	while(count) {
		unsigned long todo = count > 4096 ? 4096 : count;
		float buffer[4096 * 2];
		for(unsigned long i = 0; i < 3; ++i) {
			render_port(i, buffer, todo);

			for(unsigned long j = 0; j < todo; ++j) {
				out[j * 2 + 0] += buffer[j * 2 + 0];
				out[j * 2 + 1] += buffer[j * 2 + 1];
			}
		}
		out += todo * 2;
		count -= todo;
	}
}

void SCPlayer::shutdown() {
	for(int i = 0; i < 3; i++) {
		process_terminate(i);
	}
	initialized = false;
}

bool SCPlayer::startup() {
	if(initialized) return true;

	if(!LoadCore())
		return false;

	for(int i = 0; i < 3; i++) {
		process_write_code(i, 1);
		process_write_code(i, sizeof(uint32_t));
		process_write_code(i, uSampleRate);
		if(process_read_code(i) != 0)
			return false;
	}

	initialized = true;

	setFilterMode(mode, reverb_chorus_disabled);

	return true;
}

unsigned int SCPlayer::send_event_needs_time() {
	return 0; // 4096; This doesn't work for some reason
}
