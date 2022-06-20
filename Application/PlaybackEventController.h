//
//  PlaybackEventController.h
//  Cog
//
//  Created by Vincent Spader on 3/5/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <UserNotifications/UserNotifications.h>

#import "PlaybackController.h"

@interface PlaybackEventController
: NSObject <NSUserNotificationCenterDelegate, UNUserNotificationCenterDelegate> {
	IBOutlet PlaybackController *playbackController;

	IBOutlet NSWindow *mainWindow;
	IBOutlet NSWindow *miniWindow;
}

@end
