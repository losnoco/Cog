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

#import "AMRemovableColumnsTableView.h"

@interface PlaylistView : AMRemovableColumnsTableView {
	
	IBOutlet PlaybackController *playbackController;
	IBOutlet PlaylistController *playlistController;
}

- (IBAction)sortByPath:(id)sender;
- (IBAction)shufflePlaylist:(id)sender;

- (IBAction)toggleColumn:(id)sender;


@end
