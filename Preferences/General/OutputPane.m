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
	[self setName:NSLocalizedStringFromTableInBundle(@"Output", nil, [NSBundle bundleForClass:[self class]], @"") ];
	[self setIcon:@"output"];
}

- (IBAction) takeDeviceID:(id)sender
{
	NSDictionary *device = [[outputDevices selectedObjects] objectAtIndex:0];
	[[NSUserDefaults standardUserDefaults] setObject: device forKey:@"outputDevice"];
}


@end
