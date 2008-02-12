//
//  PlaylistView.m
//  Cog
//
//  Created by Vincent Spader on 3/20/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "PlaylistView.h"
#import "PlaybackController.h"
#import "PlaylistController.h"

#import "PlaylistHeaderView.h"

#import "IndexFormatter.h"
#import "SecondsFormatter.h"

@implementation PlaylistView

- (void)awakeFromNib
{
	[[self menu] setAutoenablesItems:NO];
	
	NSControlSize s = NSSmallControlSize;
	NSEnumerator *oe = [[self tableColumns] objectEnumerator];
	NSFont *f = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:s]];
	NSFont *bf = [[NSFontManager sharedFontManager] convertFont:f toHaveTrait:NSBoldFontMask];

	NSLayoutManager *layoutManager = [[NSLayoutManager alloc] init];
	[self setRowHeight:[layoutManager defaultLineHeightForFont:bf]];
	[layoutManager release];

	//Resize the fonts
	id c;
	while (c = [oe nextObject])
	{
		[[c dataCell] setControlSize:s];

		//Using the bold font defined from the default system font with the bold trait added seems to fix problems related to bold display with some fonts.
		[[c dataCell] setFont:bf];
	}

	NSTableHeaderView *currentTableHeaderView = [self headerView];
	PlaylistHeaderView *customTableHeaderView = [[PlaylistHeaderView alloc] init];
	
	[customTableHeaderView setFrame:[currentTableHeaderView frame]];
	[customTableHeaderView setBounds:[currentTableHeaderView bounds]];
//	[self setColumnAutoresizingStyle:NSTableViewNoColumnAutoresizing];
	
	[self setHeaderView:customTableHeaderView];

	//Set up formatters
	NSFormatter *secondsFormatter = [[SecondsFormatter alloc] init];
	[[[self tableColumnWithIdentifier:@"length"] dataCell] setFormatter:secondsFormatter];
	[secondsFormatter release];
	
	NSFormatter *indexFormatter = [[IndexFormatter alloc] init];
	[[[self tableColumnWithIdentifier:@"index"] dataCell] setFormatter:indexFormatter];
	[indexFormatter release];
	//end setting up formatters

	[self setVerticalMotionCanBeginDrag:YES];
	
	//Set up header context menu
	headerContextMenu = [[NSMenu alloc] initWithTitle:@"Playlist Header Context Menu"];
	
	NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"headerCell.title" ascending:YES];
	NSArray *sortDescriptors = [NSArray arrayWithObject:sortDescriptor];
	NSEnumerator *e = [[[self tableColumns] sortedArrayUsingDescriptors: sortDescriptors] objectEnumerator];

	int menuIndex = 0;
	NSTableColumn *col;	
	while (col = [e nextObject]) {
		NSMenuItem *contextMenuItem = [headerContextMenu insertItemWithTitle:[[col headerCell] title] action:@selector(toggleColumn:) keyEquivalent:@"" atIndex:menuIndex];
		
		[contextMenuItem setTarget:self];
		[contextMenuItem setRepresentedObject:col];
		[contextMenuItem setState:([col isHidden] ? NSOffState : NSOnState)];

		menuIndex++;
	}
	[sortDescriptor release];
	
	[[self headerView] setMenu:headerContextMenu];
}


- (IBAction)toggleColumn:(id)sender
{
	id tc = [sender representedObject];
	
	if ([sender state] == NSOffState)
	{
		[sender setState:NSOnState];

		[tc setHidden: NO];
	}
	else
	{
		[sender setState:NSOffState];
		
		[tc setHidden: YES];
	}
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (BOOL)resignFirstResponder
{
	return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)mouseDownEvent
{
	return NO;
}

- (void)mouseDown:(NSEvent *)e
{
	[super mouseDown:e];
	
	if ([e type] == NSLeftMouseDown && [e clickCount] == 2 && [self selectedRow] != -1)
	{
		[playbackController play:self];
	}
}

// enables right-click selection for "Show in Finder" contextual menu
-(NSMenu*)menuForEvent:(NSEvent*)event
{
	//Find which row is under the cursor
	[[self window] makeFirstResponder:self];
	NSPoint   menuPoint = [self convertPoint:[event locationInWindow] fromView:nil];
	int       row = [self rowAtPoint:menuPoint];
	
	/* Update the table selection before showing menu
		Preserves the selection if the row under the mouse is selected (to allow for
																		multiple items to be selected), otherwise selects the row under the mouse */
	BOOL currentRowIsSelected = [[self selectedRowIndexes] containsIndex:row];
	if (!currentRowIsSelected) {
		if (row == -1)
		{
			[self deselectAll:self];
		}
		else
		{
			[self selectRow:row byExtendingSelection:NO];
		}
	}

	if ([self numberOfSelectedRows] <=0)
	{
		//No rows are selected, so the table should be displayed with all items disabled
		NSMenu* tableViewMenu = [[self menu] copy];
		int i;
		for (i=0;i<[tableViewMenu numberOfItems];i++) {
			[[tableViewMenu itemAtIndex:i] setEnabled:NO];
		}

		return [tableViewMenu autorelease];
	}
	else
	{
		return [self menu];
	}
}


- (void)keyDown:(NSEvent *)e
{
    unsigned int   modifiers = [e modifierFlags] & (NSCommandKeyMask | NSShiftKeyMask | NSControlKeyMask | NSAlternateKeyMask);
    NSString       *characters = [e characters];
	unichar        c;

	if ([characters length] != 1) 
	{
		[super keyDown:e];
	
		return;
	}
	
	c = [characters characterAtIndex:0];
	if (modifiers == 0 && (c == NSDeleteCharacter || c == NSBackspaceCharacter || c == NSDeleteFunctionKey))
	{
		[playlistController remove:self];
	}
	else if (modifiers == 0 && c == ' ')
	{
		[playbackController playPauseResume:self];
	}
	else if (modifiers == 0 && (c == NSEnterCharacter || c == NSCarriageReturnCharacter))
	{
		[playbackController play:self];
	}
	// Escape
	else if (modifiers == 0 && c == 0x1b) 
	{ 
		[playlistController clearFilterPredicate:self];
	}
	// shift+command+p - fade to pause
	else if (modifiers == (NSCommandKeyMask | NSShiftKeyMask) && c == 0x70)
	{
		[playbackController fadeOut:self withTime:0.3];
	}
	else
	{
		[super keyDown:e];
	}
}

- (IBAction)scrollToCurrentEntry:(id)sender
{
	[self scrollRowToVisible:[(NSNumber *)[[playlistController currentEntry] index] intValue]];
}

- (IBAction)sortByPath:(id)sender
{
	[self setSortDescriptors:nil];
	[playlistController sortByPath];
}

- (IBAction)shufflePlaylist:(id)sender
{
	[self setSortDescriptors:nil];
	[playlistController randomizeList];
}

- (IBAction)undo:(id)sender
{
	[[playlistController undoManager] undo];
}

- (IBAction)redo:(id)sender
{
	[[playlistController undoManager] redo];
}


-(BOOL)validateUserInterfaceItem:(id <NSValidatedUserInterfaceItem>)anItem
{
	SEL action = [anItem action];

	if (action == @selector(undo:))
	{
		if ([[playlistController undoManager] canUndo]) 
			return YES;
		else
			return NO;
	}
	if (action == @selector(redo:))
	{
		if ([[playlistController undoManager] canRedo]) 
			return YES;
		else
			return NO;
	}

	return [super validateUserInterfaceItem:anItem];
}

@end
