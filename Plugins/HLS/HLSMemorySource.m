//
//  HLSMemorySource.m
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Mildly edited by Christopher Snowhill on 2026-05-05
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import "HLSMemorySource.h"
#import "Logging.h"

@implementation HLSMemorySource {
	NSMutableArray<NSNumber *> *segmentSizes;
}

- (id)initWithURL:(NSURL *)url mimeType:(NSString *)mimeType {
	self = [super init];
	if (self) {
		_data = [NSMutableData data];
		_position = 0;
		_positionOffset = 0;
		_url = url;
		_mimeType = mimeType;
		segmentSizes = [NSMutableArray new];
	}
	return self;
}

- (NSURL *)url {
	return _url;
}

- (NSString *)mimeType {
	return _mimeType;
}

- (BOOL)open:(NSURL *)url {
	_url = url;
	_position = 0;
	return YES;
}

- (BOOL)seekable {
	return NO;
}

- (BOOL)seek:(long)position whence:(int)whence {
	long newPosition;

	switch (whence) {
		case SEEK_SET:
			newPosition = position;
			break;
		case SEEK_CUR:
			newPosition = (long)_position + position;
			break;
		case SEEK_END:
			newPosition = (long)[_data length] + position;
			break;
		default:
			return NO;
	}

	newPosition -= _positionOffset;

	if (newPosition < 0) {
		newPosition = 0;
	}
	else if (newPosition > (long)[_data length]) {
		newPosition = (long)[_data length];
	}

	_position = (NSUInteger)newPosition + _positionOffset;
	return YES;
}

- (long)tell {
	return (long)_position;
}

- (long)read:(void *)buffer amount:(long)amount {
	NSUInteger relativePosition = _position - _positionOffset;
	NSUInteger remaining = [_data length] - relativePosition;
	NSUInteger toRead = MIN((NSUInteger)amount, remaining);

	if (toRead > 0) {
		memcpy(buffer, [_data bytes] + relativePosition, toRead);
		_position += toRead;
	}

	if([segmentSizes count] > 1) {
		relativePosition += toRead;
		if(relativePosition >= 5 * 1024) {
			relativePosition -= 5 * 1024;
			if(relativePosition >= [segmentSizes[0] integerValue]) {
				NSUInteger amountToRemove = [segmentSizes[0] integerValue];
				[segmentSizes removeObjectAtIndex:0];
				[_data replaceBytesInRange:NSMakeRange(0, amountToRemove) withBytes:NULL length:0];
				_positionOffset += amountToRemove;
			}
		}
	}

	return (long)toRead;
}

- (void)close {
	_data = [NSMutableData data];
	_position = 0;
}

- (void)dealloc {
	_data = nil;
}

+ (NSArray *)schemes { 
	return @[];
}


- (void)appendData:(NSData *)newData {
	if (newData) {
		[_data appendData:newData];
		[segmentSizes addObject:@([newData length])];
		while([segmentSizes count] > 3) {
			NSUInteger amountToRemove = [segmentSizes[0] integerValue];
			if(_positionOffset + amountToRemove > _position) break;
			[segmentSizes removeObjectAtIndex:0];
			[_data replaceBytesInRange:NSMakeRange(0, amountToRemove) withBytes:NULL length:0];
			_positionOffset += amountToRemove;
		}
	}
}

- (void)resetPosition {
	_position = 0;
}

@dynamic segmentsBuffered;
- (NSUInteger) segmentsBuffered {
	return [segmentSizes count];
}

@end
