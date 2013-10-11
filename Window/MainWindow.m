//
//  MainWindow.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "MainWindow.h"


@implementation MainWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation
{
	self = [super initWithContentRect:contentRect styleMask:windowStyle backing:bufferingType defer:deferCreation];
	if (self)
	{
		[self setExcludedFromWindowsMenu:YES];
		[self setContentBorderThickness:24.0 forEdge:NSMinYEdge];
        [self setCollectionBehavior:NSWindowCollectionBehaviorFullScreenPrimary];
    }
	
	return self;
}

- (void)awakeFromNib
{
    if ([self respondsToSelector:@selector(toggleFullScreen:)])
    {
        [itemLionSeparator setHidden:NO];
        [itemLionFullscreenToggle setHidden:NO];
    }

	[super awakeFromNib];
}

@end
