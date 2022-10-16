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

#import "AudioMetadataReader.h"

@implementation CueSheetContainer

+ (NSArray *)fileTypes {
	return @[@"cue", @"ogg", @"opus", @"flac", @"wv", @"mp3"];
}

+ (NSArray *)mimeTypes {
	return @[@"application/x-cue"]; // This is basically useless
}

+ (float)priority {
	return 16.0f;
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url {
	if(![url isFileURL]) {
		return @[];
	}

	if([url fragment]) {
		// input url already has fragment defined - no need to expand further
		return [NSMutableArray arrayWithObject:url];
	}

	NSMutableArray *tracks = [NSMutableArray array];

	BOOL embedded = NO;
	CueSheet *cuesheet = nil;
	NSDictionary *fileMetadata;

	NSString *ext = [url pathExtension];
	if([ext caseInsensitiveCompare:@"cue"] != NSOrderedSame) {
		// Embedded cuesheet check
		fileMetadata = [NSClassFromString(@"AudioMetadataReader") metadataForURL:url skipCue:YES];

		NSDictionary *alsoMetadata = [NSClassFromString(@"AudioPropertiesReader") propertiesForURL:url skipCue:YES];

		id sheet = [fileMetadata objectForKey:@"cuesheet"];
		NSString *sheetString = nil;
		if(sheet) {
			if([sheet isKindOfClass:[NSArray class]]) {
				NSArray *sheetContainer = sheet;
				if([sheetContainer count]) {
					sheetString = sheetContainer[0];
				}
			} else if([sheet isKindOfClass:[NSString class]]) {
				sheetString = sheet;
			}
		}
		if(!sheetString || ![sheetString length]) {
			sheet = [alsoMetadata objectForKey:@"cuesheet"];
			if(sheet) {
				if([sheet isKindOfClass:[NSArray class]]) {
					NSArray *sheetContainer = sheet;
					if([sheetContainer count]) {
						sheetString = sheetContainer[0];
					}
				} else if([sheet isKindOfClass:[NSString class]]) {
					sheetString = sheet;
				}
			}
		}

		if(sheetString && [sheetString length]) {
			cuesheet = [CueSheet cueSheetWithString:sheetString withFilename:[url path]];
		}
		embedded = YES;
	} else
		cuesheet = [CueSheet cueSheetWithFile:[url path]];

	if(!cuesheet)
		return tracks;

	for(CueSheetTrack *track in [cuesheet tracks]) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%@", [track track]]]];
	}

	return tracks;
}

+ (NSArray *)dependencyUrlsForContainerURL:(NSURL *)url {
	if(![url isFileURL]) {
		return @[];
	}

	if([url fragment]) {
		NSString *pathString = [url path];
		NSString *lastComponent = [url lastPathComponent];

		// Find that last component in the string from the end to make sure
		// to get the last one
		NSRange fragmentRange = [pathString rangeOfString:lastComponent options:NSBackwardsSearch];

		// Chop the fragment.
		NSString *newPathString = [pathString substringToIndex:fragmentRange.location + fragmentRange.length];

		url = [NSURL fileURLWithPath:newPathString];
	}

	NSMutableArray *tracks = [NSMutableArray array];

	CueSheet *cuesheet = nil;

	NSString *ext = [url pathExtension];
	if([ext caseInsensitiveCompare:@"cue"] != NSOrderedSame) {
		return @[];
	} else
		cuesheet = [CueSheet cueSheetWithFile:[url path]];

	if(!cuesheet)
		return @[];

	for(CueSheetTrack *track in [cuesheet tracks]) {
		if(![tracks containsObject:track.url])
			[tracks addObject:track.url];
	}

	return tracks;
}

@end
