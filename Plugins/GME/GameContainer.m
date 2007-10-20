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
	return [NSArray arrayWithObjects:@"ay", @"gbs", @"nsf", @"nsfe", @"sap", nil];
}

+ (NSArray *)mimeTypes 
{
	return nil;
}

//This really should be source...
+ (NSArray *)urlsForContainerURL:(NSURL *)url
{
	if (![url isFileURL]) {
		return nil;
	}
	
	gme_err_t error;
	Music_Emu *emu;
	error = gme_open_file([[url path] UTF8String], &emu, 44100);
	int track_count = gme_track_count(emu);
	
	NSMutableArray *tracks = [NSMutableArray array];
	
	int i;
	for (i = 0; i < track_count; i++) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
	}
	
	return tracks;
}


@end
