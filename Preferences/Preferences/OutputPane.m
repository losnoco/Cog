//
//  OutputPane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "OutputPane.h"

@implementation OutputPane

- (NSString *)title {
	return NSLocalizedPrefString(@"Output");
}

- (NSImage *)icon {
	if(@available(macOS 11.0, *))
		return [NSImage imageWithSystemSymbolName:@"hifispeaker.2.fill" accessibilityDescription:nil];
	return [[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[self class]] pathForImageResource:@"output"]];
}

- (IBAction)takeDeviceID:(id)sender {
	NSDictionary *device = [[outputDevices selectedObjects] objectAtIndex:0];
	[[NSUserDefaults standardUserDefaults] setObject:device forKey:@"outputDevice"];
}

@end
