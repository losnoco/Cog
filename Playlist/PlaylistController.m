//
//	PlaylistController.m
//	Cog
//
//	Created by Vincent Spader on 3/18/05.
//	Copyright 2005 Vincent Spader All rights reserved.
//

#import "PlaylistController.h"
#import "PlaylistEntry.h"
#import "PlaylistLoader.h"
#import "PlaybackController.h"
#import "Shuffle.h"
#import "SpotlightWindowController.h"
#import "RepeatTransformers.h"
#import "ShuffleTransformers.h"
#import "StatusImageTransformer.h"
#import "ToggleQueueTitleTransformer.h"

#import "Logging.h"


#define UNDO_STACK_LIMIT 0

@implementation PlaylistController

@synthesize currentEntry;
@synthesize totalTime;

+ (void)initialize {
	NSValueTransformer *repeatNoneTransformer = [[RepeatModeTransformer alloc] initWithMode:RepeatNone];
    [NSValueTransformer setValueTransformer:repeatNoneTransformer
                                    forName:@"RepeatNoneTransformer"];

	NSValueTransformer *repeatOneTransformer = [[RepeatModeTransformer alloc] initWithMode:RepeatOne];
    [NSValueTransformer setValueTransformer:repeatOneTransformer
                                    forName:@"RepeatOneTransformer"];

	NSValueTransformer *repeatAlbumTransformer = [[RepeatModeTransformer alloc] initWithMode:RepeatAlbum];
    [NSValueTransformer setValueTransformer:repeatAlbumTransformer
                                    forName:@"RepeatAlbumTransformer"];

	NSValueTransformer *repeatAllTransformer = [[RepeatModeTransformer alloc] initWithMode:RepeatAll];
    [NSValueTransformer setValueTransformer:repeatAllTransformer
                                    forName:@"RepeatAllTransformer"];

	NSValueTransformer *repeatModeImageTransformer = [[RepeatModeImageTransformer alloc] init];
    [NSValueTransformer setValueTransformer:repeatModeImageTransformer
                                    forName:@"RepeatModeImageTransformer"];
	

	NSValueTransformer *shuffleOffTransformer = [[ShuffleModeTransformer alloc] initWithMode:ShuffleOff];
    [NSValueTransformer setValueTransformer:shuffleOffTransformer
                                    forName:@"ShuffleOffTransformer"];
	
	NSValueTransformer *shuffleAlbumsTransformer = [[ShuffleModeTransformer alloc] initWithMode:ShuffleAlbums];
    [NSValueTransformer setValueTransformer:shuffleAlbumsTransformer
                                    forName:@"ShuffleAlbumsTransformer"];
	
	NSValueTransformer *shuffleAllTransformer = [[ShuffleModeTransformer alloc] initWithMode:ShuffleAll];
    [NSValueTransformer setValueTransformer:shuffleAllTransformer
                                    forName:@"ShuffleAllTransformer"];

	NSValueTransformer *shuffleImageTransformer = [[ShuffleImageTransformer alloc] init];
    [NSValueTransformer setValueTransformer:shuffleImageTransformer
                                    forName:@"ShuffleImageTransformer"];
	
	
	
	NSValueTransformer *statusImageTransformer = [[StatusImageTransformer alloc] init];
    [NSValueTransformer setValueTransformer:statusImageTransformer
                                    forName:@"StatusImageTransformer"];
									
	NSValueTransformer *toggleQueueTitleTransformer = [[ToggleQueueTitleTransformer alloc] init];
    [NSValueTransformer setValueTransformer:toggleQueueTitleTransformer
                                    forName:@"ToggleQueueTitleTransformer"];
}


- (void)initDefaults
{
	NSDictionary *defaultsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
										[NSNumber numberWithInteger:RepeatNone], @"repeat",
										[NSNumber numberWithInteger:ShuffleOff],  @"shuffle",
										nil];
	
	[[NSUserDefaults standardUserDefaults] registerDefaults:defaultsDictionary];
}


- (id)initWithCoder:(NSCoder *)decoder
{
	self = [super initWithCoder:decoder];
	
	if (self)
	{
		shuffleList = [[NSMutableArray alloc] init];
		queueList = [[NSMutableArray alloc] init];

        undoManager = [[NSUndoManager alloc] init];

        [undoManager setLevelsOfUndo:UNDO_STACK_LIMIT];

		[self initDefaults];
	}
	
	return self;
}


- (void)awakeFromNib
{
	[super awakeFromNib];

	[self addObserver:self forKeyPath:@"arrangedObjects" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld) context:nil];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
	if ([keyPath isEqualToString:@"arrangedObjects"])	
	{
		[self updatePlaylistIndexes];
		[self updateTotalTime];
	}
}

- (void)updatePlaylistIndexes
{
	int i;
	NSArray *arranged = [self arrangedObjects];
	for (i = 0; i < [arranged count]; i++)
	{
		PlaylistEntry *pe = [arranged objectAtIndex:i];
		if (pe.index != i) //Make sure we don't get into some kind of crazy observing loop...
			pe.index = i;
	}
}

- (void)updateTotalTime
{
	double tt = 0;
	ldiv_t hoursAndMinutes;
    ldiv_t daysAndHours;
	
	for (PlaylistEntry *pe in [self arrangedObjects]) {
        if (!isnan([pe.length doubleValue]))
            tt += [pe.length doubleValue];
	}
	
	long sec = (long)(tt);
	hoursAndMinutes = ldiv(sec/60, 60);
	
    if ( hoursAndMinutes.quot >= 24 )
    {
        daysAndHours = ldiv(hoursAndMinutes.quot, 24);
        [self setTotalTime:[NSString stringWithFormat:@"%ld days %ld hours %ld minutes %ld seconds", daysAndHours.quot, daysAndHours.rem, hoursAndMinutes.rem, sec%60]];
    }
    else
        [self setTotalTime:[NSString stringWithFormat:@"%ld hours %ld minutes %ld seconds", hoursAndMinutes.quot, hoursAndMinutes.rem, sec%60]];
}

- (void)tableView:(NSTableView *)tableView
		didClickTableColumn:(NSTableColumn *)tableColumn
{
	if ([self shuffle] != ShuffleOff)
		[self resetShuffleList];
}

- (NSString *)tableView:(NSTableView *)tv toolTipForCell:(NSCell *)cell rect:(NSRectPointer)rect tableColumn:(NSTableColumn *)tc row:(int)row mouseLocation:(NSPoint)mouseLocation
{
	DLog(@"GETTING STATUS FOR ROW: %i: %@!", row, [[[self arrangedObjects] objectAtIndex:row] statusMessage]);
	return [[[self arrangedObjects] objectAtIndex:row] statusMessage];
}

-(void)moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet*)indexSet
										toIndex:(unsigned int)insertIndex
{
	[super moveObjectsInArrangedObjectsFromIndexes:indexSet toIndex:insertIndex];

	NSUInteger lowerIndex = insertIndex;
	NSUInteger index = insertIndex;
	
	while (NSNotFound != lowerIndex) {
		lowerIndex = [indexSet indexLessThanIndex:lowerIndex];
		
		if (lowerIndex != NSNotFound)
			index = lowerIndex;
	}
	
	[playbackController playlistDidChange:self];
}

- (BOOL)tableView:(NSTableView *)aTableView writeRowsWithIndexes:(NSIndexSet *)rowIndexes toPasteboard:(NSPasteboard *)pboard
{
	[super tableView:aTableView writeRowsWithIndexes:rowIndexes toPasteboard:pboard];

	NSMutableArray *filenames = [NSMutableArray array];
	NSInteger row;
	for (row = [rowIndexes firstIndex];
		 row <= [rowIndexes lastIndex];
		 row = [rowIndexes indexGreaterThanIndex:row])
	{
		PlaylistEntry *song = [[self arrangedObjects] objectAtIndex:row];
		[filenames addObject:[[song path] stringByExpandingTildeInPath]];
	}

	[pboard addTypes:[NSArray arrayWithObject:NSFilenamesPboardType] owner:self];
    [pboard setPropertyList:filenames forType:NSFilenamesPboardType];
	
	return YES;
}

- (BOOL)tableView:(NSTableView*)tv
	   acceptDrop:(id <NSDraggingInfo>)info
			  row:(int)row
	dropOperation:(NSTableViewDropOperation)op
{
	//Check if DNDArrayController handles it.
	if ([super tableView:tv acceptDrop:info row:row dropOperation:op])
		return YES;

	if (row < 0)
		row = 0;
	
			
	// Determine the type of object that was dropped
	NSArray *supportedTypes = [NSArray arrayWithObjects:CogUrlsPboardType, NSFilenamesPboardType, iTunesDropType, nil];
	NSPasteboard *pboard = [info draggingPasteboard];
	NSString *bestType = [pboard availableTypeFromArray:supportedTypes];

	NSMutableArray *acceptedURLs = [[NSMutableArray alloc] init];
	
	// Get files from an file drawer drop
	if ([bestType isEqualToString:CogUrlsPboardType]) {
		NSArray *urls = [NSUnarchiver unarchiveObjectWithData:[[info draggingPasteboard] dataForType:CogUrlsPboardType]];
		DLog(@"URLS: %@", urls);
		//[playlistLoader insertURLs: urls atIndex:row sort:YES];
		[acceptedURLs addObjectsFromArray:urls];
	}
	
	// Get files from a normal file drop (such as from Finder)
	if ([bestType isEqualToString:NSFilenamesPboardType]) {
		NSMutableArray *urls = [[NSMutableArray alloc] init];

		for (NSString *file in [[info draggingPasteboard] propertyListForType:NSFilenamesPboardType])
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
		[self willInsertURLs:acceptedURLs origin:URLOriginInternal];
		
		if (![[self content] count]) {
			row = 0;
		}
		
		NSArray* entries = [playlistLoader insertURLs:acceptedURLs atIndex:row sort:YES];
		[self didInsertURLs:entries origin:URLOriginInternal];
	}
	
	if ([self shuffle] != ShuffleOff)
		[self resetShuffleList];
	
	return YES;
}

- (NSUndoManager *)undoManager
{
	return undoManager;
}

- (NSIndexSet *)disarrangeIndexes:(NSIndexSet *)indexes
{
    if ([[self arrangedObjects] count] <= [indexes lastIndex])
        return indexes;
    
    NSMutableIndexSet *disarrangedIndexes = [[NSMutableIndexSet alloc] init];
    
    NSUInteger index = [indexes firstIndex];
    while (index != NSNotFound)
    {
        [disarrangedIndexes addIndex:[[self content] indexOfObject:[[self arrangedObjects] objectAtIndex:index]]];
        index = [indexes indexGreaterThanIndex:index];
    }
    
    return disarrangedIndexes;
}

- (NSArray *)disarrangeObjects:(NSArray *)objects
{
    NSMutableArray *disarrangedObjects = [[NSMutableArray alloc] init];
    
    for (PlaylistEntry *pe in [self content])
    {
        if ([objects containsObject:pe])
            [disarrangedObjects addObject:pe];
    }
    
    return disarrangedObjects;
}

- (NSIndexSet *)rearrangeIndexes:(NSIndexSet *)indexes
{
    if ([[self content] count] <= [indexes lastIndex])
        return indexes;
    
    NSMutableIndexSet *rearrangedIndexes = [[NSMutableIndexSet alloc] init];
    
    NSUInteger index = [indexes firstIndex];
    while (index != NSNotFound)
    {
        [rearrangedIndexes addIndex:[[self arrangedObjects] indexOfObject:[[self content] objectAtIndex:index]]];
        index = [indexes indexGreaterThanIndex:index];
    }
    
    return rearrangedIndexes;
}

- (void)insertObjects:(NSArray *)objects atIndexes:(NSIndexSet *)indexes
{
    [self insertObjects:objects atArrangedObjectIndexes:indexes];
    [self rearrangeObjects];
}

- (void)insertObjects:(NSArray *)objects atArrangedObjectIndexes:(NSIndexSet *)indexes
{
    [[[self undoManager] prepareWithInvocationTarget:self] removeObjectsAtIndexes:[self disarrangeIndexes:indexes]];
    NSString *actionName = [NSString stringWithFormat:@"Adding %lu entries", (unsigned long)[objects count]];
    [[self undoManager] setActionName:actionName];

    [super insertObjects:objects atArrangedObjectIndexes:indexes];

    if ([self shuffle] != ShuffleOff)
        [self resetShuffleList];
}

- (void)removeObjectsAtIndexes:(NSIndexSet *)indexes
{
    [self removeObjectsAtArrangedObjectIndexes:[self rearrangeIndexes:indexes]];
}

- (void)removeObjectsAtArrangedObjectIndexes:(NSIndexSet *)indexes
{
    NSArray *objects = [[self arrangedObjects] objectsAtIndexes:indexes];
    [[[self undoManager] prepareWithInvocationTarget:self] insertObjects:[self disarrangeObjects:objects] atIndexes:[self disarrangeIndexes:indexes]];
    NSString *actionName = [NSString stringWithFormat:@"Removing %lu entries", (unsigned long)[indexes count]];
    [[self undoManager] setActionName:actionName];
    
    DLog(@"Removing indexes: %@", indexes);
    DLog(@"Current index: %i", currentEntry.index);
    
    NSMutableIndexSet *unarrangedIndexes = [[NSMutableIndexSet alloc] init];
    for (PlaylistEntry *pe in objects)
    {
        [unarrangedIndexes addIndex:[pe index]];
    }
    
    if (currentEntry.index >= 0 && [unarrangedIndexes containsIndex:currentEntry.index])
    {
        currentEntry.index = -currentEntry.index - 1;
        DLog(@"Current removed: %i", currentEntry.index);
    }
    
    if (currentEntry.index < 0) //Need to update the negative index
    {
        int i = -currentEntry.index - 1;
        DLog(@"I is %i", i);
        int j;
        for (j = i - 1; j >= 0; j--)
        {
            if ([unarrangedIndexes containsIndex:j]) {
                DLog(@"Removing 1");
                i--;
            }
        }
        currentEntry.index = -i - 1;

    }
    
    [super removeObjectsAtArrangedObjectIndexes:indexes];

    if ([self shuffle] != ShuffleOff)
        [self resetShuffleList];

    [playbackController playlistDidChange:self];
}

- (void)setSortDescriptors:(NSArray *)sortDescriptors
{
	DLog(@"Current: %@, setting: %@", [self sortDescriptors], sortDescriptors);

	//Cheap hack so the index column isn't sorted
	if (([sortDescriptors count] != 0) && [[[sortDescriptors objectAtIndex:0] key] caseInsensitiveCompare:@"index"] == NSOrderedSame)
	{
		//Remove the sort descriptors
		[super setSortDescriptors:[NSArray array]];
		[self rearrangeObjects];
		
		return;
	}

	[super setSortDescriptors:sortDescriptors];
	[self rearrangeObjects];

	[playbackController playlistDidChange:self];
}
		
- (IBAction)randomizeList:(id)sender
{
	[self setSortDescriptors:[NSArray array]];

    NSArray *unrandomized = [self content];
    [[[self undoManager] prepareWithInvocationTarget:self] unrandomizeList:unrandomized];

    [self setContent:[Shuffle shuffleList:[self content]]];

    if ([self shuffle] != ShuffleOff)
 		[self resetShuffleList];

    [[self undoManager] setActionName:@"Playlist Randomization"];
}

- (void)unrandomizeList:(NSArray *)entries
{
    [[[self undoManager] prepareWithInvocationTarget:self] randomizeList:self];
    [self setContent:entries];
}

- (IBAction)toggleShuffle:(id)sender
{
	ShuffleMode shuffle = [self shuffle];
	
	if (shuffle == ShuffleOff) {
		[self setShuffle: ShuffleAlbums];
	}
	else if (shuffle == ShuffleAlbums) {
		[self setShuffle: ShuffleAll];
	}
	else if (shuffle == ShuffleAll) {
		[self setShuffle: ShuffleOff];
	}
}

- (IBAction)toggleRepeat:(id)sender
{
	RepeatMode repeat = [self repeat];
	
	if (repeat == RepeatNone) {
		[self setRepeat: RepeatOne];
	}
	else if (repeat == RepeatOne) {
		[self setRepeat: RepeatAlbum];
	}
	else if (repeat == RepeatAlbum) {
		[self setRepeat: RepeatAll];
	}
	else if (repeat == RepeatAll) {
		[self setRepeat: RepeatNone];
	}
}

- (PlaylistEntry *)entryAtIndex:(int)i
{
	RepeatMode repeat = [self repeat];
	
	if (i < 0 || i >= [[self arrangedObjects] count] ) {
		if ( repeat != RepeatAll )
			return nil;
		
		while ( i < 0 )
			i += [[self arrangedObjects] count];
		if ( i >= [[self arrangedObjects] count])
			i %= [[self arrangedObjects] count];
	}
	
	return [[self arrangedObjects] objectAtIndex:i];
}

- (void)remove:(id)sender {
    // It's a kind of magic.
    // Plain old NSArrayController's remove: isn't working properly for some reason.
    // The method is definitely called but (overridden) removeObjectsAtArrangedObjectIndexes: isn't called
    // and no entries are removed.
    // Putting explicit call to removeObjectsAtArrangedObjectIndexes: here for now.
    // TODO: figure it out

    NSIndexSet *selected = [self selectionIndexes];
    if ([selected count] > 0)
    {
        [self removeObjectsAtArrangedObjectIndexes:selected];
    }
}

- (IBAction)removeDuplicates:(id)sender {
    NSMutableArray *originals = [[NSMutableArray alloc] init];
    NSMutableArray *duplicates = [[NSMutableArray alloc] init];
    
    for (PlaylistEntry *pe in [self content])
    {
        if ([originals containsObject:[pe URL]])
            [duplicates addObject:pe];
        else
            [originals addObject:[pe URL]];
    }
    
    if ([duplicates count] > 0)
    {
        NSArray * arrangedContent = [self arrangedObjects];
        NSMutableIndexSet * duplicatesIndex = [[NSMutableIndexSet alloc] init];
        for (PlaylistEntry *pe in duplicates)
        {
            [duplicatesIndex addIndex:[arrangedContent indexOfObject:pe]];
        }
        [self removeObjectsAtArrangedObjectIndexes:duplicatesIndex];
    }
}

- (IBAction)removeDeadItems:(id)sender {
    NSMutableArray *deadItems = [[NSMutableArray alloc] init];
    
    for (PlaylistEntry *pe in [self content])
    {
        NSURL *url = [pe URL];
        if ([url isFileURL])
            if (![[NSFileManager defaultManager] fileExistsAtPath:[url path]])
                [deadItems addObject:pe];
    }
    
    if ([deadItems count] > 0)
    {
        NSArray * arrangedContent = [self arrangedObjects];
        NSMutableIndexSet * deadItemsIndex = [[NSMutableIndexSet alloc] init];
        for (PlaylistEntry *pe in deadItems)
        {
            [deadItemsIndex addIndex:[arrangedContent indexOfObject:pe]];
        }
        [self removeObjectsAtArrangedObjectIndexes:deadItemsIndex];
    }
}

- (PlaylistEntry *)shuffledEntryAtIndex:(int)i
{
	RepeatMode repeat = [self repeat];
	
	while (i < 0)
	{
		if (repeat == RepeatAll)
		{
			[self addShuffledListToFront];
			//change i appropriately
			i += [[self arrangedObjects] count];
		}
		else
		{
			return nil;
		}
	}
	while (i >= [shuffleList count])
	{
		if (repeat == RepeatAll)
		{
			[self addShuffledListToBack];
		}
		else
		{
			return nil;
		}
	}
	
	return [shuffleList objectAtIndex:i];
}

- (PlaylistEntry *)getNextEntry:(PlaylistEntry *)pe
{
    return [self getNextEntry:pe ignoreRepeatOne:NO];
}

- (PlaylistEntry *)getNextEntry:(PlaylistEntry *)pe ignoreRepeatOne:(BOOL)ignoreRepeatOne
{
    if (!ignoreRepeatOne && [self repeat] == RepeatOne)
    {
        return pe;
    }
    
	if ([queueList count] > 0)
	{
		
		pe = [queueList objectAtIndex:0];
		[queueList removeObjectAtIndex:0];
		pe.queued = NO;
		[pe setQueuePosition:-1];
		
		int i;
		for (i = 0; i < [queueList count]; i++)
		{
			PlaylistEntry *queueItem = [queueList objectAtIndex:i];
			[queueItem setQueuePosition: i];
		}
		
		return pe;
	}
	
	if ([self shuffle] != ShuffleOff)
	{
		return [self shuffledEntryAtIndex:(pe.shuffleIndex + 1)];
	}
	else
	{
		int i;
		if (pe.index < 0) //Was a current entry, now removed.
		{
			i = -pe.index - 1;
		}
		else
		{
			i = pe.index + 1;
		}
		
		if ([self repeat] == RepeatAlbum)
		{
			PlaylistEntry *next = [self entryAtIndex:i];
			
			if ((i > [[self arrangedObjects] count]-1) || ([[next album] caseInsensitiveCompare:[pe album]]) || ([next album] == nil))
			{
				NSArray *filtered = [self filterPlaylistOnAlbum:[pe album]];
				if ([pe album] == nil)
					i--;
				else
					i = [(PlaylistEntry *)[filtered objectAtIndex:0] index];
			}
			
		}

		return [self entryAtIndex:i];
	}
}

- (NSArray *)filterPlaylistOnAlbum:(NSString *)album
{
	NSPredicate *predicate;
	if ([album length] > 0)
		predicate = [NSPredicate predicateWithFormat:@"album like %@",
							  album];
	else
		predicate = [NSPredicate predicateWithFormat:@"album == nil"];
	return [[self arrangedObjects] filteredArrayUsingPredicate:predicate];
}

- (PlaylistEntry *)getPrevEntry:(PlaylistEntry *)pe
{
    return [self getPrevEntry:pe ignoreRepeatOne:NO];
}

- (PlaylistEntry *)getPrevEntry:(PlaylistEntry *)pe ignoreRepeatOne:(BOOL)ignoreRepeatOne
{
	if (!ignoreRepeatOne && [self repeat] == RepeatOne)
	{
		return pe;
	}
    
	if ([self shuffle] != ShuffleOff)
	{
		return [self shuffledEntryAtIndex:(pe.shuffleIndex - 1)];
	}
	else
	{
		int i;
		if (pe.index < 0) //Was a current entry, now removed.
		{
			i = -pe.index - 2;
		}
		else
		{
			i = pe.index - 1;
		}
		
		return [self entryAtIndex:i];
	}
}

- (BOOL)next
{
	PlaylistEntry *pe;
	
	pe = [self getNextEntry:[self currentEntry] ignoreRepeatOne:YES];
	
	if (pe == nil)
		return NO;
	
	[self setCurrentEntry:pe];
	
	return YES;
}

- (BOOL)prev
{
	PlaylistEntry *pe;
	
    pe = [self getPrevEntry:[self currentEntry] ignoreRepeatOne:YES];
	if (pe == nil)
		return NO;
	
	[self setCurrentEntry:pe];
	
	return YES;
}

- (NSArray *)shuffleAlbums
{
	NSArray * newList = [self arrangedObjects];
	NSMutableArray * temp = [[NSMutableArray alloc] init];
	NSMutableArray * albums = [[NSMutableArray alloc] init];
	NSSortDescriptor * sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"track" ascending:YES];
	for (unsigned long i = 0, j = [newList count]; i < j; ++i) {
		PlaylistEntry * pe = [newList objectAtIndex:i];
		NSString * album = [pe album];
		if (!album)
			album = @"";
		if ([albums containsObject:album]) continue;
		[albums addObject:album];
		NSArray * albumContent = [self filterPlaylistOnAlbum:album];
		NSArray * sortedContent = [albumContent sortedArrayUsingDescriptors:[NSArray arrayWithObject:sortDescriptor]];
		[temp addObject:[sortedContent objectAtIndex:0]];
	}
	NSArray * tempList = [Shuffle shuffleList:temp];
	temp = [[NSMutableArray alloc] init];
	for (unsigned long i = 0, j = [tempList count]; i < j; ++i) {
		PlaylistEntry * pe = [tempList objectAtIndex:i];
		NSString * album = [pe album];
		NSArray * albumContent = [self filterPlaylistOnAlbum:album];
		NSArray * sortedContent = [albumContent sortedArrayUsingDescriptors:[NSArray arrayWithObject:sortDescriptor]];
		[temp addObjectsFromArray:sortedContent];
	}
	return temp;
}

- (void)addShuffledListToFront
{
	NSArray *newList;
	NSIndexSet *indexSet;
	
	if ([self shuffle] == ShuffleAlbums) {
		newList = [self shuffleAlbums];
	}
	else {
		newList = [Shuffle shuffleList:[self arrangedObjects]];
	}
	
	indexSet = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [newList count])];
	
	[shuffleList insertObjects:newList atIndexes:indexSet];
	
	int i;
	for (i = 0; i < [shuffleList count]; i++)
	{
		[[shuffleList objectAtIndex:i] setShuffleIndex:i];
	}
}

- (void)addShuffledListToBack
{
	NSArray *newList;
	NSIndexSet *indexSet;
	
	if ([self shuffle] == ShuffleAlbums) {
		newList = [self shuffleAlbums];
	}
	else {
		newList = [Shuffle shuffleList:[self arrangedObjects]];
	}
	
	indexSet = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange([shuffleList count], [newList count])];
	
	[shuffleList insertObjects:newList atIndexes:indexSet];
	
	unsigned long i;
	for (i = ([shuffleList count] - [newList count]); i < [shuffleList count]; i++)
	{
		[[shuffleList objectAtIndex:i] setShuffleIndex:(int)i];
	}
}

- (void)resetShuffleList
{
	[shuffleList removeAllObjects];

	[self addShuffledListToFront];

	if (currentEntry && currentEntry.index >= 0)
	{
        if ([self shuffle] == ShuffleAlbums) {
            NSString * currentAlbum = currentEntry.album;
            if (!currentAlbum)
                currentAlbum = @"";
            
            NSArray * wholeAlbum = [self filterPlaylistOnAlbum:currentAlbum];
            
            // First prune the shuffle list of the currently playing album
            long i, j;
            for (i = 0; i < [shuffleList count];) {
                if ([wholeAlbum containsObject:[shuffleList objectAtIndex:i]]) {
                    [shuffleList removeObjectAtIndex:i];
                }
                else {
                    ++i;
                }
            }
            
            // Then insert the playing album at the start
            NSIndexSet * indexSet = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [wholeAlbum count])];
            
            [shuffleList insertObjects:wholeAlbum atIndexes:indexSet];

            // Oops, gotta reset the shuffle indexes
            for (i = 0, j = [shuffleList count]; i < j; ++i) {
                [[shuffleList objectAtIndex:i] setShuffleIndex:(int)i];
            }
        }
        else {
            [shuffleList insertObject:currentEntry atIndex:0];
            [currentEntry setShuffleIndex:0];

            //Need to rejigger so the current entry is at the start now...
            long i, j;
            BOOL found = NO;
            for (i = 1, j = [shuffleList count]; i < j && !found; i++)
            {
                if ([shuffleList objectAtIndex:i] == currentEntry)
                {
                    found = YES;
                    [shuffleList removeObjectAtIndex:i];
                }
                else {
                    [[shuffleList objectAtIndex:i] setShuffleIndex: (int)i];
                }
            }
        }
	}
}

- (void)setCurrentEntry:(PlaylistEntry *)pe
{
	currentEntry.current = NO;
	currentEntry.stopAfter = NO;
	
	pe.current = YES;
	
	if (pe != nil)
		[self.tableView scrollRowToVisible:pe.index];
	
	currentEntry = pe;
}

- (void)setShuffle:(ShuffleMode)s
{
	[[NSUserDefaults standardUserDefaults] setInteger:s forKey:@"shuffle"];
	if (s != ShuffleOff)
		[self resetShuffleList];
	
	[playbackController playlistDidChange:self];
}
- (ShuffleMode)shuffle
{
	return (ShuffleMode) [[NSUserDefaults standardUserDefaults] integerForKey:@"shuffle"];
}
- (void)setRepeat:(RepeatMode)r
{
	[[NSUserDefaults standardUserDefaults] setInteger:r forKey:@"repeat"];
	[playbackController playlistDidChange:self];
}
- (RepeatMode)repeat
{
	return (RepeatMode) [[NSUserDefaults standardUserDefaults] integerForKey:@"repeat"];
}

- (IBAction)clear:(id)sender
{
	[self setFilterPredicate:nil];
	
	[self removeObjectsAtArrangedObjectIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [[self arrangedObjects] count])]];
}

- (IBAction)clearFilterPredicate:(id)sender
{
	[self setFilterPredicate:nil];
}

- (void)setFilterPredicate:(NSPredicate *)filterPredicate
{
	[super setFilterPredicate:filterPredicate];
}

- (IBAction)showEntryInFinder:(id)sender
{
	NSWorkspace* ws = [NSWorkspace sharedWorkspace];
	
	NSURL *url = [[[self selectedObjects] objectAtIndex:0] URL];
	if ([url isFileURL])
		[ws selectFile:[url path] inFileViewerRootedAtPath:[url path]];
}
/*
- (IBAction)showTagEditor:(id)sender
{
// call the editor & pass the url
	if ([self selectionIndex] < 0)
		return;
	
	NSURL *url = [[[self selectedObjects] objectAtIndex:0] URL];
	if ([url isFileURL])
		[TagEditorController openTagEditor:url sender:sender];
	
}
*/
- (IBAction)searchByArtist:(id)sender;
{
    PlaylistEntry *entry = [[self arrangedObjects] objectAtIndex:[self selectionIndex]];
    [spotlightWindowController searchForArtist:[entry artist]];
}
- (IBAction)searchByAlbum:(id)sender;
{
    PlaylistEntry *entry = [[self arrangedObjects] objectAtIndex:[self selectionIndex]];
    [spotlightWindowController searchForAlbum:[entry album]];
}

- (NSMutableArray *)queueList
{
	return queueList;
}

- (IBAction)emptyQueueList:(id)sender
{
	for (PlaylistEntry *queueItem in queueList)
	{
		queueItem.queued = NO;
		[queueItem setQueuePosition:-1];
	}

	[queueList removeAllObjects];
}


- (IBAction)toggleQueued:(id)sender
{
	for (PlaylistEntry *queueItem in [self selectedObjects])
	{
		if (queueItem.queued)
		{
			queueItem.queued = NO;
			queueItem.queuePosition = -1;

			[queueList removeObject:queueItem];
		}
		else
		{
			queueItem.queued = YES;
			queueItem.queuePosition = (int) [queueList count];
			
			[queueList addObject:queueItem];
		}
		
		DLog(@"TOGGLE QUEUED: %i", queueItem.queued);
	}

	int i = 0;
	for (PlaylistEntry *cur in queueList)
	{
		cur.queuePosition = i++;
	}
}

- (IBAction)removeFromQueue:(id)sender
{
    for (PlaylistEntry *queueItem in [self selectedObjects])
    {
        queueItem.queued = NO;
        queueItem.queuePosition = -1;
        
        [queueList removeObject:queueItem];
    }
    
    int i = 0;
    for (PlaylistEntry *cur in queueList)
    {
        cur.queuePosition = i++;
    }
}

- (IBAction)addToQueue:(id)sender
{
    for (PlaylistEntry *queueItem in [self selectedObjects])
    {
        queueItem.queued = YES;
        queueItem.queuePosition = (int) [queueList count];
        
        [queueList addObject:queueItem];
    }
    
    int i = 0;
    for (PlaylistEntry *cur in queueList)
    {
        cur.queuePosition = i++;
    }
}

- (IBAction)stopAfterCurrent:(id)sender
{
	currentEntry.stopAfter = !currentEntry.stopAfter;
}

-(BOOL)validateMenuItem:(NSMenuItem*)menuItem
{
	SEL action = [menuItem action];
	
	if (action == @selector(removeFromQueue:))
	{
		for (PlaylistEntry *q in [self selectedObjects])
			if (q.queuePosition >= 0)
				return YES;

		return NO;
	}

	if (action == @selector(emptyQueueList:) && ([queueList count] < 1))
		return NO;
	
	if (action == @selector(stopAfterCurrent:) && currentEntry.stopAfter)
		return NO;
	
	// if nothing is selected, gray out these
	if ([[self selectedObjects] count] < 1)
	{
		
		if (action == @selector(remove:))
			return NO;
	
		if (action == @selector(addToQueue:))
			return NO;

		if (action == @selector(searchByArtist:))
			return NO;

		if (action == @selector(searchByAlbum:))
			return NO;
	}
	
	return YES;
}

// Event inlets:
- (void)willInsertURLs:(NSArray*)urls origin:(URLOrigin)origin
{
	if (![urls count])
		return;

	CGEventRef event = CGEventCreate(NULL /*default event source*/);
	CGEventFlags mods = CGEventGetFlags(event);
	CFRelease(event);
	
	BOOL modifierPressed =  ((mods & kCGEventFlagMaskCommand)!=0)&((mods & kCGEventFlagMaskControl)!=0);
	modifierPressed |= ((mods & kCGEventFlagMaskShift)!=0);

	NSString *behavior = [[NSUserDefaults standardUserDefaults] valueForKey:@"openingFilesBehavior"];
	if (modifierPressed) {
		behavior =  [[NSUserDefaults standardUserDefaults] valueForKey:@"openingFilesAlteredBehavior"];
	}
	
	
	BOOL shouldClear = modifierPressed; // By default, internal sources should not clear the playlist
	if (origin == URLOriginExternal) { // For external insertions, we look at the preference
		//possible settings are "clearAndPlay", "enqueue", "enqueueAndPlay"
		shouldClear = [behavior isEqualToString:@"clearAndPlay"];
	}
	
	if (shouldClear) {
		[self clear:self];
	}
}

- (void)didInsertURLs:(NSArray*)urls origin:(URLOrigin)origin
{
	if (![urls count])
		return;
	
	CGEventRef event = CGEventCreate(NULL);
	CGEventFlags mods = CGEventGetFlags(event);
	CFRelease(event);
	
	BOOL modifierPressed =  ((mods & kCGEventFlagMaskCommand)!=0)&((mods & kCGEventFlagMaskControl)!=0);
	modifierPressed |= ((mods & kCGEventFlagMaskShift)!=0);
	
	NSString *behavior = [[NSUserDefaults standardUserDefaults] valueForKey:@"openingFilesBehavior"];
	if (modifierPressed) {
		behavior =  [[NSUserDefaults standardUserDefaults] valueForKey:@"openingFilesAlteredBehavior"];
	}
		
	BOOL shouldPlay = modifierPressed; // The default is NO for internal insertions
	if (origin == URLOriginExternal) { // For external insertions, we look at the preference
		shouldPlay = [behavior isEqualToString:@"clearAndPlay"] || [behavior isEqualToString:@"enqueueAndPlay"];;
	}

	//Auto start playback
	if (shouldPlay	&& [[self content] count] > 0) {
		[playbackController playEntry: [urls objectAtIndex:0]];
	}
}


@end
