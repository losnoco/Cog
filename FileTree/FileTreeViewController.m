//
//  FileTreeViewController.m
//  Cog
//
//  Created by Vincent Spader on 6/20/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "FileTreeViewController.h"
#import "FileTreeOutlineView.h"
#import "PlaybackController.h"
#import "PlaylistLoader.h"
#import "SandboxBroker.h"

@implementation FileTreeViewController {
	CGFloat appliedSafeAreaTopInset;
}

- (id)init {
	self = [super initWithNibName:@"FileTree" bundle:[NSBundle mainBundle]];
	if(self) {
		[[NSUserDefaults standardUserDefaults] registerDefaults:@{ @"FileTreeShowSideView": @NO }];
	}
	return self;
}

- (void)viewDidLoad {
	[super viewDidLoad];

	fileTreeOutlineView.style = NSTableViewStyleSourceList;
}

// The view is laid out with autoresizing masks, so when the sidebar extends
// into the titlebar (full-height layout) the top controls must be shifted
// down by the safe area inset manually. The inset changes when the toolbar
// style is toggled at runtime, hence the delta-based adjustment.
- (void)viewWillLayout {
	[super viewWillLayout];

	{
		CGFloat topInset = self.view.safeAreaInsets.top;
		CGFloat delta = topInset - appliedSafeAreaTopInset;
		if(delta != 0) {
			for(NSView *subview in self.view.subviews) {
				NSRect frame = subview.frame;
				if(subview.autoresizingMask & NSViewHeightSizable) {
					frame.size.height -= delta;
				} else {
					frame.origin.y -= delta;
				}
				subview.frame = frame;
			}
			appliedSafeAreaTopInset = topInset;
		}
	}
}

- (NSSplitViewItem *)sidebarItem {
	NSViewController *parent = self.parentViewController;
	if([parent isKindOfClass:[NSSplitViewController class]]) {
		return [(NSSplitViewController *)parent splitViewItemForViewController:self];
	}
	return nil;
}

- (IBAction)toggleSideView:(id)sender {
	NSViewController *parent = self.parentViewController;
	if([parent isKindOfClass:[NSSplitViewController class]]) {
		[(NSSplitViewController *)parent toggleSidebar:sender];
	}
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
	if([menuItem action] == @selector(toggleSideView:)) {
		NSSplitViewItem *sidebarItem = [self sidebarItem];
		[menuItem setState:(sidebarItem && !sidebarItem.isCollapsed) ? NSControlStateValueOn : NSControlStateValueOff];
	}
	return YES;
}

- (void)addToPlaylistInternal:(NSArray *)urls {
	[self doAddToPlaylist:urls origin:URLOriginInternal];
}

- (void)addToPlaylistExternal:(NSArray *)urls {
	[self doAddToPlaylist:urls origin:URLOriginExternal];
}

- (void)doAddToPlaylist:(NSArray *)urls origin:(URLOrigin)origin {
	NSDictionary *loadEntryData = @{ @"entries": urls,
		                             @"sort": @YES,
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
	NSInteger result = [panel runModal];
	if(result == NSModalResponseOK) {
		[[SandboxBroker sharedSandboxBroker] addFolderIfMissing:[panel URL]];
		[[NSUserDefaults standardUserDefaults] setValue:[[panel URL] absoluteString] forKey:@"fileTreeRootURL"];
	}
}

@end
