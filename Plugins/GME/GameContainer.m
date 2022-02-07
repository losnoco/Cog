//
//  GameFile.m
//  Cog
//
//  Created by Vincent Spader on 5/29/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <GME/gme.h>

#import "GameContainer.h"
#import "GameDecoder.h"

#import "Logging.h"

@implementation GameContainer

+ (NSArray *)fileTypes {
	// There doesn't seem to be a way to get this list. These are the only multitrack types.
	return @[@"ay", @"gbs", @"hes", @"kss", @"nsf", @"nsfe", @"sap", @"sgc"];
}

+ (NSArray *)mimeTypes {
	return nil;
}

+ (float)priority {
	return 1.0f;
}

// This really should be source...
+ (NSArray *)urlsForContainerURL:(NSURL *)url {
	if([url fragment]) {
		// input url already has fragment defined - no need to expand further
		return [NSMutableArray arrayWithObject:url];
	}

	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	if(![source open:url])
		return 0;

	if(![source seekable])
		return 0;

	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];

	void *data = malloc(size);
	[source read:data amount:size];

	Music_Emu *emu;
	gme_err_t error = gme_open_data(data, size, &emu, 44100);
	free(data);

	if(NULL != error) {
		ALog(@"GME: Error loading file: %@ %s", [url path], error);
		return @[url];
	}

	NSURL *m3uurl = [url URLByDeletingPathExtension];
	m3uurl = [m3uurl URLByAppendingPathExtension:@"m3u"];
	id<CogSource> m3usrc = [audioSourceClass audioSourceForURL:m3uurl];
	if([m3usrc open:m3uurl]) {
		if([m3usrc seekable]) {
			[m3usrc seek:0 whence:SEEK_END];
			size = [m3usrc tell];
			[m3usrc seek:0 whence:SEEK_SET];

			data = malloc(size);
			[m3usrc read:data amount:size];

			error = gme_load_m3u_data(emu, data, size);
			free(data);

			ALog(@"M3U loaded: %s", error ? error : "no error");
		}
	}

	int track_count = gme_track_count(emu);

	NSMutableArray *tracks = [NSMutableArray array];

	int i;
	for(i = 0; i < track_count; i++) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
	}

	return tracks;
}

@end
