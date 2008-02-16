//
//  SpotlightPlaylistController.h
//  Cog
//
//  Created by Matthew Grinshpun on 13/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PlaylistController.h"

@interface SpotlightPlaylistController : PlaylistController {
    NSArray *oldObjects;
}

- (BOOL)tableView:(NSTableView *)tv writeRowsWithIndexes:(NSIndexSet *)rowIndexes toPasteboard:(NSPasteboard*)pboard;

@property(retain) NSArray *oldObjects;

@end
