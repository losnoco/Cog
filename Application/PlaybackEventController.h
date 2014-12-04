//
//  PlaybackEventController.h
//  Cog
//
//  Created by Vincent Spader on 3/5/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Growl/GrowlApplicationBridge.h>

#import "PlaybackController.h"
#import "PlaylistEntry.h"

@class AudioScrobbler;
@interface PlaybackEventController : NSObject <NSUserNotificationCenterDelegate, GrowlApplicationBridgeDelegate> {
	NSOperationQueue *queue;
    
    PlaylistEntry *entry;
	
	AudioScrobbler *scrobbler;
    
    IBOutlet PlaybackController *playbackController;
    
    IBOutlet NSWindow *mainWindow;
    IBOutlet NSWindow *miniWindow;
}

@end
