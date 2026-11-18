//
//  SidContainer.mm
//  sidplay
//
//  Created by Christopher Snowhill on 12/8/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import "SidContainer.h"
#import "SidDecoder.h"
#import "SidMetadataReader.h"
#import "SidSongLengths.h"

#import "Logging.h"

@implementation SidContainer

+ (NSArray *)fileTypes {
	return [SidDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return nil;
}

+ (float)priority {
	return 0.5f;
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url {
	return [self urlsForContainerURL:url trackInfo:NULL];
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url trackInfo:(NSDictionary **)trackInfo {
	if([url fragment]) {
		// input url already has fragment defined - no need to expand further
		return [NSMutableArray arrayWithObject:url];
	}

	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	if(![source open:url])
		return @[];

	if(![source seekable])
		return @[];

	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	if(!size)
		return @[];

	void *data = malloc(size);
	if(!data)
		return @[];
	
	[source read:data amount:size];

	unsigned int subsongs = 0;
	NSDictionary *properties = nil;
	NSDictionary *metadata = nil;
	NSArray<NSNumber *> *songLengths = nil;
	double sampleRate = [SidDecoder sampleRate];
	SidTune *tune = NULL;

	try {
		tune = new SidTune((const uint_least8_t *)data, (uint_least32_t)size);

		free(data);
		data = NULL;

		if(!tune->getStatus()) {
			delete tune;
			tune = NULL;
			return @[];
		}

		const SidTuneInfo *info = tune->getInfo();

		subsongs = info->songs();

		// Every song has the tags the metadata reader finds, and plays as
		// the decoder would play it, or not at all: whether the tune loads,
		// and its channels, are the same for all songs. A tune the decoder
		// loads with a companion file is left to it.
		if(trackInfo) {
			metadata = [SidMetadataReader metadataForTuneInfo:info];

			if(![SidDecoder loadsCompanionForURL:url]) {
				sidplayfp *engine = NULL;
				sidbuilder *builder = NULL;
				int channels = 1;
				BOOL loaded = NO;

				tune->selectSong(1);

				try {
					loaded = [SidDecoder loadTune:tune sampleRate:sampleRate engine:&engine builder:&builder channels:&channels];
				} catch (std::exception &e) {
					ALog(@"Exception caught loading SID file for track info: %s", e.what());
				}

				delete builder;
				delete engine;

				if(loaded) {
					properties = [SidDecoder propertiesWithSampleRate:sampleRate length:[SidDecoder lengthForSampleRate:sampleRate] channels:channels];
					songLengths = [SidSongLengths lengthsForTune:tune url:url];
				}
			}
		}

		delete tune;
		tune = NULL;
	} catch (std::exception &e) {
		ALog(@"Exception caught processing SID file for song count: %s", e.what());
		delete tune;
		if(data) free(data);
		return @[];
	}

	NSMutableArray *tracks = [NSMutableArray array];

	for(unsigned int i = 1; i <= subsongs; ++i) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
	}

	if(trackInfo && properties) {
		// The decoder has no tags of its own, and the properties have none
		// of the reader's keys.
		NSMutableDictionary *merged = [properties mutableCopy];
		[merged addEntriesFromDictionary:metadata];
		NSDictionary *entryInfo = [NSDictionary dictionaryWithDictionary:merged];

		// Each song plays for the length the collection's database gives
		// it, as the decoder finds it, or for the default one.
		NSMutableDictionary *info = [NSMutableDictionary dictionaryWithCapacity:[tracks count]];
		NSUInteger song = 0;
		for(NSURL *track in tracks) {
			double seconds = song < [songLengths count] ? [songLengths[song] doubleValue] : 0.0;
			if(seconds > 0.0) {
				NSMutableDictionary *songInfo = [entryInfo mutableCopy];
				songInfo[@"totalFrames"] = @((long)ceil(sampleRate * seconds));
				info[track] = songInfo;
			} else {
				info[track] = entryInfo;
			}
			++song;
		}
		*trackInfo = info;
	}

	return tracks;
}

@end
