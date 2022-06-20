//
//  GeneralPane.m
//  Preferences
//
//  Created by Christopher Snowhill on 6/20/22.
//

#import "GeneralPane.h"

@implementation GeneralPane

- (NSString *)title {
	return NSLocalizedPrefString(@"General");
}

- (NSImage *)icon {
	if(@available(macOS 11.0, *))
		return [NSImage imageWithSystemSymbolName:@"gearshape.fill" accessibilityDescription:nil];
	return [[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[self class]] pathForImageResource:@"general"]];
}

- (IBAction)addPath:(id)sender {
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:NO];
	[panel setCanChooseDirectories:YES];
	[panel setCanChooseFiles:NO];
	[panel setFloatingPanel:YES];
	NSInteger result = [panel runModal];
	if(result == NSModalResponseOK) {
		[sandboxBehaviorController addUrl:[panel URL]];
	}
}

- (IBAction)deleteSelectedPaths:(id)sender {
	NSArray *selectedObjects = [sandboxBehaviorController selectedObjects];
	if(selectedObjects && [selectedObjects count]) {
		NSArray *paths = [selectedObjects valueForKey:@"path"];
		for(NSString *path in paths) {
			[sandboxBehaviorController removePath:path];
		}
	}
}

@end
