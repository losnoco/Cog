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

- (void)awakeFromNib
{
	[super awakeFromNib];
	
	NSNotificationCenter* ns = [NSNotificationCenter defaultCenter];
	[ns addObserver:self selector:@selector(handlePlaylistViewHeaderNotification:) name:@"PlaylistViewColumnSeparatorDoubleClick" object:nil];
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
		tt += [pe length];
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
		
		[p setIndex:j];
	}
}

- (void)removeObjectsAtArrangedObjectIndexes:(NSIndexSet *)indexes
{
	NSLog(@"REMOVING");
	NSArray *a = [[self arrangedObjects] objectsAtIndexes:indexes]; //Screw 10.3
	
	if ([a containsObject:currentEntry])
	{
		[currentEntry setIndex:-1];
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
	if (([sortDescriptors count] != 0) && [[[sortDescriptors objectAtIndex:0] key] caseInsensitiveCompare:@"displayIndex"] == NSOrderedSame)
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
			NSLog(@"Adding shuffled list to back!");
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
		return [self shuffledEntryAtIndex:[pe shuffleIndex] + 1];
	}
	else
	{
		return [self entryAtIndex:[pe index] + 1];
	}
}

- (PlaylistEntry *)getPrevEntry:(PlaylistEntry *)pe
{
	if (shuffle == YES)
	{
		return [self shuffledEntryAtIndex:[pe shuffleIndex] - 1];
	}
	else
	{
		//Fix for removing a track, then pressing prev with repeat turned on
		if ([pe index] == -1)
		{
			return [self entryAtIndex:[pe index]];
		}
		else
		{
			return [self entryAtIndex:[pe index] - 1];
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
		[[shuffleList objectAtIndex:i] setShuffleIndex:i];
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
		[[shuffleList objectAtIndex:i] setShuffleIndex:i];
	}
	
}

- (void)resetShuffleList
{
	[shuffleList removeAllObjects];

	[self addShuffledListToFront];

	if (currentEntry && [currentEntry index] != -1)
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

			[[shuffleList objectAtIndex:i] setShuffleIndex:i];
		}
	}
}

- (id)currentEntry
{
	return currentEntry;
}

- (void)setCurrentEntry:(PlaylistEntry *)pe
{
	[currentEntry setCurrent:NO];
	
	[pe setCurrent:YES];
	[tableView scrollRowToVisible:[pe index]];
	
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

- (void)handlePlaylistViewHeaderNotification:(NSNotification*)notif
{
	NSTableView *tv = [notif object];
	NSNumber *colIdx = [[notif userInfo] objectForKey:@"column"];
	NSTableColumn *col = [[tv tableColumns] objectAtIndex:[colIdx intValue]];
	
	//Change to use NSSelectorFromString and NSMethodSignature returnType, see http://developer.apple.com/documentation/Cocoa/Conceptual/ObjectiveC/Articles/chapter_5_section_7.html for info
	//Maybe we can pull the column bindings out somehow, instead of selectorfromstring

	// find which function to call on PlaylistEntry*
	SEL sel;
	NSString* identifier = [col identifier];
	BOOL isNumeric = NO;
	if ([identifier compare:@"title"] == NSOrderedSame)
		sel = @selector(title);
	else if ([identifier compare:@"length"] == NSOrderedSame) 
		sel = @selector(lengthString);
	else if ([identifier compare:@"index"] == NSOrderedSame) {
		sel = @selector(index);
		isNumeric = YES;
	}
	else if ([identifier compare:@"artist"] == NSOrderedSame)
		sel = @selector(artist);
	else if ([identifier compare:@"album"] == NSOrderedSame)
		sel = @selector(album);
	else if ([identifier compare:@"year"] == NSOrderedSame)
		sel = @selector(year);
	else if ([identifier compare:@"genre"] == NSOrderedSame)
		sel = @selector(genre);
	else if ([identifier compare:@"track"] == NSOrderedSame) {
		sel = @selector(track);
		isNumeric = YES;
	}
	else
		return;
	
	NSCell *cell = [col dataCell];
	NSAttributedString * as = [cell attributedStringValue];

	// find the longest string display length in that column
	NSArray *entries = [self arrangedObjects];
	NSEnumerator *enumerator = [entries objectEnumerator];
	PlaylistEntry *entry;
	float maxlength = -1;
	NSString *ret;
	while (entry = [enumerator nextObject]) {
		if (isNumeric)
			ret = [NSString stringWithFormat:@"%d", objc_msgSend(entry, sel)];
		else
			ret = objc_msgSend(entry, sel);
		if ([ret sizeWithAttributes:[as attributesAtIndex:0 effectiveRange:nil]].width > maxlength)
			maxlength = [ret sizeWithAttributes:[as attributesAtIndex:0 effectiveRange:nil]].width;
	}
	

	// set the new width (plus a 5 pixel extra to avoid "..." string substitution)
	[col setWidth:maxlength+5];
}

@end
