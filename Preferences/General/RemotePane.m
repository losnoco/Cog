//
//  FileDrawerPane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "RemotePane.h"


@implementation RemotePane

- (void)awakeFromNib
{
	[self setName:@"Remote"];
	[self setIcon:@"apple_remote"];
	
	[onlyOnActive setState:[[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"remoteOnlyOnActive"] boolValue]];
}

- (IBAction)takeBool:(id)sender
{
		[[NSUserDefaults standardUserDefaults] setObject:[NSNumber numberWithBool:[onlyOnActive state]] forKey:@"remoteOnlyOnActive"];
}

@end
