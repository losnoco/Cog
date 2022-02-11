//
//  SidDecoder.mm
//  sidplay
//
//  Created by Christopher Snowhill on 12/8/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import "SidDecoder.h"

#import <sidplayfp/residfp.h>

#import "roms.hpp"

#import "Logging.h"

#import "PlaylistController.h"

#include <vector>

static const char *extListEmpty[] = { NULL };
static const char *extListStr[] = { ".str", NULL };

@interface sid_file_container : NSObject {
	NSLock *lock;
	NSMutableDictionary *list;
}
+ (sid_file_container *)instance;
- (void)add_hint:(NSString *)path source:(id)source;
- (void)remove_hint:(NSString *)path;
- (BOOL)try_hint:(NSString *)path source:(id *)source;
@end

@implementation sid_file_container
+ (sid_file_container *)instance {
	static sid_file_container *instance;

	@synchronized(self) {
		if(!instance) {
			instance = [[self alloc] init];
		}
	}

	return instance;
}
- (sid_file_container *)init {
	if((self = [super init])) {
		lock = [[NSLock alloc] init];
		list = [[NSMutableDictionary alloc] initWithCapacity:0];
	}
	return self;
}
- (void)add_hint:(NSString *)path source:(id)source {
	[lock lock];
	[list setObject:source forKey:path];
	[lock unlock];
}
- (void)remove_hint:(NSString *)path {
	[lock lock];
	[list removeObjectForKey:path];
	[lock unlock];
}
- (BOOL)try_hint:(NSString *)path source:(id *)source {
	[lock lock];
	*source = [list objectForKey:path];
	[lock unlock];
	if(*source) {
		[*source seek:0 whence:0];
		return YES;
	} else {
		return NO;
	}
}
@end

static void sidTuneLoader(const char *fileName, std::vector<uint8_t> &bufferRef) {
	id<CogSource> source;
	BOOL usedHint = YES;
	if(![[sid_file_container instance] try_hint:[NSString stringWithUTF8String:fileName] source:&source]) {
		usedHint = NO;

		NSString *urlString = [NSString stringWithUTF8String:fileName];
		NSURL *url = [NSURL URLWithDataRepresentation:[urlString dataUsingEncoding:NSUTF8StringEncoding] relativeToURL:nil];

		id audioSourceClass = NSClassFromString(@"AudioSource");
		source = [audioSourceClass audioSourceForURL:url];

		if(![source open:url])
			return;

		if(![source seekable])
			return;
	}

	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	bufferRef.resize(size);

	[source read:&bufferRef[0] amount:size];

	if(!usedHint)
		[source close];
}

@implementation SidDecoder

- (BOOL)open:(id<CogSource>)s {
	if(![s seekable])
		return NO;

	[self setSource:s];

	NSString *path = [[s url] absoluteString];
	NSRange fragmentRange = [path rangeOfString:@"#" options:NSBackwardsSearch];
	if(fragmentRange.location != NSNotFound) {
		path = [path substringToIndex:fragmentRange.location];
	}

	currentUrl = [path stringByRemovingPercentEncoding];

	[[sid_file_container instance] add_hint:currentUrl source:s];
	hintAdded = YES;

	NSString *extension = [[s url] pathExtension];

	const char **extList = [extension isEqualToString:@"mus"] ? extListStr : extListEmpty;

	tune = new SidTune(sidTuneLoader, [currentUrl UTF8String], extList, true);

	if(!tune->getStatus())
		return NO;

	NSURL *url = [s url];
	int track_num;
	if([[url fragment] length] == 0)
		track_num = 1;
	else
		track_num = [[url fragment] intValue];

	n_channels = 1;

	length = 3 * 60 * 44100;

	tune->selectSong(track_num);

	engine = new sidplayfp;

	engine->setRoms(kernel, basic, chargen);

	if(!engine->load(tune))
		return NO;

	ReSIDfpBuilder *_builder = new ReSIDfpBuilder("ReSIDfp");
	builder = _builder;

	if(_builder) {
		_builder->create((engine->info()).maxsids());
		if(_builder->getStatus()) {
			_builder->filter(true);
			_builder->filter6581Curve(0.5);
			_builder->filter8580Curve(0.5);
		}
		if(!_builder->getStatus())
			return NO;
	} else
		return NO;

	const SidTuneInfo *tuneInfo = tune->getInfo();

	SidConfig conf = engine->config();
	conf.frequency = 44100;
	conf.sidEmulation = builder;
	conf.playback = SidConfig::MONO;
	if(tuneInfo && (tuneInfo->sidChips() > 1))
		conf.playback = SidConfig::STEREO;
	if(!engine->config(conf))
		return NO;

	if(conf.playback == SidConfig::STEREO) {
		n_channels = 2;
	}

	renderedTotal = 0;
	fadeTotal = fadeRemain = 44100 * 8;

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties {
	return @{@"bitrate": [NSNumber numberWithInt:0],
			 @"sampleRate": [NSNumber numberWithFloat:44100],
			 @"totalFrames": [NSNumber numberWithDouble:length],
			 @"bitsPerSample": [NSNumber numberWithInt:16],
			 @"floatingPoint": [NSNumber numberWithBool:NO],
			 @"channels": [NSNumber numberWithInt:n_channels],
			 @"seekable": [NSNumber numberWithBool:YES],
			 @"endian": @"host",
			 @"encoding": @"synthesized"};
}

- (NSDictionary *)metadata {
	return @{};
}

- (int)readAudio:(void *)buf frames:(UInt32)frames {
	int total = 0;
	int16_t *sampleBuffer = (int16_t *)buf;
	while(total < frames) {
		int framesToRender = 1024;
		if(framesToRender > frames)
			framesToRender = frames;
		int rendered = engine->play(sampleBuffer + total * n_channels, framesToRender * n_channels) / n_channels;

		if(rendered <= 0)
			break;

		if(n_channels == 2) {
			for(int i = 0, j = rendered * 2; i < j; i += 2) {
				int16_t *sample = sampleBuffer + total * 2 + i;
				int mid = (int)(sample[0] + sample[1]) / 2;
				int side = (int)(sample[0] - sample[1]) / 4;
				sample[0] = mid + side;
				sample[1] = mid - side;
			}
		}

		renderedTotal += rendered;

		if(!IsRepeatOneSet() && renderedTotal >= length) {
			int16_t *sampleBuf = (int16_t *)buf + total * n_channels;
			long fadeEnd = fadeRemain - rendered;
			if(fadeEnd < 0)
				fadeEnd = 0;
			float fadePosf = (float)fadeRemain / (float)fadeTotal;
			const float fadeStep = 1.0f / (float)fadeTotal;
			for(long fadePos = fadeRemain; fadePos > fadeEnd; --fadePos, fadePosf -= fadeStep) {
				long offset = (fadeRemain - fadePos) * n_channels;
				float sampleLeft = sampleBuf[offset + 0];
				sampleLeft *= fadePosf;
				sampleBuf[offset + 0] = (int16_t)sampleLeft;
				if(n_channels == 2) {
					float sampleRight = sampleBuf[offset + 1];
					sampleRight *= fadePosf;
					sampleBuf[offset + 1] = (int16_t)sampleRight;
				}
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
	if(frame < renderedTotal) {
		engine->load(tune);
		renderedTotal = 0;
	}

	int16_t sampleBuffer[1024 * 2];

	long remain = (frame - renderedTotal) % 32;
	frame /= 32;
	renderedTotal /= 32;
	engine->fastForward(100 * 32);

	while(renderedTotal < frame) {
		long todo = frame - renderedTotal;
		if(todo > 1024)
			todo = 1024;
		int done = engine->play(sampleBuffer, (uint_least32_t)(todo * n_channels)) / n_channels;

		if(done < todo) {
			if(engine->error())
				return -1;

			renderedTotal = length;
			break;
		}

		renderedTotal += todo;
	}

	renderedTotal *= 32;
	engine->fastForward(100);

	if(remain)
		renderedTotal += engine->play(sampleBuffer, (uint_least32_t)(remain * n_channels)) / n_channels;

	return renderedTotal;
}

- (void)cleanUp {
	if(builder) {
		delete builder;
		builder = NULL;
	}

	if(engine) {
		delete engine;
		engine = NULL;
	}

	if(tune) {
		delete tune;
		tune = NULL;
	}

	source = nil;
	if(hintAdded) {
		[[sid_file_container instance] remove_hint:currentUrl];
		hintAdded = NO;
	}
	currentUrl = nil;
}

- (void)close {
	[self cleanUp];
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
	return @[@"sid", @"mus"];
}

+ (NSArray *)mimeTypes {
	return nil;
}

+ (float)priority {
	return 0.5;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"SID File", @"vg.icns", @"sid"],
		@[@"SID MUS File", @"song.icns", @"mus"]
	];
}

@end
