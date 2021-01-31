//
//  FileTreeOutlineView.m
//  Cog
//
//  Created by Vincent Spader on 6/21/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "FileTreeController.h"
#import "FileTreeOutlineView.h"
#import "FileTreeViewController.h"
#import "PlaybackController.h"

@implementation FileTreeOutlineView

- (void)awakeFromNib
{
    [self setDoubleAction:@selector(addToPlaylistExternal:)];
    [self setTarget:[self delegate]];
}

- (void)keyDown:(NSEvent *)e
{
    unsigned int   modifiers = [e modifierFlags] & (NSEventModifierFlagCommand | NSEventModifierFlagShift | NSEventModifierFlagControl | NSEventModifierFlagOption);
    NSString       *characters = [e characters];
    unichar        c;
    
    if ([characters length] == 1)
    {
        c = [characters characterAtIndex:0];
        
        if (modifiers == 0 && (c == NSEnterCharacter || c == NSCarriageReturnCharacter))
        {
            [(FileTreeController *)[self delegate] addToPlaylistExternal:self];
            
            return;
        }
        else if (modifiers == 0 && c == ' ')
        {
            [(FileTreeController *)[self delegate] playPauseResume:self];
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
        [self selectRowIndexes:[NSIndexSet indexSetWithIndex:iRow] byExtendingSelection:NO];
    }
    
    return contextMenu;
}

@end
