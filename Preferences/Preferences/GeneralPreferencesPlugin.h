//
//  PreferencesController.h
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "PreferencePanePlugin.h"

#import "GeneralPane.h"
#import "HotKeyPane.h"
#import "MIDIPane.h"
#import "OutputPane.h"
#import "AppearancePane.h"
#import "RubberbandPane.h"

@interface GeneralPreferencesPlugin : NSObject <PreferencePanePlugin> {
	IBOutlet HotKeyPane *hotKeyPane;
	IBOutlet OutputPane *outputPane;
	IBOutlet MIDIPane *midiPane;
	IBOutlet GeneralPane *generalPane;
	IBOutlet AppearancePane *appearancePane;
	IBOutlet RubberbandPane *rubberbandPane;

	IBOutlet NSView *playlistView;
	IBOutlet NSView *notificationsView;

	__weak IBOutlet NSButton *iTunesStyleCheck;
}

- (HotKeyPane *)hotKeyPane;
- (OutputPane *)outputPane;
- (MIDIPane *)midiPane;
- (GeneralPane *)generalPane;

- (GeneralPreferencePane *)playlistPane;
- (GeneralPreferencePane *)notificationsPane;
- (GeneralPreferencePane *)appearancePane;
- (GeneralPreferencePane *)rubberbandPane;

@end
