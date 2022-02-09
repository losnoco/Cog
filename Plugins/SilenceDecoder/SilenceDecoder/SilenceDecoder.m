//
//  SilenceDecoder.m
//  Cog
//
//  Created by Christopher Snowhill on 2/8/15.
//  Copyright 2015 __NoWork, LLC__. All rights reserved.
//

#import "SilenceDecoder.h"

#import "Logging.h"

#import "PlaylistController.h"

@implementation SilenceDecoder

enum { sample_rate = 44100 };
enum { channels = 2 };

- (BOOL)open:(id<CogSource>)s {
	[self setSource:s];

	NSString *path = [[[s url] relativeString] substringFromIndex:10];

	int seconds = [path intValue];
	if(!seconds) seconds = 10;

	length = seconds * sample_rate;
	remain = length;

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties {
	return @{@"bitrate": [NSNumber numberWithInt:0],
			 @"sampleRate": [NSNumber numberWithFloat:sample_rate],
			 @"totalFrames": [NSNumber numberWithDouble:length],
			 @"bitsPerSample": [NSNumber numberWithInt:32],
			 @"floatingPoint": [NSNumber numberWithBool:YES],
			 @"channels": [NSNumber numberWithInt:channels],
			 @"seekable": [NSNumber numberWithBool:YES],
			 @"endian": @"host",
			 @"encoding": @"synthesized"};
}

- (int)readAudio:(void *)buf frames:(UInt32)frames {
	int total = frames;

	if(!IsRepeatOneSet()) {
		if(total > remain)
			total = (int)remain;

		remain -= total;
	}

	memset(buf, 0, sizeof(float) * total * channels);

	return total;
}

- (long)seek:(long)frame {
	if(frame > length)
		frame = length;

	remain = length - frame;

	return frame;
}

- (void)close {
}

- (void)setSource:(id<CogSource>)s {
	source = s;
}

- (id<CogSource>)source {
	return source;
}

+ (NSArray *)fileTypes {
	return @[];
}

+ (NSArray *)mimeTypes {
	return @[@"audio/x-silence"];
}

+ (float)priority {
	return 1.0;
}

+ (NSArray *)fileTypeAssociations {
	return @[];
}

@end
