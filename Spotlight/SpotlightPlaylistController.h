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

- (BOOL)tableView:(NSTableView *)tv writeRowsWithIndexes:(NSIndexSet *)rowIndexes toPasteboard:(NSPasteboard *)pboard;

@end
