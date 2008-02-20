//
//  FileTreeController.h
//  Cog
//
//  Created by Vincent Spader on 2/17/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PlaylistLoader.h"

@class PlaylistLoader;

@interface FileTreeWindowController : NSWindowController {
	IBOutlet PlaylistLoader *playlistLoader;
	IBOutlet NSOutlineView *outlineView;
}

- (IBAction)addToPlaylist:(id)sender;

@end
