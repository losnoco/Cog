//
//  SplitViewController.m
//  Cog
//
//  Created by Vincent Spader on 6/20/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "FileTreeViewController.h"
#import "PlaybackController.h"
#import "PlaylistLoader.h"
#import "SandboxBroker.h"

@implementation FileTreeViewController

- (id)init {
	return [super initWithNibName:@"FileTree" bundle:[NSBundle mainBundle]];
}

- (void)addToPlaylistInternal:(NSArray *)urls {
	[self doAddToPlaylist:urls origin:URLOriginInternal];
}

- (void)addToPlaylistExternal:(NSArray *)urls {
	[self doAddToPlaylist:urls origin:URLOriginExternal];
}

- (void)doAddToPlaylist:(NSArray *)urls origin:(URLOrigin)origin {
	NSDictionary *loadEntryData = @{ @"entries": urls,
		                             @"sort": @(YES),
		                             @"origin": @(origin) };
	[playlistLoader performSelectorInBackground:@selector(addURLsInBackground:) withObject:loadEntryData];
}

- (void)clear:(id)sender {
	[playlistLoader clear:sender];
}

- (void)playPauseResume:(NSObject *)id {
	[playbackController playPauseResume:id];
}

- (FileTreeOutlineView *)outlineView {
	return fileTreeOutlineView;
}

- (IBAction)chooseRootFolder:(id)sender {
	NSString *path = [[NSUserDefaults standardUserDefaults] stringForKey:@"fileTreeRootURL"];

	NSOpenPanel *panel = [NSOpenPanel openPanel];
	[panel setAllowsMultipleSelection:NO];
	[panel setCanChooseDirectories:YES];
	[panel setCanChooseFiles:NO];
	[panel setFloatingPanel:YES];
	if(path) {
		[panel setDirectoryURL:[NSURL fileURLWithPath:path]];
	}
	[panel setTitle:@"Open to choose tree path"];
	NSInteger result = [panel runModal];
	if(result == NSModalResponseOK) {
		[[SandboxBroker sharedSandboxBroker] addFolderIfMissing:[panel URL]];
		[[NSUserDefaults standardUserDefaults] setValue:[[panel URL] absoluteString] forKey:@"fileTreeRootURL"];
	}
}

@end
