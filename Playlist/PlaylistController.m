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
#import "SpotlightWindowController.h"
#import "RepeatTransformers.h"
#import "CogAudio/AudioPlayer.h"

@implementation PlaylistController

#define SHUFFLE_HISTORY_SIZE 100

+ (void)initialize {
	NSValueTransformer *repeatNoneTransformer = [[[RepeatModeTransformer alloc] initWithMode:RepeatNone] autorelease];
    [NSValueTransformer setValueTransformer:repeatNoneTransformer
                                    forName:@"RepeatNoneTransformer"];

	NSValueTransformer *repeatOneTransformer = [[[RepeatModeTransformer alloc] initWithMode:RepeatOne] autorelease];
    [NSValueTransformer setValueTransformer:repeatOneTransformer
                                    forName:@"RepeatOneTransformer"];

	NSValueTransformer *repeatAlbumTransformer = [[[RepeatModeTransformer alloc] initWithMode:RepeatAlbum] autorelease];
    [NSValueTransformer setValueTransformer:repeatAlbumTransformer
                                    forName:@"RepeatAlbumTransformer"];

	NSValueTransformer *repeatAllTransformer = [[[RepeatModeTransformer alloc] initWithMode:RepeatAll] autorelease];
    [NSValueTransformer setValueTransformer:repeatAllTransformer
                                    forName:@"RepeatAllTransformer"];

	NSValueTransformer *repeatModeImageTransformer = [[[RepeatModeImageTransformer alloc] init]autorelease];
    [NSValueTransformer setValueTransformer:repeatModeImageTransformer
                                    forName:@"RepeatModeImageTransformer"];
}

- (id)initWithCoder:(NSCoder *)decoder
{
	self = [super initWithCoder:decoder];
	
	if (self)
	{
		shuffleList = [[NSMutableArray alloc] init];
	}
	
	return self;
}

- (void)dealloc
{
	[shuffleList release];
	
	[super dealloc];
}

- (void)tableView:(NSTableView *)tableView
		didClickTableColumn:(NSTableColumn *)tableColumn
{
	if (shuffle == YES)
		[self resetShuffleList];

	[self updateIndexesFromRow:0];
}

-(void)moveObjectsFromArrangedObjectIndexes:(NSArray *) sources toIndexes:(NSArray *)destinations
{
	[super moveObjectsFromArrangedObjectIndexes:sources toIndexes:destinations];

	NSUInteger firstIndex = (NSUInteger)-1;
	NSUInteger i = 0;
	
	for (i = 0; i < [sources count]; i++) {
		NSUInteger source = [[sources objectAtIndex:i] unsignedIntegerValue];
		NSUInteger dest = [[destinations objectAtIndex:i] unsignedIntegerValue];

		if (source < firstIndex)
			firstIndex = source;
			
		if (dest < firstIndex)
			firstIndex = dest;
	}
	
	[self updateIndexesFromRow:firstIndex];
}

- (BOOL)tableView:(NSTableView*)tv
	   acceptDrop:(id <NSDraggingInfo>)info
			  row:(int)row
	dropOperation:(NSTableViewDropOperation)op
{
	[super tableView:tv acceptDrop:info row:row dropOperation:op];
	
	if ([info draggingSource] == tableView)
	{
		//DNDArrayController handles moving...

		return YES;
	}

	if (row < 0)
		row = 0;
		
	// Determine the type of object that was dropped
	NSArray *supportedtypes = [NSArray arrayWithObjects:CogUrlsPboardType, NSFilenamesPboardType, iTunesDropType, nil];
	NSPasteboard *pboard = [info draggingPasteboard];
	NSString *bestType = [pboard availableTypeFromArray:supportedtypes];
	
	// Get files from an file drawer drop
	if ([bestType isEqualToString:CogUrlsPboardType]) {
		NSArray *urls = [NSUnarchiver unarchiveObjectWithData:[[info draggingPasteboard] dataForType:CogUrlsPboardType]];
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
	double tt = 0;
	ldiv_t hoursAndMinutes;
	
	NSEnumerator *enumerator = [[self arrangedObjects] objectEnumerator];
	PlaylistEntry* pe;
	
	while (pe = [enumerator nextObject]) {
		tt += [[pe length] doubleValue];
	}
	
	int sec = (int)(tt);
	hoursAndMinutes = ldiv(sec/60, 60);
	
	[self setTotalTimeDisplay:[NSString stringWithFormat:@"%i minutes %02i seconds (%ld hours %ld minutes)",sec/60, sec%60, hoursAndMinutes.quot, hoursAndMinutes.rem]];
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

- (NSUndoManager *)undoManager
{
	return [entriesController undoManager];
}

- (void)insertObjects:(NSArray *)objects atArrangedObjectIndexes:(NSIndexSet *)indexes
{
	[super insertObjects:objects atArrangedObjectIndexes:indexes];
	
	[self updateIndexesFromRow:[indexes firstIndex]];
	[self updateTotalTime];

	if (shuffle == YES)
		[self resetShuffleList];
}

- (void)removeObjectsAtArrangedObjectIndexes:(NSIndexSet *)indexes
{
	NSLog(@"Removing indexes: %@", indexes);
	NSLog(@"Current index: %i", [[currentEntry index] intValue]);

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
	NSLog(@"Current: %@, setting: %@", [self sortDescriptors], sortDescriptors);

	//Cheap hack so the index column isn't sorted
	if (([sortDescriptors count] != 0) && [[[sortDescriptors objectAtIndex:0] key] caseInsensitiveCompare:@"index"] == NSOrderedSame)
	{
		//Remove the sort descriptors
		[super setSortDescriptors:nil];
		[self rearrangeObjects];
		
		return;
	}

	[super setSortDescriptors:sortDescriptors];
	[self rearrangeObjects];
	[self updateIndexesFromRow:0];
}
		
- (IBAction)sortByPath
{
	NSSortDescriptor *s = [[NSSortDescriptor alloc] initWithKey:@"url" ascending:YES selector:@selector(compare:)];
	
	[self setSortDescriptors:[NSArray arrayWithObject:s]];

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

- (IBAction)toggleRepeat:(id)sender
{
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
	if (i < 0)
	{
		if (repeat != RepeatNone)
			i += [[self arrangedObjects] count];
		else
			return nil;
	}
	else if (i >= [[self arrangedObjects] count])
	{
		if (repeat != RepeatNone)
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
	if (repeat == RepeatOne) {
		return pe;
	}
	
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
	if (repeat == RepeatOne) {
		return pe;
	}
	
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

	if (currentEntry && [[currentEntry index] intValue] >= 0)
	{
		[shuffleList insertObject:currentEntry atIndex:0];
		[currentEntry setShuffleIndex:0];
		
		//Need to rejigger so the current entry is at the start now...
		int i;
		BOOL found = NO;
		for (i = 1; i < [shuffleList count] && !found; i++)
		{
			if ([shuffleList objectAtIndex:i] == currentEntry)
			{
				found = YES;
				[shuffleList removeObjectAtIndex:i];
			}
			else {
				[[shuffleList objectAtIndex:i] setShuffleIndex:[NSNumber numberWithInt:i]];
			}
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
- (void)setRepeat:(RepeatMode)r
{
	NSLog(@"Repeat is now: %i", r);
	repeat = r;
}
- (RepeatMode)repeat
{
	return repeat;
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

	[self updateIndexesFromRow:0];
}

- (IBAction)showEntryInFinder:(id)sender
{
	NSWorkspace* ws = [NSWorkspace sharedWorkspace];
	if ([self selectionIndex] < 0)
		return;
	
	NSURL *url = [[[self selectedObjects] objectAtIndex:0] URL];
	if ([url isFileURL])
		[ws selectFile:[url path] inFileViewerRootedAtPath:[url path]];
}

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

@end
