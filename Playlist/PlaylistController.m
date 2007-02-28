//
//  PlaylistController.m
//  Cog
//
//  Created by Vincent Spader on 3/18/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

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
		acceptableFileTypes = [[AudioPlayer fileTypes] retain];
		acceptablePlaylistTypes = [[NSArray alloc] initWithObjects:@"playlist",nil];
		
		shuffleList = [[NSMutableArray alloc] init];
//		DBLog(@"DAH BUTTER CHORNAR: %@", history);
	}
	
	return self;
}

- (void)awakeFromNib
{
	[super awakeFromNib];
	
	NSNotificationCenter* ns = [NSNotificationCenter defaultCenter];
	[ns addObserver:self selector:@selector(handlePlaylistViewHeaderNotification:) name:@"PlaylistViewColumnSeparatorDoubleClick" object:nil];
}

- (NSArray *)filesAtPath:(NSString *)path
{
	BOOL isDir;
	NSFileManager *manager;
	
	manager = [NSFileManager defaultManager];
	
	NSLog(@"Checking if path is a directory: %@", path);
	if ([manager fileExistsAtPath:path isDirectory:&isDir] && isDir == YES)
	{
		DBLog(@"path is directory");
		int j;
		NSArray *subpaths;
		NSMutableArray *validPaths = [[NSMutableArray alloc] init];
		
		subpaths = [manager subpathsAtPath:path];
		
		DBLog(@"Subpaths: %@", subpaths);
		for (j = 0; j < [subpaths count]; j++)
		{
			NSString *filepath;
				
			filepath = [NSString pathWithComponents:[NSArray arrayWithObjects:path,[subpaths objectAtIndex:j],nil]];
			if ([manager fileExistsAtPath:filepath isDirectory:&isDir] && isDir == NO)
			{
				if ([acceptableFileTypes containsObject:[[filepath pathExtension] lowercaseString]] && [[NSFileManager defaultManager] fileExistsAtPath:filepath])
				{
					[validPaths addObject:filepath];
				}
			}
		}
		
		return [validPaths autorelease];
	}
	else
	{
		NSLog(@"path is a file");
		if ([acceptableFileTypes containsObject:[[path pathExtension] lowercaseString]] && [[NSFileManager defaultManager] fileExistsAtPath:path])
		{
			NSLog(@"RETURNING THING");
			return [NSArray arrayWithObject:path];
		}
		else
		{
			return nil;
		}
	}
}

- (void)insertPaths:(NSArray *)paths atIndex:(int)index sort:(BOOL)sort
{
	NSArray *sortedFiles;
	NSMutableArray *files = [[NSMutableArray alloc] init];
	NSMutableArray *entries= [[NSMutableArray alloc] init];
	int i;

	if (!paths)
		return;
	
	if (index < 0)
		index = 0;
	
	for(i=0; i < [paths count]; i++)
	{
		[files addObjectsFromArray:[self filesAtPath:[paths objectAtIndex:i]]];
		NSLog(@"files is: %i", [files count]);
	}
	
	DBLog(@"Sorting paths");
	if (sort == YES)
	{
		sortedFiles = [files sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
	}
	else
	{
		sortedFiles = files;
	}

	for (i = 0; i < [sortedFiles count]; i++)
	{
		PlaylistEntry *pe = [[PlaylistEntry alloc] init];
		
		[pe	setFilename:[sortedFiles objectAtIndex:i]];
		[pe setIndex:index+i];
		[pe setTitle:[[sortedFiles objectAtIndex:i] lastPathComponent]];
//		[pe performSelectorOnMainThread:@selector(readTags) withObject:nil waitUntilDone:NO];
//		[pe performSelectorOnMainThread:@selector(readInfo) withObject:nil waitUntilDone:NO];
		
		[entries addObject:pe];
		[pe release];
	}
	
	NSRange r = NSMakeRange(index, [entries count]);
	NSLog(@"MAking range from: %i to %i", index, index + [entries count]);
	NSIndexSet *is = [NSIndexSet indexSetWithIndexesInRange:r];
	NSLog(@"INDex set: %@", is);
	NSLog(@"Adding: %i files", [entries count]);
	[self insertObjects:entries atArrangedObjectIndexes:is];
	
	if (shuffle == YES)
		[self resetShuffleList];
	
	[self setSelectionIndex:index];
	
	//Other thread will release entries....crazy crazy bad idea...whatever
	[NSThread detachNewThreadSelector:@selector(readMetaData:) toTarget:self withObject:entries];
	
	[files release];

	return;
}

- (void)readMetaData:(id)entries
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	int i;
	for (i = 0; i < [entries count]; i++)
	{
		PlaylistEntry *pe =[entries objectAtIndex:i];
		
		[pe readInfoThreaded];

		[pe readTagsThreaded];

		//Hack so the display gets updated
		if (pe == [self currentEntry])
			[self performSelectorOnMainThread:@selector(setCurrentEntry:) withObject:[self currentEntry] waitUntilDone:YES];
	}

	[self performSelectorOnMainThread:@selector(updateTotalTime) withObject:nil waitUntilDone:NO];
	
	[entries release];
	[pool release];
}

- (void)addPaths:(NSArray *)paths sort:(BOOL)sort
{
	[self insertPaths:paths atIndex:[[self arrangedObjects] count] sort:sort];
}

- (NSArray *)acceptableFileTypes
{
	return acceptableFileTypes;
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
	int i;
	NSLog(@"DROPPED");
	[super tableView:tv acceptDrop:info row:row dropOperation:op];
	if ([info draggingSource] == tableView)
	{
		//DNDArrayController handles moving...still need to update the uhm...indices
		NSLog(@"Archive stuff");
		NSArray *rows = [NSKeyedUnarchiver unarchiveObjectWithData:[[info draggingPasteboard] dataForType: MovedRowsType]];
		NSLog(@"Whatever");
		NSIndexSet  *indexSet = [self indexSetFromRows:rows];
		int firstIndex = [indexSet firstIndex];
		if (firstIndex > row)
		{
			i = row;
		}
		else
		{
			i = firstIndex;
		}
	
		NSLog(@"Updating indexes: %i", i);
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
		NSArray *files = [[info draggingPasteboard] propertyListForType:NSFilenamesPboardType];
		
		NSLog(@"INSERTING PATHS: %@", files);
		[self insertPaths:files atIndex:row sort:YES];
	}
	
	// Get files from an iTunes drop
	if ([bestType isEqualToString:iTunesDropType]) {
		NSDictionary *iTunesDict = [pboard propertyListForType:iTunesDropType];
		NSDictionary *tracks = [iTunesDict valueForKey:@"Tracks"];

		// Convert the iTunes URLs to filenames
		NSMutableArray *files = [[NSMutableArray alloc] init];
		NSEnumerator *e = [[tracks allValues] objectEnumerator];
		NSDictionary *trackInfo;
		NSURL *url;
		while (trackInfo = [e nextObject]) {
			url = [[NSURL alloc] initWithString:[trackInfo valueForKey:@"Location"]];
			if ([url isFileURL]) {
				[files addObject:[url path]];
			}
			
			[url release];
		}
		
		NSLog(@"INSERTING ITUNES PATHS: %@", files);
		[self insertPaths:files atIndex:row sort:YES];
		[files release];
	}
	
	NSLog(@"UPDATING");
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
	NSLog(@"Displaying: %@", ttd);
}

- (NSString *)totalTimeDisplay;
{
	return totalTimeDisplay;
}

- (void)updateIndexesFromRow:(int) row
{
//	DBLog(@"UPDATE INDEXES: %i", row);
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
	unsigned int *indexBuffer;
	NSMutableArray *a = [[NSMutableArray alloc] init];
	int i;
	
	//10.3 fix
	indexBuffer = malloc([indexes count]*sizeof(unsigned int));
	[indexes getIndexes:indexBuffer maxCount:[indexes count] inIndexRange:nil];
	for (i = 0; i < [indexes count]; i++)
	{
		NSLog(@"REMOVING FROM INDEX: %i", indexBuffer[i]);
		[a addObject:[[self arrangedObjects] objectAtIndex:(indexBuffer[i])]];
	}
	
//	a = [[self arrangedObjects] objectsAtIndexes:indexes]; //10.4 only
	
	if ([a containsObject:currentEntry])
	{
		[currentEntry setIndex:-1];
	}
	
	[super removeObjectsAtArrangedObjectIndexes:indexes];
	[self updateIndexesFromRow:[indexes firstIndex]];
	[self updateTotalTime];

	if (shuffle == YES)
		[self resetShuffleList];
	
	[a release];
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
		
- (IBAction)sortByPath:(id)sender
{
	NSSortDescriptor *s = [[NSSortDescriptor alloc] initWithKey:@"filename" ascending:YES selector:@selector(compare:)];
	
	[self setSortDescriptors:[NSArray arrayWithObject:s]];
	[self rearrangeObjects];

	[s release];	

	if (shuffle == YES)
		[self resetShuffleList];

	[self updateIndexesFromRow:0];
}

- (void)randomizeList
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
	//		NSLog(@"SHUFFLE: %i %i %i %i", i, shuffleIndex, offset, [shuffleList count]);
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

/*- (PlaylistEntry *)entryAtOffset:(int)offset
{
	if (shuffle == YES)
	{
		int i = shuffleIndex;
		i += offset;
//		NSLog(@"SHUFFLE: %i %i %i %i", i, shuffleIndex, offset, [shuffleList count]);
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
	else
	{
		int i;
		i = [currentEntry index];
		i += (offset-1);

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
}
*/

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
			if (found == NO && [[shuffleList objectAtIndex:i] filename] == [currentEntry filename])
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

	int j;
	for (j = 0; j < [[self content] count]; j++)
	{
		PlaylistEntry *p;
		p = [[self content] objectAtIndex:j];
		
		[p setIndex:-1];
	}
	
	[self updateIndexesFromRow:0];
}

- (void)savePlaylist:(NSString *)filename
{
//	DBLog(@"SAVING PLAYLIST: %@", filename);
	NSString *fileContents;
	NSMutableArray *filenames = [NSMutableArray array];
	
	NSEnumerator *enumerator;
	PlaylistEntry *entry;
	enumerator = [[self content] objectEnumerator];
	while (entry = [enumerator nextObject])
	{
		[filenames addObject:[entry filename]];
	}
	
	fileContents = [filenames componentsJoinedByString:@"\n"];
	[fileContents writeToFile:filename atomically:NO];
}

- (void)loadPlaylist:(NSString *)filename
{
	NSString *fileContents;

	[self removeObjects:[self arrangedObjects]];
	fileContents = [NSString stringWithContentsOfFile:filename];
	if (fileContents)
	{
		NSArray *filenames = [fileContents componentsSeparatedByString:@"\n"];
//		DBLog(@"filenames: %@", filenames);
		[self addPaths:filenames sort:NO];
	}
}

- (NSArray *)acceptablePlaylistTypes
{
	return acceptablePlaylistTypes;
}

- (NSString *)playlistFilename
{
	return playlistFilename;
}
- (void)setPlaylistFilename:(NSString *)pf
{
	[pf retain];
	[playlistFilename release];
	
	playlistFilename = pf;
}

- (IBAction)showFileInFinder:(id)sender
{
	NSWorkspace* ws = [NSWorkspace sharedWorkspace];
	if ([self selectionIndex] < 0)
		return;
	
	PlaylistEntry* curr = [self entryAtIndex:[self selectionIndex]];
	[ws selectFile:[curr filename] inFileViewerRootedAtPath:[curr filename]];
}

- (void)handlePlaylistViewHeaderNotification:(NSNotification*)notif
{
	NSTableView *tv = [notif object];
	NSNumber *colIdx = [[notif userInfo] objectForKey:@"column"];
	NSTableColumn *col = [[tv tableColumns] objectAtIndex:[colIdx intValue]];
	
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
