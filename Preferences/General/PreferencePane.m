//
//  PreferencePane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "PreferencePane.h"


@implementation PreferencePane

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

- (void)setName:(NSString *)s
{
	[s retain];
	[name release];
	name = s;
}

- (void)setIcon:(NSString *)i
{
	[icon release];
    icon = [[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[self class]] pathForImageResource:i]];
}

@end
