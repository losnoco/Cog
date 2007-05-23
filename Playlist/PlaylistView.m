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
	[super awakeFromNib];
	
	[[self menu] setAutoenablesItems:NO];
	
	NSControlSize s = NSSmallControlSize;
	NSEnumerator *oe = [[self allTableColumns] objectEnumerator];
	NSFont *f = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:s]];
	NSFont *bf = [NSFont boldSystemFontOfSize:[NSFont systemFontSizeForControlSize:s]];
	
	[self setRowHeight:[bf defaultLineHeightForFont]];

	//Resize the fonts
	id c;
	while (c = [oe nextObject])
	{
		[[c dataCell] setControlSize:s];
		[[c dataCell] setFont:f];
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
	NSEnumerator *e = [[[[self allTableColumns] allObjects] sortedArrayUsingDescriptors: sortDescriptors] objectEnumerator];

	int menuIndex = 0;
	NSTableColumn *col;	
	while (col = [e nextObject]) {
		NSMenuItem *contextMenuItem = [headerContextMenu insertItemWithTitle:[[col headerCell] title] action:@selector(toggleColumn:) keyEquivalent:@"" atIndex:menuIndex];
		
		[contextMenuItem setTarget:self];
		[contextMenuItem setRepresentedObject:col];
		[contextMenuItem setState:([[self visibleTableColumns] containsObject:col] ? NSOnState : NSOffState)];

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

		[self showTableColumn:tc];
	}
	else
	{
		[sender setState:NSOffState];
		[self hideTableColumn:tc];
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
	if ([e type] == NSLeftMouseDown && [e clickCount] == 2 && [self selectedRow] != -1)
	{
		[playbackController play:self];
	}
	else
	{
		[super mouseDown:e];
	}
}

// enables right-click selection for "Show in Finder" contextual menu
-(NSMenu*)menuForEvent:(NSEvent*)event
{
	//Find which row is under the cursor
	[[self window] makeFirstResponder:self];
	NSPoint menuPoint = [self convertPoint:[event locationInWindow] fromView:nil];
	int row = [self rowAtPoint:menuPoint];
	
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
	NSLog(@"Number of selected rows: %i", [self numberOfSelectedRows]);
	if ([self numberOfSelectedRows] <=0)
	{
		//No rows are selected, so the table should be displayed with all items disabled
		NSMenu* tableViewMenu = [[self menu] copy];
		int i;
		for (i=0;i<[tableViewMenu numberOfItems];i++) {
			[[tableViewMenu itemAtIndex:i] setEnabled:NO];
			NSLog(@"Enabled: %@ %i", [[tableViewMenu itemAtIndex:i] title], [[tableViewMenu itemAtIndex:i] isEnabled]);
		}
		NSLog(@"All disabled!");
		return [tableViewMenu autorelease];
	}
	else
	{
		return [self menu];
	}
}


- (void)keyDown:(NSEvent *)e
{
	NSString *s;
	unichar c;
	
	s = [e charactersIgnoringModifiers];
	if ([s length] != 1)
		return;

	c = [s characterAtIndex:0];

	if (c == NSDeleteCharacter || c == NSBackspaceCharacter || c == NSDeleteFunctionKey)
	{
		[playlistController remove:self];
	}
	else if (c == ' ')
	{
		[playbackController playPauseResume:self];
	}
	else if (c == NSEnterCharacter || c == NSCarriageReturnCharacter)
	{
		[playbackController play:self];
	}
	else
	{
		[super keyDown:e];
	}
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

@end
