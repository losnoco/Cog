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

#import "AudioMetadataReader.h"
#import "NSDictionary+Merge.h"

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
    return 16.0f;
}

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
	if (![url isFileURL]) {
		return nil;
	}
    
    BOOL embedded = NO;
    CueSheet *cuesheet = nil;
    NSDictionary * fileMetadata;
    
    Class audioMetadataReader = NSClassFromString(@"AudioMetadataReader");
    
    NSString *ext = [url pathExtension];
    if ([ext caseInsensitiveCompare:@"cue"] != NSOrderedSame)
    {
        // Embedded cuesheet check
        fileMetadata = [audioMetadataReader metadataForURL:url skipCue:YES];
        NSString * sheet = [fileMetadata objectForKey:@"cuesheet"];
        if ([sheet length])
        {
            cuesheet = [CueSheet cueSheetWithString:sheet withFilename:[url path]];
            embedded = YES;
        }
    }
    else
        cuesheet = [CueSheet cueSheetWithFile:[url path]];
    
    if (!cuesheet)
        return nil;

	NSArray *tracks = [cuesheet tracks];
    for (CueSheetTrack *track in tracks)
	{
		if ([[url fragment] isEqualToString:[track track]])
		{
            // Class supplied by CogAudio, which is guaranteed to be present
            if (!embedded)
                fileMetadata = [audioMetadataReader metadataForURL:[track url] skipCue:YES];
			NSDictionary * cuesheetMetadata = [NSDictionary dictionaryWithObjectsAndKeys:
				[track artist], @"artist",
				[track album], @"album",
				[track title], @"title",
				[NSNumber numberWithInt:[[track track] intValue]], @"track",
				[track genre], @"genre",
				[NSNumber numberWithInt:[[track year] intValue]], @"year",
				nil];
		
            return [fileMetadata dictionaryByMergingWith:cuesheetMetadata];
		}
	}

	return nil;
}

@end
