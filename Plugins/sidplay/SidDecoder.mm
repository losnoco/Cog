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

@interface sid_file_object : NSObject {
	size_t refCount;
	NSString *path;
	NSData *data;
}
@property size_t refCount;
@property NSString *path;
@property NSData *data;
@end

@implementation sid_file_object
@synthesize refCount;
@synthesize path;
@synthesize data;
@end

@interface sid_file_container : NSObject {
	NSLock *lock;
	NSMutableDictionary *list;
}
+ (sid_file_container *)instance;
- (void)add_hint:(NSString *)path source:(id)source;
- (void)remove_hint:(NSString *)path;
- (BOOL)try_hint:(NSString *)path data:(NSData **)data;
@end

@implementation sid_file_container
+ (sid_file_container *)instance {
	static sid_file_container *instance;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		instance = [[self alloc] init];
	});
	return instance;
}
- (sid_file_container *)init {
	if((self = [super init])) {
		lock = [[NSLock alloc] init];
		list = [[NSMutableDictionary alloc] init];
	}
	return self;
}
- (void)add_hint:(NSString *)path source:(id)source {
	[lock lock];
	sid_file_object *obj = [list objectForKey:path];
	if(obj) {
		obj.refCount += 1;
		[lock unlock];
		return;
	}
	[lock unlock];

	obj = [[sid_file_object alloc] init];

	obj.refCount = 1;

	if(![source seekable])
		return;

	[source seek:0 whence:SEEK_END];
	size_t fileSize = [source tell];
	if(!fileSize)
		return;

	void *dataBytes = malloc(fileSize);
	if(!dataBytes)
		return;

	[source seek:0 whence:SEEK_SET];
	[source read:dataBytes amount:fileSize];

	NSData *data = [NSData dataWithBytes:dataBytes length:fileSize];
	free(dataBytes);

	obj.path = path;
	obj.data = data;

	[lock lock];
	[list setObject:obj forKey:path];
	[lock unlock];
}
- (void)remove_hint:(NSString *)path {
	[lock lock];
	sid_file_object *obj = [list objectForKey:path];
	if(obj.refCount <= 1) {
		[list removeObjectForKey:path];
	} else {
		obj.refCount--;
	}
	[lock unlock];
}
- (BOOL)try_hint:(NSString *)path data:(NSData **)data {
	sid_file_object *obj;
	[lock lock];
	obj = [list objectForKey:path];
	[lock unlock];
	if(obj) {
		*data = obj.data;
		return YES;
	} else {
		return NO;
	}
}
@end

static void sidTuneLoader(const char *fileName, std::vector<uint8_t> &bufferRef) {
	NSData *hintData = nil;

	if(![[sid_file_container instance] try_hint:[NSString stringWithUTF8String:fileName] data:&hintData]) {
		NSString *urlString = [NSString stringWithUTF8String:fileName];
		NSURL *url = [NSURL URLWithDataRepresentation:[urlString dataUsingEncoding:NSUTF8StringEncoding] relativeToURL:nil];

		id audioSourceClass = NSClassFromString(@"AudioSource");
		id<CogSource> source = [audioSourceClass audioSourceForURL:url];

		if(![source open:url])
			return;

		if(![source seekable])
			return;

		[source seek:0 whence:SEEK_END];
		long fileSize = [source tell];
		[source seek:0 whence:SEEK_SET];

		bufferRef.resize(fileSize);

		[source read:&bufferRef[0] amount:fileSize];

		[source close];
	} else {
		bufferRef.resize([hintData length]);
		memcpy(&bufferRef[0], [hintData bytes], [hintData length]);
	}
}

@implementation SidDecoder

// Need this static initializer to create the static global tables that sidplayfp doesn't really lock access to
+ (void)initialize {
	try {
		ReSIDfpBuilder *builder = new ReSIDfpBuilder("ReSIDfp");

		if(builder) {
			builder->create(1);
			if(builder->getStatus()) {
				builder->filter(true);
				builder->filter6581Curve(0.5);
				builder->filter8580Curve(0.5);
			}
			
			delete builder;
		}
	} catch (std::exception &e) {
		ALog(@"Exception caught while doing one-time initialization of SID player: %s", e.what());
	}
}

- (BOOL)open:(id<CogSource>)s {
	if(![s seekable])
		return NO;

	[self setSource:s];

	sampleRate = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthSampleRate"] doubleValue];
	if(sampleRate < 8000.0) {
		sampleRate = 44100.0;
	} else if(sampleRate > 192000.0) {
		sampleRate = 192000.0;
	}

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

	try {
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

		double defaultLength = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultSeconds"] doubleValue];

		length = (int)ceil(sampleRate * defaultLength);

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
		conf.frequency = (int)ceil(sampleRate);
		conf.sidEmulation = builder;
		conf.playback = SidConfig::MONO;
		if(tuneInfo && (tuneInfo->sidChips() > 1))
			conf.playback = SidConfig::STEREO;
		if(!engine->config(conf))
			return NO;

		if(conf.playback == SidConfig::STEREO) {
			n_channels = 2;
		}
	} catch (std::exception &e) {
		ALog(@"Exception caught loading SID file: %s", e.what());
		return NO;
	}

	double defaultFade = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultFadeSeconds"] doubleValue];
	if(defaultFade < 0.0) {
		defaultFade = 0.0;
	}

	renderedTotal = 0;
	fadeTotal = fadeRemain = (int)ceil(sampleRate * defaultFade);

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties {
	return @{ @"bitrate": @(0),
		      @"sampleRate": @(sampleRate),
		      @"totalFrames": @(length),
		      @"bitsPerSample": @(16),
		      @"floatingPoint": @(NO),
		      @"channels": @(n_channels),
		      @"seekable": @(YES),
		      @"endian": @"host",
		      @"encoding": @"synthesized" };
}

- (NSDictionary *)metadata {
	return @{};
}

- (AudioChunk *)readAudio {
	int total = 0;
	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];

	double streamTimestamp = (double)(renderedTotal) / sampleRate;

	int16_t buffer[1024 * n_channels];

	int framesToRender = 1024;
	int rendered = 0;
	try {
		rendered = engine->play(buffer, framesToRender * n_channels) / n_channels;
	} catch (std::exception &e) {
		ALog(@"Exception caught while playing SID file: %s", e.what());
		return nil;
	}

	if(rendered <= 0)
		return nil;

	if(n_channels == 2) {
		for(int i = 0, j = rendered * 2; i < j; i += 2) {
			int16_t *sample = buffer + total * 2 + i;
			int mid = (int)(sample[0] + sample[1]) / 2;
			int side = (int)(sample[0] - sample[1]) / 4;
			sample[0] = mid + side;
			sample[1] = mid - side;
		}
	}

	if(!IsRepeatOneSet() && renderedTotal >= length) {
		int16_t *sampleBuf = buffer;
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

	[chunk setStreamTimestamp:streamTimestamp];

	[chunk assignSamples:buffer frameCount:rendered];

	return chunk;
}

- (long)seek:(long)frame {
	if(frame < renderedTotal) {
		renderedTotal = 0;
		try {
			engine->load(tune);
		} catch (std::exception &e) {
			ALog(@"Exception caught reloading SID tune for seeking: %s", e.what());
			return -1;
		}
	}

	int16_t sampleBuffer[1024 * 2];

	long remain = (frame - renderedTotal) % 32;
	frame /= 32;
	renderedTotal /= 32;

	try {
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
	} catch (std::exception &e) {
		ALog(@"Exception caught while brute force seeking SID file: %s", e.what());
		return -1;
	}

	return renderedTotal;
}

- (void)cleanUp {
	try {
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
	} catch (std::exception &e) {
		ALog(@"Exception caught while deleting SID player instances: %s", e.what());
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
