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
#import "OutputPane.h"

@interface PrefPaneController : NSObject <SS_PreferencePaneProtocol> {
	IBOutlet HotKeyPane *hotKeyPane;
	IBOutlet FileDrawerPane *fileDrawerPane;
	IBOutlet OutputPane *outputPane;

	IBOutlet NSView *scrobblerView;
	IBOutlet NSView *remoteView;
	IBOutlet NSView *updatesView;
}

- (HotKeyPane *)hotKeyPane;
- (FileDrawerPane *)fileDrawerPane;
- (OutputPane *)outputPane;

- (PreferencePane *)remotePane;
- (PreferencePane *)updatesPane;
- (PreferencePane *)scrobblerPane;

@end
