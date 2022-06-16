//
//  SpotlightPlaylistEntry.h
//  Cog
//
//  Created by Matthew Grinshpun on 11/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "PlaylistEntry.h"

@interface SpotlightPlaylistEntry : NSObject
+ (PlaylistEntry *)playlistEntryWithMetadataItem:(NSMetadataItem *)metadataItem;
@end
