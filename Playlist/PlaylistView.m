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

@implementation PlaylistView

- (void)awakeFromNib
{
	id c;
	NSControlSize s = NSSmallControlSize;
	NSEnumerator *oe = [[self tableColumns] objectEnumerator];
	NSFont *f = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:s]];
	
	[self setRowHeight:[f defaultLineHeightForFont]];

	//Resize the fonts
	while (c = [oe nextObject])
	{
		[[c dataCell] setControlSize:s];
		[[c dataCell] setFont:f];
	}
	
	NSTableHeaderView *currentTableHeaderView = [self headerView];
	PlaylistHeaderView *customTableHeaderView = [[PlaylistHeaderView alloc] init];
	
	[customTableHeaderView setFrame:[currentTableHeaderView frame]];
	[customTableHeaderView setBounds:[currentTableHeaderView bounds]];
	// has to be disabled for optimal resizing to work properly...
	[self setColumnAutoresizingStyle:NSTableViewNoColumnAutoresizing];
	
	[self setHeaderView:customTableHeaderView];
	
	[self setVerticalMotionCanBeginDrag:YES];
	
	
	//Test for hiding columns
//	[self setupColumns];
}

- (void)setupColumns
{
	NSLog(@"SETTING UP COLUMNS");
	NSEnumerator *columnEnumerator = [[self tableColumns] objectEnumerator];
	NSTableColumn *nextColumn;
	while( nextColumn = [columnEnumerator nextObject] )
	{
		NSString *identifier = [nextColumn identifier];
		if(![self shouldShowColumn:identifier])
			[self removeTableColumn:nextColumn];
	}
}

- (BOOL)shouldShowColumn:(NSString *)identifier
{
	if ([identifier isEqualToString:@"title"] || [identifier isEqualToString:@"artist"] || [identifier isEqualToString:@"length"])
		return YES;
	
	return NO;
}

//FUN HACKS SO COLUMNS DONT DISAPPEAR WHEN THE TABLE IS AUTOSAVED
- (void)removeTableColumn:(NSTableColumn *)aTableColumn
{
    if (aTableColumn)
    {
        if (!_removedColumns)
            _removedColumns = [[NSMutableArray alloc] init];
		
        // Cache the removed table column so we don't have to set it up again.
        [_removedColumns addObject:aTableColumn];
    }
    [super removeTableColumn:aTableColumn];
}

- (NSTableColumn *)tableColumnWithIdentifier:(id)anObject
{
    NSTableColumn *tc = [super tableColumnWithIdentifier:anObject];
	
    if (!tc && _removedColumns)
    {
        NSEnumerator *e = [_removedColumns objectEnumerator];
        NSTableColumn *t = nil;
		
        while (t = [e nextObject])
        {
            // Locate cached version if there is one.
            if ([[t identifier] isEqual:anObject])
            {
                // Remove it from the array and release the array if it isn't needed any more.
                [_removedColumns removeObject:t];
                if ([_removedColumns count] == 0) [_removedColumns release];
                return t;
            }
        }
    }
	
    return tc;
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
	NSLog(@"MOUSE DOWN");
	if ([e type] == NSLeftMouseDown && [e clickCount] == 2)
	{
		[playbackController play:self];
	}
	else
	{
		NSLog(@"Super");
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
	if (!currentRowIsSelected)
		[self selectRow:row byExtendingSelection:NO];

	if ([self numberOfSelectedRows] <=0)
	{
		//No rows are selected, so the table should be displayed with all items disabled
		NSMenu* tableViewMenu = [[self menu] copy];
		int i;
		for (i=0;i<[tableViewMenu numberOfItems];i++)
			[[tableViewMenu itemAtIndex:i] setEnabled:NO];
		return [tableViewMenu autorelease];
	}
	else
		return [self menu];
}


- (void)keyDown:(NSEvent *)e
{
	NSString *s;
	unichar c;
	
	s = [e charactersIgnoringModifiers];
	if ([s length] != 1)
		return;

	c = [s characterAtIndex:0];
	if (c == NSDeleteCharacter)
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
