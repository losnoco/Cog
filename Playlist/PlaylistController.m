//
//  PlaylistController.m
//  Cog
//
//  Created by Vincent Spader on 3/18/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "PlaylistController.h"
#import "PlaylistEntry.h"

@implementation PlaylistController

#define HISTORY_SIZE 100

- (id)initWithCoder:(NSCoder *)decoder
{
	self = [super initWithCoder:decoder];
	
	if (self)
	{
		acceptableFileTypes = [[NSArray alloc] initWithObjects:@"shn",@"wv",@"ogg",@"wav",@"mpc",@"flac",@"ape",@"mp3",@"aiff",@"aif",@"aac",nil];
		acceptablePlaylistTypes = [[NSArray alloc] initWithObjects:@"playlist",nil];
		history = [[NSMutableArray alloc] init];
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
		[pe setIndex:index];
		[pe readTags];
		[pe readInfo];
		
		[self insertObject:pe atArrangedObjectIndex:index];	

		return 1;
	}
	
	return 0;
}

- (int)insertPath:(NSString *)path atIndex:(int)index
{
	BOOL isDir;
	NSFileManager *manager;
	
	manager = [NSFileManager defaultManager];
	
	if ([manager fileExistsAtPath:path isDirectory:&isDir] && isDir == YES)
	{
		int count;
		int j;
		NSArray *subpaths;
		count = 0;
		subpaths = [manager subpathsAtPath:path];
		
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
	
	if (sort == YES)
	{
		sortedFiles = [paths sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
		[paths release];
	}
	else
	{
		sortedFiles = paths;
	}
	
	for(i=0; i < [sortedFiles count]; i++)
	{
		int j;
		NSString *f;
		f = [sortedFiles objectAtIndex:i];
		
//		DBLog(@"Adding file to index: %i", index+count);
		j = [self insertPath:f atIndex:(index+count)];
//		DBLog(@"Number added: %i", j);
		count+=j;
	}
	
	if (shuffle == YES)
		[self generateShuffleList];
	
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
		NSArray *rows = [NSKeyedUnarchiver unarchiveObjectWithData:[[info draggingPasteboard] dataForType: MovedRowsType]];
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
	
	if (shuffle == YES)
		[self generateShuffleList];
	
	return YES;
}

- (void)updateIndexesFromRow:(int) row
{
//	DBLog(@"UPDATE INDEXES: %i", row);
	int j;
	for (j = row; j < [[self content] count]; j++)
	{
		PlaylistEntry *p;
		p = [[self content] objectAtIndex:j];
		
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
		[a addObject:[[self arrangedObjects] objectAtIndex:(indexBuffer[i])]];
	}
	
//	a = [[self arrangedObjects] objectsAtIndexes:indexes]; //10.4 only
	
	if ([a containsObject:currentEntry])
	{
		[currentEntry setIndex:-1];
	}
	
	[super removeObjectsAtArrangedObjectIndexes:indexes];
	[self updateIndexesFromRow:[indexes firstIndex]];
	
	if (shuffle == YES)
		[self generateShuffleList];
	
	[history removeObjectsInArray:a];
	
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

- (void)next
{
	PlaylistEntry *pe;
	
	pe = [self nextEntry];
	if (pe == nil)
		return;
	
	if (shuffle == YES)
	{
		[shuffleList removeObject:pe];
	}
	[self setCurrentEntry:pe addToHistory:YES];
	[self setNextEntry:nil];
}

- (void)prev
{
	PlaylistEntry *pe;
	
	pe = [self prevEntry];
	if (pe == nil)
		return;
	
	if (pe != [history objectAtIndex:1])
		DBLog(@"History inconcistency");
	[history removeObjectAtIndex:0];

	[self setCurrentEntry:pe addToHistory:NO];
	[self setPrevEntry:nil];
	
	//If one goes back, and goes forward, one shall receive unto thee a new song
	if (shuffle == YES)
		[self generateShuffleList];
}

- (PlaylistEntry *)prevEntry
{
	if (prevEntry == nil)
		[self getPrevEntry];
	
	return prevEntry;
}

- (PlaylistEntry *)nextEntry
{
	if (nextEntry == nil)
		[self getNextEntry];
	
	return nextEntry;
}

- (void)reset
{
	nextEntry = nil;
	prevEntry = nil;
	[self generateShuffleList];
}

- (void)generateShuffleList
{
//	[shuffleHistory removeAllObjects];
//	DBLog(@"Generated Shuffle List");
	[shuffleList removeAllObjects];
	[shuffleList addObjectsFromArray:[self arrangedObjects]];
	if (currentEntry != nil)
		[shuffleList removeObject:currentEntry];
}

- (void)setCurrentEntry:(PlaylistEntry *)pe addToHistory:(BOOL)h
{
	[self setCurrentEntry:pe];	
	
	if (h == YES)
	{
		[history insertObject:pe atIndex:0];
		if ([history count] > HISTORY_SIZE)
			[history removeObjectAtIndex:([history count] - 1)];
		
	}
}

- (void)getNextEntry
{
	PlaylistEntry *pe;

	if (nextEntry != nil)
		return;
	
	if (shuffle == YES)
	{
//		DBLog(@"SHUFFLE IS TEH ON: %i", [shuffleList count]);
		if ([shuffleList count] == 0) //out of tuuuunes
		{
			if (repeat == YES)
			{
				[self generateShuffleList];
			}
			else
			{
				[self setNextEntry:nil];
				return;
			}
		}
		int r;
		
		r = random() % [shuffleList count];
//		DBLog(@"PICKING SONG %i FROM SHUFFLE LIST", r);
		pe = [shuffleList objectAtIndex:r];
		
		[self setNextEntry:pe];
	}
	else
	{
		int i = [currentEntry index] + 1;

		if (i >= [[self arrangedObjects] count]) //out of tuuuunes
		{
			if (repeat == YES)
			{
				i = 0;
			}
			else
			{
				[self setNextEntry:nil];
				return;
			}
		}
		
		pe = [[self arrangedObjects] objectAtIndex:i];
		
		[self setNextEntry:pe];
	}
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
	[tableView scrollRowToVisible:[(PlaylistEntry *)pe index]];
	
	[pe retain];
	[currentEntry release];
	
	currentEntry = pe;
}	

- (void)getPrevEntry
{
	PlaylistEntry *pe;
	
//	DBLog(@"GETTING PREVIOUS ENTRY");
	
	if (prevEntry != nil)
		return;
	//NOTE: 1 contains the current entry
	if ([history count] == 1) //Cant go back any further
	{
//		DBLog(@"HISTORY IS TEH EMPTY");
		[self setPrevEntry:nil];
		return;
	}
	else
	{
//		DBLog(@"IN TEH HISTORY");

		pe = [history objectAtIndex:1];
		[self setPrevEntry:pe];
			
		return;
	}
}

- (void)setPrevEntry:(PlaylistEntry *)pe
{
	[pe retain];
	[prevEntry release];
	prevEntry = pe;
}

- (void)setNextEntry:(PlaylistEntry *)pe
{
	[pe retain];
	[nextEntry release];
	nextEntry = pe;
}

- (void)setShuffle:(BOOL)s
{
	shuffle = s;
	if (shuffle == YES)
		[self generateShuffleList];
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
