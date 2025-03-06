//
//  HotKeyPane.h
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "GeneralPreferencePane.h"
#import <Cocoa/Cocoa.h>
#import <MASShortcut/Shortcut.h>

@interface HotKeyPane : GeneralPreferencePane

@property(strong) IBOutlet MASShortcutView *playShortcutView;
@property(strong) IBOutlet MASShortcutView *nextShortcutView;
@property(strong) IBOutlet MASShortcutView *prevShortcutView;
@property(strong) IBOutlet MASShortcutView *spamShortcutView;
@property(strong) IBOutlet MASShortcutView *fadeShortcutView;
@property(strong) IBOutlet MASShortcutView *seekBkwdShortcutView;
@property(strong) IBOutlet MASShortcutView *seekFwdShortcutView;

- (IBAction)resetToDefaultShortcuts:(id)sender;

@end
