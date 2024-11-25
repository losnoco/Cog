//
//  AppearancePane.m
//  General
//
//  Created by Christopher Snowhill on 11/24/24.
//
//

#import "AppearancePane.h"

static NSString *CogCustomDockIconsReloadNotification = @"CogCustomDockIconsReloadNotification";

@implementation AppearancePane

- (NSString *)title {
	return NSLocalizedPrefString(@"Appearance");
}

- (NSImage *)icon {
	if(@available(macOS 11.0, *))
		return [NSImage imageWithSystemSymbolName:@"paintpalette.fill" accessibilityDescription:nil];
	return [[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[self class]] pathForImageResource:@"appearance"]];
}

- (void)setDockIcon:(NSString *)baseName {
	NSArray *fileTypes = @[@"jpg", @"jpeg", @"png", @"gif", @"webp", @"avif", @"heic"];
	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:NO];
	[panel setCanChooseDirectories:NO];
	[panel setCanChooseFiles:YES];
	[panel setFloatingPanel:YES];
	[panel setAllowedFileTypes:fileTypes];
	NSInteger result = [panel runModal];
	if(result == NSModalResponseOK) {
		NSError *error = nil;
		NSData *iconData = [NSData dataWithContentsOfURL:[panel URL] options:NSDataReadingMappedIfSafe error:&error];
		if(iconData && !error) {
			NSImage *icon = [[NSImage alloc] initWithData:iconData];
			if(icon) {
				CGImageRef cgRef = [icon CGImageForProposedRect:NULL
														context:nil
														  hints:nil];

				if(cgRef) {
					NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
					NSString *basePath = [[paths firstObject] stringByAppendingPathComponent:@"Cog"];
					basePath = [basePath stringByAppendingPathComponent:@"Icons"];
					NSFileManager *fileManager = [NSFileManager defaultManager];
					BOOL isDirectory = NO;
					if(![fileManager fileExistsAtPath:basePath isDirectory:&isDirectory] || !isDirectory) {
						if(!isDirectory) {
							[fileManager removeItemAtPath:basePath error:&error];
						}
						[fileManager createDirectoryAtURL:[NSURL fileURLWithPath:basePath] withIntermediateDirectories:YES attributes:nil error:&error];
					}

					NSString *filePath = [basePath stringByAppendingPathComponent:baseName];
					filePath = [filePath stringByAppendingPathExtension:@"png"];

					NSBitmapImageRep *newRep =
					[[NSBitmapImageRep alloc] initWithCGImage:cgRef];
					NSData *pngData = [newRep
									   representationUsingType:NSBitmapImageFileTypePNG
									   properties:@{}];
					[pngData writeToURL:[NSURL fileURLWithPath:filePath] atomically:YES];

					// Now to refresh the icons by a little trickery, if enabled
					BOOL enabled = [[NSUserDefaults standardUserDefaults] boolForKey:@"customDockIcons"];
					if(enabled) {
						[[NSNotificationCenter defaultCenter] postNotificationName:CogCustomDockIconsReloadNotification object:nil];
					}
				}
			}
		}
	}
}

- (IBAction)setDockIconStop:(id)sender {
	[self setDockIcon:@"Stop"];
}

- (IBAction)setDockIconPlay:(id)sender {
	[self setDockIcon:@"Play"];
}

- (IBAction)setDockIconPause:(id)sender {
	[self setDockIcon:@"Pause"];
}

@end
