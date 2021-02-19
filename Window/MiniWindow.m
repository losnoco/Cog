//
//  MiniWindow.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "MiniWindow.h"

@implementation MiniWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation
{
    self = [super initWithContentRect:contentRect styleMask:windowStyle backing:bufferingType defer:deferCreation];
    if (self)
    {
        [self setShowsResizeIndicator:NO];
        [self setExcludedFromWindowsMenu:YES];
        [[self standardWindowButton:NSWindowZoomButton] setEnabled:NO];
        // Disallow height resize.
        [self setContentMinSize:NSMakeSize(675, 0)];
        [self setContentMaxSize:NSMakeSize(CGFLOAT_MAX, 0)];
        [self setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];
    }

    return self;
}

- (void)toggleToolbarShown:(id)sender {
    // Mini window IS the toolbar, no point in hiding it.
    // Do nothing!
}

- (void)keyDown:(NSEvent *)e {
    BOOL modifiersUsed =
        ([e modifierFlags] & (NSEventModifierFlagShift |
                              NSEventModifierFlagControl |
                              NSEventModifierFlagOption |
                              NSEventModifierFlagCommand)) ? YES : NO;
    NSString *characters = [e charactersIgnoringModifiers];
    
    if (modifiersUsed || [characters length] != 1)
    {
        [super keyDown:e];

        return;
    }

    unichar c = [characters characterAtIndex:0];
    switch (c) {
        case 0x20: // Spacebar
            [playbackController playPauseResume:self];
            break;

        case NSEnterCharacter:
        case NSCarriageReturnCharacter:
            [playbackController play:self];
            break;

        case NSLeftArrowFunctionKey:
            [playbackController eventSeekBackward:self];
            break;

        case NSRightArrowFunctionKey:
            [playbackController eventSeekForward:self];
            break;

        case NSUpArrowFunctionKey:
            [playbackController volumeUp:self];
            break;

        case NSDownArrowFunctionKey:
            [playbackController volumeDown:self];
            break;

        default:
            [super keyDown:e];
            break;
    }
}


@end
