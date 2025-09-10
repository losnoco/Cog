//
//  MiniWindow.h
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "PlaybackController.h"
#import "PlaylistController.h"
#import <Cocoa/Cocoa.h>

@interface MiniWindow : NSWindow <NSDraggingDestination> {
	IBOutlet PlaybackController *playbackController;
	IBOutlet PlaylistController *playlistController;
	IBOutlet NSToolbar *miniToolbar;
}

@end
