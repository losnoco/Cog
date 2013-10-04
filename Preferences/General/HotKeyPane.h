//
//  HotKeyPane.h
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "GeneralPreferencePane.h"
#import "HotKeyControl.h"

@interface HotKeyPane : GeneralPreferencePane {
	IBOutlet HotKeyControl *playHotKeyControl;
	IBOutlet HotKeyControl *prevHotKeyControl;
	IBOutlet HotKeyControl *nextHotKeyControl;
    IBOutlet HotKeyControl *spamHotKeyControl;
}

- (IBAction) grabPlayHotKey:(id)sender;
- (IBAction) grabPrevHotKey:(id)sender;
- (IBAction) grabNextHotKey:(id)sender;
- (IBAction) grabSpamHotKey:(id)sender;

- (IBAction) hotKeyChanged:(id)sender;

@end
