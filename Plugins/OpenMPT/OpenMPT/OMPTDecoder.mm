//
//  OMPTDecoder.m
//  OpenMPT
//
//  Created by Christopher Snowhill on 1/4/18.
//  Copyright 2018 __LoSnoCo__. All rights reserved.
//

#import "OMPTDecoder.h"

#import "Logging.h"

#import "PlaylistController.h"

static void g_push_archive_extensions(std::vector<std::string> &list) {
	try {
		static std::string archive_extensions[] = {
			"mdz", "mdr", "s3z", "xmz", "itz", "mptmz"
		};
		for(unsigned i = 0, j = 6; i < j; ++i) {
			if(list.empty() || std::find(list.begin(), list.end(), archive_extensions[i]) == list.end())
				list.push_back(archive_extensions[i]);
		}
	} catch (std::exception &e) {
		ALog(@"Exception caught pushing archive extensions for OpenMPT: %s", e.what());
	}
}

@implementation OMPTDecoder

- (id)init {
	self = [super init];
	if(self) {
		mod = NULL;
	}
	return self;
}

- (BOOL)open:(id<CogSource>)s {
	[self setSource:s];

	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	sampleRate = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthSampleRate"] doubleValue];
	if(sampleRate < 8000.0) {
		sampleRate = 44100.0;
	} else if(sampleRate > 192000.0) {
		sampleRate = 192000.0;
	}

	try {
		std::vector<char> data(static_cast<std::size_t>(size));

		[source read:data.data() amount:size];

		int track_num;
		if([[source.url fragment] length] == 0)
			track_num = 0;
		else
			track_num = [[source.url fragment] intValue];

		int interp = 8;
		NSString *resampling = [[NSUserDefaults standardUserDefaults] stringForKey:@"resampling"];
		if([resampling isEqualToString:@"zoh"])
			interp = 1;
		else if([resampling isEqualToString:@"blep"])
			interp = 1;
		else if([resampling isEqualToString:@"linear"])
			interp = 2;
		else if([resampling isEqualToString:@"blam"])
			interp = 2;
		else if([resampling isEqualToString:@"cubic"])
			interp = 4;
		else if([resampling isEqualToString:@"sinc"])
			interp = 8;
		std::map<std::string, std::string> ctls;
		ctls["seek.sync_samples"] = "1";
		mod = new openmpt::module(data, std::clog, ctls);

		mod->select_subsong(track_num);

		length = mod->get_duration_seconds() * sampleRate;

		mod->set_repeat_count(IsRepeatOneSet() ? -1 : 0);
		mod->set_render_param(openmpt::module::RENDER_MASTERGAIN_MILLIBEL, 0);
		mod->set_render_param(openmpt::module::RENDER_STEREOSEPARATION_PERCENT, 100);
		mod->set_render_param(openmpt::module::RENDER_INTERPOLATIONFILTER_LENGTH, interp);
		mod->set_render_param(openmpt::module::RENDER_VOLUMERAMPING_STRENGTH, -1);
		mod->ctl_set_boolean("render.resampler.emulate_amiga", true);
	} catch(std::exception &e) {
		ALog(@"Exception caught opening module with OpenMPT: %s", e.what());
		return NO;
	}

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties {
	return @{ @"bitrate": @(0),
		      @"sampleRate": @(sampleRate),
		      @"totalFrames": @(length),
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
	try {
		mod->set_repeat_count(IsRepeatOneSet() ? -1 : 0);

		double streamTimestamp = mod->get_position_seconds();

		int frames = 1024;
		float buffer[frames * 2];
		void *buf = (void *)buffer;

		int total = 0;
		while(total < frames) {
			int framesToRender = 1024;
			if(framesToRender > frames)
				framesToRender = frames;

			std::size_t count = mod->read_interleaved_stereo(sampleRate, framesToRender, ((float *)buf) + total * 2);
			if(count == 0)
				break;

			total += count;

			if(count < framesToRender)
				break;
		}

		id audioChunkClass = NSClassFromString(@"AudioChunk");
		AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];
		[chunk setStreamTimestamp:streamTimestamp];
		[chunk assignSamples:buffer frameCount:total];

		return chunk;
	} catch (std::exception &e) {
		ALog(@"Exception caught while playing with OpenMPT: %s", e.what());
		return nil;
	}
}

- (long)seek:(long)frame {
	try {
		mod->set_position_seconds(frame / sampleRate);
	} catch (std::exception &e) {
		ALog(@"Exception caught while seeking with OpenMPT: %s", e.what());
		return -1;
	}

	return frame;
}

- (void)cleanUp {
	try {
		delete mod;
	} catch (std::exception &e) {
		ALog(@"Exception caught while deleting instance of OpenMPT: %s", e.what());
	}
	mod = NULL;
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
	try {
		std::vector<std::string> extensions = openmpt::get_supported_extensions();
		g_push_archive_extensions(extensions);
		NSMutableArray *array = [NSMutableArray array];

		for(std::vector<std::string>::iterator ext = extensions.begin(); ext != extensions.end(); ++ext) {
			[array addObject:[NSString stringWithUTF8String:ext->c_str()]];
		}

		return [NSArray arrayWithArray:array];
	} catch (std::exception &e) {
		ALog(@"Exception caught while enumerating OpenMPT file extensions: %s", e.what());
		return @[];
	}
}

+ (NSArray *)mimeTypes {
	return @[@"audio/x-it", @"audio/x-xm", @"audio/x-s3m", @"audio/x-mod"];
}

+ (float)priority {
	return 1.0;
}

+ (NSArray *)fileTypeAssociations {
	NSMutableArray *ret = [NSMutableArray new];
	[ret addObject:@"libOpenMPT Module Files"];
	[ret addObject:@"song.icns"];
	[ret addObjectsFromArray:[self fileTypes]];

	return @[[NSArray arrayWithArray:ret]];
}

@end
