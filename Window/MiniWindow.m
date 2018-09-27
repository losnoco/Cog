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
        [self setCollectionBehavior:NSWindowCollectionBehaviorFullScreenAuxiliary];
	}
	
	return self;
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize {
	// Do not allow height to change
	proposedFrameSize.height = [self frame].size.height;
	
	return proposedFrameSize;
}

- (void)toggleToolbarShown:(id)sender {
    // Mini window IS the toolbar, no point in hiding it.
    // Do nothing!
}

- (void)keyDown:(NSEvent *)e {
    unsigned int   modifiers = [e modifierFlags] & (NSCommandKeyMask | NSShiftKeyMask | NSControlKeyMask | NSAlternateKeyMask);
    NSString       *characters = [e characters];
    unichar        c;
    
    if ([characters length] != 1)
    {
        [super keyDown:e];
        
        return;
    }
    
    c = [characters characterAtIndex:0];
    if (modifiers == 0 && c == ' ')
    {
        [playbackController playPauseResume:self];
    }
    else if (modifiers == 0 && (c == NSEnterCharacter || c == NSCarriageReturnCharacter))
    {
        [playbackController play:self];
    }
    else if (modifiers == 0 && c == NSLeftArrowFunctionKey)
    {
        [playbackController eventSeekBackward:self];
    }
    else if (modifiers == 0 && c == NSRightArrowFunctionKey)
    {
        [playbackController eventSeekForward:self];
    }
    else
    {
        [super keyDown:e];
    }

}


@end
