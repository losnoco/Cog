//
//  HLSMemorySource.m
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Rewritten by Christopher Snowhill on 2026-05-06
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import "HLSMemorySource.h"
#import "Logging.h"

@implementation HLSMemorySource {
	NSMutableArray<NSData *> *_chunks; // queued segment data
	NSUInteger _frontOffset;            // bytes already consumed within _chunks[0]
	int64_t _position;                  // total bytes returned by -read: since start/last reset
	BOOL _eof;                          // producer has signaled end of stream
	BOOL _closed;                       // consumer has called -close
	NSCondition *_cond;                 // guards _chunks / _frontOffset / flags

	NSURL *_url;
	NSString *_mimeType;
}

#pragma mark - Lifecycle

- (instancetype)initWithURL:(NSURL *)url mimeType:(NSString *)mimeType {
	self = [super init];
	if(self) {
		_chunks = [NSMutableArray array];
		_frontOffset = 0;
		_position = 0;
		_eof = NO;
		_closed = NO;
		_cond = [[NSCondition alloc] init];
		_url = url;
		_mimeType = [mimeType copy];
	}
	return self;
}

- (void)dealloc {
	[self close];
}

#pragma mark - Producer API

- (void)appendData:(NSData *)data {
	if([data length] == 0) return;
	[_cond lock];
	if(!_closed) {
		[_chunks addObject:data];
		[_cond broadcast];
	}
	[_cond unlock];
}

- (void)markEndOfStream {
	[_cond lock];
	_eof = YES;
	[_cond broadcast];
	[_cond unlock];
}

- (void)reset {
	[_cond lock];
	[_chunks removeAllObjects];
	_frontOffset = 0;
	_position = 0;
	_eof = NO;
	[_cond broadcast];
	[_cond unlock];
}

- (NSUInteger)bufferedSegmentCount {
	[_cond lock];
	NSUInteger n = [_chunks count];
	[_cond unlock];
	return n;
}

- (void)setUrl:(NSURL *)url {
	[_cond lock];
	_url = url;
	[_cond unlock];
}

- (void)setMimeType:(NSString *)mimeType {
	[_cond lock];
	_mimeType = [mimeType copy];
	[_cond unlock];
}

#pragma mark - CogSource

+ (NSArray *)schemes {
	return @[]; // private; HLSDecoder hands instances to underlying decoders directly
}

- (NSURL *)url {
	[_cond lock];
	NSURL *u = _url;
	[_cond unlock];
	return u;
}

- (NSString *)mimeType {
	[_cond lock];
	NSString *m = _mimeType;
	[_cond unlock];
	return m;
}

- (BOOL)open:(NSURL *)url {
	[_cond lock];
	_url = url;
	[_cond unlock];
	return YES;
}

- (BOOL)seekable {
	return NO;
}

- (BOOL)seek:(long)position whence:(int)whence {
	// We don't support seeking. HLSDecoder reuses this source by calling
	// -reset and re-feeding it from a new segment, never by seeking it.
	return NO;
}

- (long)tell {
	[_cond lock];
	long p = (long)_position;
	[_cond unlock];
	return p;
}

- (long)read:(void *)buffer amount:(long)amount {
	if(amount <= 0) return 0;

	uint8_t *dst = (uint8_t *)buffer;
	long totalRead = 0;

	[_cond lock];
	while(totalRead < amount) {
		if(_closed) break;

		// Wait for data if the queue is empty and the producer hasn't
		// signaled EOF. The consumer may sit here for a long time
		// (network RTT + segment download), but it gets unblocked
		// promptly by appendData / markEndOfStream / close.
		while([_chunks count] == 0 && !_eof && !_closed) {
			[_cond wait];
		}

		if([_chunks count] == 0) {
			// Either EOF or closed -- return what we have, even if 0.
			break;
		}

		NSData *front = _chunks[0];
		NSUInteger available = [front length] - _frontOffset;
		NSUInteger toCopy = MIN(available, (NSUInteger)(amount - totalRead));

		if(toCopy > 0) {
			memcpy(dst + totalRead, (const uint8_t *)[front bytes] + _frontOffset, toCopy);
			_frontOffset += toCopy;
			_position += (int64_t)toCopy;
			totalRead += (long)toCopy;
		}

		if(_frontOffset >= [front length]) {
			[_chunks removeObjectAtIndex:0];
			_frontOffset = 0;
			// Wake any backpressure-waiters in the producer. Currently
			// the segment manager polls bufferedSegmentCount rather than
			// waiting, but broadcast is cheap and future-proof.
			[_cond broadcast];
		}
	}
	[_cond unlock];

	return totalRead;
}

- (void)close {
	[_cond lock];
	_closed = YES;
	_eof = YES;
	[_chunks removeAllObjects];
	_frontOffset = 0;
	[_cond broadcast];
	[_cond unlock];
}

@end
