//
//	PlaylistController.m
//	Cog
//
//	Created by Vincent Spader on 3/18/05.
//	Copyright 2005 Vincent Spader All rights reserved.
//

#import "PlaylistLoader.h"
#import "PlaylistController.h"
#import "PlaybackController.h"
#import "EntriesController.h"
#import "PlaylistEntry.h"
#import "Shuffle.h"
#import "SpotlightWindowController.h"
#import "RepeatTransformers.h"
#import "ShuffleTransformers.h"
#import "StatusImageTransformer.h"
#import "ToggleQueueTitleTransformer.h"
#import "TagEditorController.h"

#import "CogAudio/AudioPlayer.h"

#import "Logging.h"

@implementation PlaylistController

@synthesize currentEntry;
@synthesize totalTime;

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

	NSValueTransformer *repeatModeImageTransformer = [[[RepeatModeImageTransformer alloc] init] autorelease];
    [NSValueTransformer setValueTransformer:repeatModeImageTransformer
                                    forName:@"RepeatModeImageTransformer"];
	

	NSValueTransformer *shuffleOffTransformer = [[[ShuffleModeTransformer alloc] initWithMode:ShuffleOff] autorelease];
    [NSValueTransformer setValueTransformer:shuffleOffTransformer
                                    forName:@"ShuffleOffTransformer"];
	
	NSValueTransformer *shuffleAlbumsTransformer = [[[ShuffleModeTransformer alloc] initWithMode:ShuffleAlbums] autorelease];
    [NSValueTransformer setValueTransformer:shuffleAlbumsTransformer
                                    forName:@"ShuffleAlbumsTransformer"];
	
	NSValueTransformer *shuffleAllTransformer = [[[ShuffleModeTransformer alloc] initWithMode:ShuffleAll] autorelease];
    [NSValueTransformer setValueTransformer:shuffleAllTransformer
                                    forName:@"ShuffleAllTransformer"];

	NSValueTransformer *shuffleImageTransformer = [[[ShuffleImageTransformer alloc] init] autorelease];
    [NSValueTransformer setValueTransformer:shuffleImageTransformer
                                    forName:@"ShuffleImageTransformer"];
	
	
	
	NSValueTransformer *statusImageTransformer = [[[StatusImageTransformer alloc] init] autorelease];
    [NSValueTransformer setValueTransformer:statusImageTransformer
                                    forName:@"StatusImageTransformer"];
									
	NSValueTransformer *toggleQueueTitleTransformer = [[[ToggleQueueTitleTransformer alloc] init] autorelease];
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
		[self initDefaults];
	}
	
	return self;
}


- (void)dealloc
{
	[shuffleList release];
	[queueList release];
	
	[super dealloc];
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
	
	for (PlaylistEntry *pe in [self arrangedObjects]) {
        if (!isnan([pe.length doubleValue]))
            tt += [pe.length doubleValue];
	}
	
	int sec = (int)(tt);
	hoursAndMinutes = ldiv(sec/60, 60);
	
	[self setTotalTime:[NSString stringWithFormat:@"%ld hours %ld minutes %d seconds", hoursAndMinutes.quot, hoursAndMinutes.rem, sec%60]];
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
		[urls release];
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
		[urls release];
	}
	
	if ([acceptedURLs count])
	{
		[self willInsertURLs:acceptedURLs origin:URLOriginInternal];
		
		if (![[entriesController entries] count]) {
			row = 0;
		}
		
		NSArray* entries = [playlistLoader insertURLs:acceptedURLs atIndex:row sort:YES];
		[self didInsertURLs:entries origin:URLOriginInternal];
	}
	
	[acceptedURLs release];
	
	if ([self shuffle] != ShuffleOff)
		[self resetShuffleList];
	
	return YES;
}

- (NSUndoManager *)undoManager
{
	return [entriesController undoManager];
}

- (void)insertObjects:(NSArray *)objects atArrangedObjectIndexes:(NSIndexSet *)indexes
{
	[super insertObjects:objects atArrangedObjectIndexes:indexes];
	
	if ([self shuffle] != ShuffleOff)
		[self resetShuffleList];
}

- (void)removeObjectsAtArrangedObjectIndexes:(NSIndexSet *)indexes
{
	DLog(@"Removing indexes: %@", indexes);
	DLog(@"Current index: %i", currentEntry.index);

	if (currentEntry.index >= 0 && [indexes containsIndex:currentEntry.index])
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
			if ([indexes containsIndex:j]) {
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
		[super setSortDescriptors:nil];
		[self rearrangeObjects];
		
		return;
	}

	[super setSortDescriptors:sortDescriptors];
	[self rearrangeObjects];

	[playbackController playlistDidChange:self];
}
		
- (IBAction)sortByPath
{
	NSSortDescriptor *s = [[NSSortDescriptor alloc] initWithKey:@"url" ascending:YES selector:@selector(compare:)];
	
	[self setSortDescriptors:[NSArray arrayWithObject:s]];

	[s release];	

	if ([self shuffle] != ShuffleOff)
		[self resetShuffleList];
}

- (IBAction)randomizeList
{
	[self setSortDescriptors:nil];

	[self setContent:[Shuffle shuffleList:[self content]]];
	if ([self shuffle] != ShuffleOff)
		[self resetShuffleList];
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
	if ([self repeat] == RepeatOne) {
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
	NSPredicate *predicate = [NSPredicate predicateWithFormat:@"album like %@",
							  album];		
	return [[self arrangedObjects] filteredArrayUsingPredicate:predicate];
}

- (PlaylistEntry *)getPrevEntry:(PlaylistEntry *)pe
{
	if ([self repeat] == RepeatOne) {
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

	if (currentEntry && currentEntry.index >= 0)
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
				[[shuffleList objectAtIndex:i] setShuffleIndex: i];
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
		[tableView scrollRowToVisible:pe.index];
	
	[pe retain];
	[currentEntry release];
	
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
	return [[NSUserDefaults standardUserDefaults] integerForKey:@"shuffle"];
}
- (void)setRepeat:(RepeatMode)r
{
	[[NSUserDefaults standardUserDefaults] setInteger:r forKey:@"repeat"];
	[playbackController playlistDidChange:self];
}
- (RepeatMode)repeat
{
	return [[NSUserDefaults standardUserDefaults] integerForKey:@"repeat"];
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
			queueItem.queuePosition = [queueList count];
			
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
	if (shouldPlay	&& [[entriesController entries] count] > 0) {
		[playbackController playEntry: [urls objectAtIndex:0]];
	}
}


@end
