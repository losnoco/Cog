//
//  SpotlightPlaylistEntry.m
//  Cog
//
//  Created by Matthew Grinshpun on 11/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SpotlightPlaylistEntry.h"

// Class array for metadata keys we want
static NSArray * mdKeys;

// Corresponding array for playlist entry keys
static NSArray * entryKeys; 

// extramdKeys represents those keys that require additional processing
static NSArray * extramdKeys;
                     
// And the dictionary that matches them
static NSDictionary * tags;

@implementation SpotlightPlaylistEntry

+ (void)initialize
{
    mdKeys = [NSArray arrayWithObjects: 
                            @"kMDItemTitle",
                            @"kMDItemAuthors",
                            @"kMDItemAlbum",
                            @"kMDItemAudioTrackNumber",
                            @"kMDItemRecordingYear",
                            @"kMDItemMusicalGenre",
                            nil];
    entryKeys = [NSArray arrayWithObjects: 
                            @"title",
                            @"artist",
                            @"album",
                            @"track",
                            @"year",
                            @"genre",
                            nil];
    extramdKeys = [NSArray arrayWithObjects:
                            @"kMDItemPath",
                            nil];
    tags = [NSDictionary dictionaryWithObjects:entryKeys forKeys:mdKeys];
}

+ (SpotlightPlaylistEntry *)playlistEntryWithMetadataItem:(NSMetadataItem *)metadataItem
{
    // use the matching tag sets to generate a playlist entry
    SpotlightPlaylistEntry *entry = [[[SpotlightPlaylistEntry alloc] init] autorelease];
    NSDictionary *songAttributes = [metadataItem valuesForAttributes:mdKeys];
    NSDictionary *extraAttributes = [metadataItem valuesForAttributes:extramdKeys];
    for (NSString * mdKey in tags) {
        [entry setValue: [songAttributes objectForKey:mdKey]
                 forKey:[tags objectForKey:mdKey]];
    
    }
    // URL needs to be generated from the simple path stored in kMDItemPath
    [entry setURL: [NSURL fileURLWithPath: [extraAttributes objectForKey:@"kMDItemPath"]]];
    return entry;
}

- (id)init
{
    if (self = [super init])
    {
        
    }
    return self;
}
@end
