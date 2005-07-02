//
//  PlaylistView.h
//  Cog
//
//  Created by Vincent Spader on 3/20/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "SoundController.h"
#import "PlaylistController.h"

@interface PlaylistView : NSTableView {
	
	IBOutlet SoundController *soundController;
	IBOutlet PlaylistController *playlistController;
	
}

@end
