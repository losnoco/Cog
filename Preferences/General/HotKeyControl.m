//
//  HotKeyControl.m
//  General
//
//  Created by Zaphod Beeblebrox on 9/4/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "HotKeyControl.h"

typedef int CGSConnection;
typedef enum {
    CGSGlobalHotKeyEnable = 0,
    CGSGlobalHotKeyDisable = 1,
} CGSGlobalHotKeyOperatingMode;

extern CGSConnection _CGSDefaultConnection(void);

extern CGError CGSGetGlobalHotKeyOperatingMode(
                                               CGSConnection connection, CGSGlobalHotKeyOperatingMode *mode);

extern CGError CGSSetGlobalHotKeyOperatingMode(CGSConnection connection, 
                                               CGSGlobalHotKeyOperatingMode mode);

@implementation HotKeyControl

- (void)awakeFromNib
{
	observing = NO;
}

- (void)disableAllHotKeys
{
	CGSConnection conn = _CGSDefaultConnection();
    CGSSetGlobalHotKeyOperatingMode(conn, CGSGlobalHotKeyDisable);
}

- (void)enableAllHotKeys
{
    CGSConnection conn = _CGSDefaultConnection();
    CGSSetGlobalHotKeyOperatingMode(conn, CGSGlobalHotKeyEnable);	
}

- (void)startObserving
{
	[self disableAllHotKeys];
	
	observing = YES;
	[self setStringValue:NSLocalizedStringFromTableInBundle(@"Press Key...", nil, [NSBundle bundleForClass:[self class]], @"")];
}

- (void)stopObserving
{	
	[self enableAllHotKeys];
	observing = NO;

	[self updateStringValue];
}	

- (BOOL)becomeFirstResponder
{
	[self startObserving];
	
	return YES;
}

- (BOOL)resignFirstResponder
{
	[self stopObserving];
	
	return YES;
}

- (BOOL)performKeyEquivalent:(NSEvent*)anEvent
{
	if (observing == YES)
	{
		return [super performKeyEquivalent:anEvent];
	}
	else
	{
		return NO;
	}
}

- (void)keyDown:(NSEvent *)theEvent
{
	if (observing == YES)
	{
		[super keyDown:theEvent];
	}
}

- (void)mouseDown:(NSEvent *)theEvent
{
	[self startObserving];
}

- (void)setKeyCode: (unsigned short)k
{
	keyCode = k;
}

- (void)setCharacter: (unichar)c
{
	character = c;
}

- (void)setModifierFlags: (unsigned long)m
{
	modifierFlags = m;
}

- (void)updateStringValue
{
	[self setStringValue:stringForKeyCodeAndModifierFlags( keyCode, character, modifierFlags )];
}
@end
