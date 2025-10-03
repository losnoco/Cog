//
//  AdPlugDecoder.m
//  AdPlug
//
//  Created by Christopher Snowhill on 1/27/18.
//  Copyright 2018 __LoSnoCo__. All rights reserved.
//

#import "AdPlugDecoder.h"

#import <libAdPlug/nemuopl.h>

#import "fileprovider.h"

#import "Logging.h"

#import "PlaylistController.h"

@implementation AdPlugDecoder

static CAdPlugDatabase *g_database = NULL;

+ (void)initialize {
	if(!g_database) {
		NSURL *dbUrl = [[NSBundle bundleWithIdentifier:@"net.kode54.AdPlug"] URLForResource:@"adplug" withExtension:@"db"];

		NSString *dbPath = [dbUrl path];

		if(dbPath) {
			g_database = new CAdPlugDatabase;
			g_database->load([dbPath UTF8String]);

			CAdPlug::set_database(g_database);
		}
	}
}

- (id)init {
	self = [super init];
	if(self) {
		m_player = NULL;
		m_emu = NULL;
	}
	return self;
}

- (BOOL)open:(id<CogSource>)s {
	[self setSource:s];

	sampleRate = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthSampleRate"] doubleValue];
	if(sampleRate < 8000.0) {
		sampleRate = 44100.0;
	} else if(sampleRate > 192000.0) {
		sampleRate = 192000.0;
	}

	m_emu = new CNemuopl(sampleRate);

	NSString *path = [[s url] absoluteString];
	NSRange fragmentRange = [path rangeOfString:@"#" options:NSBackwardsSearch];
	if(fragmentRange.location != NSNotFound) {
		path = [path substringToIndex:fragmentRange.location];
	}

	std::string _path = [[path stringByRemovingPercentEncoding] UTF8String];
	m_player = CAdPlug::factory(_path, m_emu, CAdPlug::players, CProvider_cog(_path, source));

	if(!m_player)
		return 0;

	if([[source.url fragment] length] == 0)
		subsong = 0;
	else
		subsong = [[source.url fragment] intValue];

	samples_todo = 0;

	length = m_player->songlength(subsong) * sampleRate / 1000.0;
	current_pos = 0;

	m_player->rewind(subsong);

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties {
	return @{ @"bitrate": @(0),
		      @"sampleRate": @(sampleRate),
		      @"totalFrames": @(length),
		      @"bitsPerSample": @(16), // Samples are short
		      @"floatingPoint": @(NO),
		      @"channels": @(2), // output from gme_play is in stereo
		      @"seekable": @(YES),
		      @"codec": guess_encoding_of_string(m_player->gettype().c_str()),
		      @"encoding": @"synthesized",
		      @"endian": @"host" };
}

- (NSDictionary *)metadata {
	return @{};
}

- (AudioChunk *)readAudio {
	int frames = 1024;
	int16_t buffer[1024 * 2];

	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];
	void *buf = (void *)buffer;

	int total = 0;
	bool dont_loop = !IsRepeatOneSet();
	if(dont_loop && current_pos + frames > length)
		frames = (UInt32)(length - current_pos);
	while(total < frames) {
		bool running = true;
		if(!samples_todo) {
			running = m_player->update() && running;
			if(!dont_loop || running) {
				samples_todo = (int)ceil(sampleRate / m_player->getrefresh());
				current_pos += samples_todo;
			}
		}
		if(!samples_todo)
			break;
		int samples_now = samples_todo;
		if(samples_now > (frames - total))
			samples_now = frames - total;
		m_emu->update((short *)buf, samples_now);
		buf = ((short *)buf) + samples_now * 2;
		samples_todo -= samples_now;
		total += samples_now;
	}

	double streamTimestamp = (double)(current_pos) / sampleRate;
	[chunk setStreamTimestamp:streamTimestamp];

	[chunk assignSamples:buffer frameCount:total];

	return chunk;
}

- (long)seek:(long)frame {
	if(frame < current_pos) {
		current_pos = 0;
		m_player->rewind(subsong);
	}

	while(current_pos < frame) {
		m_player->update();
		current_pos += (long)ceil(sampleRate / m_player->getrefresh());
	}

	samples_todo = (UInt32)(current_pos - frame);

	return frame;
}

- (void)cleanUp {
	delete m_player;
	m_player = NULL;
	delete m_emu;
	m_emu = NULL;
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
	const CPlayers &pl = CAdPlug::players;
	CPlayers::const_iterator i;
	unsigned j;
	NSMutableArray *array = [NSMutableArray array];

	for(i = pl.begin(); i != pl.end(); ++i) {
		for(j = 0; (*i)->get_extension(j); ++j) {
			[array addObject:[NSString stringWithUTF8String:(*i)->get_extension(j) + 1]];
		}
	}

	return [NSArray arrayWithArray:array];
}

+ (NSArray *)mimeTypes {
	return nil;
}

+ (float)priority {
	return 0.5;
}

+ (NSArray *)fileTypeAssociations {
	NSMutableArray *ret = [NSMutableArray new];
	[ret addObject:@"AdPlug Files"];
	[ret addObject:@"vg.icns"];
	[ret addObjectsFromArray:[self fileTypes]];

	return @[[NSArray arrayWithArray:ret]];
}

@end
