//
//  InvertedToolbarWindow.m
//  Cog
//
//  Created by Vincent Spader on 10/31/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "DualWindow.h"


@implementation DualWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation
{
	self = [super initWithContentRect:contentRect styleMask:windowStyle backing:bufferingType defer:deferCreation];
	if (self)
	{
	}
	
	return self;
}

- (NSString *)hiddenDefaultsKey
{
	if ([self frameAutosaveName])
	{
		return [[self frameAutosaveName] stringByAppendingString:@" Window Content Height"];
	}
	
	return nil;
}

- (BOOL)isHidden
{
	return [[NSUserDefaults standardUserDefaults] boolForKey:[self hiddenDefaultsKey]];
}

- (void)setHidden:(BOOL)h
{
	[[NSUserDefaults standardUserDefaults] setBool:h forKey:[self hiddenDefaultsKey]];
}

- (void)toggleToolbarShown:(id)sender
{
	[otherWindow showWindow];
}

- (void)showWindow
{
	[otherWindow close];
	[self makeKeyAndOrderFront:self];
}

@end
