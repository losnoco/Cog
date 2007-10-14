//
//  CueSheetContainer.m
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CueSheetContainer.h"

#import "CueSheet.h"
#import "CueSheetTrack.h"

@implementation CueSheetContainer

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObject:@"cue"];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"application/x-cue", nil]; //This is basically useless
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url
{
	if (![url isFileURL]) {
		return [NSArray array];
	}
	
	NSMutableArray *tracks = [NSMutableArray array];
	
	CueSheet *cuesheet = [CueSheet cueSheetWithFile:[url path]];

	NSEnumerator *e = [[cuesheet tracks] objectEnumerator];
	CueSheetTrack  *track;
	while (track = [e nextObject]) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%@", [track track]]]];
	}
	
	return tracks;
}

@end
