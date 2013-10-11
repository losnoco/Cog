//
//  MiniWindow.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "MiniWindow.h"


@implementation MiniWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation
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


@end
