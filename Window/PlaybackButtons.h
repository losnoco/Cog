//
//  PlaybackButtons.h
//  Cog
//
//  Created by Vincent Spader on 2/28/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class PlaybackController;

@interface PlaybackButtons : NSSegmentedControl {
	IBOutlet PlaybackController *playbackController;
}

- (void)startObserving;
- (void)stopObserving;

@end
