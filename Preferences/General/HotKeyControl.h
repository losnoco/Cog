//
//  HotKeyControl.h
//  General
//
//  Created by Zaphod Beeblebrox on 9/4/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "NDHotKeyControl.h"

@interface HotKeyControl : NDHotKeyControl {
	BOOL observing;
}

- (void)enableAllHotKeys;
- (void)disableAllHotKeys;

- (void)startObserving;
- (void)stopObserving;

- (void)setKeyCode: (unsigned short)k;
- (void)setCharacter: (unichar)c;
- (void)setModifierFlags: (unsigned long)m;

- (void)updateStringValue;

@end
