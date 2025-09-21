//
//  jxsDecoder.m
//  Syntrax-c
//
//  Created by Christopher Snowhill on 03/14/16.
//  Copyright 2016 __NoWork, Inc__. All rights reserved.
//

#import "jxsDecoder.h"

#import "Logging.h"

#import "PlaylistController.h"

@implementation jxsDecoder

- (id)init {
	self = [super init];
	if (self) {
		synSong = NULL;
		synPlayer = NULL;
	}
	return self;
}

- (BOOL)open:(id<CogSource>)s {
	if(![s seek:0 whence:SEEK_END])
		return NO;
	size_t size = [s tell];
	if(size <= 0)
		return NO;
	if(![s seek:0 whence:SEEK_SET])
		return NO;

    void * data = malloc(size);
	if(!data)
		return NO;
	if([s read:data amount:size] != size) {
		free(data);
		return NO;
	}

	if ([[[s url] fragment] length] == 0)
		track_num = 0;
	else
		track_num = [[[s url] fragment] intValue];

	if (jxsfile_readSongMem(data, size, &synSong)) {
		free(data);
		return NO;
	}

    free(data);

	synPlayer = jaytrax_init();
	if (!synPlayer)
		return NO;

	if (!jaytrax_loadSong(synPlayer, synSong))
		return NO;

	sampleRate = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthSampleRate"] doubleValue];
	if(sampleRate < 8000.0) {
		sampleRate = 44100.0;
	} else if(sampleRate > 192000.0) {
		sampleRate = 192000.0;
	}

	double defaultFade = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultFadeSeconds"] doubleValue];
	if(defaultFade < 0.0) {
		defaultFade = 0.0;
	}

	framesLength = jaytrax_getLength(synPlayer, track_num, 2, sampleRate);
	totalFrames = framesLength + ((synPlayer->playFlg) ? sampleRate * defaultFade : 0);

	framesRead = 0;

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (BOOL)decoderInitialize {
	jaytrax_changeSubsong(synPlayer, track_num);

	uint8_t interp = ITP_CUBIC;
	NSString * resampling = [[NSUserDefaults standardUserDefaults] stringForKey:@"resampling"];
	if ([resampling isEqualToString:@"zoh"])
		interp = ITP_NEAREST;
	else if ([resampling isEqualToString:@"blep"])
		interp = ITP_NEAREST;
	else if ([resampling isEqualToString:@"linear"])
		interp = ITP_LINEAR;
	else if ([resampling isEqualToString:@"blam"])
		interp = ITP_LINEAR;
	else if ([resampling isEqualToString:@"cubic"])
		interp = ITP_CUBIC;
	else if ([resampling isEqualToString:@"sinc"])
		interp = ITP_SINC;

	jaytrax_setInterpolation(synPlayer, interp);

	framesRead = 0;

	return YES;
}

- (void)decoderShutdown {
	if (synPlayer) {
		jaytrax_stopSong(synPlayer);
	}

	framesRead = 0;
}

- (NSDictionary *)properties {
	return @{@"bitrate": @(0),
			 @"sampleRate": @(sampleRate),
			 @"totalFrames": @(totalFrames),
			 @"bitsPerSample": @(16),
			 @"floatingPoint": @NO,
			 @"channels": @(2),
			 @"seekable": @YES,
			 @"codec": @"Syntrax",
			 @"endian": @"host"};
}

- (AudioChunk *)readAudio {
	BOOL repeat_one = IsRepeatOneSet();

	if ( !repeat_one && framesRead >= totalFrames )
		return nil;
    
	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];

	double streamTimestamp = (double)(synPlayer->absPosition) / sampleRate;

	if ( !framesRead ) {
		if ( ![self decoderInitialize] )
			return nil;
	}
    
	if ( synPlayer && !synPlayer->playFlg )
		return nil;

	const int frames = 2048;
	int total = 0;
	while ( total < frames ) {
		int framesToRender = 512;
		if ( !repeat_one && framesToRender > totalFrames - framesRead )
			framesToRender = (int)(totalFrames - framesRead);
		if ( framesToRender > frames - total )
			framesToRender = frames - total;

		int16_t * sampleBuf = sampleBuffer + total * 2;
        
		jaytrax_renderChunk(synPlayer, sampleBuf, framesToRender, sampleRate);

		if ( !repeat_one && framesRead + framesToRender > framesLength ) {
			long fadeStart = ( framesLength > framesRead ) ? framesLength : framesRead;
			long fadeEnd = ( framesRead + framesToRender < totalFrames ) ? framesRead + framesToRender : totalFrames;
			const long fadeTotal = totalFrames - framesLength;
			for ( long fadePos = fadeStart; fadePos < fadeEnd; ++fadePos ) {
				const long scale = ( fadeTotal - ( fadePos - framesLength ) );
				const long offset = fadePos - framesRead;
				int16_t * samples = sampleBuf + offset * 2;
				samples[ 0 ] = (int16_t)(samples[ 0 ] * scale / fadeTotal);
				samples[ 1 ] = (int16_t)(samples[ 1 ] * scale / fadeTotal);
			}

			framesToRender = (int)(fadeEnd - framesRead);
		}

		if ( !framesToRender )
			break;

		total += framesToRender;
		framesRead += framesToRender;

		if ( !synPlayer->playFlg )
			break;
	}

	[chunk setStreamTimestamp:streamTimestamp];

	[chunk assignSamples:sampleBuffer frameCount:total];

	return chunk;
}

- (long)seek:(long)frame {
	if ( frame < framesRead || !synPlayer ) {
		[self decoderShutdown];
		if ( ![self decoderInitialize] )
			return -1;
	}

	while ( framesRead < frame ) {
		int frames_todo = INT_MAX;
		if ( frames_todo > frame - framesRead )
			frames_todo = (int)( frame - framesRead );
		jaytrax_renderChunk(synPlayer, NULL, frames_todo, sampleRate);
		framesRead += frames_todo;
	}

	framesRead = frame;

	return frame;
}

- (void)close {
	[self decoderShutdown];

	if (synPlayer) {
		jaytrax_free(synPlayer);
		synPlayer = NULL;
	}

	if (synSong) {
		jxsfile_freeSong(synSong);
		synSong = NULL;
	}
}

- (void)dealloc {
	[self close];
}

+ (NSArray *)fileTypes {
	return @[@"jxs"];
}

+ (NSArray *)mimeTypes {
	return @[@"audio/x-jxs"];
}

+ (float)priority {
    return 1.0;
}

+ (NSArray *)fileTypeAssociations { 
	return @[
		@[@"Syntrax File", @"song.icns", @"jxs"],
	];
}

- (NSDictionary *)metadata { 
	return @{};
}

@end
