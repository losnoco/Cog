//
//  OutputPane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "OutputPane.h"

static NSString *CogPlaybackDidResetHeadTracking = @"CogPlaybackDidResetHeadTracking";

@implementation OutputPane

- (NSString *)title {
	return NSLocalizedPrefString(@"Output");
}

- (NSImage *)icon {
	if(@available(macOS 14.0, *)) {
		/* do nothing */
	} else {
		[headTracking setHidden:YES];
		[headRecenter setHidden:YES];
	}
	if(@available(macOS 11.0, *))
		return [NSImage imageWithSystemSymbolName:@"hifispeaker.2.fill" accessibilityDescription:nil];
	return [[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[self class]] pathForImageResource:@"output"]];
}

- (IBAction)takeDeviceID:(id)sender {
	NSDictionary *device = [[outputDevices selectedObjects] objectAtIndex:0];
	[[NSUserDefaults standardUserDefaults] setObject:device forKey:@"outputDevice"];
}

- (IBAction)resetHeadTracking:(id)sender {
	[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidResetHeadTracking object:nil];
}

@end
