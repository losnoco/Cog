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
//	[self setColumnAutoresizingStyle:NSTableViewNoColumnAutoresizing];
	
	[self setHeaderView:customTableHeaderView];
	
	[self setVerticalMotionCanBeginDrag:YES];
	
	//Hack for bindings and columns
	_tableColumnsCache = [[NSArray alloc] initWithArray:[self tableColumns] copyItems:NO];
}

- (IBAction)takeBoolForTitle:(id)sender
{
	[self showColumn:sender withIdentifier:@"title"];
}

- (IBAction)takeBoolForArtist:(id)sender
{
	[self showColumn:sender withIdentifier:@"artist"];
}

- (IBAction)takeBoolForAlbum:(id)sender
{
	[self showColumn:sender withIdentifier:@"album"];
}

- (IBAction)takeBoolForLength:(id)sender
{
	[self showColumn:sender withIdentifier:@"length"];
}

- (IBAction)takeBoolForYear:(id)sender
{
	[self showColumn:sender withIdentifier:@"year"];
}

- (IBAction)takeBoolForGenre:(id)sender
{
	[self showColumn:sender withIdentifier:@"genre"];
}

- (IBAction)takeBoolForTrack:(id)sender
{
	[self showColumn:sender withIdentifier:@"track"];
}

- (void)showColumn:(id)sender withIdentifier:(NSString *)identifier
{
	if ([sender state] == NSOffState)
	{
		[sender setState:NSOnState];
		[self showColumnWithIdentifier:identifier];
	}
	else
	{
		[sender setState:NSOffState];
		[self hideColumnWithIdentifier:identifier];
	}
}

- (void)hideColumnWithIdentifier:(NSString *)identifier
{
	NSTableColumn *tc = [super tableColumnWithIdentifier:identifier];
	if (!tc)
		return;
	
	[self removeTableColumn:tc];
}

- (void)showColumnWithIdentifier:(NSString *)identifier
{
	if ([super tableColumnWithIdentifier:identifier])
		return;

	NSEnumerator *e = [_tableColumnsCache objectEnumerator];
	NSTableColumn *t = nil;
	
	while (t = [e nextObject])
	{
		// Locate cached version if there is one.
		if ([[t identifier] isEqualToString:identifier])
			// Remove it from the array and release the array if it isn't needed any more.
			[self addTableColumn:t];
	}	
}

//FUN HACKS SO COLUMNS DONT DISAPPEAR WHEN THE TABLE IS AUTOSAVED
- (NSTableColumn *)tableColumnWithIdentifier:(id)anObject
{
    NSTableColumn *tc = [super tableColumnWithIdentifier:anObject];

    if (!tc)
    {
        NSEnumerator *e = [_tableColumnsCache objectEnumerator];
        NSTableColumn *t = nil;
		
        while (t = [e nextObject])
        {
            // Locate cached version if there is one.
            if ([[t identifier] isEqual:anObject])
                // Remove it from the array and release the array if it isn't needed any more.
                return t;
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
