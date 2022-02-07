//
//  SpotlightPlaylistEntry.h
//  Cog
//
//  Created by Matthew Grinshpun on 11/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import "PlaylistEntry.h"
#import <Cocoa/Cocoa.h>

@interface SpotlightPlaylistEntry : PlaylistEntry {
	NSNumber *length;
	NSString *spotlightTrack;
}

+ (SpotlightPlaylistEntry *)playlistEntryWithMetadataItem:(NSMetadataItem *)metadataItem;

@property(retain, readwrite) NSNumber *length;
@property(retain) NSString *spotlightTrack;
@end
