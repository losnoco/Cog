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

+ (float)priority
{
    return 1.0f;
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url
{
	if (![url isFileURL]) {
		return [NSArray array];
	}
	
    if ([url fragment]) {
        // input url already has fragment defined - no need to expand further
        return [NSMutableArray arrayWithObject:url];
    }
    
	NSMutableArray *tracks = [NSMutableArray array];
	
	CueSheet *cuesheet = [CueSheet cueSheetWithFile:[url path]];

    for (CueSheetTrack *track in [cuesheet tracks]) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%@", [track track]]]];
	}
	
	return tracks;
}

@end
