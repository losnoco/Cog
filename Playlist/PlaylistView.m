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

#import "Logging.h"

@implementation PlaylistView

- (void)awakeFromNib
{
    [[self menu] setAutoenablesItems:NO];
    
    // Configure bindings to scale font size and row height
    NSControlSize s = NSControlSizeSmall;
    NSFont *f = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:s]];
    // NSFont *bf = [[NSFontManager sharedFontManager] convertFont:f toHaveTrait:NSBoldFontMask];
    
    for (NSTableColumn *col in [self tableColumns]) {
        [[col dataCell] setControlSize:s];
        [[col dataCell] setFont:f];
    }
    
    //Set up formatters
    NSFormatter *secondsFormatter = [[SecondsFormatter alloc] init];
    [[[self tableColumnWithIdentifier:@"length"] dataCell] setFormatter:secondsFormatter];
    
    NSFormatter *indexFormatter = [[IndexFormatter alloc] init];
    [[[self tableColumnWithIdentifier:@"index"] dataCell] setFormatter:indexFormatter];
    
    NSFormatter *blankZeroFormatter = [[BlankZeroFormatter alloc] init];
    [[[self tableColumnWithIdentifier:@"track"] dataCell] setFormatter:blankZeroFormatter];
    [[[self tableColumnWithIdentifier:@"year"] dataCell] setFormatter:blankZeroFormatter];
    //end setting up formatters
    
    [self setVerticalMotionCanBeginDrag:YES];
    
    //Set up header context menu
    headerContextMenu = [[NSMenu alloc] initWithTitle:@"Playlist Header Context Menu"];
    
    NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"identifier" ascending:YES];
    NSArray *sortDescriptors = [NSArray arrayWithObject:sortDescriptor];
    
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
    
    if ([e type] == NSEventTypeLeftMouseDown && [e clickCount] == 2 && [[self selectedRowIndexes] count] == 1)
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
    unsigned int   modifiers = [e modifierFlags] & (NSEventModifierFlagCommand | NSEventModifierFlagShift | NSEventModifierFlagControl | NSEventModifierFlagOption);
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
    else if (modifiers == 0 && c == NSLeftArrowFunctionKey)
    {
        [playbackController eventSeekBackward:self];
    }
    else if (modifiers == 0 && c == NSRightArrowFunctionKey)
    {
        [playbackController eventSeekForward:self];
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

- (IBAction)copy:(id)sender
{
    NSPasteboard *pboard = [NSPasteboard generalPasteboard];
    
    [pboard clearContents];
    
    NSMutableArray *selectedURLs = [[NSMutableArray alloc] init];
    
    for (PlaylistEntry *pe in [[playlistController content] objectsAtIndexes:[playlistController selectionIndexes]])
    {
        [selectedURLs addObject:[pe URL]];
    }
    
    [pboard setData:[NSArchiver archivedDataWithRootObject:selectedURLs] forType:CogUrlsPboardType];
    
    NSMutableDictionary * tracks = [[NSMutableDictionary alloc] init];
    
    unsigned long i = 0;
    for (NSURL *url in selectedURLs)
    {
        NSMutableDictionary * track = [NSMutableDictionary dictionaryWithObjectsAndKeys:[url absoluteString], @"Location", nil];
        [tracks setObject:track forKey:[NSString stringWithFormat:@"%lu", i]];
        ++i;
    }
    
    NSMutableDictionary * itunesPlist = [NSMutableDictionary dictionaryWithObjectsAndKeys:tracks, @"Tracks", nil];
    
    [pboard setPropertyList:itunesPlist forType:iTunesDropType];
    
    NSMutableArray *filePaths = [[NSMutableArray alloc] init];
    
    for (NSURL *url in selectedURLs)
    {
        if ([url isFileURL])
            [filePaths addObject:[url path]];
    }
    
    if ([filePaths count])
        [pboard setPropertyList:filePaths forType:NSFilenamesPboardType];
}

- (IBAction)cut:(id)sender
{
    [self copy:sender];
    
    [playlistController removeObjectsAtArrangedObjectIndexes:[playlistController selectionIndexes]];
    
    if ([playlistController shuffle] != ShuffleOff)
        [playlistController resetShuffleList];
}

- (IBAction)paste:(id)sender
{
    // Determine the type of object that was dropped
    NSArray *supportedTypes = [NSArray arrayWithObjects:CogUrlsPboardType, NSFilenamesPboardType, iTunesDropType, nil];
    NSPasteboard *pboard = [NSPasteboard generalPasteboard];
    NSString *bestType = [pboard availableTypeFromArray:supportedTypes];
    
    NSMutableArray *acceptedURLs = [[NSMutableArray alloc] init];
    
    // Get files from an file drawer drop
    if ([bestType isEqualToString:CogUrlsPboardType]) {
        NSArray *urls = [NSUnarchiver unarchiveObjectWithData:[pboard dataForType:CogUrlsPboardType]];
        DLog(@"URLS: %@", urls);
        //[playlistLoader insertURLs: urls atIndex:row sort:YES];
        [acceptedURLs addObjectsFromArray:urls];
    }
    
    // Get files from a normal file drop (such as from Finder)
    if ([bestType isEqualToString:NSFilenamesPboardType]) {
        NSMutableArray *urls = [[NSMutableArray alloc] init];
        
        for (NSString *file in [pboard propertyListForType:NSFilenamesPboardType])
        {
            [urls addObject:[NSURL fileURLWithPath:file]];
        }
        
        //[playlistLoader insertURLs:urls atIndex:row sort:YES];
        [acceptedURLs addObjectsFromArray:urls];
    }
    
    // Get files from an iTunes drop
    if ([bestType isEqualToString:iTunesDropType]) {
        NSDictionary *iTunesDict = [pboard propertyListForType:iTunesDropType];
        NSDictionary *tracks = [iTunesDict valueForKey:@"Tracks"];
        
        // Convert the iTunes URLs to URLs....MWAHAHAH!
        NSMutableArray *urls = [[NSMutableArray alloc] init];
        
        for (NSDictionary *trackInfo in [tracks allValues]) {
            [urls addObject:[NSURL URLWithString:[trackInfo valueForKey:@"Location"]]];
        }
        
        //[playlistLoader insertURLs:urls atIndex:row sort:YES];
        [acceptedURLs addObjectsFromArray:urls];
    }
    
    if ([acceptedURLs count])
    {
        NSUInteger row = [[playlistController content] count];
        
        [playlistController willInsertURLs:acceptedURLs origin:URLOriginInternal];
        
        NSArray* entries = [playlistLoader insertURLs:acceptedURLs atIndex:(int)row sort:NO];
        [playlistLoader didInsertURLs:entries origin:URLOriginInternal];
        
        if ([playlistController shuffle] != ShuffleOff)
            [playlistController resetShuffleList];
    }
}

- (IBAction)delete:(id)sender
{
    [playlistController removeObjectsAtArrangedObjectIndexes:[playlistController selectionIndexes]];
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
    if (action == @selector(cut:) || action == @selector(copy:) || action == @selector(delete:))
    {
        if ([[playlistController selectionIndexes] count] == 0)
            return NO;
        else
            return YES;
    }
    if (action == @selector(paste:))
    {
        NSPasteboard *pboard = [NSPasteboard generalPasteboard];
        
        NSArray *supportedTypes = [NSArray arrayWithObjects:CogUrlsPboardType, NSFilenamesPboardType, iTunesDropType, nil];
        
        NSString *bestType = [pboard availableTypeFromArray:supportedTypes];
        
        if (bestType != nil)
            return YES;
        else
            return NO;
    }
    
    if (action == @selector(scrollToCurrentEntry:) && (([playbackController playbackStatus] == kCogStatusStopped) || ([playbackController playbackStatus] == kCogStatusStopping)))
        return NO;
    
    return [super validateUserInterfaceItem:anItem];
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
