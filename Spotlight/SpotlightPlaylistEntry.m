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

@implementation SpotlightPlaylistEntry

+ (void)initialize
{
    // We need to translate the path string to a full URL
    NSArray *URLTransform = 
        [NSArray arrayWithObjects:@"URL", @"StringToURLTransformer", nil];
        
    // Extract the artist name from the authors array
    NSArray *artistTransform = 
        [NSArray arrayWithObjects:@"artist", @"AuthorToArtistTransformer", nil];
    
    importKeys = [[NSDictionary dictionaryWithObjectsAndKeys:
        @"title",                   @"kMDItemTitle",
        @"album",                   @"kMDItemAlbum",
        @"track",                   @"kMDItemAudioTrackNumber",
        @"year",                    @"kMDItemRecordingYear",
        @"genre",                   @"kMDItemMusicalGenre",
        @"length",                  @"kMDItemDurationSeconds",
        URLTransform,               @"kMDItemPath",
        artistTransform,            @"kMDItemAuthors",
        nil]retain];
}

+ (SpotlightPlaylistEntry *)playlistEntryWithMetadataItem:(NSMetadataItem *)metadataItem
{
    SpotlightPlaylistEntry *entry = [[[SpotlightPlaylistEntry alloc] init] autorelease];
    
    // Dictionary of the metadata values
    NSDictionary *songAttributes = 
        [metadataItem valuesForAttributes:[importKeys allKeys]];
    
    // loop through the keys we want to extract
    for (NSString *mdKey in importKeys) {
        id importTarget = [importKeys objectForKey:mdKey];
        // Just copy the object from metadata
        if ([importTarget isKindOfClass:[NSString class]])
        {
            [entry setValue: [songAttributes objectForKey:mdKey]
                     forKey: importTarget];
        }
        // Transform the value in metadata before copying it in
        else if ([importTarget isKindOfClass:[NSArray class]])
        {
            NSString * importKey = [importTarget objectAtIndex:0];
            NSValueTransformer *transformer = 
                [NSValueTransformer valueTransformerForName:[importTarget objectAtIndex:1]];
            id transformedValue = [transformer transformedValue:
                                    [songAttributes objectForKey:mdKey]];
            [entry setValue: transformedValue
                     forKey: importKey];
        }
        // The importKeys dictionary contains something strange...
        else
        {
            NSString *errString = 
                [NSString stringWithFormat:@"ERROR: Could not import key %@", mdKey];
            NSAssert(NO, errString);
        }
    }
    return entry;
}

@synthesize length;

@end