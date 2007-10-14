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

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
	if (![url isFileURL]) {
		return nil;
	}

	CueSheet *cuesheet = [CueSheet cueSheetWithFile:[url path]];

	NSArray *tracks = [cuesheet tracks];
	CueSheetTrack *track;
	NSEnumerator *e = [tracks objectEnumerator];
	while (track = [e nextObject])
	{
		if ([[url fragment] isEqualToString:[track track]])
		{
			return [NSDictionary dictionaryWithObjectsAndKeys:
				[track artist], @"artist",
				[track album], @"album",
				[track title], @"title",
				[NSNumber numberWithInt:[[track track] intValue]], @"track",
				[track genre], @"genre",
				[track year], @"year",
				nil];
		
		}
	}

	return nil;
}

@end
