//
//  BASSDecoder.m
//  Cog
//
//  Created by Christopher Snowhill on 7/03/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#include <mutex>

#import "BASSDecoder.h"

#import "umx.h"

#import "Logging.h"

#import "PlaylistController.h"

static class Bass_Initializer {
	std::mutex lock;

	bool initialized;

	public:
	Bass_Initializer()
	: initialized(false) {
	}

	~Bass_Initializer() {
		if(initialized) {
			BASS_Free();
		}
	}

	bool check_initialized() {
		std::lock_guard<std::mutex> lock(this->lock);
		return initialized;
	}

	bool initialize() {
		std::lock_guard<std::mutex> lock(this->lock);
		if(!initialized) {
			BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
			initialized = !!BASS_Init(0, 44100, 0, NULL, NULL);
		}
		return initialized;
	}
} g_initializer;

static void SyncProc(HSYNC handle, DWORD channel, DWORD data, void *user) {
	BASSDecoder *decoder = (__bridge BASSDecoder *)user;
	[decoder sync];
}

@implementation BASSDecoder

+ (void)initialize {
	if(self == [BASSDecoder class]) {
		g_initializer.initialize();
	}
}

- (void)sync {
	++loops;
}

- (BOOL)open:(id<CogSource>)s {
	[self setSource:s];

	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	void *data = malloc(size);
	[source read:data amount:size];

	void *try_data = unpackUmx(data, &size);
	if(try_data) {
		free(data);
		data = try_data;
	}

	if(size < 4 || (memcmp(data, "mo3", 3) != 0 && memcmp(data, "IMPM", 4) != 0)) {
		ALog(@"BASS was passed a non-IT module");
		free(data);
		return NO;
	}

	NSURL *url = [s url];
	int track_num;
	if([[url fragment] length] == 0)
		track_num = 0;
	else
		track_num = [[url fragment] intValue];

	int resampling_int = -1;
	NSString *resampling = [[NSUserDefaults standardUserDefaults] stringForKey:@"resampling"];
	if([resampling isEqualToString:@"zoh"])
		resampling_int = 0;
	else if([resampling isEqualToString:@"blep"])
		resampling_int = 0;
	else if([resampling isEqualToString:@"linear"])
		resampling_int = 1;
	else if([resampling isEqualToString:@"blam"])
		resampling_int = 1;
	else if([resampling isEqualToString:@"cubic"])
		resampling_int = 1;
	else if([resampling isEqualToString:@"sinc"])
		resampling_int = 2;

	music = BASS_MusicLoad(1, data, 0, (DWORD)size, BASS_SAMPLE_FLOAT | BASS_SAMPLE_LOOP | BASS_MUSIC_STOPBACK | BASS_MUSIC_RAMP | BASS_MUSIC_PRESCAN | BASS_MUSIC_DECODE | (resampling_int == 0 ? BASS_MUSIC_NONINTER : ((resampling_int == 2) ? BASS_MUSIC_SINCINTER : 0)), 44100);

	if(!music) {
		ALog(@"BASS MusicLoad error: %u", BASS_ErrorGetCode());
		free(data);
		return NO;
	}

	BASS_ChannelSetPosition(music, MAKELONG(track_num, 0), BASS_POS_MUSIC_ORDER);

	length = BASS_ChannelGetLength(music, BASS_POS_BYTE) / (sizeof(float) * 2);

	BASS_ChannelSetSync(music, BASS_SYNC_END, 0, SyncProc, (__bridge void *)self);

	loops = 0;
	fadeTotal = fadeRemain = 44100 * 8;

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties {
	return [NSDictionary dictionaryWithObjectsAndKeys:
	                     [NSNumber numberWithInt:0], @"bitrate",
	                     [NSNumber numberWithFloat:44100], @"sampleRate",
	                     [NSNumber numberWithDouble:length], @"totalFrames",
	                     [NSNumber numberWithInt:32], @"bitsPerSample", // Samples are short
	                     [NSNumber numberWithBool:YES], @"floatingPoint",
	                     [NSNumber numberWithInt:2], @"channels", // output from gme_play is in stereo
	                     [NSNumber numberWithBool:[source seekable]], @"seekable",
	                     @"host", @"endian",
	                     nil];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames {
	int total = 0;
	while(total < frames) {
		float *fbuf = (float *)buf + total * 2;
		int framesToRender = 1024;
		if(framesToRender > frames)
			framesToRender = frames;
		int rendered = BASS_ChannelGetData(music, fbuf, (framesToRender * 2 * sizeof(float)) | BASS_DATA_FLOAT);
		if(rendered < 0) {
			ALog(@"BASS ChannelGetData error: %u", BASS_ErrorGetCode());
			return 0;
		}
		rendered /= sizeof(float) * 2;

		if(rendered <= 0)
			break;

		if(!IsRepeatOneSet() && loops >= 2) {
			float *sampleBuf = (float *)buf + total * 2;
			long fadeEnd = fadeRemain - rendered;
			if(fadeEnd < 0)
				fadeEnd = 0;
			float fadePosf = (float)fadeRemain / fadeTotal;
			const float fadeStep = 1.0 / fadeTotal;
			for(long fadePos = fadeRemain; fadePos > fadeEnd; --fadePos, fadePosf -= fadeStep) {
				long offset = (fadeRemain - fadePos) * 2;
				float sampleLeft = sampleBuf[offset + 0];
				float sampleRight = sampleBuf[offset + 1];
				sampleLeft *= fadePosf;
				sampleRight *= fadePosf;
				sampleBuf[offset + 0] = sampleLeft;
				sampleBuf[offset + 1] = sampleRight;
			}
			rendered = (int)(fadeRemain - fadeEnd);
			fadeRemain = fadeEnd;
		}

		total += rendered;

		if(rendered < framesToRender)
			break;
	}

	return total;
}

- (long)seek:(long)frame {
	long pos = BASS_ChannelGetPosition(music, BASS_POS_BYTE) / (sizeof(float) * 2);

	if(frame < pos) {
		// Reset. Dumb cannot seek backwards. It's dumb.
		[self cleanUp];

		[source seek:0 whence:SEEK_SET];
		[self open:source];

		pos = 0;
	}

	BASS_ChannelSetPosition(music, frame * (sizeof(float) * 2), BASS_POS_BYTE | BASS_POS_DECODETO);

	return frame;
}

- (void)cleanUp {
	if(music) {
		BASS_MusicFree(music);
		music = 0;
	}
}

- (void)close {
	[self cleanUp];

	if(source) {
		[source close];
		[self setSource:nil];
	}
}

- (void)dealloc {
	[self close];
}

- (void)setSource:(id<CogSource>)s {
	source = s;
}

- (id<CogSource>)source {
	return source;
}

+ (NSArray *)fileTypes {
	return [NSArray arrayWithObjects:@"it", @"itz", @"umx", @"mo3", nil];
}

+ (NSArray *)mimeTypes {
	return [NSArray arrayWithObjects:@"audio/x-it", nil];
}

+ (float)priority {
	return 1.5;
}

@end
