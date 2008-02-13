//
//  SpotlightPlaylistController.h
//  Cog
//
//  Created by Matthew Grinshpun on 13/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PlaylistController.h"
#import "SpotlightWindowController.h"

@interface SpotlightPlaylistController : PlaylistController {
    IBOutlet SpotlightWindowController * spotlightWindowController;
}

- (BOOL)tableView:(NSTableView *)tv writeRowsWithIndexes:(NSIndexSet *)rowIndexes toPasteboard:(NSPasteboard*)pboard;

@end
