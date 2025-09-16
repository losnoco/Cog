//
//  jxsMetadataReader.m
//  Syntrax-c
//
//  Created by Christopher Snowhill on 03/14/16.
//  Copyright 2016 __NoWork, Inc__. All rights reserved.
//

#import "jxsMetadataReader.h"
#import "jxsDecoder.h"

#import <Syntrax_c/jxs.h>
#import <Syntrax_c/jaytrax.h>

#import "Logging.h"

@implementation jxsMetadataReader

+ (NSArray *)fileTypes {
	return [jxsDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return [jxsDecoder mimeTypes];
}

+ (float)priority {
    return [jxsDecoder priority];
}

+ (NSDictionary *)metadataForURL:(NSURL *)url {
	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	if (![source open:url])
		return nil;

	if (![source seekable])
		return nil;

	if(![source seek:0 whence:SEEK_END])
		return nil;
	long size = [source tell];
	if(size <= 0)
		return nil;
    if(![source seek:0 whence:SEEK_SET])
		return nil;

    void * data = malloc(size);
	if(!data)
		return nil;
	if([source read:data amount:size] != size) {
		free(data);
		return nil;
	}

	JT1Song* synSong;
	if (jxsfile_readSongMem(data, size, &synSong)) {
		ALog(@"Open failed for file: %@", [url absoluteString]);
		free(data);
		return nil;
	}

	free(data);

	JT1Player * synPlayer = jaytrax_init();
	if (!synPlayer) {
		ALog(@"Failed to create player for file: %@", [url absoluteString]);
		jxsfile_freeSong(synSong);
		return nil;
	}

	if (!jaytrax_loadSong(synPlayer, synSong)) {
		ALog(@"Load failed for file: %@", [url absoluteString]);
		jaytrax_free(synPlayer);
		jxsfile_freeSong(synSong);
		return nil;
	}

	int track_num;
	if ([[url fragment] length] == 0) {
		track_num = 0;
	} else {
		track_num = [[url fragment] intValue];
	}

	jaytrax_changeSubsong(synPlayer, track_num);

	//Some titles are all spaces?!
	NSString *title = [guess_encoding_of_string(synPlayer->subsong->name) stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]];

	jaytrax_free(synPlayer);
	jxsfile_freeSong(synSong);

	if (title == nil) {
		title = @"";
	}

	return @{@"title": title};
}

@end
