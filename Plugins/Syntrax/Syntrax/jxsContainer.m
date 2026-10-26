//
//  jxsContainer.m
//  Syntrax-c
//
//  Created by Christopher Snowhill on 03/14/16.
//  Copyright 2016 __NoWork, Inc__. All rights reserved.
//

#import <Syntrax_c/jxs.h>
#import <Syntrax_c/jaytrax.h>

#import "jxsContainer.h"
#import "jxsDecoder.h"
#import "jxsMetadataReader.h"

#import "Logging.h"

@implementation jxsContainer

+ (NSArray *)fileTypes
{
	return [jxsDecoder fileTypes];
}

+ (NSArray *)mimeTypes 
{
	return [jxsDecoder mimeTypes];
}

+ (float)priority
{
	return [jxsDecoder priority];
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url
{
	return [self urlsForContainerURL:url trackInfo:NULL];
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url trackInfo:(NSDictionary **)trackInfo
{
	if ([url fragment]) {
		// input url already has fragment defined - no need to expand further
		return [NSMutableArray arrayWithObject:url];
    }

	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	if (![source open:url])
		return 0;

	if (![source seekable])
		return 0;

	if(![source seek:0 whence:SEEK_END])
		return 0;
	long size = [source tell];
	if(size <= 0)
		return 0;
	if(![source seek:0 whence:SEEK_SET])
		return 0;

	void * data = malloc(size);
	if(!data)
		return 0;
	if([source read:data amount:size] != size) {
		free(data);
		return 0;
	}

	JT1Song * synSong;
	if (jxsfile_readSongMem(data, size, &synSong)) {
		ALog(@"Open failed for file: %@", [url absoluteString]);
		free(data);
		return nil;
	}

	free(data);

	int i;
	int subsongs = synSong->nrofsongs;

	NSMutableArray *tracks = [NSMutableArray new];

	if ( subsongs ) {
		for (i = 0; i < subsongs; i++) {
			[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
		}
	}

	if (trackInfo) {
		*trackInfo = [self trackInfoForSong:synSong tracks:tracks];
	}

	jxsfile_freeSong(synSong);

	return [NSArray arrayWithArray:tracks];
}

// Every track's length and title from the song just read, as the decoder
// and the metadata reader would find them.
+ (NSDictionary *)trackInfoForSong:(JT1Song *)synSong tracks:(NSArray *)tracks
{
	double sampleRate = [jxsDecoder sampleRate];
	NSMutableDictionary *info = [NSMutableDictionary dictionaryWithCapacity:[tracks count]];

	// Measuring a track rewrites its subsong's tempo and groove as the
	// track's own commands change them, and starting any track reads the
	// first subsong's. The decoder measures a song fresh from the file, so
	// put them back after each track.
	int subsongs = synSong->nrofsongs;
	int32_t * songspd = malloc(sizeof(int32_t) * subsongs * 2);
	if (!songspd)
		return info;
	int32_t * groove = songspd + subsongs;
	for (int i = 0; i < subsongs; i++) {
		songspd[i] = synSong->subsongs[i]->songspd;
		groove[i] = synSong->subsongs[i]->groove;
	}

	int i = 0;
	for (NSURL *track in tracks) {
		// A player of its own, as the decoder has
		JT1Player * synPlayer = jaytrax_init();
		if (!synPlayer)
			break;

		if (jaytrax_loadSong(synPlayer, synSong)) {
			long framesLength, totalFrames;
			[jxsDecoder getFramesLength:&framesLength totalFrames:&totalFrames forPlayer:synPlayer track:i sampleRate:sampleRate];

			NSDictionary *properties = [jxsDecoder propertiesWithSampleRate:sampleRate totalFrames:totalFrames];
			NSDictionary *metadata = [jxsMetadataReader metadataForSubsong:synSong->subsongs[i]];
			NSMutableDictionary *entryInfo = [properties mutableCopy];
			[entryInfo addEntriesFromDictionary:metadata];
			info[track] = entryInfo;
		}

		jaytrax_free(synPlayer);

		for (int j = 0; j < subsongs; j++) {
			synSong->subsongs[j]->songspd = songspd[j];
			synSong->subsongs[j]->groove = groove[j];
		}

		++i;
	}

	free(songspd);

	return info;
}

@end
