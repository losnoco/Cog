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
#import "OutputPane.h"

@interface PrefPaneController : NSObject <SS_PreferencePaneProtocol> {
	IBOutlet HotKeyPane *hotKeyPane;
	IBOutlet OutputPane *outputPane;

	IBOutlet NSView *fileTreeView;
	IBOutlet NSView *scrobblerView;
	IBOutlet NSView *remoteView;
	IBOutlet NSView *updatesView;
}

- (HotKeyPane *)hotKeyPane;
- (OutputPane *)outputPane;

- (PreferencePane *)fileTreePane;
- (PreferencePane *)remotePane;
- (PreferencePane *)updatesPane;
- (PreferencePane *)scrobblerPane;

@end
