//
//  MiniWindow.h
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PlaybackController.h"

@interface MiniWindow : NSWindow {
    IBOutlet PlaybackController *playbackController;
}

@end
