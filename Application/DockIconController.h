//
//  DockIconController.h
//  Cog
//
//  Created by Vincent Spader on 2/28/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class PlaybackController;

@interface DockIconController : NSObject {
	NSImage *dockImage;

	NSInteger lastDockCustom;
	NSInteger lastDockCustomPlaque;
	NSInteger dockCustomLoaded;
	NSImage *dockCustomStop;
	NSImage *dockCustomPlay;
	NSImage *dockCustomPause;

	IBOutlet PlaybackController *playbackController;

	NSInteger lastPlaybackStatus;
	NSInteger lastColorfulStatus;
	NSNumber *lastProgressStatus;

	NSImageView *imageView;
	NSProgressIndicator *progressIndicator;
}

@end
