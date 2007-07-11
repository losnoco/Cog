//
//  PlaylistController.m
//  Cog
//
//  Created by Vincent Spader on 3/18/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "PlaylistLoader.h"
#import "PlaylistController.h"
#import "PlaylistEntry.h"
#import "Shuffle.h"

#import "CogAudio/AudioPlayer.h"

@implementation PlaylistController

#define SHUFFLE_HISTORY_SIZE 100

- (id)initWithCoder:(NSCoder *)decoder
{
	self = [super initWithCoder:decoder];
	
	if (self)
	{
		shuffleList = [[NSMutableArray alloc] init];
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
	if ([info draggingSource] == tableView)
	{
		//DNDArrayController handles moving...still need to update the indexes

		int i;
		NSArray *rows = [NSKeyedUnarchiver unarchiveObjectWithData:[[info draggingPasteboard] dataForType: MovedRowsType]];
		int firstIndex = [[self indexSetFromRows:rows] firstIndex];

		if (firstIndex > row)
			i = row;
		else
			i = firstIndex;
	
		[self updateIndexesFromRow:i];

		return YES;
	}

	if (row < 0)
		row = 0;
		
	// Determine the type of object that was dropped
	NSArray *supportedtypes = [NSArray arrayWithObjects:NSFilenamesPboardType, iTunesDropType, nil];
	NSPasteboard *pboard = [info draggingPasteboard];
	NSString *bestType = [pboard availableTypeFromArray:supportedtypes];
	
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
	
	int sec = (int)(tt/1000.0);
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

- (void)removeObjectsAtArrangedObjectIndexes:(NSIndexSet *)indexes
{
	NSArray *a = [[self arrangedObjects] objectsAtIndexes:indexes]; //Screw 10.3
	
	if ([a containsObject:currentEntry])
	{
		[currentEntry setIndex:[NSNumber numberWithInt:-1]];
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
		return [self entryAtIndex:([[pe index] intValue] + 1)];
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
		//Fix for removing a track, then pressing prev with repeat turned on
		if ([[pe index] intValue] == -1)
		{
			return [self entryAtIndex:[[pe index] intValue]];
		}
		else
		{
			return [self entryAtIndex:[[pe index] intValue] - 1];
		}
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
