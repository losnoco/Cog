//
//  KFTypeSelectTableView.m
//  KFTypeSelectTableView v1.0.4
//
//  ------------------------------------------------------------------------
//  Copyright (c) 2005, Ken Ferry All rights reserved.
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are
//  met:
//
//  (1) Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//  (2) Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//      
//  (3) Neither Ken Ferry's name nor the names of other contributors
//      may be used to endorse or promote products derived from this software
//      without specific prior written permission.
//
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
//  IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
//  TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
//  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
//  OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//  ------------------------------------------------------------------------


#import <KFTypeSelectTableView.h>
#import <CoreServices/CoreServices.h>
#include <mach/mach.h>
#include <mach/mach_time.h>

static uint64_t SecondsToMachAbsolute(double seconds);

NSString *KFTypeSelectTableViewPatternDidChangeNotification = @"KFTypeSelectTableViewPatternDidChange";

/* NOTE - because of behavior detailed at cocoadev.com/index.pl?PosingWithCategoriesSuperGotcha, 
 * it's important that private methods be _implemented_ in the main implementation of the class,
 * not in a category.  It's okay for the declarations to be in a category, as below.
 * (Summary of link:  messages to super act like messages to self in categories on a posing class
 * in system 10.3)
 */
@interface KFTypeSelectTableView (Private)

// responding to events
static BOOL KFKeyEventIsBeginFindEvent(NSEvent *keyEvent);
static BOOL KFKeyEventIsExtendFindEvent(NSEvent *keyEvent);
static BOOL KFKeyEventIsFindNextEvent(NSEvent *keyEvent);
static BOOL KFKeyEventIsFindPreviousEvent(NSEvent *keyEvent);
static BOOL KFKeyEventIsDeleteEvent(NSEvent *keyEvent);
static BOOL KFKeyEventIsCancelEvent(NSEvent *keyEvent);

// finding strings
- (void)kfFindPattern:(NSString *)pattern 
           initialRow:(int)initialRow 
          topToBottom:(BOOL)topToBottom
       allowExtension:(BOOL)allowPatternExtension;
- (BOOL)kfWorkUnitGetMatch:(NSString **)match
                     range:(NSRange *)matchRange
           lastSearchedRow:(int *)lastSearchedRow
                forPattern:(NSString *)pattern
              matchOptions:(unsigned)patternMatchOptions
                initialRow:(int)initialRow
               boundaryRow:(int)boundaryRow
              rowIncrement:(int)rowIncrement
             searchColumns:(NSArray *)searchColumns
                   timeout:(uint64_t)timeout;
- (BOOL)kfShouldAcceptMatch:(NSString *)match 
                      range:(NSRange)matchedRange 
                      inRow:(int)row;
- (BOOL)kfCanPerformTypeSelect;
- (BOOL)kfSelectionShouldChange;
- (BOOL)kfCanGetTableData;
- (NSString *)kfStringValueForTableColumn:(NSTableColumn *)column row:(int)row;
- (NSArray *)kfSearchColumns;
- (BOOL)kfSearchTopToBottom;
- (int)kfInitialRowForNewSearch;

// taking action
- (void)kfPatternDidChange:(id)sender;
- (void)kfDidFindMatch:(NSString *)match 
                 range:(NSRange)matchedRange 
                 inRow:(int)row;
- (void)kfDidFailToFindMatchSearchingToRow:(int)row;
- (void)kfResetSearch;
- (void)kfConfigureDelegateIfNeeded;

// utility
- (BOOL)kfRowIsVisible:(int)row;
- (void)kfScrollRectToCenter:(NSRect)aRect vertical:(BOOL)scrollVertical horizontal:(BOOL)scrollHorizontal;

// simulated ivars infrastructure
- (NSMutableDictionary *)kfSimulatedIvars;
- (id)kfIdentifier;
- (void)kfSetUpSimulatedIvars;
- (void)kfTearDownSimulatedIvars;

// accessors
- (int)kfSavedRowForExtensionSearch;
- (void)setKfSavedRowForExtensionSearch:(int)row;
- (NSString *)kfLastSuccessfullyMatchedPattern;
- (void)setKfLastSuccessfullyMatchedPattern:(NSString *)string;
- (BOOL)kfCanExtendFind;
- (void)setKfCanExtendFind:(BOOL)flag;
- (id)kfLastConfiguredDelegate;
- (void)setKfLastConfiguredDelegate:(id)anObject;
- (NSInvocation *)kfTimeoutInvocation;
- (void)setKfTimeoutInvocation:(NSInvocation *)anInvocation;
- (void)setPattern:(NSString *)pattern;
@end


@implementation KFTypeSelectTableView 

#pragma mark -
#pragma mark SETUP/TEARDOWN
#pragma mark -


// Note: don't use init.  Won't receive it for preexisting objects when posing.

- (void)dealloc
{
    NSInvocation *timeoutInvocation = [self kfTimeoutInvocation];
    [[timeoutInvocation class] cancelPreviousPerformRequestsWithTarget:[self kfTimeoutInvocation]
                                                              selector:@selector(invoke) 
                                                                object:nil];
    [self kfTearDownSimulatedIvars];
    [super dealloc];
}

#pragma mark -
#pragma mark BODY
#pragma mark -

#pragma mark responding to events

- (void)keyDown:(NSEvent *)keyEvent
{
    // Will we drop this event to super? 
    BOOL eatEvent = NO;
    
    if ([self kfCanPerformTypeSelect] && ([[self window] firstResponder] == self))
    { 
        BOOL canExtendFind = [self kfCanExtendFind];

        if (canExtendFind && KFKeyEventIsExtendFindEvent(keyEvent))
        {
            NSText *fieldEditor = [[self window] fieldEditor:YES forObject:self];
            [fieldEditor interpretKeyEvents:[NSArray arrayWithObject:keyEvent]];

            [self kfFindPattern:[fieldEditor string]
                     initialRow:[self kfSavedRowForExtensionSearch]
                    topToBottom:[self kfSearchTopToBottom]
                 allowExtension:YES];            
            eatEvent = YES;
        }
        else if (KFKeyEventIsBeginFindEvent(keyEvent))
        {
            NSText *fieldEditor = [[self window] fieldEditor:YES forObject:self];
            [fieldEditor setString:@""];
            [fieldEditor interpretKeyEvents:[NSArray arrayWithObject:keyEvent]];
            
            NSString *newPattern = [fieldEditor string];
            
            if (![newPattern isEqualToString:@""])
            {
                [self kfFindPattern:[fieldEditor string]
                         initialRow:[self kfInitialRowForNewSearch]
                        topToBottom:[self kfSearchTopToBottom]
                     allowExtension:YES];
            }
            
            eatEvent = YES;
        }
        else if (canExtendFind && KFKeyEventIsDeleteEvent(keyEvent))
        {
            // User might expect us to knock a character off the pattern - that'd be dangerous.
            // If the user mistimed he could trigger a table view delete action.
            // Best to squelch the behavior by not doing anything useful.
            
            eatEvent = YES;
        }        
        else if (KFKeyEventIsFindNextEvent(keyEvent))
        {
            [self findNext:self];
            eatEvent = YES;
        }
        else if (KFKeyEventIsFindPreviousEvent(keyEvent))
        {
            [self findPrevious:self];
            eatEvent = YES;
        }
        else if (KFKeyEventIsCancelEvent(keyEvent))
        {
            // this is superfluous in 10.2 and 10.3, but may be useful on systems prior to
            // 10.2.  I haven't had a chance to find out.
            [self cancelOperation:self];
            eatEvent = YES;
        }
    }
    
    if (!eatEvent)
    {
        // FIXME - hack  
        // I can't find a decent way to clear a hanging dead-key (i.e. option-e) in kfResetSearch.
        // Sending an event following a dead key event through interpretKeyEvents is the 
        // only thing I've found to do it, so we make sure that any keyEvent that we don't understand
        // goes through the field editor's interpretKeyEvents.  It won't cause any damage because the field
        // editor has no delegate and will be cleared before it's used again anyway.  
        // Without this workaround, entering "option-e, f" will stick this table in a state where all 
        // key-events start with character "«", which means type-select won't work. The state is only 
        // exited when a different control starts processing text.
        //
        // This workaround kills the above problem, but is suboptimal in that a dead-key never times out (besides just
        // being nasty).
        NSText *fieldEditor = [[self window] fieldEditor:YES forObject:self];
        [fieldEditor interpretKeyEvents:[NSArray arrayWithObject:keyEvent]];
        // end hack
        
        [self setKfCanExtendFind:NO];
        [super keyDown:keyEvent];
    }
}

- (void)cancelOperation:(id)sender
{
    [self kfResetSearch];
}

// 10.2 private version of cancelOperation
// I'm not sure how far back this will work.
- (void)_cancelKey:(id)sender
{
    [self cancelOperation:sender];
}


// an outline view catches control-down and control-up
// and uses them to select next and previous rows (same as plain up and down)
// We can catch the events by implementing moveDown and moveUp.

- (void)moveDown:(id)sender
{
    NSEvent *currentEvent = [NSApp currentEvent];
    if ([currentEvent type] == NSKeyDown && KFKeyEventIsFindNextEvent(currentEvent))
    {
        [self findNext:self];
    }
}

- (void)moveUp:(id)sender
{
    NSEvent *currentEvent = [NSApp currentEvent];
    if ([currentEvent type] == NSKeyDown && KFKeyEventIsFindPreviousEvent(currentEvent))
    {
        [self findPrevious:self];
    }    
}


- (BOOL)resignFirstResponder
{
    BOOL shouldResign = [super resignFirstResponder];
    if (shouldResign)
    {
        [self setKfCanExtendFind:NO];
    }
    
    return shouldResign;
}

static unsigned int modifierFlagsICareAboutMask = NSCommandKeyMask | NSShiftKeyMask | NSControlKeyMask | NSAlternateKeyMask | NSFunctionKeyMask;

// yes if every character in the event is alphanumeric and no command, control or function modifiers 
static BOOL KFKeyEventIsBeginFindEvent(NSEvent *keyEvent)
{
    unsigned int modifiers = [keyEvent modifierFlags] & modifierFlagsICareAboutMask;
    NSString *characters = [keyEvent characters];
    int numCharacters = [characters length];
    
    if ((modifiers & (NSCommandKeyMask | NSControlKeyMask | NSFunctionKeyMask)) != 0)
    {
        return NO;
    }
    
    NSMutableCharacterSet *beginFindCharacterSet = [[[NSCharacterSet alphanumericCharacterSet] mutableCopy] autorelease];
    [beginFindCharacterSet formUnionWithCharacterSet:[NSCharacterSet punctuationCharacterSet]];

    unichar character;
	int i;
    for (i = 0; i < numCharacters; i++)
    {
        character = [characters characterAtIndex:i];
        if (![beginFindCharacterSet characterIsMember:character])
        {
            return NO;
        }
    }
    
    return YES;    
}

// yes if every character in the event is alphanumeric, punctuation or a space, and no command, control or function modifiers 
static BOOL KFKeyEventIsExtendFindEvent(NSEvent *keyEvent)
{
    unsigned int modifiers = [keyEvent modifierFlags] & modifierFlagsICareAboutMask;
    NSString *characters = [keyEvent characters];
    int numCharacters = [characters length];
    
    if ((modifiers & (NSCommandKeyMask | NSControlKeyMask | NSFunctionKeyMask)) != 0)
    {
        return NO;
    }
    
    NSMutableCharacterSet *extendFindCharacterSet = [[[NSCharacterSet alphanumericCharacterSet] mutableCopy] autorelease];
    [extendFindCharacterSet formUnionWithCharacterSet:[NSCharacterSet punctuationCharacterSet]];
    [extendFindCharacterSet addCharactersInString:@" "];
    
    unichar character;
	int i;
    for (i = 0; i < numCharacters; i++)
    {
        character = [characters characterAtIndex:i];
        if (![extendFindCharacterSet characterIsMember:character])
        {
            return NO;
        }
    }
    
    return YES;    
}

static BOOL KFKeyEventIsFindNextEvent(NSEvent *keyEvent)
{
    unsigned int modifiers = [keyEvent modifierFlags] & modifierFlagsICareAboutMask;
    NSString *characters = [keyEvent characters];
    int numCharacters = [characters length];
    
    if (numCharacters == 1 && [characters characterAtIndex:0] == NSDownArrowFunctionKey && modifiers == (NSControlKeyMask | NSFunctionKeyMask))
    {
        return YES;
    }
    
    
    return NO;    
}

static BOOL KFKeyEventIsFindPreviousEvent(NSEvent *keyEvent)
{
    unsigned int modifiers = [keyEvent modifierFlags] & modifierFlagsICareAboutMask;
    NSString *characters = [keyEvent characters];
    int numCharacters = [characters length];
    
    if (numCharacters == 1 && [characters characterAtIndex:0] == NSUpArrowFunctionKey && modifiers == (NSControlKeyMask | NSFunctionKeyMask))
    {
        return YES;
    }
    
    return NO;    
}

static BOOL KFKeyEventIsDeleteEvent(NSEvent *keyEvent)
{
    unsigned int modifiers = [keyEvent modifierFlags] & modifierFlagsICareAboutMask;
    NSString *characters = [keyEvent characters];
    int numCharacters = [characters length];
    
    if (numCharacters == 1 && [characters characterAtIndex:0] == NSDeleteCharacter && modifiers == 0)
    {
        return YES;
    }
    if (numCharacters == 1 && [characters characterAtIndex:0] == NSBackspaceCharacter && modifiers == 0)
    {
        return YES;
    }    
    
    return NO;    
}

static BOOL KFKeyEventIsCancelEvent(NSEvent *keyEvent)
{
    unsigned int modifiers = [keyEvent modifierFlags] & modifierFlagsICareAboutMask;
    NSString *characters = [keyEvent characters];
    int numCharacters = [characters length];
    
    const unichar EscapeKeyCharacter = 0x1b;
    
    if ((modifiers == NSCommandKeyMask) && [characters isEqualToString:@"."])
    {
        return YES;
    }
    if (numCharacters == 1 && [characters characterAtIndex:0] == EscapeKeyCharacter && modifiers == 0)
    {
        return YES;
    }    
    
    return NO;    
}


#pragma mark finding patterns

- (void)findNext:(id)sender
{
    NSString *lastPattern = [self kfLastSuccessfullyMatchedPattern];
    
    if (lastPattern == nil || ![self kfCanPerformTypeSelect])
    {
        NSBeep();
    }
    else
    {
        [self kfFindPattern:lastPattern
                 initialRow:[self selectedRow] + 1 
                topToBottom:YES
             allowExtension:NO];        
    }
}

- (void)findPrevious:(id)sender
{
    NSString *lastPattern = [self kfLastSuccessfullyMatchedPattern];
    
    if (lastPattern == nil || ![self kfCanPerformTypeSelect])
    {
        NSBeep();
    }
    else
    {
        [self kfFindPattern:lastPattern
                 initialRow:[self selectedRow] - 1 
                topToBottom:NO
             allowExtension:NO];        
    }
}


- (void)kfFindPattern:(NSString *)pattern 
           initialRow:(int)initialRow 
          topToBottom:(BOOL)topToBottom
       allowExtension:(BOOL)allowPatternExtension
{
    NSArray *searchColumns = [self kfSearchColumns];
	
    BOOL shouldWrap = [self searchWraps];
    unsigned patternMatchOptions;
    NSDate *distantPast = [NSDate distantPast];
    NSMutableArray *suspendedEvents = [NSMutableArray array];
    const uint64_t eventCheckFrequency = SecondsToMachAbsolute(.01);
    NSString *match = nil;
    NSRange matchRange = {0,0};
    
    // we'll translate topToBottom into these parameters
    // so that we can use a single loop for both directions
    int rowIncrement, boundaryRow;

    if (topToBottom)
    {
        rowIncrement = 1;
        boundaryRow = [self numberOfRows];
        initialRow = (initialRow < boundaryRow) ? initialRow : boundaryRow;
        initialRow = (initialRow > 0) ? initialRow : 0;
        if (initialRow == 0)
            shouldWrap = NO;
    }
    else
    {
        rowIncrement = -1;
        boundaryRow = -1;
        initialRow = (initialRow > boundaryRow) ? initialRow : boundaryRow;
        initialRow = (initialRow < [self numberOfRows] - 1) ? initialRow : [self numberOfRows] - 1;
        if (initialRow == [self numberOfRows] - 1)
            shouldWrap = NO;
    }
    
    // keep ivars in sync
    [self setPattern:pattern];
    [self setKfCanExtendFind:allowPatternExtension];
    
    // set up pattern match options
    if ([self matchAlgorithm] == KFPrefixMatchAlgorithm)
    {
        patternMatchOptions = NSCaseInsensitiveSearch | NSAnchoredSearch;
    }
    else // substring match
    {
        patternMatchOptions = NSCaseInsensitiveSearch;
    }
    
    BOOL finished = NO;
    int row = initialRow;
    while (!finished)
    {
        // Mail generates 3MB in autoreleased objects in a search through 15000 rows
        // there's a noticable pause when they're deallocated.  We'll avoid it by using
        // our own autorelease pool. 
        // (Update - implementing typeSelectTableView:stringValueForTableColumn:row: in the mail
        // plugin dropped the number of allocations)
        //
        // Note: checking for new input no more often than 100 times a second drops time spent 
        // in -[NSApplication nextEventMatchingMask::::] from 30-60% of total function time to .2-.5% 
        // at a cost of < 1% for timing functions.
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        finished = [self kfWorkUnitGetMatch:&match
                                      range:&matchRange
                            lastSearchedRow:&row
                                 forPattern:pattern
                               matchOptions:patternMatchOptions
                                 initialRow:row
                                boundaryRow:boundaryRow
                               rowIncrement:rowIncrement
                              searchColumns:searchColumns
                                    timeout:eventCheckFrequency];
        [match retain];
        [pool release];
        [match autorelease];
        
        if (!finished)
            row += rowIncrement;
        
        if (finished && match == nil && shouldWrap)
        {
            if (topToBottom)
                row = 0;
            else
                row = [self numberOfRows] - 1;
            
            boundaryRow = initialRow;
            shouldWrap = NO;
            finished = NO;
        }
        
        if (!finished)
        {
            NSEvent *keyEvent;
            while ((keyEvent = [NSApp nextEventMatchingMask:NSKeyDownMask
                                                  untilDate:distantPast // means grab events that have already occurred
                                                     inMode:NSEventTrackingRunLoopMode 
                                                    dequeue:YES]) != nil)
            {
                if (KFKeyEventIsCancelEvent(keyEvent))
                {
                    [self kfResetSearch];
                    // we intentionally dump the suspended events in this case
                    return;
                }
                else if (allowPatternExtension  && KFKeyEventIsExtendFindEvent(keyEvent))
                {
                    NSText *fieldEditor = [[self window] fieldEditor:YES forObject:self];
                    [fieldEditor interpretKeyEvents:[NSArray arrayWithObject:keyEvent]];
                    pattern = [fieldEditor string];
                    [self setPattern:pattern];
                }
                else if (KFKeyEventIsDeleteEvent(keyEvent))
                {
                    // eat the event, do nothing.
                    // User might expect us to knock a character off the pattern - that'd be dangerous.
                    // If the user mistimed he could trigger a table view delete action.  
                    // Best to squelch the behavior by not doing anything useful.
                }
                else
                {
                    [suspendedEvents addObject:keyEvent];
                }
            }            
        }
    }
        
    if (match != nil)
    {
        [self kfDidFindMatch:match
                       range:matchRange
                       inRow:row];
    }
    else
    {
        [self kfDidFailToFindMatchSearchingToRow:row];
    }
    
    int numSuspendedEvents, i;
    numSuspendedEvents = [suspendedEvents count];
    for (i = numSuspendedEvents-1; i >= 0; i--)
    {
        [NSApp postEvent:[suspendedEvents objectAtIndex:i] atStart:YES];
    }
}

- (BOOL)kfWorkUnitGetMatch:(NSString **)match
                     range:(NSRange *)matchRange
           lastSearchedRow:(int *)lastSearchedRow
                forPattern:(NSString *)pattern
              matchOptions:(unsigned)patternMatchOptions
                initialRow:(int)initialRow
               boundaryRow:(int)boundaryRow
              rowIncrement:(int)rowIncrement
             searchColumns:(NSArray *)searchColumns
                   timeout:(uint64_t)timeout // times are mach absolute times
{
    int row, col;
    int numCols = [searchColumns count];
    NSString *candidateMatch;
    NSRange rangeOfPattern;
    
    uint64_t stopTime = mach_absolute_time() + timeout;
    for (row = initialRow; row != boundaryRow; row += rowIncrement)
    {
        for (col = 0; col < numCols; col++)
        {
            candidateMatch = [self kfStringValueForTableColumn:[searchColumns objectAtIndex:col] row:row];

            rangeOfPattern = [candidateMatch rangeOfString:pattern options:patternMatchOptions];
            if (   (rangeOfPattern.location != NSNotFound)
                && [self kfShouldAcceptMatch:candidateMatch range:rangeOfPattern inRow:row])
            {
                *match = candidateMatch;
                *matchRange = rangeOfPattern;
                *lastSearchedRow = row;
                return YES;
            }
        }
        
        // think of this as part of the loop condition, but we want to make sure that
        // the loop completes at least one iteration
        if (mach_absolute_time() > stopTime) 
        {
            row += rowIncrement;
            break;
        }
    }
    
    *match = nil;
    *matchRange = NSMakeRange(NSNotFound, 0);
    *lastSearchedRow = row - rowIncrement;
    return row == boundaryRow;
}

- (BOOL)kfShouldAcceptMatch:(NSString *)match 
                      range:(NSRange)matchedRange 
                      inRow:(int)row
{
    id delegate = [self delegate];
    
    if (   [self isKindOfClass:[NSOutlineView class]] 
           && [delegate respondsToSelector:@selector(outlineView:shouldSelectItem:)])
    {
        return [delegate outlineView:(NSOutlineView *)self shouldSelectItem:[(NSOutlineView *)self itemAtRow:row]];
    }
    else if ([delegate respondsToSelector:@selector(tableView:shouldSelectRow:)])
    {
        return [delegate tableView:self shouldSelectRow:row];
    }
    else
    {
        return YES;
    }
}

- (NSTimeInterval)kfPatternTimeoutInterval
{
    // from Dan Wood's 'Table Techniques Taught Tastefully', as pointed out by someone
    // on cocoadev.com
    
    // Timeout is two times the key repeat rate "InitialKeyRepeat" user default.
    // (converted from sixtieths of a second to seconds), but no more than two seconds.
    // This behavior is determined based on Inside Macintosh documentation on the List Manager.
    
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    int keyThreshTicks = [defaults integerForKey:@"InitialKeyRepeat"]; // undocumented key.  Still valid in 10.3. 
    if (0 == keyThreshTicks)	// missing value in defaults?  Means user has never changed the default.
    {
        keyThreshTicks = 35;	// apparent default value. translates to 1.17 sec timeout.
    }
    
    return MIN(2.0/60.0*keyThreshTicks, 2.0);
}

- (BOOL)kfCanPerformTypeSelect
{
    return [self kfCanGetTableData] && [self kfSelectionShouldChange];
}

- (BOOL)kfSelectionShouldChange
{
    id delegate = [self delegate];
    
    if (   [self isKindOfClass:[NSOutlineView class]] 
           && [delegate respondsToSelector:@selector(selectionShouldChangeInOutlineView:)])
    {
        return [delegate selectionShouldChangeInOutlineView:(NSOutlineView *)self];
    }
    else if ([delegate respondsToSelector:@selector(selectionShouldChangeInTableView:)])
    {
        return [delegate selectionShouldChangeInTableView:self];
    }
    else
    {
        return YES;
    }    
}

- (BOOL)kfCanGetTableData
{    
    // First case:  datasource implements NSTableViewDataSource protocol.  Usually not true when 
    //              table view uses bindings or is actually an outline view.
    // Second case: self is an outline view and datasource implements NSOutlineViewDataSource protocol.  This could arise when using class posing.
    // Third case:  our delegate supplies the info we need.
    return ([[self dataSource] respondsToSelector:@selector(tableView:objectValueForTableColumn:row:)] || 
            ([self isKindOfClass:[NSOutlineView class]] && [[self dataSource] respondsToSelector:@selector(outlineView:objectValueForTableColumn:byItem:)]) ||
            [[self delegate] respondsToSelector:@selector(typeSelectTableView:stringValueForTableColumn:row:)]);
}

- (NSString *)kfStringValueForTableColumn:(NSTableColumn *)column row:(int)row
{
    // There are three ways we can get this information: (1) our delegate supplies it, (2) our datasource 
    // supplies it like an NSTableViewDataSource, (3) our datasource supplies it like an NSOutlineViewDataSource
    
    // could optimize by factoring into three separate methods and precomputing which one to call (from keyDown:).
    // current sharking indicates this wouldn't help much.
    
    id delegate = [self delegate];
    NSString *stringValue = nil;
    
    if ([delegate respondsToSelector:@selector(typeSelectTableView:stringValueForTableColumn:row:)])
    {
        stringValue = [delegate typeSelectTableView:self stringValueForTableColumn:column row:row];
    }
    else
    {
        id objectValue = nil;
        id dataSource = [self dataSource];
        
        // why do we check our own class?  outline view is not bindings enabled, so datasource could be
        // acting as a datasource for an outline while being a binding data source for us
        if ([self isKindOfClass:[NSOutlineView class]] 
            && [dataSource respondsToSelector:@selector(outlineView:objectValueForTableColumn:byItem:)])
        {
            objectValue = [dataSource outlineView:(NSOutlineView *)self 
                        objectValueForTableColumn:column 
                                           byItem:[(NSOutlineView *)self itemAtRow:row]];
        }
        else if ([dataSource respondsToSelector:@selector(tableView:objectValueForTableColumn:row:)])
        {
            objectValue = [dataSource tableView:self objectValueForTableColumn:column row:row];
        }
        
        NSCell *dataCell = [column dataCellForRow:row];
        [dataCell setObjectValue:objectValue];
        
        // sometimes the delegate changes the cell value in tableView:willDisplayCell:forTableColumn:row:
        if ([self isKindOfClass:[NSOutlineView class]] 
            && [delegate respondsToSelector:@selector(outlineView:willDisplayCell:forTableColumn:item:)])
        {
            [delegate outlineView:(NSOutlineView *)self 
                  willDisplayCell:dataCell
                   forTableColumn:column 
                             item:[(NSOutlineView *)self itemAtRow:row]];
        }
        else if ([delegate respondsToSelector:@selector(tableView:willDisplayCell:forTableColumn:row:)])
        {
            [delegate tableView:self 
                willDisplayCell:dataCell
                 forTableColumn:column 
                            row:row];
        }
        
        stringValue = [dataCell stringValue];
    }
    
    if (stringValue == nil)
    {
        stringValue = @"";
    }
    
    return stringValue;
}

- (NSArray *)kfSearchColumns
{
    NSArray *searchColumns;
    NSSet *searchColumnIdentifiers = [self searchColumnIdentifiers];
    
    if (searchColumnIdentifiers != nil)
    {
        NSMutableArray *partialSearchColumns;
        NSArray *candidateColumns = [self tableColumns];
        NSTableColumn *column;
        int numCols, col;
        
        partialSearchColumns = [NSMutableArray array];
        
        numCols = [candidateColumns count];
        for (col = 0; col < numCols; col++)
        {
            column = [candidateColumns objectAtIndex:col];
            if ([searchColumnIdentifiers containsObject:[column identifier]])
            {
                [partialSearchColumns addObject:column];
            }
        }
        
        searchColumns = partialSearchColumns;
    }
    else
    {
        searchColumns = [self tableColumns];
    }
    
    return searchColumns;
}


- (BOOL)kfSearchTopToBottom
{
    BOOL topToBottom = YES;
    
    id delegate = [self delegate];
    if ([delegate respondsToSelector:@selector(typeSelectTableViewSearchTopToBottom:)])
    {
        topToBottom = [delegate typeSelectTableViewSearchTopToBottom:self];
    }
    
    return topToBottom;
}

- (int)kfInitialRowForNewSearch
{
    int row;
    
    id delegate = [self delegate];
    if ([delegate respondsToSelector:@selector(typeSelectTableViewInitialSearchRow:)])
    {
        row = [delegate typeSelectTableViewInitialSearchRow:self];
    }
    else
    {
        if ([self kfSearchTopToBottom])
        {
            row = 0;
        }
        else
        {
            row = [self numberOfRows] - 1;
        }
    }
    
    return row;
}

#pragma mark taking action 

-(void)kfPatternDidChange:(id)sender 
{
    NSInvocation *timeoutInvocation = [self kfTimeoutInvocation];
    [[timeoutInvocation class] cancelPreviousPerformRequestsWithTarget:[self kfTimeoutInvocation]
                                                              selector:@selector(invoke) 
                                                                object:nil];
    
    id delegate = [self delegate];
    if ([delegate respondsToSelector:@selector(typeSelectTableViewPatternDidChange:)])
        [delegate typeSelectTableViewPatternDidChange:sender];
}

- (void)kfDidFindMatch:(NSString *)match 
                 range:(NSRange)matchedRange 
                 inRow:(int)row
{   
    NSString *pattern = [self pattern];
    
    // update ivars
    [self setKfLastSuccessfullyMatchedPattern:pattern];    
    if ([self kfCanExtendFind])
    {
        [self setKfSavedRowForExtensionSearch:row];
    }
    
    // select row
    [self selectRow:row byExtendingSelection:NO];
    if (![self kfRowIsVisible:row])
    {
        // this is what NSTextView does when it finds patterns, and it's what Mail does
        // when moving through message table with up and down arrows
        [self kfScrollRectToCenter:[self rectOfRow:row] vertical:YES horizontal:NO];
    }
    
    // start pattern timeout timer (see kfTimeoutInvocation for details)
    [[self kfTimeoutInvocation] performSelector:@selector(invoke)
                                     withObject:nil
                                     afterDelay:[self kfPatternTimeoutInterval] 
                                        inModes:[NSArray arrayWithObjects:NSDefaultRunLoopMode, NSModalPanelRunLoopMode, nil]];

    
    // inform the delegate
    id delegate = [self delegate];
    if ([delegate respondsToSelector:@selector(typeSelectTableView:didFindMatch:range:forPattern:)])
    {
        [delegate typeSelectTableView:self didFindMatch:match range:matchedRange forPattern:pattern];
    }
}

- (void)kfDidFailToFindMatchSearchingToRow:(int)row
{
    if ([self kfCanExtendFind])
    {
        [self setKfSavedRowForExtensionSearch:row];
    }
    
    // start pattern timeout timer (see kfTimeoutInvocation for details)
    [[self kfTimeoutInvocation] performSelector:@selector(invoke)
                                     withObject:nil
                                     afterDelay:[self kfPatternTimeoutInterval] 
                                        inModes:[NSArray arrayWithObjects:NSDefaultRunLoopMode, NSModalPanelRunLoopMode, nil]];

    
    id delegate = [self delegate];
    if ([delegate respondsToSelector:@selector(typeSelectTableView:didFailToFindMatchForPattern:)])
    {
        [delegate typeSelectTableView:self didFailToFindMatchForPattern:[self pattern]];
    }
    else
    {
        NSBeep();
    }
}

- (void)kfResetSearch
{
    // note - doesn't clear hanging dead key.  See keyDown for discussion and workaround.
    [self setPattern:@""];
    [self setKfCanExtendFind:NO];
}

// note: don't use setDelegate: to call this. NSOutlineView doesn't call through to 
// -[NSTableView setDelegate:], so it messes us up when posing.
- (void)kfConfigureDelegateIfNeeded
{
    id delegate = [self delegate];
    if (delegate != [self kfLastConfiguredDelegate])
    {
        // order is important here
        // We don't want to go into a recursion if the delegate tries to access a configurable value from 
        // configureTypeSelectTableView.  The delegate is interested in the pre-configuration values anyway.
        [self setKfLastConfiguredDelegate:delegate];
        if ([delegate respondsToSelector:@selector(configureTypeSelectTableView:)])
            [delegate configureTypeSelectTableView:self];
    }    
}
#pragma mark utility

- (BOOL)kfRowIsVisible:(int)row 
{
    NSScrollView *enclosingScrollView = [self enclosingScrollView];
    if (enclosingScrollView == nil)
    {
        return NO;
    }
    else
    {
        NSRect visibleRect = [enclosingScrollView documentVisibleRect];
        NSRect rowRect = [self rectOfRow:row];
        
        // only care about whether we're onscreen vertically
        return (   (NSMaxY(visibleRect) >= NSMaxY(rowRect))
                && (NSMinY(visibleRect) <= NSMinY(rowRect)));
    }
}

- (void)kfScrollRectToCenter:(NSRect)aRect vertical:(BOOL)scrollVertical horizontal:(BOOL)scrollHorizontal
{
    NSScrollView *scrollView = [self enclosingScrollView];
    
    if (scrollView != nil)
    {
        NSRect newVisibleRect = [scrollView documentVisibleRect];
        
        if (scrollVertical)
            newVisibleRect.origin.y += NSMidY(aRect) - NSMidY([scrollView documentVisibleRect]);
        if (scrollHorizontal)
            newVisibleRect.origin.x += NSMidX(aRect) - NSMidX([scrollView documentVisibleRect]);
        
        newVisibleRect = NSIntersectionRect(newVisibleRect,[self bounds]);
        
        [self scrollRectToVisible:newVisibleRect];
    }
}

#pragma mark -
#pragma mark ACCESSORS
#pragma mark -

#pragma mark simulated ivars setup

static NSMutableDictionary *idToSimulatedIvarsMap = nil; 

- (NSMutableDictionary *)kfSimulatedIvars
{
    NSMutableDictionary *simulatedIvars = [idToSimulatedIvarsMap objectForKey:[self kfIdentifier]];
    
    if (simulatedIvars == nil)
    {
        [self kfSetUpSimulatedIvars];
        simulatedIvars = [idToSimulatedIvarsMap objectForKey:[self kfIdentifier]];
    }
    
    return simulatedIvars;
}

// can avoid memory allocation if we use CFDictionary or NSMapTable and work with self directly
- (id)kfIdentifier
{
    return [NSValue valueWithPointer:self];
}

- (void)kfSetUpSimulatedIvars
{
    // prime idToSimulatedIvarsMap
    if (idToSimulatedIvarsMap == nil)
    {
        idToSimulatedIvarsMap = [[NSMutableDictionary alloc] init];
    }
        
    // if the simulatedIvars dict doesn't exist yet, create it
    NSMutableDictionary *simulatedIvars = [idToSimulatedIvarsMap objectForKey:[self kfIdentifier]];
    if (!simulatedIvars)
    {
        simulatedIvars = [NSMutableDictionary dictionary];
        [idToSimulatedIvarsMap setObject:simulatedIvars forKey:[self kfIdentifier]];
    }
}

- (void)kfTearDownSimulatedIvars
{
    [idToSimulatedIvarsMap removeObjectForKey:[self kfIdentifier]];
    
    if ([idToSimulatedIvarsMap count] == 0)
    {
        [idToSimulatedIvarsMap release];
        idToSimulatedIvarsMap = nil;
    }
}

#pragma mark private accessors

- (int)kfSavedRowForExtensionSearch
{
    int row;
    NSNumber *rowNumber = [[self kfSimulatedIvars] objectForKey:@"initialRowForExtensionSearch"];
    
    // default value
    if (rowNumber == nil)
        row = NSNotFound;
    else
    {
        row = [rowNumber intValue];
    }
    
    return row;
}

- (void)setKfSavedRowForExtensionSearch:(int)row
{
    [[self kfSimulatedIvars]  setObject:[NSNumber numberWithInt:row]
                                 forKey:@"initialRowForExtensionSearch"];
}

- (NSString *)kfLastSuccessfullyMatchedPattern
{
    NSString *string = [[self kfSimulatedIvars] objectForKey:@"lastPattern"];
    
    // defaults to nil
    
    return string;
}

- (void)setKfLastSuccessfullyMatchedPattern:(NSString *)string
{
    if (string == nil)
        [[self kfSimulatedIvars] removeObjectForKey:@"lastPattern"];
    else
        [[self kfSimulatedIvars] setObject:[[string copy] autorelease] forKey:@"lastPattern"];
}

-(BOOL)kfCanExtendFind
{
    NSNumber *canExtendFindNumber = [[self kfSimulatedIvars] objectForKey:@"canExtendFind"];
    
    // default value
    if (canExtendFindNumber == nil)
        return NO;
    else
        return [canExtendFindNumber boolValue];
}

-(void)setKfCanExtendFind:(BOOL)flag
{
    [[self kfSimulatedIvars] setObject:[NSNumber numberWithBool:flag]
                                forKey:@"canExtendFind"];
}

// keep track of the last delegate for which we tried to run configureTypeSelectTableView
- (id)kfLastConfiguredDelegate
{
    return [[[self kfSimulatedIvars] objectForKey:@"lastConfiguredDelegate"] nonretainedObjectValue];
}

- (void)setKfLastConfiguredDelegate:(id)anObject
{
    if (anObject == nil)
        [[self kfSimulatedIvars] removeObjectForKey:@"lastConfiguredDelegate"];
    else
        [[self kfSimulatedIvars] setObject:[NSValue valueWithNonretainedObject:anObject] forKey:@"lastConfiguredDelegate"];
}

//
//  the timeoutNotification encapsulates the message that we send to self when the timeout
//  (for clearing the input buffer) expires.  Invoke it with a delayed message send.
//
//  Why do we use an invocation instead of doing the delayed message send directly?   
//  -[NSObject performSelector:afterDelay:] retains the receiver until after the message send is
//  performed.  That can extend the life of the tableView past the life of the delegate, which is
//  bad mojo.  Yielded a crash in Adium.  By buffering with an invocation that doesn't retain its
//  target, we can avoid the problem.  Any pending delayed messages are cancelled when the table
//  table is dealloc'd.
//
- (NSInvocation *)kfTimeoutInvocation
{
    NSInvocation *invocation = [[self kfSimulatedIvars] objectForKey:@"timeoutInvocation"];
    
    // defaults to a message to kfResetSearch
    if (invocation == nil)
    {
        SEL selector = @selector(kfResetSearch);
        invocation = [NSInvocation invocationWithMethodSignature:[self methodSignatureForSelector:selector]];
        [invocation setTarget:self];
        [invocation setSelector:selector];
        
        [self setKfTimeoutInvocation:invocation];
    }
    
    return invocation;
}

- (void)setKfTimeoutInvocation:(NSInvocation *)anInvocation
{
    if (anInvocation == nil)
        [[self kfSimulatedIvars] removeObjectForKey:@"timeoutInvocation"];
    else
        [[self kfSimulatedIvars] setObject:anInvocation forKey:@"timeoutInvocation"];    
}


-(void)setPattern:(NSString *)pattern
{
    NSString *oldPattern = [self pattern];
    
    if (pattern == nil)
        pattern = @"";
    
    [[self kfSimulatedIvars] setObject:[[pattern copy] autorelease]
                                forKey:@"pattern"];
    
    NSNotification *patternChangedNotification = [NSNotification notificationWithName:KFTypeSelectTableViewPatternDidChangeNotification
                                                                              object:self
                                                                            userInfo:[NSDictionary dictionaryWithObject:oldPattern forKey:@"oldPattern"]];
    [self kfPatternDidChange:patternChangedNotification];
    [[NSNotificationCenter defaultCenter] postNotification:patternChangedNotification];
}

#pragma mark public accessors

-(NSString *)pattern
{
    NSString *pattern = [[self kfSimulatedIvars] objectForKey:@"pattern"];
    
    if (pattern == nil)
        pattern = @"";
    
    return [[pattern retain] autorelease];
}

static KFTypeSelectMatchAlgorithm defaultMatchAlgorith = KFPrefixMatchAlgorithm;
+ (KFTypeSelectMatchAlgorithm)defaultMatchAlgorithm
{
    return defaultMatchAlgorith;
}

+ (void)setDefaultMatchAlgorithm:(KFTypeSelectMatchAlgorithm)algorithm
{
    defaultMatchAlgorith = algorithm;
}


-(KFTypeSelectMatchAlgorithm)matchAlgorithm
{
    [self kfConfigureDelegateIfNeeded];
    
    NSNumber *algorithmNumber = [[self kfSimulatedIvars] objectForKey:@"matchAlgorithm"];

    if (algorithmNumber == nil)
        return defaultMatchAlgorith;
    else 
        return [algorithmNumber intValue];
}

-(void)setMatchAlgorithm:(KFTypeSelectMatchAlgorithm)algorithm
{
    [[self kfSimulatedIvars] setObject:[NSNumber numberWithInt:algorithm] forKey:@"matchAlgorithm"];
}

- (BOOL)searchWraps
{
    [self kfConfigureDelegateIfNeeded];

    NSNumber *searchWraphsNum = [[self kfSimulatedIvars] objectForKey:@"searchWraps"];

    // default value
    if (searchWraphsNum == nil)
        return NO;
    else
        return [searchWraphsNum boolValue];
}

-(void)setSearchWraps:(BOOL)flag
{
    [[self kfSimulatedIvars] setObject:[NSNumber numberWithBool:flag]
                                forKey:@"searchWraps"];
}


- (NSSet *)searchColumnIdentifiers
{
    [self kfConfigureDelegateIfNeeded];

    return [[self kfSimulatedIvars] objectForKey:@"searchColumnIdentifiers"];
}

- (void)setSearchColumnIdentifiers:(NSSet *)identifiers
{
    if (identifiers == nil)
        [[self kfSimulatedIvars] removeObjectForKey:@"searchColumnIdentifiers"];
    else
        [[self kfSimulatedIvars] setObject:identifiers forKey:@"searchColumnIdentifiers"];
}

@end

#pragma mark -
#pragma mark HELPER 
#pragma mark -

// need a time function, don't want it to be sensitive to time zone switches or
// clock syncs, would rather not require linking Carbon. We'll go with mach_absolute_time.
// Written with reference to <http://developer.apple.com/qa/qa2004/qa1398.html>.
static uint64_t SecondsToMachAbsolute(double seconds)
{
    double nanoseconds_d = seconds * 1000000000;
    Nanoseconds nanoseconds_n = UInt64ToUnsignedWide((uint64_t) nanoseconds_d);
    return UnsignedWideToUInt64(NanosecondsToAbsolute(nanoseconds_n));
}


