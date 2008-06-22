//
//  InvertedToolbarWindow.m
//  Cog
//
//  Created by Vincent Spader on 10/31/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "InvertedToolbarWindow.h"


@implementation InvertedToolbarWindow

+ (void)initialize
{
	NSMutableDictionary *userDefaultsValuesDict = [NSMutableDictionary dictionary];
	
	[userDefaultsValuesDict setObject:[NSNumber numberWithBool:NO] forKey:@"WindowContentHidden"];

	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
}

- (void)awakeFromNib
{
	contentHidden = [[NSUserDefaults standardUserDefaults] boolForKey:@"WindowContentHidden"];
	if (contentHidden)
	{
		[self hideContentwithAnimation:YES];
	}
}

- (void)toggleToolbarShown:(id)sender
{
	if (contentHidden) //Show
	{
		[self showContent];
	}
	else //Hide
	{
		[self hideContent];
	}
}

- (void)hideContent
{
	[self hideContentwithAnimation:YES];
}

- (void)hideContentwithAnimation:(BOOL)animate
{
	NSRect newFrame = [self frame];

	contentSize = [[self contentView] bounds].size;

	newFrame.origin.y += contentSize.height;
	newFrame.size.height -= contentSize.height;
	
	[self setShowsResizeIndicator:NO];
	
	[[self contentView] setAutoresizesSubviews:NO];
	[self setFrame:newFrame display:YES animate:animate];
	
	contentHidden = YES;
	[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"WindowContentHidden"];
}

- (void)showContent
{
	NSRect newFrame = [self frame];
	
	newFrame.origin.y -= contentSize.height;
	newFrame.size.height += contentSize.height;
	
	[[self contentView] resizeSubviewsWithOldSize:NSMakeSize(contentSize.width, 0)];
	
	[self setFrame:newFrame display:YES animate:YES];
	
	[[self contentView] setAutoresizesSubviews:YES];
	
	[self setShowsResizeIndicator:YES];

	contentHidden = NO;
	[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"WindowContentHidden"];
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize {
	if (contentHidden) {
		proposedFrameSize.height = [self frame].size.height;
	}
	
	return proposedFrameSize;
}

@end
