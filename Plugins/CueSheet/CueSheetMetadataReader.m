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

		NSString *sheet = [fileMetadata objectForKey:@"cuesheet"];
		if(!sheet || ![sheet length]) sheet = [alsoMetadata objectForKey:@"cuesheet"];

		if([sheet length]) {
			cuesheet = [CueSheet cueSheetWithString:sheet withFilename:[url path]];
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
	NSMutableDictionary *cuesheetMetadata = [[NSMutableDictionary alloc] init];

	if([track artist]) [cuesheetMetadata setValue:[track artist] forKey:@"artist"];
	if([track album]) [cuesheetMetadata setValue:[track album] forKey:@"album"];
	if([track title]) [cuesheetMetadata setValue:[track title] forKey:@"title"];
	if([[track track] intValue]) [cuesheetMetadata setValue:[NSNumber numberWithInt:[[track track] intValue]] forKey:@"track"];
	if([track genre]) [cuesheetMetadata setValue:[track genre] forKey:@"genre"];
	if([[track year] intValue]) [cuesheetMetadata setValue:[NSNumber numberWithInt:[[track year] intValue]] forKey:@"year"];
	if([track albumGain]) [cuesheetMetadata setValue:[NSNumber numberWithFloat:[track albumGain]] forKey:@"replayGainAlbumGain"];
	if([track albumPeak]) [cuesheetMetadata setValue:[NSNumber numberWithFloat:[track albumPeak]] forKey:@"replayGainAlbumPeak"];
	if([track trackGain]) [cuesheetMetadata setValue:[NSNumber numberWithFloat:[track trackGain]] forKey:@"replayGainTrackGain"];
	if([track trackPeak]) [cuesheetMetadata setValue:[NSNumber numberWithFloat:[track trackPeak]] forKey:@"replayGainTrackPeak"];

	return [NSDictionary dictionaryWithDictionary:cuesheetMetadata];
}

@end
