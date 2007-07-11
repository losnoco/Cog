//
//  HotKeyPane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "HotKeyPane.h"
#import "NDHotKeyEvent.h"

@implementation HotKeyPane

- (void)awakeFromNib
{
	[self setName:NSLocalizedStringFromTableInBundle(@"Hot Keys", nil, [NSBundle bundleForClass:[self class]], @"") ];
	[self setIcon:@"hot_keys"];	

//	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidBecomeKey:) name:NSWindowDidBecomeKeyNotification object: [view window]];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowDidResignKey:) name:NSWindowDidResignKeyNotification object: [view window]];
	
	[prevHotKeyControl setKeyCode: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyPreviousKeyCode"] ];
	[prevHotKeyControl setCharacter: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyPreviousCharacter"] ];
	[prevHotKeyControl setModifierFlags: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyPreviousModifiers"] ];
	
	[prevHotKeyControl updateStringValue];
	
	[nextHotKeyControl setKeyCode: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyNextKeyCode"] ];
	[nextHotKeyControl setCharacter: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyNextCharacter"] ];
	[nextHotKeyControl setModifierFlags: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyNextModifiers"] ];
	
	[nextHotKeyControl updateStringValue];
	
	[playHotKeyControl setKeyCode: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyPlayKeyCode"] ];
	[playHotKeyControl setCharacter: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyPlayCharacter"] ];
	[playHotKeyControl setModifierFlags: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyPlayModifiers"] ];
	
	[playHotKeyControl updateStringValue];
}

/*- (void)windowDidBecomeKey:(id)notification
{
	if ([notification object] == [view window]) {
		[playHotKeyControl startObserving];
		[prevHotKeyControl startObserving];
		[nextHotKeyControl startObserving];
	}
}
*/
- (void)windowDidResignKey:(id)notification
{
	if ([notification object] == [view window]) {
		[playHotKeyControl stopObserving];
		[prevHotKeyControl stopObserving];
		[nextHotKeyControl stopObserving];
	}
}

- (IBAction) grabPlayHotKey:(id)sender
{
	[playHotKeyControl startObserving];
}

- (IBAction) grabPrevHotKey:(id)sender
{
	[prevHotKeyControl startObserving];
}

- (IBAction) grabNextHotKey:(id)sender
{
	[nextHotKeyControl startObserving];
}

- (IBAction) hotKeyChanged:(id)sender
{
	if (sender == playHotKeyControl) {
		[[NSUserDefaults standardUserDefaults] setInteger:[playHotKeyControl character] forKey:@"hotKeyPlayCharacter"];
		[[NSUserDefaults standardUserDefaults] setInteger:[playHotKeyControl modifierFlags] forKey:@"hotKeyPlayModifiers"];
		[[NSUserDefaults standardUserDefaults] setInteger:[playHotKeyControl keyCode] forKey:@"hotKeyPlayKeyCode"];
	}
	else if (sender == prevHotKeyControl) {
		[[NSUserDefaults standardUserDefaults] setInteger:[prevHotKeyControl character] forKey:@"hotKeyPreviousCharacter"];
		[[NSUserDefaults standardUserDefaults] setInteger:[prevHotKeyControl modifierFlags] forKey:@"hotKeyPreviousModifiers"];
		[[NSUserDefaults standardUserDefaults] setInteger:[prevHotKeyControl keyCode] forKey:@"hotKeyPreviousKeyCode"];
	}
	else if (sender == nextHotKeyControl) {
		[[NSUserDefaults standardUserDefaults] setInteger:[nextHotKeyControl character] forKey:@"hotKeyNextCharacter"];
		[[NSUserDefaults standardUserDefaults] setInteger:[nextHotKeyControl modifierFlags] forKey:@"hotKeyNextModifiers"];
		[[NSUserDefaults standardUserDefaults] setInteger:[nextHotKeyControl keyCode] forKey:@"hotKeyNextKeyCode"];
	}
	
	[sender stopObserving];
}

@end
