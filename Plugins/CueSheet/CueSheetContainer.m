//
//  CueSheetContainer.m
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CueSheetContainer.h"

#import "CueSheet.h"
#import "CueSheetDecoder.h"
#import "CueSheetMetadataReader.h"
#import "CueSheetTrack.h"

#import "AudioMetadataReader.h"

#import "NSDictionary+Merge.h"

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
	return [self urlsForContainerURL:url trackInfo:NULL];
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url trackInfo:(NSDictionary **)trackInfo {
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
	NSDictionary *fileMetadata = nil;
	NSDictionary *alsoMetadata = nil;

	NSString *ext = [url pathExtension];
	if([ext caseInsensitiveCompare:@"cue"] != NSOrderedSame) {
		// Embedded cuesheet check
		fileMetadata = [NSClassFromString(@"AudioMetadataReader") metadataForURL:url skipCue:YES];

		alsoMetadata = [NSClassFromString(@"AudioPropertiesReader") propertiesForURL:url skipCue:YES];

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
		// A file with no sheet reads with this plugin as without it, as just
		// read: the file itself, then, with no need to read it again.
		if(trackInfo && alsoMetadata) {
			*trackInfo = @{ url: [NSDictionary dictionaryByMerging:alsoMetadata with:fileMetadata ?: @{}] };
		}
		return tracks;
	}

	for(CueSheetTrack *track in [cuesheet tracks]) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%@", [track track]]]];
	}

	if(trackInfo) {
		// An embedded sheet's file was just read, with and without its
		// decoder; a .cue file's are read as they come up.
		NSMutableDictionary *fileProperties = [NSMutableDictionary new];
		NSMutableDictionary *fileTags = [NSMutableDictionary new];
		if(embedded) {
			fileProperties[url] = alsoMetadata ?: [NSNull null];
			fileTags[url] = fileMetadata ?: [NSNull null];
		}
		*trackInfo = [self trackInfoForURLs:tracks ofCueSheet:cuesheet url:url embedded:embedded fileProperties:fileProperties fileMetadata:fileTags];
	}

	return tracks;
}

// Every track's properties and tags, as the decoder and the metadata reader
// would give them, from the sheet just parsed and the properties and tags of
// each audio file it plays, read once. The caches hold those, by file URL.
+ (NSDictionary *)trackInfoForURLs:(NSArray *)urls ofCueSheet:(CueSheet *)cuesheet url:(NSURL *)url embedded:(BOOL)embedded fileProperties:(NSMutableDictionary *)fileProperties fileMetadata:(NSMutableDictionary *)fileMetadata {
	// The decoder opens the file it plays itself; a file whose properties
	// come from a properties reader instead is read the usual way. So is a
	// track whose properties would come from one.
	id<CogPluginController> pluginController = [NSClassFromString(@"PluginController") sharedPluginController];
	NSDictionary *propertiesReaders = [pluginController propertiesReadersByExtension];
	if([propertiesReaders objectForKey:[[url pathExtension] lowercaseString]])
		return nil;

	NSArray *cueTracks = [cuesheet tracks];
	NSMutableDictionary *info = [NSMutableDictionary dictionaryWithCapacity:[urls count]];
	for(NSURL *trackURL in urls) {
		NSString *fragment = [trackURL fragment];
		if(![fragment length] || [info objectForKey:trackURL])
			continue;

		// The first track numbered so, as the decoder and metadata reader find it
		NSUInteger index = [cueTracks indexOfObjectPassingTest:^BOOL(CueSheetTrack *t, NSUInteger idx, BOOL *stop) {
			return [[t track] isEqualToString:fragment];
		}];
		if(index == NSNotFound)
			continue;
		CueSheetTrack *track = [cueTracks objectAtIndex:index];

		NSURL *fileURL = embedded ? url : [track url];
		if(![fileURL isFileURL] || [propertiesReaders objectForKey:[[fileURL pathExtension] lowercaseString]])
			continue;

		if(![fileProperties objectForKey:fileURL]) {
			fileProperties[fileURL] = [NSClassFromString(@"AudioPropertiesReader") propertiesForURL:fileURL skipCue:YES] ?: [NSNull null];
			fileMetadata[fileURL] = [NSClassFromString(@"AudioMetadataReader") metadataForURL:fileURL skipCue:YES] ?: [NSNull null];
		}
		NSDictionary *properties = fileProperties[fileURL];
		NSDictionary *tags = fileMetadata[fileURL];
		if(properties == (id)[NSNull null])
			continue;
		if(tags == (id)[NSNull null])
			tags = nil;

		long start, end;
		[CueSheetDecoder getStart:&start end:&end forTrackAtIndex:index ofTracks:cueTracks embedded:embedded fileProperties:properties];
		// The decoder's properties merge with its tags, which would fill a
		// length of 0 if they had one; the merged file properties can't say.
		if(end - start == 0)
			continue;

		NSDictionary *metadata = [CueSheetMetadataReader metadataForTrack:track fileMetadata:tags];
		// Where the tags have no art, the cover beside the sheet would
		// replace the decoder's; that is added later, only where none is.
		if(![metadata objectForKey:@"albumArt"] && [properties objectForKey:@"albumArt"])
			continue;

		// The track's tags replace the properties' unconditionally, as the
		// playlist merges a cue sheet track's.
		NSMutableDictionary *entryInfo = [[CueSheetDecoder propertiesWithFileProperties:properties start:start end:end] mutableCopy];
		[entryInfo addEntriesFromDictionary:metadata];
		info[trackURL] = entryInfo;
	}

	return info;
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
