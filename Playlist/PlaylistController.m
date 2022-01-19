//
//	PlaylistController.m
//	Cog
//
//	Created by Vincent Spader on 3/18/05.
//	Copyright 2005 Vincent Spader All rights reserved.
//

#import "PlaylistController.h"
#import "PlaybackController.h"
#import "PlaylistEntry.h"
#import "PlaylistLoader.h"
#import "RepeatTransformers.h"
#import "Shuffle.h"
#import "ShuffleTransformers.h"
#import "SpotlightWindowController.h"
#import "StatusImageTransformer.h"
#import "ToggleQueueTitleTransformer.h"
#import "SQLiteStore.h"

#import "Logging.h"

#define UNDO_STACK_LIMIT 0

@implementation PlaylistController

@synthesize currentEntry;
@synthesize totalTime;

+ (void)initialize {
    NSValueTransformer *repeatNoneTransformer =
            [[RepeatModeTransformer alloc] initWithMode:RepeatModeNoRepeat];
    [NSValueTransformer setValueTransformer:repeatNoneTransformer forName:@"RepeatNoneTransformer"];

    NSValueTransformer *repeatOneTransformer =
            [[RepeatModeTransformer alloc] initWithMode:RepeatModeRepeatOne];
    [NSValueTransformer setValueTransformer:repeatOneTransformer forName:@"RepeatOneTransformer"];

    NSValueTransformer *repeatAlbumTransformer =
            [[RepeatModeTransformer alloc] initWithMode:RepeatModeRepeatAlbum];
    [NSValueTransformer setValueTransformer:repeatAlbumTransformer
                                    forName:@"RepeatAlbumTransformer"];

    NSValueTransformer *repeatAllTransformer =
            [[RepeatModeTransformer alloc] initWithMode:RepeatModeRepeatAll];
    [NSValueTransformer setValueTransformer:repeatAllTransformer forName:@"RepeatAllTransformer"];

    NSValueTransformer *repeatModeImageTransformer = [[RepeatModeImageTransformer alloc] init];
    [NSValueTransformer setValueTransformer:repeatModeImageTransformer
                                    forName:@"RepeatModeImageTransformer"];

    NSValueTransformer *shuffleOffTransformer =
            [[ShuffleModeTransformer alloc] initWithMode:ShuffleOff];
    [NSValueTransformer setValueTransformer:shuffleOffTransformer forName:@"ShuffleOffTransformer"];

    NSValueTransformer *shuffleAlbumsTransformer =
            [[ShuffleModeTransformer alloc] initWithMode:ShuffleAlbums];
    [NSValueTransformer setValueTransformer:shuffleAlbumsTransformer
                                    forName:@"ShuffleAlbumsTransformer"];

    NSValueTransformer *shuffleAllTransformer =
            [[ShuffleModeTransformer alloc] initWithMode:ShuffleAll];
    [NSValueTransformer setValueTransformer:shuffleAllTransformer forName:@"ShuffleAllTransformer"];

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

- (void)initDefaults {
    NSDictionary *defaultsDictionary = @{@"repeat": @(RepeatModeNoRepeat), @"shuffle": @(ShuffleOff)};

    [[NSUserDefaults standardUserDefaults] registerDefaults:defaultsDictionary];
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];

    if (self) {
        shuffleList = [[NSMutableArray alloc] init];
        queueList = [[NSMutableArray alloc] init];

        undoManager = [[NSUndoManager alloc] init];

        [undoManager setLevelsOfUndo:UNDO_STACK_LIMIT];

        [self initDefaults];
    }

    return self;
}

- (void)awakeFromNib {
    [super awakeFromNib];

    [self addObserver:self
           forKeyPath:@"arrangedObjects"
              options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
              context:nil];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
    if ([keyPath isEqualToString:@"arrangedObjects"]) {
        [self updatePlaylistIndexes];
        [self updateTotalTime];
    }
}

static inline void dispatch_sync_reentrant(dispatch_queue_t queue, dispatch_block_t block) {
    if (dispatch_queue_get_label(queue) == dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL)) {
        block();
    }
    else {
        dispatch_sync(queue, block);
    }
}

- (void)setProgressBarStatus:(double)status {
    dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
        [self->playbackController setProgressBarStatus:status];
        [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.001]];
    });
}

- (void)updatePlaylistIndexes {
    NSArray *arranged = [self arrangedObjects];
    NSUInteger n = [arranged count];
    BOOL updated = NO;
    for (NSUInteger i = 0; i < n; i++) {
        PlaylistEntry *pe = arranged[i];
        if (pe.index != i) {  // Make sure we don't get into some kind of crazy observing loop...
            pe.index = i;
            updated = YES;
        }
    }
    if (updated) {
        [[SQLiteStore sharedStore] syncPlaylistEntries:arranged progressCall:^(double progress) {
            [self setProgressBarStatus:progress];
        }];
    }
}

- (void)updateTotalTime {
    double tt = 0;
    ldiv_t hoursAndMinutes;
    ldiv_t daysAndHours;
    ldiv_t weeksAndDays;

    for (PlaylistEntry *pe in [self arrangedObjects]) {
        if (!isnan([pe.length doubleValue])) tt += [pe.length doubleValue];
    }

    long sec = (long) (tt);
    hoursAndMinutes = ldiv(sec / 60, 60);

    if (hoursAndMinutes.quot >= 24) {
        daysAndHours = ldiv(hoursAndMinutes.quot, 24);
        if (daysAndHours.quot >= 7) {
            weeksAndDays = ldiv(daysAndHours.quot, 7);
            [self setTotalTime:[NSString stringWithFormat:@"%ld week%@ %ld day%@ %ld hour%@ %ld minute%@ %ld second%@",
                                weeksAndDays.quot,
                                weeksAndDays.quot != 1 ? @"s" : @"",
                                weeksAndDays.rem,
                                weeksAndDays.rem != 1 ? @"s" : @"",
                                daysAndHours.rem,
                                daysAndHours.rem != 1 ? @"s" : @"",
                                hoursAndMinutes.rem,
                                hoursAndMinutes.rem != 1 ? @"s" : @"",
                                sec % 60,
                                (sec % 60) != 1 ? @"s" : @""]];
        } else {
            [self setTotalTime:[NSString stringWithFormat:@"%ld day%@ %ld hour%@ %ld minute%@ %ld second%@",
                                daysAndHours.quot,
                                daysAndHours.quot != 1 ? @"s" : @"",
                                daysAndHours.rem,
                                daysAndHours.rem != 1 ? @"s" : @"",
                                hoursAndMinutes.rem,
                                hoursAndMinutes.rem != 1 ? @"s" : @"",
                                sec % 60,
                                (sec % 60) != 1 ? @"s" : @""]];
        }
    } else {
        if (hoursAndMinutes.quot > 0) {
            [self setTotalTime:[NSString stringWithFormat:@"%ld hour%@ %ld minute%@ %ld second%@",
                                hoursAndMinutes.quot,
                                hoursAndMinutes.quot != 1 ? @"s" : @"",
                                hoursAndMinutes.rem,
                                hoursAndMinutes.rem != 1 ? @"s" : @"",
                                sec % 60,
                                (sec % 60) != 1 ? @"s" : @""]];
        } else {
            if (hoursAndMinutes.rem > 0) {
                [self setTotalTime:[NSString stringWithFormat:@"%ld minute%@ %ld second%@",
                                    hoursAndMinutes.rem,
                                    hoursAndMinutes.rem != 1 ? @"s" : @"",
                                    sec % 60,
                                    (sec % 60) != 1 ? @"s" : @""]];
            } else {
                [self setTotalTime:[NSString stringWithFormat:@"%ld second%@",
                                    sec,
                                    sec != 1 ? @"s" : @""]];
            }
        }
    }
}

- (void)tableView:(NSTableView *)tableView didClickTableColumn:(NSTableColumn *)tableColumn {
    if ([self shuffle] != ShuffleOff) [self resetShuffleList];
}

- (NSString *)tableView:(NSTableView *)tv
         toolTipForCell:(NSCell *)cell
                   rect:(NSRectPointer)rect
            tableColumn:(NSTableColumn *)tc
                    row:(NSInteger)row
          mouseLocation:(NSPoint)mouseLocation {
    DLog(@"GETTING STATUS FOR ROW: %li: %@!", row,
            [[[self arrangedObjects] objectAtIndex:row] statusMessage]);
    return [[[self arrangedObjects] objectAtIndex:row] statusMessage];
}

- (void)moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet *)indexSet
                                        toIndex:(NSUInteger)insertIndex {
    [super moveObjectsInArrangedObjectsFromIndexes:indexSet toIndex:insertIndex];

#if 0 // syncPlaylistEntries is already called for rearrangement
    [[SQLiteStore sharedStore] playlistMoveObjectsInArrangedObjectsFromIndexes:indexSet toIndex:insertIndex];
#endif
    
    [playbackController playlistDidChange:self];
}

- (id <NSPasteboardWriting>)tableView:(NSTableView *)tableView
               pasteboardWriterForRow:(NSInteger)row {
    NSPasteboardItem *item = (NSPasteboardItem *) [super tableView:tableView
                                            pasteboardWriterForRow:row];
    if (!item) {
        item = [[NSPasteboardItem alloc] init];
    }

    NSMutableArray *filenames = [NSMutableArray array];
    PlaylistEntry *song = [[self arrangedObjects] objectAtIndex:row];
    [filenames addObject:[[song path] stringByExpandingTildeInPath]];

    if (@available(macOS 10.13, *)) {
        [item setData:[song.URL dataRepresentation] forType:NSPasteboardTypeFileURL];
    }
    else {
        [item setPropertyList:@[song.URL] forType:NSFilenamesPboardType];
    }

    return item;
}

- (BOOL)tableView:(NSTableView *)tv
       acceptDrop:(id <NSDraggingInfo>)info
              row:(NSInteger)row
    dropOperation:(NSTableViewDropOperation)op {
    // Check if DNDArrayController handles it.
    if ([super tableView:tv acceptDrop:info row:row dropOperation:op]) return YES;

    if (row < 0) row = 0;

    // Determine the type of object that was dropped
    NSPasteboardType fileType;
    if (@available(macOS 10.13, *)) {
        fileType = NSPasteboardTypeFileURL;
    }
    else {
        fileType = NSFilenamesPboardType;
    }
    NSArray *supportedTypes =
            @[CogUrlsPboardType, fileType, iTunesDropType];
    NSPasteboard *pboard = [info draggingPasteboard];
    NSString *bestType = [pboard availableTypeFromArray:supportedTypes];

    NSMutableArray *acceptedURLs = [[NSMutableArray alloc] init];

    // Get files from an file drawer drop
    if ([bestType isEqualToString:CogUrlsPboardType]) {
        NSError *error;
        NSData *data = [pboard dataForType:CogUrlsPboardType];
        NSArray *urls;
        if (@available(macOS 11.0, *)) {
            urls = [NSKeyedUnarchiver unarchivedArrayOfObjectsOfClass:[NSURL class]
                                                             fromData:data
                                                                error:&error];
        } else {
            if (@available(macOS 10.13, *)) {
                NSSet *allowed = [NSSet setWithArray:@[[NSArray class], [NSURL class]]];
                urls = [NSKeyedUnarchiver unarchivedObjectOfClasses:allowed
                                                           fromData:data
                                                              error:&error];
            }
            else {
                urls = [NSUnarchiver unarchiveObjectWithData:data];
            }
        }
        if (!urls) {
            DLog(@"%@", error);
        } else {
            DLog(@"URLS: %@", urls);
        }
        //[playlistLoader insertURLs: urls atIndex:row sort:YES];
        [acceptedURLs addObjectsFromArray:urls];
    }

    // Get files from a normal file drop (such as from Finder)
    if ([bestType isEqualToString:fileType]) {
        NSArray<Class> *classes = @[[NSURL class]];
        NSDictionary *options = @{};
        NSArray<NSURL*> *files = [pboard readObjectsForClasses:classes options:options];

        //[playlistLoader insertURLs:urls atIndex:row sort:YES];
        [acceptedURLs addObjectsFromArray:files];
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

    if ([acceptedURLs count]) {
        [self willInsertURLs:acceptedURLs origin:URLOriginInternal];

        if (![[self content] count]) {
            row = 0;
        }

        NSArray *entries = [playlistLoader insertURLs:acceptedURLs atIndex:row sort:YES];
        [self didInsertURLs:entries origin:URLOriginInternal];
    }

    if ([self shuffle] != ShuffleOff) [self resetShuffleList];

    return YES;
}

- (NSUndoManager *)undoManager {
    return undoManager;
}

- (NSIndexSet *)disarrangeIndexes:(NSIndexSet *)indexes {
    if ([[self arrangedObjects] count] <= [indexes lastIndex]) return indexes;

    NSMutableIndexSet *disarrangedIndexes = [[NSMutableIndexSet alloc] init];

    [indexes enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL * _Nonnull stop) {
        [disarrangedIndexes addIndex:[[self content] indexOfObject:[[self arrangedObjects] objectAtIndex:idx]]];
    }];

    return disarrangedIndexes;
}

- (NSArray *)disarrangeObjects:(NSArray *)objects {
    NSMutableArray *disarrangedObjects = [[NSMutableArray alloc] init];

    for (PlaylistEntry *pe in [self content]) {
        if ([objects containsObject:pe]) [disarrangedObjects addObject:pe];
    }

    return disarrangedObjects;
}

- (NSIndexSet *)rearrangeIndexes:(NSIndexSet *)indexes {
    if ([[self content] count] <= [indexes lastIndex]) return indexes;

    NSMutableIndexSet *rearrangedIndexes = [[NSMutableIndexSet alloc] init];

    [indexes enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL * _Nonnull stop) {
        [rearrangedIndexes addIndex:[[self arrangedObjects] indexOfObject:[[self content] objectAtIndex:idx]]];
    }];

    return rearrangedIndexes;
}

- (void)insertObjects:(NSArray *)objects atIndexes:(NSIndexSet *)indexes {
    [self insertObjects:objects atArrangedObjectIndexes:indexes];
    [self rearrangeObjects];
}

- (void)insertObjectsUnsynced:(NSArray *)objects atArrangedObjectIndexes:(NSIndexSet *)indexes {
    [super insertObjects:objects atArrangedObjectIndexes:indexes];
    
    if ([self shuffle] != ShuffleOff) [self resetShuffleList];
}

- (void)insertObjects:(NSArray *)objects atArrangedObjectIndexes:(NSIndexSet *)indexes {
    [[[self undoManager] prepareWithInvocationTarget:self]
            removeObjectsAtIndexes:[self disarrangeIndexes:indexes]];
    NSString *actionName =
            [NSString stringWithFormat:@"Adding %lu entries", (unsigned long) [objects count]];
    [[self undoManager] setActionName:actionName];
    
    [[SQLiteStore sharedStore] playlistInsertTracks:objects atObjectIndexes:indexes progressCall:^(double progress) {
        [self setProgressBarStatus:progress];
    }];

    [super insertObjects:objects atArrangedObjectIndexes:indexes];
    
    if ([self shuffle] != ShuffleOff) [self resetShuffleList];
}

- (void)removeObjectsAtIndexes:(NSIndexSet *)indexes {
    [self removeObjectsAtArrangedObjectIndexes:[self rearrangeIndexes:indexes]];
}

- (void)removeObjectsAtArrangedObjectIndexes:(NSIndexSet *)indexes {
    NSArray *objects = [[self arrangedObjects] objectsAtIndexes:indexes];
    [[[self undoManager] prepareWithInvocationTarget:self]
            insertObjects:[self disarrangeObjects:objects]
                atIndexes:[self disarrangeIndexes:indexes]];
    NSString *actionName =
            [NSString stringWithFormat:@"Removing %lu entries", (unsigned long) [indexes count]];
    [[self undoManager] setActionName:actionName];

    DLog(@"Removing indexes: %@", indexes);
    DLog(@"Current index: %li", currentEntry.index);

    NSMutableIndexSet *unarrangedIndexes = [[NSMutableIndexSet alloc] init];
    for (PlaylistEntry *pe in objects) {
        [unarrangedIndexes addIndex:[pe index]];
    }

    if (currentEntry.index >= 0 && [unarrangedIndexes containsIndex:currentEntry.index]) {
        currentEntry.index = -currentEntry.index - 1;
        DLog(@"Current removed: %li", currentEntry.index);
    }

    if (currentEntry.index < 0)  // Need to update the negative index
    {
        NSInteger i = -currentEntry.index - 1;
        DLog(@"I is %li", i);
        NSInteger j;
        for (j = i - 1; j >= 0; j--) {
            if ([unarrangedIndexes containsIndex:j]) {
                DLog(@"Removing 1");
                i--;
            }
        }
        currentEntry.index = -i - 1;
    }
    
    [[SQLiteStore sharedStore] playlistRemoveTracksAtIndexes:unarrangedIndexes progressCall:^(double progress) {
        [self setProgressBarStatus:progress];
    }];

    [super removeObjectsAtArrangedObjectIndexes:indexes];

    if ([self shuffle] != ShuffleOff) [self resetShuffleList];

    [playbackController playlistDidChange:self];
}

- (void)setSortDescriptors:(NSArray *)sortDescriptors {
    DLog(@"Current: %@, setting: %@", [self sortDescriptors], sortDescriptors);

    // Cheap hack so the index column isn't sorted
    if (([sortDescriptors count] != 0) && [[sortDescriptors[0] key]
            caseInsensitiveCompare:@"index"] == NSOrderedSame) {
        // Remove the sort descriptors
        [super setSortDescriptors:@[]];
        [self rearrangeObjects];

        return;
    }

    [super setSortDescriptors:sortDescriptors];
    [self rearrangeObjects];

    [playbackController playlistDidChange:self];
}

- (IBAction)randomizeList:(id)sender {
    [self setSortDescriptors:@[]];

    NSArray *unrandomized = [self content];
    [[[self undoManager] prepareWithInvocationTarget:self] unrandomizeList:unrandomized];

    [self setContent:[Shuffle shuffleList:[self content]]];

    if ([self shuffle] != ShuffleOff) [self resetShuffleList];

    [[self undoManager] setActionName:@"Playlist Randomization"];
}

- (void)unrandomizeList:(NSArray *)entries {
    [[[self undoManager] prepareWithInvocationTarget:self] randomizeList:self];
    [self setContent:entries];
}

- (IBAction)toggleShuffle:(id)sender {
    ShuffleMode shuffle = [self shuffle];

    if (shuffle == ShuffleOff) {
        [self setShuffle:ShuffleAlbums];
    } else if (shuffle == ShuffleAlbums) {
        [self setShuffle:ShuffleAll];
    } else if (shuffle == ShuffleAll) {
        [self setShuffle:ShuffleOff];
    }
}

- (IBAction)toggleRepeat:(id)sender {
    RepeatMode repeat = [self repeat];

    if (repeat == RepeatModeNoRepeat) {
        [self setRepeat:RepeatModeRepeatOne];
    } else if (repeat == RepeatModeRepeatOne) {
        [self setRepeat:RepeatModeRepeatAlbum];
    } else if (repeat == RepeatModeRepeatAlbum) {
        [self setRepeat:RepeatModeRepeatAll];
    } else if (repeat == RepeatModeRepeatAll) {
        [self setRepeat:RepeatModeNoRepeat];
    }
}

- (PlaylistEntry *)entryAtIndex:(NSInteger)i {
    RepeatMode repeat = [self repeat];

    if (i < 0 || i >= [[self arrangedObjects] count]) {
        if (repeat != RepeatModeRepeatAll) return nil;

        while (i < 0) i += [[self arrangedObjects] count];
        if (i >= [[self arrangedObjects] count]) i %= [[self arrangedObjects] count];
    }

    return [[self arrangedObjects] objectAtIndex:i];
}

- (void)remove:(id)sender {
    // It's a kind of magic.
    // Plain old NSArrayController's remove: isn't working properly for some reason.
    // The method is definitely called but (overridden) removeObjectsAtArrangedObjectIndexes: isn't
    // called and no entries are removed. Putting explicit call to
    // removeObjectsAtArrangedObjectIndexes: here for now.
    // TODO: figure it out

    NSIndexSet *selected = [self selectionIndexes];
    if ([selected count] > 0) {
        [self removeObjectsAtArrangedObjectIndexes:selected];
    }
}

- (IBAction)removeDuplicates:(id)sender {
    NSMutableArray *originals = [[NSMutableArray alloc] init];
    NSMutableArray *duplicates = [[NSMutableArray alloc] init];

    for (PlaylistEntry *pe in [self content]) {
        if ([originals containsObject:[pe URL]])
            [duplicates addObject:pe];
        else
            [originals addObject:[pe URL]];
    }

    if ([duplicates count] > 0) {
        NSArray *arrangedContent = [self arrangedObjects];
        NSMutableIndexSet *duplicatesIndex = [[NSMutableIndexSet alloc] init];
        for (PlaylistEntry *pe in duplicates) {
            [duplicatesIndex addIndex:[arrangedContent indexOfObject:pe]];
        }
        [self removeObjectsAtArrangedObjectIndexes:duplicatesIndex];
    }
}

- (IBAction)removeDeadItems:(id)sender {
    NSMutableArray *deadItems = [[NSMutableArray alloc] init];

    for (PlaylistEntry *pe in [self content]) {
        NSURL *url = [pe URL];
        if ([url isFileURL])
            if (![[NSFileManager defaultManager] fileExistsAtPath:[url path]])
                [deadItems addObject:pe];
    }

    if ([deadItems count] > 0) {
        NSArray *arrangedContent = [self arrangedObjects];
        NSMutableIndexSet *deadItemsIndex = [[NSMutableIndexSet alloc] init];
        for (PlaylistEntry *pe in deadItems) {
            [deadItemsIndex addIndex:[arrangedContent indexOfObject:pe]];
        }
        [self removeObjectsAtArrangedObjectIndexes:deadItemsIndex];
    }
}

- (PlaylistEntry *)shuffledEntryAtIndex:(NSInteger)i {
    RepeatMode repeat = [self repeat];

    while (i < 0) {
        if (repeat == RepeatModeRepeatAll) {
            [self addShuffledListToFront];
            // change i appropriately
            i += [[self arrangedObjects] count];
        } else {
            return nil;
        }
    }
    while (i >= [shuffleList count]) {
        if (repeat == RepeatModeRepeatAll) {
            [self addShuffledListToBack];
        } else {
            return nil;
        }
    }

    return shuffleList[i];
}

- (PlaylistEntry *)getNextEntry:(PlaylistEntry *)pe {
    return [self getNextEntry:pe ignoreRepeatOne:NO];
}

- (PlaylistEntry *)getNextEntry:(PlaylistEntry *)pe ignoreRepeatOne:(BOOL)ignoreRepeatOne {
    if (!ignoreRepeatOne && [self repeat] == RepeatModeRepeatOne) {
        return pe;
    }

    if ([queueList count] > 0) {
        pe = queueList[0];
        [queueList removeObjectAtIndex:0];
        [[SQLiteStore sharedStore] queueRemoveItem:0];
        pe.queued = NO;
        [pe setQueuePosition:-1];

        int i;
        for (i = 0; i < [queueList count]; i++) {
            PlaylistEntry *queueItem = queueList[i];
            [queueItem setQueuePosition:i];
        }

        return pe;
    }

    if ([self shuffle] != ShuffleOff) {
        return [self shuffledEntryAtIndex:(pe.shuffleIndex + 1)];
    } else {
        NSInteger i;
        if (pe.index < 0)  // Was a current entry, now removed.
        {
            i = -pe.index - 1;
        } else {
            i = pe.index + 1;
        }

        if ([self repeat] == RepeatModeRepeatAlbum) {
            PlaylistEntry *next = [self entryAtIndex:i];

            if ((i > [[self arrangedObjects] count] - 1) ||
                    ([[next album] caseInsensitiveCompare:[pe album]]) || ([next album] == nil)) {
                NSArray *filtered = [self filterPlaylistOnAlbum:[pe album]];
                if ([pe album] == nil)
                    i--;
                else
                    i = [(PlaylistEntry *) filtered[0] index];
            }
        }

        return [self entryAtIndex:i];
    }
}

- (NSArray *)filterPlaylistOnAlbum:(NSString *)album {
    NSPredicate *predicate;
    if ([album length] > 0)
        predicate = [NSPredicate predicateWithFormat:@"album like %@", album];
    else
        predicate = [NSPredicate predicateWithFormat:@"album == nil"];
    return [[self arrangedObjects] filteredArrayUsingPredicate:predicate];
}

- (PlaylistEntry *)getPrevEntry:(PlaylistEntry *)pe {
    return [self getPrevEntry:pe ignoreRepeatOne:NO];
}

- (PlaylistEntry *)getPrevEntry:(PlaylistEntry *)pe ignoreRepeatOne:(BOOL)ignoreRepeatOne {
    if (!ignoreRepeatOne && [self repeat] == RepeatModeRepeatOne) {
        return pe;
    }

    if ([self shuffle] != ShuffleOff) {
        return [self shuffledEntryAtIndex:(pe.shuffleIndex - 1)];
    } else {
        NSInteger i;
        if (pe.index < 0)  // Was a current entry, now removed.
        {
            i = -pe.index - 2;
        } else {
            i = pe.index - 1;
        }

        return [self entryAtIndex:i];
    }
}

- (BOOL)next {
    PlaylistEntry *pe;

    pe = [self getNextEntry:[self currentEntry] ignoreRepeatOne:YES];

    if (pe == nil) return NO;

    [self setCurrentEntry:pe];

    return YES;
}

- (BOOL)prev {
    PlaylistEntry *pe;

    pe = [self getPrevEntry:[self currentEntry] ignoreRepeatOne:YES];
    if (pe == nil) return NO;

    [self setCurrentEntry:pe];

    return YES;
}

- (NSArray *)shuffleAlbums {
    NSArray *newList = [self arrangedObjects];
    NSMutableArray *temp = [[NSMutableArray alloc] init];
    NSMutableArray *albums = [[NSMutableArray alloc] init];
    NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"track"
                                                                   ascending:YES];
    for (unsigned long i = 0, j = [newList count]; i < j; ++i) {
        PlaylistEntry *pe = newList[i];
        NSString *album = [pe album];
        if (!album) album = @"";
        if ([albums containsObject:album]) continue;
        [albums addObject:album];
        NSArray *albumContent = [self filterPlaylistOnAlbum:album];
        NSArray *sortedContent =
                [albumContent sortedArrayUsingDescriptors:@[sortDescriptor]];
        [temp addObject:sortedContent[0]];
    }
    NSArray *tempList = [Shuffle shuffleList:temp];
    temp = [[NSMutableArray alloc] init];
    for (unsigned long i = 0, j = [tempList count]; i < j; ++i) {
        PlaylistEntry *pe = tempList[i];
        NSString *album = [pe album];
        NSArray *albumContent = [self filterPlaylistOnAlbum:album];
        NSArray *sortedContent =
                [albumContent sortedArrayUsingDescriptors:@[sortDescriptor]];
        [temp addObjectsFromArray:sortedContent];
    }
    return temp;
}

- (void)addShuffledListToFront {
    NSArray *newList;
    NSIndexSet *indexSet;

    if ([self shuffle] == ShuffleAlbums) {
        newList = [self shuffleAlbums];
    } else {
        newList = [Shuffle shuffleList:[self arrangedObjects]];
    }

    indexSet = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [newList count])];

    [shuffleList insertObjects:newList atIndexes:indexSet];

    int i;
    for (i = 0; i < [shuffleList count]; i++) {
        [shuffleList[i] setShuffleIndex:i];
    }
}

- (void)addShuffledListToBack {
    NSArray *newList;
    NSIndexSet *indexSet;

    if ([self shuffle] == ShuffleAlbums) {
        newList = [self shuffleAlbums];
    } else {
        newList = [Shuffle shuffleList:[self arrangedObjects]];
    }

    indexSet =
            [NSIndexSet indexSetWithIndexesInRange:NSMakeRange([shuffleList count], [newList count])];

    [shuffleList insertObjects:newList atIndexes:indexSet];

    unsigned long i;
    for (i = ([shuffleList count] - [newList count]); i < [shuffleList count]; i++) {
        [shuffleList[i] setShuffleIndex:(int) i];
    }
}

- (void)resetShuffleList {
    [shuffleList removeAllObjects];

    [self addShuffledListToFront];

    if (currentEntry && currentEntry.index >= 0) {
        if ([self shuffle] == ShuffleAlbums) {
            NSString *currentAlbum = currentEntry.album;
            if (!currentAlbum) currentAlbum = @"";

            NSArray *wholeAlbum = [self filterPlaylistOnAlbum:currentAlbum];

            // First prune the shuffle list of the currently playing album
            long i, j;
            for (i = 0; i < [shuffleList count];) {
                if ([wholeAlbum containsObject:shuffleList[i]]) {
                    [shuffleList removeObjectAtIndex:i];
                } else {
                    ++i;
                }
            }

            // Then insert the playing album at the start
            NSIndexSet *indexSet =
                    [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [wholeAlbum count])];

            [shuffleList insertObjects:wholeAlbum atIndexes:indexSet];

            // Oops, gotta reset the shuffle indexes
            for (i = 0, j = [shuffleList count]; i < j; ++i) {
                [shuffleList[i] setShuffleIndex:(int) i];
            }
        } else {
            [shuffleList insertObject:currentEntry atIndex:0];
            [currentEntry setShuffleIndex:0];

            // Need to rejigger so the current entry is at the start now...
            long i, j;
            BOOL found = NO;
            for (i = 1, j = [shuffleList count]; i < j && !found; i++) {
                if (shuffleList[i] == currentEntry) {
                    found = YES;
                    [shuffleList removeObjectAtIndex:i];
                } else {
                    [shuffleList[i] setShuffleIndex:(int) i];
                }
            }
        }
    }
}

- (void)setCurrentEntry:(PlaylistEntry *)pe {
    currentEntry.current = NO;
    currentEntry.stopAfter = NO;

    pe.current = YES;

    if (pe != nil) [self.tableView scrollRowToVisible:pe.index];

    currentEntry = pe;
}

- (void)setShuffle:(ShuffleMode)s {
    [[NSUserDefaults standardUserDefaults] setInteger:s forKey:@"shuffle"];
    if (s != ShuffleOff) [self resetShuffleList];

    [playbackController playlistDidChange:self];
}

- (ShuffleMode)shuffle {
    return (ShuffleMode) [[NSUserDefaults standardUserDefaults] integerForKey:@"shuffle"];
}

- (void)setRepeat:(RepeatMode)r {
    [[NSUserDefaults standardUserDefaults] setInteger:r forKey:@"repeat"];
    [playbackController playlistDidChange:self];
}

- (RepeatMode)repeat {
    return (RepeatMode) [[NSUserDefaults standardUserDefaults] integerForKey:@"repeat"];
}

- (IBAction)clear:(id)sender {
    [self setFilterPredicate:nil];

    [self
            removeObjectsAtArrangedObjectIndexes:
                    [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [[self arrangedObjects] count])]];
}

- (IBAction)clearFilterPredicate:(id)sender {
    [self setFilterPredicate:nil];
}

- (void)setFilterPredicate:(NSPredicate *)filterPredicate {
    [super setFilterPredicate:filterPredicate];
}

- (IBAction)showEntryInFinder:(id)sender {
    NSWorkspace *ws = [NSWorkspace sharedWorkspace];

    NSURL *url = [[self selectedObjects][0] URL];
    if ([url isFileURL]) [ws selectFile:[url path] inFileViewerRootedAtPath:[url path]];
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
- (IBAction)searchByArtist:(id)sender; {
    PlaylistEntry *entry = [[self arrangedObjects] objectAtIndex:[self selectionIndex]];
    [spotlightWindowController searchForArtist:[entry artist]];
}

- (IBAction)searchByAlbum:(id)sender; {
    PlaylistEntry *entry = [[self arrangedObjects] objectAtIndex:[self selectionIndex]];
    [spotlightWindowController searchForAlbum:[entry album]];
}

- (NSMutableArray *)queueList {
    return queueList;
}

- (IBAction)emptyQueueList:(id)sender {
    [self emptyQueueListUnsynced];
    [[SQLiteStore sharedStore] queueEmpty];
}

- (void)emptyQueueListUnsynced {
    for (PlaylistEntry *queueItem in queueList) {
        queueItem.queued = NO;
        [queueItem setQueuePosition:-1];
    }

    [queueList removeAllObjects];
}

- (IBAction)toggleQueued:(id)sender {
    SQLiteStore *store = [SQLiteStore sharedStore];
    
    for (PlaylistEntry *queueItem in [self selectedObjects]) {
        if (queueItem.queued) {
            queueItem.queued = NO;
            queueItem.queuePosition = -1;

            [queueList removeObject:queueItem];
            
            [store queueRemovePlaylistItems:@[queueItem]];
        } else {
            queueItem.queued = YES;
            queueItem.queuePosition = (int) [queueList count];

            [queueList addObject:queueItem];
            
            [store queueAddItem:[queueItem index]];
        }

        DLog(@"TOGGLE QUEUED: %i", queueItem.queued);
    }

    int i = 0;
    for (PlaylistEntry *cur in queueList) {
        cur.queuePosition = i++;
    }
}

- (IBAction)removeFromQueue:(id)sender {
    SQLiteStore *store = [SQLiteStore sharedStore];
    
    for (PlaylistEntry *queueItem in [self selectedObjects]) {
        queueItem.queued = NO;
        queueItem.queuePosition = -1;

        [queueList removeObject:queueItem];
        [store queueRemovePlaylistItems:@[queueItem]];
    }

    int i = 0;
    for (PlaylistEntry *cur in queueList) {
        cur.queuePosition = i++;
    }
}

- (IBAction)addToQueue:(id)sender {
    SQLiteStore *store = [SQLiteStore sharedStore];
    
    for (PlaylistEntry *queueItem in [self selectedObjects]) {
        queueItem.queued = YES;
        queueItem.queuePosition = (int) [queueList count];

        [queueList addObject:queueItem];
        [store queueAddItem:[queueItem index]];
    }

    int i = 0;
    for (PlaylistEntry *cur in queueList) {
        cur.queuePosition = i++;
    }
}

- (IBAction)stopAfterCurrent:(id)sender {
    currentEntry.stopAfter = !currentEntry.stopAfter;
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
    SEL action = [menuItem action];

    if (action == @selector(removeFromQueue:)) {
        for (PlaylistEntry *q in [self selectedObjects])
            if (q.queuePosition >= 0) return YES;

        return NO;
    }

    if (action == @selector(emptyQueueList:) && ([queueList count] < 1)) return NO;

    if (action == @selector(stopAfterCurrent:) && currentEntry.stopAfter) return NO;

    // if nothing is selected, gray out these
    if ([[self selectedObjects] count] < 1) {
        if (action == @selector(remove:)) return NO;

        if (action == @selector(addToQueue:)) return NO;

        if (action == @selector(searchByArtist:)) return NO;

        if (action == @selector(searchByAlbum:)) return NO;
    }

    return YES;
}

// Event inlets:
- (void)willInsertURLs:(NSArray *)urls origin:(URLOrigin)origin {
    if (![urls count]) return;

    CGEventRef event = CGEventCreate(NULL /*default event source*/);
    CGEventFlags mods = CGEventGetFlags(event);
    CFRelease(event);

    BOOL modifierPressed =
            ((mods & kCGEventFlagMaskCommand) != 0) & ((mods & kCGEventFlagMaskControl) != 0);
    modifierPressed |= ((mods & kCGEventFlagMaskShift) != 0);

    NSString *behavior =
            [[NSUserDefaults standardUserDefaults] valueForKey:@"openingFilesBehavior"];
    if (modifierPressed) {
        behavior =
                [[NSUserDefaults standardUserDefaults] valueForKey:@"openingFilesAlteredBehavior"];
    }

    BOOL shouldClear =
            modifierPressed;  // By default, internal sources should not clear the playlist
    if (origin == URLOriginExternal) {  // For external insertions, we look at the preference
        // possible settings are "clearAndPlay", "enqueue", "enqueueAndPlay"
        shouldClear = [behavior isEqualToString:@"clearAndPlay"];
    }

    if (shouldClear) {
        [self clear:self];
    }
}

- (void)didInsertURLs:(NSArray *)urls origin:(URLOrigin)origin {
    if (![urls count]) return;

    CGEventRef event = CGEventCreate(NULL);
    CGEventFlags mods = CGEventGetFlags(event);
    CFRelease(event);

    BOOL modifierPressed =
            ((mods & kCGEventFlagMaskCommand) != 0) & ((mods & kCGEventFlagMaskControl) != 0);
    modifierPressed |= ((mods & kCGEventFlagMaskShift) != 0);

    NSString *behavior =
            [[NSUserDefaults standardUserDefaults] valueForKey:@"openingFilesBehavior"];
    if (modifierPressed) {
        behavior =
                [[NSUserDefaults standardUserDefaults] valueForKey:@"openingFilesAlteredBehavior"];
    }

    BOOL shouldPlay = modifierPressed;  // The default is NO for internal insertions
    if (origin == URLOriginExternal) {  // For external insertions, we look at the preference
        shouldPlay = [behavior isEqualToString:@"clearAndPlay"] ||
                [behavior isEqualToString:@"enqueueAndPlay"];;
    }

    // Auto start playback
    if (shouldPlay && [[self content] count] > 0) {
        [playbackController playEntry:urls[0]];
    }
}

- (IBAction)reloadTags:(id)sender {
    NSArray * selectedobjects = [self selectedObjects];
    if ([selectedobjects count]) {
        for (PlaylistEntry *pe in selectedobjects) {
            pe.metadataLoaded = NO;
        }
        
        [playlistLoader performSelectorInBackground:@selector(loadInfoForEntries:) withObject:selectedobjects];
    }
}

@end
