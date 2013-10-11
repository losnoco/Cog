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

#import "IndexFormatter.h"
#import "SecondsFormatter.h"
#import "BlankZeroFormatter.h"
#import "PlaylistEntry.h"

#import "CogAudio/Status.h"

@implementation PlaylistView

- (void)awakeFromNib
{
	[[self menu] setAutoenablesItems:NO];
	
    // Configure bindings to scale font size and row height
    NSControlSize s = NSSmallControlSize;
	NSFont *f = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:s]];
    // NSFont *bf = [[NSFontManager sharedFontManager] convertFont:f toHaveTrait:NSBoldFontMask];
			 
	for (NSTableColumn *col in [self tableColumns]) {
        [[col dataCell] setControlSize:s];
        [[col dataCell] setFont:f];
	}

	//Set up formatters
	NSFormatter *secondsFormatter = [[SecondsFormatter alloc] init];
	[[[self tableColumnWithIdentifier:@"length"] dataCell] setFormatter:secondsFormatter];
	[secondsFormatter release];
	
	NSFormatter *indexFormatter = [[IndexFormatter alloc] init];
	[[[self tableColumnWithIdentifier:@"index"] dataCell] setFormatter:indexFormatter];
	[indexFormatter release];
	
	NSFormatter *blankZeroFormatter = [[BlankZeroFormatter alloc] init];
	[[[self tableColumnWithIdentifier:@"track"] dataCell] setFormatter:blankZeroFormatter];
	[[[self tableColumnWithIdentifier:@"year"] dataCell] setFormatter:blankZeroFormatter];
	[blankZeroFormatter release];
	//end setting up formatters

	[self setVerticalMotionCanBeginDrag:YES];
	
	//Set up header context menu
	headerContextMenu = [[NSMenu alloc] initWithTitle:@"Playlist Header Context Menu"];
	
	NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"identifier" ascending:YES];
	NSArray *sortDescriptors = [NSArray arrayWithObject:sortDescriptor];
	[sortDescriptor release];
	
	int visibleTableColumns = 0;
	int menuIndex = 0;
	for (NSTableColumn *col in [[self tableColumns] sortedArrayUsingDescriptors: sortDescriptors]) 
	{
		NSString *title;
		if ([[col identifier]  isEqualToString:@"status"])
		{
			title = @"Status";
		}
		else if ([[col identifier] isEqualToString:@"index"])
		{
			title = @"Index";
		}
		else
		{
			title = [[col headerCell] title];
		}
		
		NSMenuItem *contextMenuItem = [headerContextMenu insertItemWithTitle:title action:@selector(toggleColumn:) keyEquivalent:@"" atIndex:menuIndex];
		
		[contextMenuItem setTarget:self];
		[contextMenuItem setRepresentedObject:col];
		[contextMenuItem setState:([col isHidden] ? NSOffState : NSOnState)];

		visibleTableColumns += ![col isHidden];
		menuIndex++;
	}
	
	if (visibleTableColumns == 0) {
		for (NSTableColumn *col in [self tableColumns]) {
			[col setHidden:NO];
		}
	}
	
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
	
	if ([e type] == NSLeftMouseDown && [e clickCount] == 2 && [[self selectedRowIndexes] count] == 1)
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
	NSInteger iRow = [self rowAtPoint:menuPoint];
    NSMenu* tableViewMenu = [self menu];
	
	/* Update the table selection before showing menu
		Preserves the selection if the row under the mouse is selected (to allow for
																		multiple items to be selected), otherwise selects the row under the mouse */
	BOOL currentRowIsSelected = [[self selectedRowIndexes] containsIndex:iRow];
	if (!currentRowIsSelected) {
		if (iRow == -1)
		{
			[self deselectAll:self];
		}
		else
		{
			[self selectRowIndexes:[NSIndexSet indexSetWithIndex:iRow] byExtendingSelection:NO];
		}
	}

	if ([self numberOfSelectedRows] <=0)
	{
		//No rows are selected, so the table should be displayed with all items disabled
		int i;
		for (i=0;i<[tableViewMenu numberOfItems];i++) {
			[[tableViewMenu itemAtIndex:i] setEnabled:NO];
		}
	}
	
	return tableViewMenu;
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
	else
	{
		[super keyDown:e];
	}
}

- (IBAction)scrollToCurrentEntry:(id)sender
{
	[self scrollRowToVisible:[[playlistController currentEntry] index]];
	[self selectRowIndexes:[NSIndexSet indexSetWithIndex:[[playlistController currentEntry] index]] byExtendingSelection:NO];
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
	
	if (action == @selector(scrollToCurrentEntry:) && ([playbackController playbackStatus] == kCogStatusStopped))
		return NO;
	
	return [super validateUserInterfaceItem:anItem];
}

- (NSDragOperation)draggingSourceOperationMaskForLocal:(BOOL)isLocal
{
	if (isLocal)
		return NSDragOperationNone;
	else
		return NSDragOperationCopy;
}


@end
