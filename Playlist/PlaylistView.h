//
//  PlaylistView.h
//  Cog
//
//  Created by Vincent Spader on 3/20/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "PlaybackController.h"
#import "PlaylistController.h"
#import "PlaylistLoader.h"

@interface PlaylistView : NSTableView {
	IBOutlet PlaybackController *playbackController;
	IBOutlet PlaylistController *playlistController;
	IBOutlet PlaylistLoader *playlistLoader;

	NSMenu *headerContextMenu;

	NSTimer *syncTimer;

	// What the last copy put on the pasteboard, to paste as copies while
	// the pasteboard still holds it, without loading the files again.
	NSArray<PlaylistEntry *> *copiedEntries;
	NSInteger copiedChangeCount;

	// While the layout is simple: the columns it put away, in their order,
	// and what it changed of the table to show one full-width column.
	NSArray<NSTableColumn *> *detailColumns;
	NSTableHeaderView *detailHeaderView;
	NSTableViewColumnAutoresizingStyle detailAutoresizingStyle;
	// The row height the font size setting gives the columns.
	CGFloat detailRowHeight;
}

/// One full-width column of two-line rows, as the iOS app has it, instead
/// of the columns; they come back as they were.
@property(nonatomic) BOOL simpleLayout;

- (IBAction)toggleColumn:(NSMenuItem *)sender;

- (IBAction)scrollToCurrentEntry:(id)sender;

- (IBAction)refreshTrack:(id)sender;

- (IBAction)saveSelectionAsPlaylist:(id)sender;

@end
