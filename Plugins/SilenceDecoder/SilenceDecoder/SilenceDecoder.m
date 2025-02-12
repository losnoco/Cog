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
	seconds = 0.0;
	
	buffer = (float *) calloc(sizeof(float), 1024 * channels);
	if(!buffer) {
		ALog(@"Out of memory!");
		return NO;
	}

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties {
	return @{ @"bitrate": @(0),
		      @"sampleRate": @(sample_rate),
		      @"totalFrames": @(length),
		      @"bitsPerSample": @(32),
		      @"floatingPoint": @(YES),
		      @"channels": @(channels),
		      @"seekable": @(YES),
		      @"endian": @"host",
		      @"encoding": @"synthesized" };
}

- (NSDictionary *)metadata {
	return @{};
}

- (AudioChunk *)readAudio {
	int frames = 1024;

	if(!IsRepeatOneSet()) {
		if(frames > remain)
			frames = (int)remain;

		remain -= frames;
	}

	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];
	[chunk setStreamTimestamp:seconds];
	[chunk assignSamples:buffer frameCount:frames];

	seconds += [chunk duration];

	return chunk;
}

- (long)seek:(long)frame {
	if(frame > length)
		frame = length;

	remain = length - frame;

	seconds = (double)(frame) / sample_rate;

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

- (BOOL)isSilence {
	return YES;
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
