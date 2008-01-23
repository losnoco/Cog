//
//  InvertedToolbarWindow.m
//  Cog
//
//  Created by Vincent Spader on 10/31/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "InvertedToolbarWindow.h"


@implementation InvertedToolbarWindow

- (void)awakeFromNib
{
	contentHidden = NO;
}

- (void)toggleToolbarShown:(id)sender
{
	if (contentHidden) //Show
	{
		NSRect newFrame = [self frame];
	
		newFrame.origin.y -= contentHeight;
		newFrame.size.height += contentHeight;
		
		[[self contentView] resizeSubviewsWithOldSize:NSMakeSize(contentWidth, 0)];

		[self setFrame:newFrame display:YES animate:YES];
		
		[[self contentView] setAutoresizesSubviews:YES];

		[self setShowsResizeIndicator:YES];
	}
	else //Hide
	{
		NSRect newFrame = [self frame];
	
		contentWidth = [[self contentView] bounds].size.width;
		contentHeight = [[self contentView] bounds].size.height;
		
		newFrame.origin.y += contentHeight;
		newFrame.size.height -= contentHeight;
		
		[self setShowsResizeIndicator:NO];

		[[self contentView] setAutoresizesSubviews:NO];
		[self setFrame:newFrame display:YES animate:YES];
	}
	
	contentHidden = !contentHidden;
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize {
	if (contentHidden) {
		proposedFrameSize.height = [self frame].size.height;
	}
	
	return proposedFrameSize;
}

@end
