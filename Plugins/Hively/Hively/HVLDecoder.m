//
//  HVLDecoder.m
//  Hively
//
//  Created by Christopher Snowhill on 10/29/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "HVLDecoder.h"

#import "PlaylistController.h"

#import <Accelerate/Accelerate.h>

static void oneTimeInit(void) {
	static BOOL initialized = NO;
	if(!initialized) {
		hvl_InitReplayer();
		initialized = YES;
	}
}

@implementation HVLDecoder

+ (void)initialize {
	if([self class] == [HVLDecoder class])
		oneTimeInit();
}

- (BOOL)open:(id<CogSource>)s {
	[s seek:0 whence:SEEK_END];
	long size = [s tell];
	[s seek:0 whence:SEEK_SET];

	if(size > UINT_MAX)
		return NO;

	sampleRate = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthSampleRate"] doubleValue];
	if(sampleRate < 8000.0) {
		sampleRate = 44100.0;
	} else if(sampleRate > 192000.0) {
		sampleRate = 192000.0;
	}

	void *data = malloc(size);
	[s read:data amount:size];

	tune = hvl_LoadTune(data, (uint32_t)size, (int)sampleRate, 2);
	free(data);
	if(!tune)
		return NO;

	unsigned long safety = 2 * 60 * 60 * 50 * tune->ht_SpeedMultiplier;

	NSURL *url = [s url];
	if([[url fragment] length] == 0)
		trackNumber = 0;
	else
		trackNumber = [[url fragment] intValue];

	hvl_InitSubsong(tune, trackNumber);

	unsigned long loops = 0;

	while(loops < 2 && safety) {
		while(!tune->ht_SongEndReached && safety) {
			hvl_play_irq(tune);
			--safety;
		}
		tune->ht_SongEndReached = 0;
		++loops;
	}

	double defaultFade = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultFadeSeconds"] doubleValue];
	if(defaultFade < 0.0) {
		defaultFade = 0.0;
	}

	framesLength = tune->ht_PlayingTime * sampleRate / (tune->ht_SpeedMultiplier * 50);
	framesFade = (int)ceil(sampleRate * defaultFade);
	totalFrames = framesLength + framesFade;

	framesRead = 0;
	framesInBuffer = 0;

	buffer = malloc(sizeof(int32_t) * ((int)ceil(sampleRate) / 50) * 2);

	hvl_InitSubsong(tune, trackNumber);

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties {
	return @{ @"bitrate": @(0),
		      @"sampleRate": @(sampleRate),
		      @"totalFrames": @(totalFrames),
		      @"bitsPerSample": @(32),
		      @"floatingPoint": @(YES),
		      @"channels": @(2),
		      @"seekable": @(YES),
		      @"endian": @"host",
		      @"encoding": @"synthesized" };
}

- (NSDictionary *)metadata {
	return @{};
}

- (AudioChunk *)readAudio {
	int frames = 1024;
	void *buf = (void *)sampleBuffer;

	BOOL repeatone = IsRepeatOneSet();

	if(!repeatone && framesRead >= totalFrames)
		return 0;

	int total = 0;
	while(total < frames) {
		if(framesInBuffer) {
			float *outbuffer = ((float *)buf) + total * 2;
			int framesToCopy = frames - total;
			if(framesToCopy > framesInBuffer)
				framesToCopy = (int)framesInBuffer;
			vDSP_vflt32(&buffer[0], 1, &outbuffer[0], 1, framesToCopy * 2);
			float scale = 16777216.0f;
			vDSP_vsdiv(&outbuffer[0], 1, &scale, &outbuffer[0], 1, framesToCopy * 2);
			framesInBuffer -= framesToCopy;
			total += framesToCopy;
			if(framesInBuffer) {
				memcpy(buffer, buffer + framesToCopy * 2, sizeof(int32_t) * framesInBuffer * 2);
				break;
			}
		}
		hvl_DecodeFrame(tune, (int8_t *)buffer, ((int8_t *)buffer) + 4, 8);
		framesInBuffer = (int)ceil(sampleRate / 50);
	}

	if(!repeatone && framesRead + total > framesLength) {
		long fadeStart = (framesLength > framesRead) ? framesLength : framesRead;
		long fadeEnd = (framesRead + total) > totalFrames ? totalFrames : (framesRead + total);
		long fadePos;

		float *buff = (float *)buf;

		float fadeScale = (float)(totalFrames - fadeStart) / (float)framesFade;
		float fadeStep = 1.0 / (float)framesFade;
		for(fadePos = fadeStart; fadePos < fadeEnd; ++fadePos) {
			buff[0] *= fadeScale;
			buff[1] *= fadeScale;
			buff += 2;
			fadeScale -= fadeStep;
			if(fadeScale <= 0.0) break;
		}
		total = (int)(fadePos - fadeStart);
	}

	framesRead += total;

	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];
	[chunk assignSamples:sampleBuffer frameCount:total];

	return chunk;
}

- (long)seek:(long)frame {
	if(frame < framesRead) {
		hvl_InitSubsong(tune, trackNumber);
		framesRead = 0;
	}

	while(framesRead < frame) {
		hvl_play_irq(tune);
		framesRead += (int)ceil(sampleRate / 50);
	}

	return framesRead;
}

- (void)close {
	if(tune) {
		hvl_FreeTune(tune);
		tune = NULL;
	}

	if(buffer) {
		free(buffer);
		buffer = NULL;
	}
}

- (void)dealloc {
	[self close];
}

+ (NSArray *)fileTypes {
	return @[@"hvl", @"ahx"];
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"Hively Tracker File", @"song.icns", @"hvl"],
		@[@"Abyss' Highest eXperience File", @"song.icns", @"ahx"]
	];
}

+ (NSArray *)mimeTypes {
	return nil;
}

+ (float)priority {
	return 1.0;
}

@end
