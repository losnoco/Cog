//
//  SpotlightPlaylistEntry.m
//  Cog
//
//  Created by Matthew Grinshpun on 11/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import "SpotlightPlaylistEntry.h"

// Class array for metadata keys we want
static NSArray * mdKeys;

// Corresponding array for playlist entry keys
static NSArray * entryKeys; 

// extramdKeys represents those keys that require additional processing
static NSArray * extramdKeys;

// allmdKeys is a combined array of both mdKeys and entryKeys
static NSArray * allmdKeys;
                     
// tags matches mdKeys and entryKeys for automated extraction
static NSDictionary * tags;

@implementation SpotlightPlaylistEntry

+ (void)initialize
{
    mdKeys = [[NSArray arrayWithObjects: 
                            @"kMDItemTitle",
                            @"kMDItemAlbum",
                            @"kMDItemAudioTrackNumber",
                            @"kMDItemRecordingYear",
                            @"kMDItemMusicalGenre",
                            @"kMDItemDurationSeconds",
                            nil] retain];
    entryKeys = [[NSArray arrayWithObjects: 
                            @"title",
                            @"album",
                            @"track",
                            @"year",
                            @"genre",
                            @"length",
                            nil]retain];
    extramdKeys = [[NSArray arrayWithObjects:
                            @"kMDItemPath",
                            @"kMDItemAuthors",
                            nil]retain];
    allmdKeys = [[mdKeys arrayByAddingObjectsFromArray:extramdKeys]retain];
    tags = [[NSDictionary dictionaryWithObjects:entryKeys forKeys:mdKeys]retain];
}

// Use this to access the array of all the keys we want.
+ (NSArray *)allmdKeys
{
    return allmdKeys;
}

+ (SpotlightPlaylistEntry *)playlistEntryWithMetadataItem:(NSMetadataItem *)metadataItem
{
    // use the matching tag sets to generate a playlist entry
    SpotlightPlaylistEntry *entry = [[[SpotlightPlaylistEntry alloc] init] autorelease];
    
    NSDictionary *songAttributes = [metadataItem valuesForAttributes:allmdKeys];
    for (NSString * mdKey in tags) {
        [entry setValue: [songAttributes objectForKey:mdKey]
                 forKey:[tags objectForKey:mdKey]];
    
    }
    
    // URL needs to be generated from the simple path stored in kMDItemPath
    [entry setURL: [NSURL fileURLWithPath: [songAttributes objectForKey:@"kMDItemPath"]]];
    
    // Authors is an array, but we only care about the first item in it
    
    [entry setArtist: [[songAttributes objectForKey:@"kMDItemAuthors"] objectAtIndex:0]];
    return entry;
}

- (id)init
{
    if (self = [super init])
    {
        length = nil;
    }
    return self;
}

- (void)dealloc
{
    [length release];
    [super dealloc];
}

@synthesize length;
@end