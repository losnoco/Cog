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
#import "PlaylistEntry.h"

@implementation PlaylistView

- (void)awakeFromNib
{
	[[self menu] setAutoenablesItems:NO];
	
    NSControlSize s = NSSmallControlSize;
	NSFont *f = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:s]];
    // NSFont *bf = [[NSFontManager sharedFontManager] convertFont:f toHaveTrait:NSBoldFontMask];
	
	for(NSTableColumn *col in [self tableColumns])
	{
        [[col dataCell] setControlSize:s];
        [[col dataCell] setFont:f];
        [col            bind:@"fontSize" 
                    toObject:[NSUserDefaultsController sharedUserDefaultsController] 
                 withKeyPath:@"values.fontSize" 
                     options:nil];
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
	[sortDescriptor release];
	
	int visibleTableColumns = 0;
	int menuIndex = 0;
	for (NSTableColumn *col in [[self tableColumns] sortedArrayUsingDescriptors: sortDescriptors]) {
		NSMenuItem *contextMenuItem = [headerContextMenu insertItemWithTitle:[[col headerCell] title] action:@selector(toggleColumn:) keyEquivalent:@"" atIndex:menuIndex];
		
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
    NSMenu* tableViewMenu = [[self menu] copy];
	
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
			[self selectRow:iRow byExtendingSelection:NO];
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
	else
	{
		// Add Spotlight search items
        PlaylistEntry *song = [[playlistController arrangedObjects]objectAtIndex:iRow];
        NSString *artist = [song artist];
        NSString *album = [song album];
        unsigned addedItems = 0; // Count the number of added items, used for separator
        
        if(album)
        {
            NSMenuItem *albumMenuItem = [NSMenuItem alloc];
            NSString *title = [NSString 
                stringWithFormat:@"Search for Songs from %@...", album];
            [albumMenuItem initWithTitle:title
                                  action:@selector(searchByAlbum:)
                           keyEquivalent:@""];
            albumMenuItem.target = playlistController;
            [tableViewMenu insertItem:albumMenuItem atIndex:0];
            [albumMenuItem release];
            addedItems++;
        }
        if(artist)
        {
            NSMenuItem *artistMenuItem = [NSMenuItem alloc];
            NSString *title = [NSString
                stringWithFormat:@"Search for Songs by %@...", artist];
            [artistMenuItem initWithTitle:title
                                   action:@selector(searchByArtist:)
                            keyEquivalent:@""];
            artistMenuItem.target = playlistController;
            [tableViewMenu insertItem:artistMenuItem atIndex:0];
            [artistMenuItem release];
            addedItems++;
        }
        if(addedItems)
        {
            // add a separator in the right place
            [tableViewMenu insertItem:[NSMenuItem separatorItem] atIndex:addedItems];
        }
	}
	
	return [tableViewMenu autorelease];
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
		[playbackController fade:self withTime:0.1];
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
