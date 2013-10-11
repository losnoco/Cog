//
//  HotKeyControl.m
//  General
//
//  Created by Zaphod Beeblebrox on 9/4/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "HotKeyControl.h"
#import <NDHotKey/NDHotKeyEvent.h>

@implementation HotKeyControl

- (void)mouseDown:(NSEvent *)theEvent
{
    [self setStringValue:NSLocalizedStringFromTableInBundle(@"Press Key...", nil, [NSBundle bundleForClass:[self class]], @"")];
    [self setRequiresModifierKeys:YES];
    [self setReadyForHotKeyEvent:YES];
}

@end
