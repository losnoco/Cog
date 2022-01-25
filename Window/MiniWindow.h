//
//  MiniWindow.h
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PlaybackController.h"
#import "PlaylistController.h"

@interface MiniWindow : NSWindow<NSDraggingDestination> {
    IBOutlet PlaybackController *playbackController;
    IBOutlet PlaylistController *playlistController;
    IBOutlet NSToolbar *miniToolbar;
    NSImage *hdcdLogo;
}

- (void)showHDCDLogo:(BOOL)show;

@end
