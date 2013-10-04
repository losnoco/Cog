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

@implementation GameContainer

+ (NSArray *)fileTypes
{
	//There doesn't seem to be a way to get this list. These are the only multitrack types.
	return [NSArray arrayWithObjects:@"ay", @"gbs", @"hes", @"kss", @"nsf", @"nsfe", @"sap", @"sgc", nil];
}

+ (NSArray *)mimeTypes 
{
	return nil;
}

//This really should be source...
+ (NSArray *)urlsForContainerURL:(NSURL *)url
{
    id audioSourceClass = NSClassFromString(@"AudioSource");
    id<CogSource> source = [audioSourceClass audioSourceForURL:url];
    
    if (![source open:url])
        return 0;
    
    if (![source seekable])
        return 0;
	
    [source seek:0 whence:SEEK_END];
    long size = [source tell];
    [source seek:0 whence:SEEK_SET];
    
    void * data = malloc(size);
    [source read:data amount:size];
    
	Music_Emu *emu;
	gme_err_t error = gme_open_data(data, size, &emu, 44100);
    free(data);
    
	if (NULL != error) {
		NSLog(@"GME: Error loading file: %@ %s", [url path], error);
		return [NSArray arrayWithObject:url];
	}
	int track_count = gme_track_count(emu);
	
	NSMutableArray *tracks = [NSMutableArray array];
	
	int i;
	for (i = 0; i < track_count; i++) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
	}
	
	return tracks;
}


@end
