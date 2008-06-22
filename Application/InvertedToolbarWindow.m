//
//  InvertedToolbarWindow.m
//  Cog
//
//  Created by Vincent Spader on 10/31/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "InvertedToolbarWindow.h"


@implementation InvertedToolbarWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation
{
	self = [super initWithContentRect:contentRect styleMask:windowStyle backing:bufferingType defer:deferCreation];
	if (self)
	{
		contentHidden = NO;
	}
	
	return self;
}

- (NSString *)contentHiddenDefaultsKey
{
	if ([self frameAutosaveName])
	{
		return [[self frameAutosaveName] stringByAppendingString:@" Window Content Hidden"];
	}
	
	return nil;
}

- (NSString *)contentHeightDefaultsKey
{
	if ([self frameAutosaveName])
	{
		return [[self frameAutosaveName] stringByAppendingString:@" Window Content Height"];
	}
	
	return nil;
}

- (void)awakeFromNib
{
	NSString *contentHiddenDefaultsKey = [self contentHiddenDefaultsKey];
	if (contentHiddenDefaultsKey != nil)
	{
		NSMutableDictionary *userDefaultsValuesDict = [NSMutableDictionary dictionary];
		[userDefaultsValuesDict setObject:[NSNumber numberWithBool:NO] forKey:contentHiddenDefaultsKey];
		[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
	}
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:contentHiddenDefaultsKey])
	{
		//Rejigger the height
		float contentHeight = [[NSUserDefaults standardUserDefaults] floatForKey:[self contentHeightDefaultsKey]];
		
		contentSize = [[self contentView] bounds].size;
		
		NSRect newFrame = [self frame];
		newFrame.size.height -= contentSize.height;
		newFrame.size.height += contentHeight;
		
		[self setFrame:newFrame display:NO animate:NO];

		[self hideContent];
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
	NSRect newFrame = [self frame];

	BOOL animate = YES;
	contentSize = [[self contentView] bounds].size;
	[[NSUserDefaults standardUserDefaults] setFloat:contentSize.height forKey:[self contentHeightDefaultsKey]];
	
	if (contentHidden)
	{
		//Content is already set as hidden...so we are pretending we were hidden the whole time.
		//Currently, this only happens on init.
		
		animate = NO;
	}

	newFrame.origin.y += contentSize.height;
	newFrame.size.height -= contentSize.height;
	
	[self setShowsResizeIndicator:NO];
	
	[[self contentView] setAutoresizesSubviews:NO];
	[self setFrame:newFrame display:YES animate:animate];
	
	[self setContentHidden:YES];
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

	[self setContentHidden:NO];
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize {
	if (contentHidden) {
		proposedFrameSize.height = [self frame].size.height;
	}
	
	return proposedFrameSize;
}


- (void)setContentHidden:(BOOL)hidden
{
	[[NSUserDefaults standardUserDefaults] setBool:hidden forKey:[self contentHiddenDefaultsKey]];
	contentHidden = hidden;
}

@end
