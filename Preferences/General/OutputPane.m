//
//  OutputPane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "OutputPane.h"


@implementation OutputPane

- (NSString *)title
{
	return NSLocalizedStringFromTableInBundle(@"Output", nil, [NSBundle bundleForClass:[self class]], @"");
}

- (NSImage *)icon
{
	return [[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[self class]] pathForImageResource:@"output"]];
}



- (IBAction) takeDeviceID:(id)sender
{
	NSDictionary *device = [[outputDevices selectedObjects] objectAtIndex:0];
	[[NSUserDefaults standardUserDefaults] setObject: device forKey:@"outputDevice"];
}


@end
