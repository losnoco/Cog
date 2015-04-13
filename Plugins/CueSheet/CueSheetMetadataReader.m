//
//  CueSheetMetadataReader.m
//  CueSheet
//
//  Created by Vincent Spader on 10/12/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CueSheetMetadataReader.h"
#import "CueSheetDecoder.h"

#import "CueSheet.h"
#import "CueSheetTrack.h"

@implementation CueSheetMetadataReader

+ (NSArray *)fileTypes
{
	return [CueSheetDecoder fileTypes];
}

+ (NSArray *)mimeTypes
{
	return [CueSheetDecoder mimeTypes];
}

+ (float)priority
{
    return 1.0f;
}

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
	if (![url isFileURL]) {
		return nil;
	}

	CueSheet *cuesheet = [CueSheet cueSheetWithFile:[url path]];

	NSArray *tracks = [cuesheet tracks];
    for (CueSheetTrack *track in tracks)
	{
		if ([[url fragment] isEqualToString:[track track]])
		{
			return [NSDictionary dictionaryWithObjectsAndKeys:
				[track artist], @"artist",
				[track album], @"album",
				[track title], @"title",
				[NSNumber numberWithInt:[[track track] intValue]], @"track",
				[track genre], @"genre",
				[NSNumber numberWithInt:[[track year] intValue]], @"year",
				nil];
		
		}
	}

	return nil;
}

@end
