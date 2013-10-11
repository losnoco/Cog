//
//  SideBarController.h
//  Cog
//
//  Created by Vincent Spader on 6/21/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SideViewController.h"

@class PlaylistLoader;
@class PlaybackController;
@class FileTreeOutlineView;
@interface FileTreeViewController : SideViewController {
	IBOutlet PlaylistLoader *playlistLoader;
	IBOutlet PlaybackController *playbackController;
    IBOutlet FileTreeOutlineView *fileTreeOutlineView;
}

- (FileTreeOutlineView*)outlineView;

@end
