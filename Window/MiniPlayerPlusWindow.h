//
//  MiniPlayerPlusWindow.h
//  Cog
//

#import "PlaybackController.h"
#import "PlaylistController.h"
#import <Cocoa/Cocoa.h>

@interface MiniPlayerPlusWindow : NSWindow <NSDraggingDestination> {
	IBOutlet PlaybackController *playbackController;
	IBOutlet PlaylistController *playlistController;
}

@end
