//
//  PlaylistView.m
//  Cog
//
//  Created by Vincent Spader on 3/20/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "PlaylistView.h"

#import <Carbon/Carbon.h>

#import "PlaylistEntry.h"

#import "CogAudio/Status.h"

#import "Logging.h"

@implementation PlaylistView

- (void)awakeFromNib {
	[[self menu] setAutoenablesItems:NO];

	// Configure bindings to scale font size and row height
	NSControlSize s = NSControlSizeSmall;
	NSFont *f = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:s]];
	// NSFont *bf = [[NSFontManager sharedFontManager] convertFont:f toHaveTrait:NSBoldFontMask];

	NSArray<NSTableColumn *> *columns = [[NSSet setWithArray:[self tableColumns]] allObjects];

	if([columns count] < [[self tableColumns] count]) {
		// borkage in saved state
		NSArray<NSTableColumn *> *borkCols = [[self tableColumns] copy];
		for(NSTableColumn *col in borkCols) {
			[self removeTableColumn:col];
		}
		for(NSTableColumn *col in columns) {
			[self addTableColumn:col];
		}
	}

	for(NSTableColumn *col in columns) {
		[[col dataCell] setControlSize:s];
		[[col dataCell] setFont:f];
	}

	[self setVerticalMotionCanBeginDrag:YES];

	// Set up header context menu
	headerContextMenu = [[NSMenu alloc] initWithTitle:NSLocalizedString(@"PlaylistHeaderContextMenuTitle", @"")];

	NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"identifier"
	                                                               ascending:YES];
	NSArray *sortDescriptors = @[sortDescriptor];

	int visibleTableColumns = 0;
	int menuIndex = 0;
	for(NSTableColumn *col in [columns sortedArrayUsingDescriptors:sortDescriptors]) {
		NSString *title;
		if([[col identifier] isEqualToString:@"status"]) {
			title = NSLocalizedString(@"PlaylistStatusColumn", @"");
		} else if([[col identifier] isEqualToString:@"index"]) {
			title = NSLocalizedString(@"PlaylistIndexColumn", @"");
		} else {
			title = [[col headerCell] title];
		}

		NSMenuItem *contextMenuItem =
		[headerContextMenu insertItemWithTitle:title
		                                action:@selector(toggleColumn:)
		                         keyEquivalent:@""
		                               atIndex:menuIndex];

		[contextMenuItem setTarget:self];
		[contextMenuItem setRepresentedObject:col];
		[contextMenuItem setState:([col isHidden] ? NSControlStateValueOff : NSControlStateValueOn)];

		visibleTableColumns += ![col isHidden];
		menuIndex++;
	}

	if(visibleTableColumns == 0) {
		for(NSTableColumn *col in columns) {
			[col setHidden:NO];
		}
	}

	[[self headerView] setMenu:headerContextMenu];
}

- (IBAction)toggleColumn:(id)sender {
	id tc = [sender representedObject];

	if([sender state] == NSControlStateValueOff) {
		[sender setState:NSControlStateValueOn];

		[tc setHidden:NO];
	} else {
		[sender setState:NSControlStateValueOff];

		[tc setHidden:YES];
	}
}

- (BOOL)acceptsFirstResponder {
	return YES;
}

- (BOOL)resignFirstResponder {
	return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)mouseDownEvent {
	return YES;
}

- (void)mouseDown:(NSEvent *)e {
	[super mouseDown:e];

	if([e type] == NSEventTypeLeftMouseDown) {
		if([e clickCount] == 1) {
			NSPoint globalLocation = [e locationInWindow];
			NSPoint localLocation = [self convertPoint:globalLocation fromView:nil];
			NSInteger clickedRow = [self rowAtPoint:localLocation];
			NSInteger clickedColumn = [self columnAtPoint:localLocation];
			
			if(clickedRow != -1 && clickedColumn != -1) {
				NSView *cellView = [self viewAtColumn:clickedColumn row:clickedRow makeIfNecessary:YES];
				NSPoint cellPoint = [cellView convertPoint:localLocation fromView:self];
			
				[playlistController tableView:self didClickRow:clickedRow column:clickedColumn atPoint:cellPoint];
			}
		} else if([e clickCount] == 2 && [[self selectedRowIndexes] count] == 1) {
			[playbackController play:self];
		}
	}
}

// enables right-click selection for "Show in Finder" contextual menu
- (NSMenu *)menuForEvent:(NSEvent *)event {
	// Find which row is under the cursor
	[[self window] makeFirstResponder:self];
	NSPoint menuPoint = [self convertPoint:[event locationInWindow] fromView:nil];
	NSInteger iRow = [self rowAtPoint:menuPoint];
	NSMenu *tableViewMenu = [self menu];

	/* Update the table selection before showing menu
	 Preserves the selection if the row under the mouse is selected (to allow for
	 multiple items to be selected), otherwise selects the row under the mouse */
	BOOL currentRowIsSelected = [[self selectedRowIndexes] containsIndex:(NSUInteger)iRow];
	if(!currentRowIsSelected) {
		if(iRow == -1) {
			[self deselectAll:self];
		} else {
			[self selectRowIndexes:[NSIndexSet indexSetWithIndex:(NSUInteger)iRow] byExtendingSelection:NO];
		}
	}

	if([self numberOfSelectedRows] <= 0) {
		// No rows are selected, so the table should be displayed with all items disabled
		int i;
		for(i = 0; i < [tableViewMenu numberOfItems]; i++) {
			[[tableViewMenu itemAtIndex:i] setEnabled:NO];
		}
	}

	return tableViewMenu;
}

- (void)keyDown:(NSEvent *)event {
	BOOL modifiersUsed = ([event modifierFlags] & (NSEventModifierFlagShift |
	                                               NSEventModifierFlagControl |
	                                               NSEventModifierFlagOption |
	                                               NSEventModifierFlagCommand)) ?
	                     YES :
                         NO;
	if(modifiersUsed) {
		[super keyDown:event];
		return;
	}

	switch([event keyCode]) {
		case kVK_Space:
			[playbackController playPauseResume:self];
			break;
		case kVK_Escape:
			[playlistController clearFilterPredicate:self];
			break;

		case kVK_Delete:
		case kVK_ForwardDelete:
			[playlistController remove:self];
			break;

		case kVK_LeftArrow:
			[playbackController eventSeekBackward:self];
			break;

		case kVK_RightArrow:
			[playbackController eventSeekForward:self];
			break;

		case kVK_Return:
			[playbackController play:self];
			break;

		default:
			[super keyDown:event];
			break;
	}
}

- (IBAction)scrollToCurrentEntry:(id)sender {
	[self scrollRowToVisible:[[playlistController currentEntry] index]];
	[self selectRowIndexes:[NSIndexSet indexSetWithIndex:(NSUInteger)[[playlistController currentEntry] index]]
	  byExtendingSelection:NO];
}

- (IBAction)undo:(id)sender {
	[[playlistController undoManager] undo];
}

- (IBAction)redo:(id)sender {
	[[playlistController undoManager] redo];
}

- (IBAction)copy:(id)sender {
	NSPasteboard *pboard = [NSPasteboard generalPasteboard];
	[pboard clearContents];

	NSArray *entries =
	[[playlistController content] objectsAtIndexes:[playlistController selectionIndexes]];
	NSUInteger capacity = [entries count];
	NSMutableArray *selectedURLs = [NSMutableArray arrayWithCapacity:capacity];

	NSMutableArray *fileSpams = [NSMutableArray array];

	for(PlaylistEntry *pe in entries) {
		[selectedURLs addObject:pe.url];
		[fileSpams addObject:pe.indexedSpam];
	}

	[pboard writeObjects:@[[fileSpams componentsJoinedByString:@"\n"]]];

	NSError *error;
	NSData *data = [NSKeyedArchiver archivedDataWithRootObject:selectedURLs
	                                     requiringSecureCoding:YES
	                                                     error:&error];

	if(!data) {
		DLog(@"Error: %@", error);
	}
	[pboard setData:data forType:CogUrlsPboardType];

	NSMutableDictionary *tracks = [NSMutableDictionary dictionaryWithCapacity:capacity];

	unsigned long i = 0;
	for(NSURL *url in selectedURLs) {
		tracks[[NSString stringWithFormat:@"%lu", i++]] = @{ @"Location": [url absoluteString] };
	}

	NSDictionary *itunesPlist = @{ @"Tracks": tracks };

	[pboard setPropertyList:itunesPlist forType:iTunesDropType];

	NSMutableArray *filePaths = [NSMutableArray array];

	for(NSURL *url in selectedURLs) {
		if([url isFileURL]) {
			[filePaths addObject:url];
		}
	}

	if([filePaths count]) {
		[pboard writeObjects:filePaths];
	}
}

- (IBAction)cut:(id)sender {
	[self copy:sender];

	[playlistController removeObjectsAtArrangedObjectIndexes:[playlistController selectionIndexes]];

	if([playlistController shuffle] != ShuffleOff) [playlistController resetShuffleList];
}

- (IBAction)paste:(id)sender {
	// Determine the type of object that was dropped
	NSArray *supportedTypes = @[CogUrlsPboardType, NSPasteboardTypeFileURL, iTunesDropType];
	NSPasteboard *pboard = [NSPasteboard generalPasteboard];
	NSPasteboardType bestType = [pboard availableTypeFromArray:supportedTypes];
#ifdef _DEBUG
	DLog(@"All types:");
	for(NSPasteboardType type in [pboard types]) {
		DLog(@"    Type: %@", type);
	}
	DLog(@"Supported types:");
	for(NSPasteboardType type in supportedTypes) {
		DLog(@"    Type: %@", type);
	}
	DLog(@"Best type: %@", bestType);
#endif

	NSMutableArray *acceptedURLs = [NSMutableArray array];

	// Get files from an file drawer drop
	if([bestType isEqualToString:CogUrlsPboardType]) {
		NSError *error;
		NSData *data = [pboard dataForType:CogUrlsPboardType];
		NSArray *urls;
		if(@available(macOS 11.0, *)) {
			urls = [NSKeyedUnarchiver unarchivedArrayOfObjectsOfClass:[NSURL class]
			                                                 fromData:data
			                                                    error:&error];
		} else {
			NSSet *allowed = [NSSet setWithArray:@[[NSArray class], [NSURL class]]];
			urls = [NSKeyedUnarchiver unarchivedObjectOfClasses:allowed
			                                           fromData:data
			                                              error:&error];
		}
		if(!urls) {
			DLog(@"%@", error);
		} else {
			DLog(@"URLS: %@", urls);
		}
		//[playlistLoader insertURLs: urls atIndex:row sort:YES];
		[acceptedURLs addObjectsFromArray:urls];
	}

	// Get files from a normal file drop (such as from Finder)
	if([bestType isEqualToString:NSPasteboardTypeFileURL]) {
		NSMutableArray *urls = [[NSMutableArray alloc] init];

		for(NSString *file in [pboard propertyListForType:NSPasteboardTypeFileURL]) {
			[urls addObject:[NSURL fileURLWithPath:file]];
		}

		//[playlistLoader insertURLs:urls atIndex:row sort:YES];
		[acceptedURLs addObjectsFromArray:urls];
	}

	// Get files from an iTunes drop
	if([bestType isEqualToString:iTunesDropType]) {
		NSDictionary *iTunesDict = [pboard propertyListForType:iTunesDropType];
		NSDictionary *tracks = [iTunesDict valueForKey:@"Tracks"];

		// Convert the iTunes URLs to URLs....MWAHAHAH!
		NSMutableArray *urls = [[NSMutableArray alloc] init];

		for(NSDictionary *trackInfo in [tracks allValues]) {
			[urls addObject:[NSURL URLWithString:[trackInfo valueForKey:@"Location"]]];
		}

		//[playlistLoader insertURLs:urls atIndex:row sort:YES];
		[acceptedURLs addObjectsFromArray:urls];
	}

	if([acceptedURLs count]) {
		NSUInteger row = [[playlistController content] count];

		[playlistController willInsertURLs:acceptedURLs origin:URLOriginInternal];

		NSArray *entries = [playlistLoader insertURLs:acceptedURLs atIndex:(int)row sort:NO];
		[playlistLoader didInsertURLs:entries origin:URLOriginInternal];

		if([playlistController shuffle] != ShuffleOff) [playlistController resetShuffleList];
	}
}

- (IBAction)delete:(id)sender {
	[playlistController removeObjectsAtArrangedObjectIndexes:[playlistController selectionIndexes]];
}

- (BOOL)validateUserInterfaceItem:(id<NSValidatedUserInterfaceItem>)anItem {
	SEL action = [anItem action];

	if(action == @selector(undo:)) {
		return [[playlistController undoManager] canUndo];
	}
	if(action == @selector(redo:)) {
		return [[playlistController undoManager] canRedo];
	}
	if(action == @selector(cut:) || action == @selector(copy:) || action == @selector(delete:)) {
		return [[playlistController selectionIndexes] count] != 0;
	}
	if(action == @selector(paste:)) {
		NSPasteboard *pboard = [NSPasteboard generalPasteboard];

		NSArray *supportedTypes = @[CogUrlsPboardType, NSPasteboardTypeFileURL, iTunesDropType];

		NSString *bestType = [pboard availableTypeFromArray:supportedTypes];

		return bestType != nil;
	}

	if(action == @selector(scrollToCurrentEntry:) &&
	   (([playbackController playbackStatus] == CogStatusStopped) ||
	    ([playbackController playbackStatus] == CogStatusStopping)))
		return NO;

	return [super validateUserInterfaceItem:anItem];
}

- (IBAction)refreshCurrentTrack:(id)sender {
	[self refreshTrack:[playlistController currentEntry]];
}

- (IBAction)refreshTrack:(id)sender {
	PlaylistEntry *pe = (PlaylistEntry *)sender;
	if(pe && !pe.deLeted) {
		unsigned long columns = [[self tableColumns] count];
		[self reloadDataForRowIndexes:[NSIndexSet indexSetWithIndex:pe.index] columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, columns)]];
	}
}

#if 0
- (NSDragOperation)draggingSourceOperationMaskForLocal:(BOOL)isLocal
{
    if (isLocal)
        return NSDragOperationNone;
    else
        return NSDragOperationCopy;
}
#endif

@end
