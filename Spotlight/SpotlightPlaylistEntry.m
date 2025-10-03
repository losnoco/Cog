//
//  SpotlightPlaylistEntry.m
//  Cog
//
//  Created by Matthew Grinshpun on 11/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import "SpotlightPlaylistEntry.h"

// dictionary that lets us translate from and mdKey to an entryKey
// if we need the help of a transformer, we use an nsarray
// with format (entryKey, transformerName)
static NSDictionary *importKeys;

extern NSPersistentContainer *kPersistentContainer;

@implementation SpotlightPlaylistEntry

+ (void)initialize {
	// We need to translate the path string to a full URL
	NSArray *URLTransform =
	@[@"url", @"PathToURLTransformer"];

	// Extract the artist name from the authors array
	NSArray *artistTransform =
	@[@"artist", @"AuthorToArtistTransformer"];

	importKeys = @{ @"kMDItemTitle": @"title",
		            @"kMDItemAlbum": @"album",
		            @"kMDItemAudioTrackNumber": @"track",
		            @"kMDItemRecordingYear": @"year",
		            @"kMDItemMusicalGenre": @"genre",
		            @"kMDItemDurationSeconds": @"length",
		            @"kMDItemPath": URLTransform,
		            @"kMDItemAuthors": artistTransform };
}

+ (PlaylistEntry *)playlistEntryWithMetadataItem:(NSMetadataItem *)metadataItem {
	__block PlaylistEntry *entry = nil;
	[kPersistentContainer.viewContext performBlockAndWait:^{
		entry = [NSEntityDescription insertNewObjectForEntityForName:@"PlaylistEntry" inManagedObjectContext:kPersistentContainer.viewContext];

		entry.deLeted = YES;

		NSMutableDictionary *dict = [NSMutableDictionary new];

		// loop through the keys we want to extract
		for(NSString *mdKey in importKeys) {
			if(![metadataItem valueForAttribute:mdKey]) continue;
			id importTarget = [importKeys objectForKey:mdKey];
			// Just copy the object from metadata
			if([importTarget isKindOfClass:[NSString class]]) {
				if([importTarget isEqualToString:@"length"]) {
					// fake it
					NSNumber *number = [metadataItem valueForAttribute:mdKey];
					[dict setValue:@(44100.0) forKey:@"samplerate"];
					[dict setValue:@(44100.0 * [number doubleValue]) forKey:@"totalFrames"];
				} else {
					[dict setValue:[metadataItem valueForAttribute:mdKey]
					        forKey:importTarget];
				}
			}
			// Transform the value in metadata before copying it in
			else if([importTarget isKindOfClass:[NSArray class]]) {
				NSString *importKey = [importTarget objectAtIndex:0];
				NSValueTransformer *transformer =
				[NSValueTransformer valueTransformerForName:[importTarget objectAtIndex:1]];
				id transformedValue = [transformer transformedValue:
				                                   [metadataItem valueForAttribute:mdKey]];
				[dict setValue:transformedValue forKey:importKey];
			}
			// The importKeys dictionary contains something strange...
			else {
				NSString *errString =
				[NSString stringWithFormat:@"ERROR: Could not import key %@", mdKey];
				NSAssert(NO, errString);
			}
		}

		NSURL *url = [dict objectForKey:@"url"];
		[dict removeObjectForKey:@"url"];

		entry.url = url;

		[entry setMetadata:dict];
	}];

	return entry;
}

@end
