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

#import "AMRemovableColumnsTableView.h"

@interface PlaylistView : AMRemovableColumnsTableView {
	
	IBOutlet PlaybackController *playbackController;
	IBOutlet PlaylistController *playlistController;
}

- (IBAction)sortByPath:(id)sender;
- (IBAction)shufflePlaylist:(id)sender;

- (IBAction)toggleColumnForIndex:(id)sender;
- (IBAction)toggleColumnForTitle:(id)sender;
- (IBAction)toggleColumnForArtist:(id)sender;
- (IBAction)toggleColumnForAlbum:(id)sender;
- (IBAction)toggleColumnForLength:(id)sender;
- (IBAction)toggleColumnForYear:(id)sender;
- (IBAction)toggleColumnForGenre:(id)sender;
- (IBAction)toggleColumnForTrack:(id)sender;


@end
