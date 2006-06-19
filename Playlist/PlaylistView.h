//
//  PlaylistView.h
//  Cog
//
//  Created by Vincent Spader on 3/20/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "AppController.h"
#import "PlaybackController.h"
#import "PlaylistController.h"

@interface PlaylistView : NSTableView {
	
	IBOutlet PlaybackController *playbackController;
	IBOutlet PlaylistController *playlistController;
	
	NSArray *_tableColumnsCache;
	NSMutableArray *_removedColumns;
}

- (IBAction)sortByPath:(id)sender;
- (IBAction)shufflePlaylist:(id)sender;

- (IBAction)takeBoolForTitle:(id)sender;
- (IBAction)takeBoolForArtist:(id)sender;
- (IBAction)takeBoolForAlbum:(id)sender;
- (IBAction)takeBoolForLength:(id)sender;
- (IBAction)takeBoolForYear:(id)sender;
- (IBAction)takeBoolForGenre:(id)sender;
- (IBAction)takeBoolForTrack:(id)sender;


@end
