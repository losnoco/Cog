//
//  FileTreeOutlineView.m
//  Cog
//
//  Created by Vincent Spader on 6/21/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "FileTreeOutlineView.h"
#import "FileTreeViewController.h"
#import "PlaybackController.h"

@implementation FileTreeOutlineView

- (void)awakeFromNib
{
	[[self menu] setAutoenablesItems:NO];
	[self setDoubleAction:@selector(addToPlaylist:)];
	[self setTarget:[self delegate]];
}

- (void)keyDown:(NSEvent *)e
{
    unsigned int   modifiers = [e modifierFlags] & (NSCommandKeyMask | NSShiftKeyMask | NSControlKeyMask | NSAlternateKeyMask);
    NSString       *characters = [e characters];
	unichar        c;
	
	if ([characters length] == 1) 
	{
		c = [characters characterAtIndex:0];
		
		if (modifiers == 0 && (c == NSEnterCharacter || c == NSCarriageReturnCharacter))
		{
			[[self delegate] addToPlaylist:self];

			return;
		}
		else if (modifiers == 0 && c == ' ')
		{
			[[self delegate] playPauseResume:self];
			return;
		}
	}
	
	[super keyDown:e];

	return;
}

// enables right-click selection for "Show in Finder" contextual menu
-(NSMenu*)menuForEvent:(NSEvent*)event
{
	//Find which row is under the cursor
	[[self window] makeFirstResponder:self];
	BOOL isDir;
	NSPoint   menuPoint = [self convertPoint:[event locationInWindow] fromView:nil];
	NSInteger iRow = [self rowAtPoint:menuPoint];
	NSMenu* contextMenu = [self menu];

	/* Update the file tree selection before showing menu
	 Preserves the selection if the row under the mouse is selected (to allow for
	 multiple items to be selected), otherwise selects the row under the mouse */
	BOOL currentRowIsSelected = [[self selectedRowIndexes] containsIndex:iRow];

	if (iRow == -1)
	{
		[self deselectAll:self];		
	}
	else if (!currentRowIsSelected)
	{
		[self selectRow:iRow byExtendingSelection:NO];
	}

	if ([self numberOfSelectedRows] > 0)
	{
		[[contextMenu itemWithTag:1] setEnabled:YES];	// Add to Playlist
		[[contextMenu itemWithTag:2] setEnabled:YES];	// Set as Playlist
		[[contextMenu itemWithTag:3] setEnabled:YES];	// Show in Finder
		
		// Only let directories be Set as Root
		[[NSFileManager defaultManager] fileExistsAtPath:[[[self itemAtRow:iRow] URL] path] isDirectory:&isDir];
		[[contextMenu itemWithTag:4] setEnabled:(isDir? YES : NO)];
	}
	else
	{
		//No rows are selected, so the menu should be displayed with all items disabled
		int i;
		for (i=0;i<[contextMenu numberOfItems];i++) {
			[[contextMenu itemAtIndex:i] setEnabled:NO];
		}
	}
	 
	return contextMenu;
}

@end
