//
//  OutputPane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "OutputPane.h"
#import "HeadphoneFilter.h"

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

- (IBAction)setHrir:(id)sender {
	NSArray *fileTypes = @[@"wav", @"wv"];
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:NO];
	[panel setCanChooseDirectories:NO];
	[panel setCanChooseFiles:YES];
	[panel setFloatingPanel:YES];
	[panel setAllowedFileTypes:fileTypes];
	NSString *oldPath = [[NSUserDefaults standardUserDefaults] stringForKey:@"hrirPath"];
	if(oldPath != nil)
		[panel setDirectoryURL:[NSURL fileURLWithPath:oldPath]];
	NSInteger result = [panel runModal];
	if(result == NSModalResponseOK) {
		NSString *path = [[panel URL] path];
		if([NSClassFromString(@"HeadphoneFilter") validateImpulseFile:[NSURL fileURLWithPath:path]])
			[[NSUserDefaults standardUserDefaults] setValue:[[panel URL] path] forKey:@"hrirPath"];
		else {
			NSAlert *alert = [[NSAlert alloc] init];
			[alert setMessageText:@"Invalid impulse"];
			[alert setInformativeText:@"The selected file does not conform to the HeSuVi HRIR specification."];
			[alert addButtonWithTitle:@"Ok"];
			[alert runModal];
		}
	}
}

- (IBAction)clearHrir:(id)sender {
	[[NSUserDefaults standardUserDefaults] setValue:@"" forKey:@"hrirPath"];
}

@end
