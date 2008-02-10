//
//	PlaylistController.m
//	Cog
//
//	Created by Vincent Spader on 3/18/05.
//	Copyright 2005 Vincent Spader All rights reserved.
//

#import "PlaylistLoader.h"
#import "PlaylistController.h"
#import "PlaylistEntry.h"
#import "Shuffle.h"
#import "UndoObject.h"

#import "CogAudio/AudioPlayer.h"

@implementation PlaylistController

#define SHUFFLE_HISTORY_SIZE 100
#define UNDO_STACK_LIMIT 25

- (id)initWithCoder:(NSCoder *)decoder
{
	self = [super initWithCoder:decoder];
	
	if (self)
	{
		shuffleList = [[NSMutableArray alloc] init];
		undoManager = [[NSUndoManager alloc] init];

		[undoManager setLevelsOfUndo:UNDO_STACK_LIMIT];
	}
	
	return self;
}

- (void)tableView:(NSTableView *)tableView
		didClickTableColumn:(NSTableColumn *)tableColumn
{
	if (shuffle == YES)
		[self resetShuffleList];

	[self updateIndexesFromRow:0];
}

- (BOOL)tableView:(NSTableView*)tv
	   acceptDrop:(id <NSDraggingInfo>)info
			  row:(int)row
	dropOperation:(NSTableViewDropOperation)op
{
	[super tableView:tv acceptDrop:info row:row dropOperation:op];
	
	UndoObject *undoEntry;
	NSMutableArray *undoEntries = [[NSMutableArray alloc] init];
	
	if ([info draggingSource] == tableView)
	{
		//DNDArrayController handles moving...still need to update the indexes

		NSArray			*rows = [NSKeyedUnarchiver unarchiveObjectWithData:[[info draggingPasteboard] dataForType: MovedRowsType]];
		NSIndexSet		*indexes = [self indexSetFromRows:rows];
		int				firstIndex = [indexes firstIndex];
		int				indexesSize = [indexes count];
		NSUInteger		indexBuffer[indexesSize];

		int				i;
		int				adjustment;
		int				counter;
		
		if (firstIndex > row)
			i = row;
		else
			i = firstIndex;

		[indexes getIndexes:indexBuffer maxCount:indexesSize inIndexRange:nil];

		if (row > firstIndex)
			adjustment = 1;
		else
			adjustment = 0;
		
		// create an UndoObject for each entry being moved, and store it away
		// in the undoEntries array
		for (counter = 0; counter < indexesSize; counter++)
		{
			undoEntry = [[UndoObject alloc] init];

			[undoEntry setOrigin: row - adjustment];
			[undoEntry setMovedTo: indexBuffer[counter]];
			[undoEntries addObject: undoEntry];
		}
				
		[[self undoManager] registerUndoWithTarget:self
				 selector:@selector(undoMove:)
				 object:undoEntries];
				 

	
		[self updateIndexesFromRow:i];

		return YES;
	}

	if (row < 0)
		row = 0;
		
	// Determine the type of object that was dropped
	NSArray *supportedtypes = [NSArray arrayWithObjects:CogUrlsPbboardType, NSFilenamesPboardType, iTunesDropType, nil];
	NSPasteboard *pboard = [info draggingPasteboard];
	NSString *bestType = [pboard availableTypeFromArray:supportedtypes];
	
	// Get files from an file drawer drop
	if ([bestType isEqualToString:CogUrlsPbboardType]) {
		NSArray *urls = [NSUnarchiver unarchiveObjectWithData:[[info draggingPasteboard] dataForType:CogUrlsPbboardType]];
		NSLog(@"URLS: %@", urls);
		[playlistLoader insertURLs: urls atIndex:row sort:YES];
	}
	
	// Get files from a normal file drop (such as from Finder)
	if ([bestType isEqualToString:NSFilenamesPboardType]) {
		NSMutableArray *urls = [[NSMutableArray alloc] init];

		NSEnumerator *e = [[[info draggingPasteboard] propertyListForType:NSFilenamesPboardType] objectEnumerator];
		NSString *file; 
		while (file = [e nextObject])
		{
			[urls addObject:[NSURL fileURLWithPath:file]];
		}
		
		[playlistLoader insertURLs:urls atIndex:row sort:YES];

		[urls release];
	}
	
	// Get files from an iTunes drop
	if ([bestType isEqualToString:iTunesDropType]) {
		NSDictionary *iTunesDict = [pboard propertyListForType:iTunesDropType];
		NSDictionary *tracks = [iTunesDict valueForKey:@"Tracks"];

		// Convert the iTunes URLs to URLs....MWAHAHAH!
		NSMutableArray *urls = [[NSMutableArray alloc] init];

		NSEnumerator *e = [[tracks allValues] objectEnumerator];
		NSDictionary *trackInfo;
		while (trackInfo = [e nextObject]) {
			[urls addObject:[NSURL URLWithString:[trackInfo valueForKey:@"Location"]]];
		}
		
		[playlistLoader insertURLs:urls atIndex:row sort:YES];
		[urls release];
	}
	
	[self updateIndexesFromRow:row];
	[self updateTotalTime];
	
	if (shuffle == YES)
		[self resetShuffleList];
	
	return YES;
}

- (void)updateTotalTime
{
	double tt=0;
	
	NSEnumerator *enumerator = [[self arrangedObjects] objectEnumerator];
	PlaylistEntry* pe;
	
	while (pe = [enumerator nextObject]) {
		tt += [[pe length] doubleValue];
	}
	
	int sec = (int)(tt);
	[self setTotalTimeDisplay:[NSString stringWithFormat:@"%i:%02i",sec/60, sec%60]];
}

- (void)setTotalTimeDisplay:(NSString *)ttd
{
	[ttd retain];
	[totalTimeDisplay release];
	totalTimeDisplay = ttd;
}

- (NSString *)totalTimeDisplay;
{
	return totalTimeDisplay;
}

- (void)updateIndexesFromRow:(int) row
{
	int j;
	for (j = row; j < [[self arrangedObjects] count]; j++)
	{
		PlaylistEntry *p;
		p = [[self arrangedObjects] objectAtIndex:j];
		
		[p setIndex:[NSNumber numberWithInt:j]];
	}
}

-(void)undoDelete:(NSMutableArray *)undoEntries
{
	NSEnumerator   *enumerator = [undoEntries objectEnumerator];
	UndoObject	   *current;

	while (current = [enumerator nextObject])
	{
		[playlistLoader 
			insertURLs: [NSArray arrayWithObject:[current path]]
			atIndex:[current origin] 
			sort:YES];
		// make sure to dealloc the undo object after reinserting it
		[current dealloc];
					
	}
	[self updateIndexesFromRow: 0];
}

-(void)undoMove:(NSMutableArray *) undoEntries
{
	NSArray			*objects = [super arrangedObjects];
	NSEnumerator	*enumerator = [undoEntries objectEnumerator];
	UndoObject		*current;
	id				object;
	int				len = [undoEntries count];
	int				iterations = 0;
	int				playlistLocation;

	// register an undo for the undo with the undoManager, 
	// so it knows what to do if a redo is requested
	[[self undoManager] registerUndoWithTarget:self
				 selector:@selector(undoMove:)
				 object:undoEntries];
	
	while (current = [enumerator nextObject])
	{
		/* the exact opposite of an undo is required during a redo, hence
		we have to check what we are dealing with and act accordingly */
		
		// originally moved entry up the list
		if (([current origin] > [current movedTo])) 
		{
			if ([[self undoManager] isUndoing]) // we are undoing
			{
				playlistLocation = ([current origin] - (len - 1)) + iterations++;
				object = [objects objectAtIndex: playlistLocation];
				[object retain];
				[super insertObject:object atArrangedObjectIndex:[current movedTo]];
				[super removeObjectAtArrangedObjectIndex:playlistLocation + 1];
			}
			else // we are redoing the undo
			{
				playlistLocation = [current movedTo] -	iterations++;
				object = [objects objectAtIndex: playlistLocation];
				[object retain];
				[super removeObjectAtArrangedObjectIndex:playlistLocation];
				[super insertObject:object atArrangedObjectIndex:[current origin]];
			}

		}
		// originally moved entry down the list
		else
		{
			if ([[self undoManager] isUndoing])
			{
				object = [objects objectAtIndex: [current origin]];
				[object retain];
				[super insertObject:object atArrangedObjectIndex:[current movedTo] + len--];
				[super removeObjectAtArrangedObjectIndex:[current origin]];
			}
			else
			{
				object = [objects objectAtIndex: [current movedTo]];
				[object retain];
				[super removeObjectAtArrangedObjectIndex:[current movedTo]];
				[super insertObject:object atArrangedObjectIndex:[current origin] + iterations++];
			}

		}
		[object release];
		
	}
	
	[self updateIndexesFromRow: 0];

}

- (NSUndoManager *)undoManager
{
	return undoManager;
}

- (void)removeObjectsAtArrangedObjectIndexes:(NSIndexSet *)indexes
{
	int				 i; // loop counter
	int				 indexesSize = [indexes count];
	NSUInteger		 indexBuffer[indexesSize];
	UndoObject		 *undoEntry;
	PlaylistEntry	 *pre;
	NSMutableArray	 *undoEntries = [[NSMutableArray alloc] init];

	NSLog(@"Removing indexes: %@", indexes);
	NSLog(@"Current index: %i", [[currentEntry index] intValue]);

	// get indexes from IndexSet indexes and put them in indexBuffer
	[indexes getIndexes:indexBuffer maxCount:indexesSize inIndexRange:nil];
	
	// loop through the list of indexes, saving each entry away
	// in an undo object
	for (i = 0; i < indexesSize; i++)
	{
		// alllocate a new object for each undo entry
		undoEntry = [[UndoObject alloc] init];
	
		pre = [self entryAtIndex:indexBuffer[i]];
	
		[undoEntry setOrigin:indexBuffer[i] andPath: [pre url]];
		[undoEntries addObject:undoEntry];
	}
	
	// register the removals with the undoManager
	[[self undoManager] registerUndoWithTarget:self
				 selector:@selector(undoDelete:)
				 object:undoEntries];
				 
	
	if ([[currentEntry index] intValue] >= 0 && [indexes containsIndex:[[currentEntry index] intValue]])
	{
		[currentEntry setIndex:[NSNumber numberWithInt:-[[currentEntry index] intValue] - 1]];
		NSLog(@"Current removed: %i", [[currentEntry index] intValue]);
	}
	
	if ([[currentEntry index] intValue] < 0) //Need to update the negative index
	{
		int i = -[[currentEntry index] intValue] - 1;
		NSLog(@"I is %i", i);
		int j;
		for (j = i - 1; j >= 0; j--)
		{
			if ([indexes containsIndex:j]) {
				NSLog(@"Removing 1");
				i--;
			}
		}
		[currentEntry setIndex: [NSNumber numberWithInt:-i - 1]];

		NSLog(@"UPDATING INDEX: %@", [currentEntry index]);
	}
	
	[super removeObjectsAtArrangedObjectIndexes:indexes];
	[self updateIndexesFromRow:[indexes firstIndex]];
	[self updateTotalTime];

	if (shuffle == YES)
		[self resetShuffleList];
}

- (void)setSortDescriptors:(NSArray *)sortDescriptors
{
	//Cheap hack so the index column isn't sorted
	if (([sortDescriptors count] != 0) && [[[sortDescriptors objectAtIndex:0] key] caseInsensitiveCompare:@"index"] == NSOrderedSame)
	{
		//Remove the sort descriptors
		[super setSortDescriptors:nil];
		[self rearrangeObjects];
		
		return;
	}

	[super setSortDescriptors:sortDescriptors];
}
		
- (IBAction)sortByPath
{
	NSSortDescriptor *s = [[NSSortDescriptor alloc] initWithKey:@"url" ascending:YES selector:@selector(compare:)];
	
	[self setSortDescriptors:[NSArray arrayWithObject:s]];
	[self rearrangeObjects];

	[s release];	

	if (shuffle == YES)
		[self resetShuffleList];

	[self updateIndexesFromRow:0];
}

- (IBAction)randomizeList
{
	[self setSortDescriptors:nil];

	[self setContent:[Shuffle shuffleList:[self content]]];
	if (shuffle == YES)
		[self resetShuffleList];

	[self updateIndexesFromRow:0];
}

- (IBAction)takeShuffleFromObject:(id)sender
{
	if( [sender respondsToSelector: @selector(boolValue)] )
		[self setShuffle: [sender boolValue]];
	else
		[self setShuffle: [sender state]];
}
- (IBAction)takeRepeatFromObject:(id)sender
{
	if( [sender respondsToSelector: @selector(boolValue)] )
		[self setRepeat: [sender boolValue]];
	else
		[self setRepeat: [sender state]];
}

- (PlaylistEntry *)entryAtIndex:(int)i
{
	if (i < 0)
	{
		if (repeat == YES)
			i += [[self arrangedObjects] count];
		else
			return nil;
	}
	else if (i >= [[self arrangedObjects] count])
	{
		if (repeat == YES)
			i -= [[self arrangedObjects] count];
		else
			return nil;
	}
	
	return [[self arrangedObjects] objectAtIndex:i];
}

- (PlaylistEntry *)shuffledEntryAtIndex:(int)i
{
	while (i < 0)
	{
		if (repeat == YES)
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
		if (repeat == YES)
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
	if (shuffle == YES)
	{
		return [self shuffledEntryAtIndex:([[pe shuffleIndex] intValue] + 1)];
	}
	else
	{
		int i;
		if ([[pe index] intValue] < 0) //Was a current entry, now removed.
		{
			i = -[[pe index] intValue] - 1;
		}
		else
		{
			i = [[pe index] intValue] + 1;
		}
		
		return [self entryAtIndex:i];
	}
}

- (PlaylistEntry *)getPrevEntry:(PlaylistEntry *)pe
{
	if (shuffle == YES)
	{
		return [self shuffledEntryAtIndex:([[pe shuffleIndex] intValue] - 1)];
	}
	else
	{
		int i;
		if ([[pe index] intValue] < 0) //Was a current entry, now removed.
		{
			i = -[[pe index] intValue] - 2;
		}
		else
		{
			i = [[pe index] intValue] - 1;
		}
		
		return [self entryAtIndex:i];
	}
}

- (BOOL)next
{
	PlaylistEntry *pe;
	
	pe = [self getNextEntry:[self currentEntry]];
	if (pe == nil)
		return NO;
	
	[self setCurrentEntry:pe];
	
	return YES;
}

- (BOOL)prev
{
	PlaylistEntry *pe;
	
	pe = [self getPrevEntry:[self currentEntry]];
	if (pe == nil)
		return NO;
	
	[self setCurrentEntry:pe];
	
	return YES;
}

//Need to do...when first generated, need to have current entry at the beginning of the list.

- (void)addShuffledListToFront
{
	NSArray *newList = [Shuffle shuffleList:[self arrangedObjects]];
	NSIndexSet *indexSet = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [newList count])];
	
	[shuffleList insertObjects:newList atIndexes:indexSet];
	
	int i;
	for (i = 0; i < [shuffleList count]; i++)
	{
		[[shuffleList objectAtIndex:i] setShuffleIndex:[NSNumber numberWithInt:i]];
	}
}

- (void)addShuffledListToBack
{
	NSArray *newList = [Shuffle shuffleList:[self arrangedObjects]];
	NSIndexSet *indexSet = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange([shuffleList count], [newList count])];

	[shuffleList insertObjects:newList atIndexes:indexSet];

	int i;
	for (i = ([shuffleList count] - [newList count]); i < [shuffleList count]; i++)
	{
		[[shuffleList objectAtIndex:i] setShuffleIndex:[NSNumber numberWithInt:i]];
	}
}

- (void)resetShuffleList
{
	[shuffleList removeAllObjects];

	[self addShuffledListToFront];

	if (currentEntry && [[currentEntry index] intValue] != -1)
	{
		[shuffleList insertObject:currentEntry atIndex:0];
		[currentEntry setShuffleIndex:0];
		
		//Need to rejigger so the current entry is at the start now...
		int i;
		BOOL found = NO;
		for (i = 1; i < [shuffleList count]; i++)
		{
			if (found == NO && [[shuffleList objectAtIndex:i] url] == [currentEntry url])
			{
				found = YES;
				[shuffleList removeObjectAtIndex:i];
			}

			[[shuffleList objectAtIndex:i] setShuffleIndex:[NSNumber numberWithInt:i]];
		}
	}
}

- (id)currentEntry
{
	return currentEntry;
}

- (void)setCurrentEntry:(PlaylistEntry *)pe
{
	[currentEntry setCurrent:[NSNumber numberWithBool:NO]];
	
	[pe setCurrent:[NSNumber numberWithBool:YES]];
	[tableView scrollRowToVisible:[[pe index] intValue]];
	
	[pe retain];
	[currentEntry release];
	
	currentEntry = pe;
}	

- (void)setShuffle:(BOOL)s
{
	shuffle = s;
	if (shuffle == YES)
		[self resetShuffleList];
}
- (BOOL)shuffle
{
	return shuffle;
}
- (void)setRepeat:(BOOL)r
{
	repeat = r;
}
- (BOOL)repeat
{
	return repeat;
}

- (IBAction)clear:(id)sender
{
	[currentEntry setIndex:[NSNumber numberWithInt:-1]];
	
	[self removeObjects:[self content]];
	[self updateTotalTime];

	if (shuffle == YES)
		[self resetShuffleList];
}

- (IBAction)clearFilterPredicate:(id)sender
{
	[self setFilterPredicate:nil];
}

- (void)setFilterPredicate:(NSPredicate *)filterPredicate
{
	[super setFilterPredicate:filterPredicate];

	[self updateIndexesFromRow:0];
}

- (IBAction)showEntryInFinder:(id)sender
{
	NSWorkspace* ws = [NSWorkspace sharedWorkspace];
	if ([self selectionIndex] < 0)
		return;
	
	NSURL *url = [[self entryAtIndex:[self selectionIndex]] url];
	if ([url isFileURL])
		[ws selectFile:[url path] inFileViewerRootedAtPath:[url path]];
}

@end
