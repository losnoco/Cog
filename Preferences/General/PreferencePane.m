//
//  PreferencePane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "PreferencePane.h"


@implementation PreferencePane

+ (PreferencePane *)preferencePaneWithView:(NSView *)v name:(NSString *)n icon:(NSString *)i
{
	PreferencePane *pane = [[[PreferencePane alloc] init] autorelease];
	if (pane)
	{
		[pane setView:v];
		[pane setName:n];
		[pane setIcon:i];
	}
	
	return pane;
}

- (NSView *)paneView
{
	return view;
}

- (NSString *)paneName
{
	return name;
}

- (NSImage *)paneIcon
{
	return icon;
}

- (NSString *)paneToolTip
{
	return nil;
}

- (BOOL)allowsHorizontalResizing
{
	return NO;
}

- (BOOL)allowsVerticalResizing
{
	return NO;
}

- (void)setView:(NSView *)v
{
	[v retain];
	[view release];
	view = v;
}

- (void)setName:(NSString *)n
{
	[n retain];
	[name release];
	name = n;
}

- (void)setIcon:(NSString *)i
{
	[icon release];
    icon = [[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[self class]] pathForImageResource:i]];
}

@end
