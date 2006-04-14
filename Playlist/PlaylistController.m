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

@implementation PlaylistController

#define SHUFFLE_HISTORY_SIZE 100

- (id)initWithCoder:(NSCoder *)decoder
{
	self = [super initWithCoder:decoder];
	
	if (self)
	{
		acceptableFileTypes = [[NSArray alloc] initWithObjects:@"shn",@"wv",@"ogg",@"wav",@"mpc",@"flac",@"ape",@"mp3",@"aiff",@"aif",@"aac",nil];
		acceptablePlaylistTypes = [[NSArray alloc] initWithObjects:@"playlist",nil];
		shuffleList = [[NSMutableArray alloc] init];
//		DBLog(@"DAH BUTTER CHORNAR: %@", history);
	}
	
	return self;
}

- (void)awakeFromNib
{
	[super awakeFromNib];
}

- (int)insertFile:(NSString *)filename atIndex:(int)index
{
	if ([acceptableFileTypes containsObject:[[filename pathExtension] lowercaseString]] && [[NSFileManager defaultManager] fileExistsAtPath:filename])
	{
		PlaylistEntry *pe = [[PlaylistEntry alloc] init];

		[pe	setFilename:filename]; //Setfilename takes car of opening the soundfile..cheap hack, but works for now
		[pe setIndex:(index+1)];
		[pe readTags];
		[pe readInfo];
		
		[self insertObject:pe atArrangedObjectIndex:index];	
		[pe release];
		
		return 1;
	}
	
	return 0;
}

- (int)insertPath:(NSString *)path atIndex:(int)index
{
	BOOL isDir;
	NSFileManager *manager;
	
	manager = [NSFileManager defaultManager];
	
	DBLog(@"Checking if path is a directory: %@", path);
	if ([manager fileExistsAtPath:path isDirectory:&isDir] && isDir == YES)
	{
		DBLog(@"path is directory");
		int count;
		int j;
		NSArray *subpaths;
		count = 0;
		subpaths = [manager subpathsAtPath:path];
		
		DBLog(@"Subpaths: %@", subpaths);
		for (j = 0; j < [subpaths count]; j++)
		{
			NSString *filepath;
				
			filepath = [NSString pathWithComponents:[NSArray arrayWithObjects:path,[subpaths objectAtIndex:j],nil]];

			count += [self insertFile:filepath atIndex:index+count];
		}
		
		return count;
	}
	else
	{
//		DBLog(@"Adding fiiiiile: %@", path);
		DBLog(@"path is a file");
		return [self insertFile:path atIndex:index];
	}
}

- (int)insertPaths:(NSArray *)paths atIndex:(int)index sort:(BOOL)sort
{
	NSArray *sortedFiles;
	int count;
	int i;

	if (!paths)
		return 0;
	
	if (index < 0)
		index = 0;
	
	count = 0;
	
	DBLog(@"Sorting paths");
	if (sort == YES)
	{
		sortedFiles = [paths sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
	}
	else
	{
		sortedFiles = paths;
	}
	
	DBLog(@"Paths sorted: %@", sortedFiles);
	for(i=0; i < [sortedFiles count]; i++)
	{
		int j;
		NSString *f;
		f = [sortedFiles objectAtIndex:i];
		
		DBLog(@"Inserting path");
		j = [self insertPath:f atIndex:(index+count)];
//		DBLog(@"Number added: %i", j);
		count+=j;
	}
	
	if (shuffle == YES)
		[self resetShuffleList];
	
	
	[self setSelectionIndex:index];
	[self updateTotalTime];
	
	return count;
}

- (int)addFile:(NSString *)filename
{
	return [self insertFile:filename atIndex:[[self arrangedObjects] count]];
}

- (int)addPath:(NSString *)path
{
	return [self insertPath:path atIndex:[[self arrangedObjects] count]];
}

- (int)addPaths:(NSArray *)paths sort:(BOOL)sort
{
	return [self insertPaths:paths atIndex:[[self arrangedObjects] count] sort:sort];
}

- (NSArray *)acceptableFileTypes
{
	return acceptableFileTypes;
}

- (BOOL)tableView:(NSTableView*)tv
	   acceptDrop:(id <NSDraggingInfo>)info
			  row:(int)row
	dropOperation:(NSTableViewDropOperation)op
{
	int i;
	
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
	
		[self updateIndexesFromRow:i];

		return YES;
	}

	if (row < 0)
		row = 0;

	NSArray *files = [[info draggingPasteboard] propertyListForType:NSFilenamesPboardType];
	[self insertPaths:files atIndex:row sort:YES];

	[self updateIndexesFromRow:row];
	[self updateTotalTime];
	
	if (shuffle == YES)
		[self resetShuffleList];
	
	return YES;
}

- (void)updateTotalTime
{
	double tt=0;
	
	NSEnumerator *enumerator = [[self content] objectEnumerator];
	PlaylistEntry* pe;
	
	while (pe = [enumerator nextObject]) {
		NSLog(@"Updating :%i", pe);
		NSLog(@"Updating :%@", pe);
		tt += [pe length];
		NSLog(@"UpdateD");
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
	for (j = row; j < [[self content] count]; j++)
	{
		PlaylistEntry *p;
		p = [[self content] objectAtIndex:j];
		
		[p setIndex:(j+1)];
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
	//Need to fix for SHUFFLE MODE! holy fix.
	if (shuffle == NO)
	{
		i--;
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
/*	else
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
	}*/
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
- (BOOL)next
{
	PlaylistEntry *pe;
	
	pe = [self entryAtIndex:[currentEntry index] + 1];
	if (pe == nil)
		return NO;
	
	[self setCurrentEntry:pe];
	
	return YES;
}

- (BOOL)prev
{
	PlaylistEntry *pe;
	
	pe = [self entryAtIndex:[currentEntry index] - 1];
	if (pe == nil)
		return NO;
	
	[self setCurrentEntry:pe];
	
	return YES;
}

- (void)addShuffledListToBack
{
	NSArray *newList = [Shuffle shuffleList:[self arrangedObjects]];
	NSIndexSet *indexSet = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [newList count])];
	
	[shuffleList insertObjects:newList atIndexes:indexSet];
}

- (void)addShuffledListToFront
{
	NSArray *newList = [Shuffle shuffleList:[self arrangedObjects]];
	NSIndexSet *indexSet = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange([shuffleList count], [newList count])];
	NSLog(@"%@", newList);
	[shuffleList insertObjects:newList atIndexes:indexSet];
}

- (void)resetShuffleList
{
	[shuffleList removeAllObjects];
	[self addShuffledListToFront];
	
//	shuffleIndex = 0;
}

- (id)currentEntry
{
//	DBLog(@"WOAH EGE ");
	return currentEntry;
}

- (void)setCurrentEntry:(id)pe
{
	[currentEntry setCurrent:NO];
	
	[pe setCurrent:YES];
	[tableView scrollRowToVisible:([(PlaylistEntry *)pe index]-1)];
	
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

- (void)savePlaylist:(NSString *)filename
{
//	DBLog(@"SAVING PLAYLIST: %@", filename);
	NSString *fileContents;
	NSMutableArray *filenames = [NSMutableArray array];
	
	NSEnumerator *enumerator;
	PlaylistEntry *entry;
	enumerator = [[self arrangedObjects] objectEnumerator];
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

@end
