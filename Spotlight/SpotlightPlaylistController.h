//
//  SpotlightPlaylistController.h
//  Cog
//
//  Created by Matthew Grinshpun on 13/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import "PlaylistController.h"
#import <Cocoa/Cocoa.h>

@interface SpotlightPlaylistController : PlaylistController {
}

- (BOOL)tableView:(NSTableView *_Nonnull)tv writeRowsWithIndexes:(NSIndexSet *_Nonnull)rowIndexes toPasteboard:(NSPasteboard *_Nonnull)pboard;
- (NSView *_Nullable)tableView:(NSTableView *_Nonnull)tableView viewForTableColumn:(NSTableColumn *_Nullable)tableColumn row:(NSInteger)row;

@end
