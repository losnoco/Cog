//
//  FileTreeViewController.h
//  Cog
//
//  Created by Vincent Spader on 6/21/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "PlaylistController.h"
#import <Cocoa/Cocoa.h>

@class PlaylistLoader;
@class PlaybackController;
@class FileTreeOutlineView;
@interface FileTreeViewController : NSViewController {
	IBOutlet PlaylistLoader *playlistLoader;
	IBOutlet PlaybackController *playbackController;
	IBOutlet FileTreeOutlineView *fileTreeOutlineView;
}

- (FileTreeOutlineView *)outlineView;

- (IBAction)toggleSideView:(id)sender;

- (IBAction)chooseRootFolder:(id)sender;

- (void)doAddToPlaylist:(NSArray *)urls origin:(URLOrigin)origin;

- (void)clear:(id)sender;

- (void)playPauseResume:(id)sender;

@end
