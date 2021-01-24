//
//  PreferencesController.h
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "PreferencePanePlugin.h"

#import "HotKeyPane.h"
#import "OutputPane.h"
#import "MIDIPane.h"

@interface GeneralPreferencesPlugin : NSObject <PreferencePanePlugin> {
    IBOutlet HotKeyPane *hotKeyPane;
    IBOutlet OutputPane *outputPane;
    IBOutlet MIDIPane *midiPane;

    IBOutlet NSView *playlistView;
    IBOutlet NSView *scrobblerView;
    IBOutlet NSView *updatesView;
    IBOutlet NSView *notificationsView;
    IBOutlet NSView *appearanceView;
    
    __weak IBOutlet NSButton *iTunesStyleCheck;
}

- (HotKeyPane *)hotKeyPane;
- (OutputPane *)outputPane;
- (MIDIPane *)midiPane;

- (GeneralPreferencePane *)updatesPane;
- (GeneralPreferencePane *)scrobblerPane;
- (GeneralPreferencePane *)playlistPane;
- (GeneralPreferencePane *)notificationsPane;
- (GeneralPreferencePane *)appearancePane;


@end
