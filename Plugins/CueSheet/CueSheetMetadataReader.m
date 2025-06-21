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

#import "AudioMetadataReader.h"
#import "NSDictionary+Merge.h"
#import "NSDictionary+Optional.h"

@implementation CueSheetMetadataReader

+ (NSArray *)fileTypes {
	return [CueSheetDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return [CueSheetDecoder mimeTypes];
}

+ (float)priority {
	return 16.0f;
}

+ (NSDictionary *)metadataForURL:(NSURL *)url {
	if(![url isFileURL]) {
		return nil;
	}

	BOOL embedded = NO;
	CueSheet *cuesheet = nil;
	NSDictionary *fileMetadata;

	Class audioMetadataReader = NSClassFromString(@"AudioMetadataReader");

	NSString *ext = [url pathExtension];
	if([ext caseInsensitiveCompare:@"cue"] != NSOrderedSame) {
		// Embedded cuesheet check
		fileMetadata = [audioMetadataReader metadataForURL:url skipCue:YES];

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
			embedded = YES;
		}
	} else
		cuesheet = [CueSheet cueSheetWithFile:[url path]];

	if(!cuesheet) {
		return fileMetadata;
	}

	NSArray *tracks = [cuesheet tracks];
	for(CueSheetTrack *track in tracks) {
		if([[url fragment] isEqualToString:[track track]]) {
			// Class supplied by CogAudio, which is guaranteed to be present
			if(!embedded)
				fileMetadata = [audioMetadataReader metadataForURL:[track url] skipCue:YES];

			NSDictionary *cuesheetMetadata = [CueSheetMetadataReader processDataForTrack:track];

			return [cuesheetMetadata dictionaryByMergingWith:fileMetadata];
		}
	}

	return nil;
}

+ (NSDictionary *)processDataForTrack:(CueSheetTrack *)track {
	const NSString* keys[] = {
		@"artist",
		@"album",
		@"title",
		@"track",
		@"genre",
		@"year",
		@"replaygain_album_gain",
		@"replaygain_album_peak",
		@"replaygain_track_gain",
		@"replaygain_track_peak"
	};
	const id values[] = {
		[track artist],
		[track album],
		[track title],
		@([[track track] intValue]),
		[track genre],
		@([[track year] intValue]),
		@([track albumGain]),
		@([track albumPeak]),
		@([track trackGain]),
		@([track trackPeak])
	};
	return [NSDictionary initWithOptionalObjects:values forKeys:keys count:sizeof(keys) / sizeof(keys[0])];
}

@end
