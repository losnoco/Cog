//
//  PlaylistView.h
//  Cog
//
//  Created by Vincent Spader on 3/20/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "PlaybackController.h"
#import "PlaylistController.h"
#import "PlaylistLoader.h"

@interface PlaylistView : NSTableView {
	IBOutlet PlaybackController *playbackController;
	IBOutlet PlaylistController *playlistController;
	IBOutlet PlaylistLoader *playlistLoader;

	NSMenu *headerContextMenu;

	NSTimer *syncTimer;
}

- (IBAction)toggleColumn:(id)sender;

- (IBAction)scrollToCurrentEntry:(id)sender;

- (IBAction)refreshCurrentTrack:(id)sender;
- (IBAction)refreshTrack:(id)sender;

- (IBAction)saveSelectionAsPlaylist:(id)sender;

@end
