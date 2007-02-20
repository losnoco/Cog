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

@interface PrefPaneController : NSObject <SS_PreferencePaneProtocol> {
	IBOutlet HotKeyPane *hotKeyPane;
	IBOutlet FileDrawerPane *fileDrawerPane;
	IBOutlet RemotePane *remotePane;
	IBOutlet UpdatesPane *updatesPane;
	IBOutlet OutputPane *outputPane;
}

- (HotKeyPane *)hotKeyPane;
- (FileDrawerPane *)fileDrawerPane;
- (RemotePane *)remotePane;
- (UpdatesPane *)updatesPane;
- (UpdatesPane *)outputPane;

@end
