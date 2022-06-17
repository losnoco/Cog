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

extern NSPersistentContainer *__persistentContainer;

@implementation SpotlightPlaylistEntry

+ (void)initialize {
	// We need to translate the path string to a full URL
	NSArray *URLTransform =
	@[@"url", @"PathToURLTransformer"];

	// Extract the artist name from the authors array
	NSArray *artistTransform =
	@[@"artist", @"AuthorToArtistTransformer"];

	// Track numbers must sometimes be converted from NSNumber to NSString
	NSArray *trackTransform =
	@[@"spotlightTrack", @"NumberToStringTransformer"];

	importKeys = @{ @"kMDItemTitle": @"title",
		            @"kMDItemAlbum": @"album",
		            @"kMDItemAudioTrackNumber": trackTransform,
		            @"kMDItemRecordingYear": @"year",
		            @"kMDItemMusicalGenre": @"genre",
		            @"kMDItemDurationSeconds": @"spotlightLength",
		            @"kMDItemPath": URLTransform,
		            @"kMDItemAuthors": artistTransform };
}

+ (PlaylistEntry *)playlistEntryWithMetadataItem:(NSMetadataItem *)metadataItem {
	PlaylistEntry *entry = [NSEntityDescription insertNewObjectForEntityForName:@"PlaylistEntry" inManagedObjectContext:__persistentContainer.viewContext];

	entry.deLeted = YES;

	// loop through the keys we want to extract
	for(NSString *mdKey in importKeys) {
		id importTarget = [importKeys objectForKey:mdKey];
		// Just copy the object from metadata
		if([importTarget isKindOfClass:[NSString class]]) {
			[entry setValue:[metadataItem valueForAttribute:mdKey]
			         forKey:importTarget];
		}
		// Transform the value in metadata before copying it in
		else if([importTarget isKindOfClass:[NSArray class]]) {
			NSString *importKey = [importTarget objectAtIndex:0];
			NSValueTransformer *transformer =
			[NSValueTransformer valueTransformerForName:[importTarget objectAtIndex:1]];
			id transformedValue = [transformer transformedValue:
			                                   [metadataItem valueForAttribute:mdKey]];
			[entry setValue:transformedValue forKey:importKey];
		}
		// The importKeys dictionary contains something strange...
		else {
			NSString *errString =
			[NSString stringWithFormat:@"ERROR: Could not import key %@", mdKey];
			NSAssert(NO, errString);
		}
	}
	return entry;
}

@end
