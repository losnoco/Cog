//
//  HVLDecoder.m
//  Hively
//
//  Created by Christopher Snowhill on 10/29/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "HVLDecoder.h"

#import "PlaylistController.h"

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

	void *data = malloc(size);
	[s read:data amount:size];

	tune = hvl_LoadTune(data, (uint32_t)size, 44100, 2);
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

	framesLength = tune->ht_PlayingTime * 44100 / (tune->ht_SpeedMultiplier * 50);
	framesFade = 44100 * 8;
	totalFrames = framesLength + framesFade;

	framesRead = 0;
	framesInBuffer = 0;

	buffer = malloc(sizeof(int32_t) * (44100 / 50) * 2);

	hvl_InitSubsong(tune, trackNumber);

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties {
	return [NSDictionary dictionaryWithObjectsAndKeys:
	                     [NSNumber numberWithInt:0], @"bitrate",
	                     [NSNumber numberWithFloat:44100], @"sampleRate",
	                     [NSNumber numberWithDouble:totalFrames], @"totalFrames",
	                     [NSNumber numberWithInt:32], @"bitsPerSample",
	                     [NSNumber numberWithBool:YES], @"floatingPoint",
	                     [NSNumber numberWithInt:2], @"channels",
	                     [NSNumber numberWithBool:YES], @"seekable",
	                     @"host", @"endian",
	                     @"synthesized", @"encoding",
	                     nil];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames {
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
			for(int i = 0; i < framesToCopy; ++i) {
				outbuffer[0] = buffer[i * 2 + 0] * (1.0f / 16777216.0f);
				outbuffer[1] = buffer[i * 2 + 1] * (1.0f / 16777216.0f);
				outbuffer += 2;
			}
			framesInBuffer -= framesToCopy;
			total += framesToCopy;
			if(framesInBuffer) {
				memcpy(buffer, buffer + framesToCopy * 2, sizeof(int32_t) * framesInBuffer * 2);
				break;
			}
		}
		hvl_DecodeFrame(tune, (int8_t *)buffer, ((int8_t *)buffer) + 4, 8);
		framesInBuffer = 44100 / 50;
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

	return total;
}

- (long)seek:(long)frame {
	if(frame < framesRead) {
		hvl_InitSubsong(tune, trackNumber);
		framesRead = 0;
	}

	while(framesRead < frame) {
		hvl_play_irq(tune);
		framesRead += 44100 / 50;
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
