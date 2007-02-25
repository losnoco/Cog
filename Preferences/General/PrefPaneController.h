//
//  PreferencesController.h
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SS_PreferencePaneProtocol.h"

#import "HotKeyPane.h"
#import "FileDrawerPane.h"
#import "RemotePane.h"
#import "UpdatesPane.h"
#import "OutputPane.h"
#import "ScrobblerPane.h"

@interface PrefPaneController : NSObject <SS_PreferencePaneProtocol> {
	IBOutlet HotKeyPane *hotKeyPane;
	IBOutlet FileDrawerPane *fileDrawerPane;
	IBOutlet RemotePane *remotePane;
	IBOutlet UpdatesPane *updatesPane;
	IBOutlet OutputPane *outputPane;
	IBOutlet ScrobblerPane *scrobblerPane;
}

- (HotKeyPane *)hotKeyPane;
- (FileDrawerPane *)fileDrawerPane;
- (RemotePane *)remotePane;
- (UpdatesPane *)updatesPane;
- (OutputPane *)outputPane;
- (ScrobblerPane *)scrobblerPane;

@end
