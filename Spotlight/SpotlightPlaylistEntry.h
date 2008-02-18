//
//  SpotlightPlaylistEntry.h
//  Cog
//
//  Created by Matthew Grinshpun on 11/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PlaylistEntry.h"


@interface SpotlightPlaylistEntry : PlaylistEntry {
    NSNumber *length;
}

+ (SpotlightPlaylistEntry *)playlistEntryWithMetadataItem:(NSMetadataItem *)metadataItem;

// New length getters/setters

@property(retain) NSNumber *length;
@end
