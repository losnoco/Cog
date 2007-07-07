//
//  OutputPane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "OutputPane.h"


@implementation OutputPane

- (void)awakeFromNib
{
	NSLog(@"AWOKEN!");
	[self setName:NSLocalizedString(@"Output", @"")];
	[self setIcon:@"output"];
}

- (IBAction) takeDeviceID:(id)sender
{
	NSLog(@"Taking thing: %@", [outputDevices selectedObjects]);
	NSDictionary *device = [[outputDevices selectedObjects] objectAtIndex:0];
	[[NSUserDefaults standardUserDefaults] setObject: device forKey:@"outputDevice"];
}


@end
