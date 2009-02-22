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
		[self setContentBorderThickness:24.0 forEdge:NSMinYEdge];
	}
	
	return self;
}

- (void)awakeFromNib
{
	if ([self hiddenDefaultsKey]) {
		// Hide the mini window by default.
		[[NSUserDefaults standardUserDefaults] registerDefaults:[NSDictionary dictionaryWithObject:[NSNumber numberWithBool:YES] forKey:[self hiddenDefaultsKey]]];
	}
	
	[super awakeFromNib];
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize {
	// Do not allow height to change
	proposedFrameSize.height = [self frame].size.height;
	
	return proposedFrameSize;
}

@end
