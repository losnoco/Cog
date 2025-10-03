//
//  FileTreeController.m
//  Cog
//
//  Created by Vincent Spader on 2/17/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "FileTreeController.h"
#import "PlaylistController.h"
#import "SideViewController.h"

@implementation FileTreeController

- (IBAction)addToPlaylist:(id)sender {
	[self doAddToPlaylist:sender origin:URLOriginInternal];
}

- (void)doAddToPlaylist:(id)sender origin:(URLOrigin)origin {
	NSUInteger index;
	NSIndexSet *selectedIndexes = [outlineView selectedRowIndexes];
	NSMutableArray *urls = [NSMutableArray new];

	for(index = [selectedIndexes firstIndex];
	    index != NSNotFound; index = [selectedIndexes indexGreaterThanIndex:index]) {
		[urls addObject:[[outlineView itemAtRow:index] URL]];
	}

	[controller doAddToPlaylist:urls origin:origin];
}

- (void)addToPlaylistExternal:(id)sender {
	[self doAddToPlaylist:sender origin:URLOriginExternal];
}

- (IBAction)setAsPlaylist:(id)sender {
	[controller clear:sender];
	[self addToPlaylist:sender];
}

- (IBAction)playPauseResume:(NSObject *)id {
	[controller playPauseResume:id];
}

- (IBAction)showEntryInFinder:(id)sender {
	NSUInteger index;
	NSWorkspace *ws = [NSWorkspace sharedWorkspace];
	NSIndexSet *selectedIndexes = [outlineView selectedRowIndexes];

	for(index = [selectedIndexes firstIndex];
	    index != NSNotFound; index = [selectedIndexes indexGreaterThanIndex:index]) {
		NSURL *url = [[outlineView itemAtRow:index] URL];
		[ws selectFile:[url path] inFileViewerRootedAtPath:[url path]];
	}
}

- (IBAction)setAsRoot:(id)sender {
	NSUInteger index = [[outlineView selectedRowIndexes] firstIndex];

	if(index != NSNotFound) {
		[dataSource changeURL:[[outlineView itemAtRow:index] URL]];
	}
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
	SEL action = [menuItem action];

	if([outlineView numberOfSelectedRows] == 0)
		return NO;

	if(action == @selector(setAsRoot:)) {
		BOOL isDir;
		NSInteger row = [outlineView selectedRow];

		if([outlineView numberOfSelectedRows] > 1)
			return NO;

		// Only let directories be Set as Root
		[[NSFileManager defaultManager] fileExistsAtPath:[[[outlineView itemAtRow:row] URL] path] isDirectory:&isDir];
		return isDir;
	}

	return YES;
}
@end
