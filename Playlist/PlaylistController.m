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

#import "NSString+CogSort.h"
#import "NSString+FinderCompare.h"

#import "Logging.h"

#import "Cog-Swift.h"

#import "AppController.h"
#import "SandboxBroker.h"

#define UNDO_STACK_LIMIT 0

extern BOOL kAppControllerShuttingDown;

NSPersistentContainer *kPersistentContainer = nil;

@implementation PlaylistController

@synthesize currentEntry;
@synthesize totalTime;
@synthesize currentStatus;

static NSArray *cellIdentifiers = nil;

NSMutableDictionary<NSString *, AlbumArtwork *> *kArtworkDictionary = nil;

static void *playlistControllerContext = &playlistControllerContext;

+ (void)initialize {
	cellIdentifiers = @[@"index", @"status", @"title", @"albumartist", @"artist",
		                @"album", @"length", @"year", @"genre", @"track", @"path",
		                @"filename", @"codec", @"rating", @"samplerate",
		                @"bitspersample", @"bitrate", @"composer", @"playcount"];

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
	NSDictionary *defaultsDictionary = @{ @"repeat": @(RepeatModeRepeatAll), @"shuffle": @(ShuffleOff) };

	[[NSUserDefaults standardUserDefaults] registerDefaults:defaultsDictionary];
}

- (id)initWithCoder:(NSCoder *)decoder {
	self = [super initWithCoder:decoder];
	if(!self) return nil;

	shuffleList = [[NSMutableArray alloc] init];
	queueList = [[NSMutableArray alloc] init];

	undoManager = [[NSUndoManager alloc] init];

	[undoManager setLevelsOfUndo:UNDO_STACK_LIMIT];

	[self initDefaults];

	_persistentContainer = [[NSPersistentContainer alloc] initWithName:@"DataModel"];
	[self.persistentContainer loadPersistentStoresWithCompletionHandler:^(NSPersistentStoreDescription *description, NSError *error) {
		if(error != nil) {
			ALog(@"Failed to load Core Data stack: %@", error);
			abort();
		}
	}];

	kPersistentContainer = self.persistentContainer;

	self.persistentContainer.viewContext.mergePolicy = NSMergeByPropertyObjectTrumpMergePolicy;

	_persistentArtStorage = [[NSMutableDictionary alloc] init];
	kArtworkDictionary = self.persistentArtStorage;

	return self;
}

+ (NSPersistentContainer *)sharedPersistentContainer {
	return kPersistentContainer;
}

- (void)awakeFromNib {
	[super awakeFromNib];

	statusImageTransformer = [NSValueTransformer valueTransformerForName:@"StatusImageTransformer"];
	numberHertzToStringTransformer = [NSValueTransformer valueTransformerForName:@"NumberHertzToStringTransformer"];

	NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"index" ascending:YES];
	[self.tableView setSortDescriptors:@[sortDescriptor]];

	[self addObserver:self
	       forKeyPath:@"arrangedObjects"
	          options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld)
	          context:playlistControllerContext];
	[playbackController addObserver:self
	                     forKeyPath:@"progressOverall"
	                        options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionOld)
	                        context:playlistControllerContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.fontSize" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial) context:playlistControllerContext];

	observersRegistered = YES;

	[self.tableView setDraggingSourceOperationMask:NSDragOperationCopy forLocal:NO];
}

- (void)deinit {
	if(observersRegistered) {
		[self removeObserver:self forKeyPath:@"arrangedObjects" context:playlistControllerContext];
		[playbackController removeObserver:self forKeyPath:@"progressOverall" context:playlistControllerContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.fontSize" context:playlistControllerContext];
	}
}

- (void)startObservingProgress:(NSProgress *)progress {
	[progress addObserver:self forKeyPath:@"localizedDescription" options:0 context:playlistControllerContext];
	[progress addObserver:self forKeyPath:@"fractionCompleted" options:0 context:playlistControllerContext];
}

- (void)stopObservingProgress:(NSProgress *)progress {
	[progress removeObserver:self forKeyPath:@"localizedDescription" context:playlistControllerContext];
	[progress removeObserver:self forKeyPath:@"fractionCompleted" context:playlistControllerContext];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
	if(context == playlistControllerContext) {
		if([keyPath isEqualToString:@"arrangedObjects"]) {
			[self updatePlaylistIndexes];
			[self updateTotalTime];
			[self.tableView reloadData];
		} else if([keyPath isEqualToString:@"values.fontSize"]) {
			[self updateRowSize];
		} else if([keyPath isEqualToString:@"progressOverall"]) {
			id objNew = [change objectForKey:NSKeyValueChangeNewKey];
			id objOld = [change objectForKey:NSKeyValueChangeOldKey];

			NSProgress *progressNew = nil, *progressOld = nil;

			if(objNew && [objNew isKindOfClass:[NSProgress class]])
				progressNew = (NSProgress *)objNew;
			if(objOld && [objOld isKindOfClass:[NSProgress class]])
				progressOld = (NSProgress *)objOld;

			if(progressOld) {
				[self stopObservingProgress:progressOld];
			}

			if(progressNew) {
				[self startObservingProgress:progressNew];
			}

			[self updateProgressText];
		} else if([keyPath isEqualToString:@"localizedDescription"] ||
		          [keyPath isEqualToString:@"fractionCompleted"]) {
			[self updateProgressText];
		}
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)updateProgressText {
	NSString *description = nil;
	if(playbackController.progressOverall) {
		if(playbackController.progressJob) {
			description = [NSString stringWithFormat:@"%@ - %@", playbackController.progressOverall.localizedDescription, playbackController.progressJob.localizedDescription];
		} else {
			description = playbackController.progressOverall.localizedDescription;
		}
	}

	if(description) {
		[self setTotalTime:nil];
		description = [description stringByAppendingFormat:@" - %.2f%% complete", playbackController.progressOverall.fractionCompleted * 100.0];
		[self setCurrentStatus:description];
	} else {
		[self setCurrentStatus:nil];
		[self updateTotalTime];
	}
}

- (void)commitPersistentStore {
	[self.persistentContainer.viewContext performBlockAndWait:^{
		NSError *error = nil;
		[self.persistentContainer.viewContext save:&error];
		if(error) {
			ALog(@"Error committing playlist storage: %@", [error localizedDescription]);
		}
	}];
}

- (void)updatePlayCountForTrack:(PlaylistEntry *)pe {
	if(!pe || pe.deLeted || pe.countAdded) return;
	pe.countAdded = YES;
	
	__block PlayCount *pc = pe.playCountItem;
	
	if(pc) {
		[self.persistentContainer.viewContext performBlockAndWait:^{
			pc.count += 1;
			pc.lastPlayed = [NSDate date];
		}];
	} else {
		[self.persistentContainer.viewContext performBlockAndWait:^{
			pc = [NSEntityDescription insertNewObjectForEntityForName:@"PlayCount" inManagedObjectContext:self.persistentContainer.viewContext];
			pc.count = 1;
			pc.firstSeen = pc.lastPlayed = [NSDate date];
			pc.album = pe.album;
			pc.artist = pe.artist;
			pc.title = pe.title;
			pc.filename = pe.filenameFragment;
		}];
	}
	
	[self commitPersistentStore];
	
	NSIndexSet *refreshRow = [NSIndexSet indexSetWithIndex:pe.index];
	NSIndexSet *refreshColumns = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [[self.tableView tableColumns] count])];

	[self.tableView reloadDataForRowIndexes:refreshRow columnIndexes:refreshColumns];
}

- (void)firstSawTrack:(PlaylistEntry *)pe {
	__block PlayCount *pc = pe.playCountItem;

	if(!pc) {
		[self.persistentContainer.viewContext performBlockAndWait:^{
			pc = [NSEntityDescription insertNewObjectForEntityForName:@"PlayCount" inManagedObjectContext:self.persistentContainer.viewContext];
			pc.count = 0;
			pc.firstSeen = [NSDate date];
			pc.album = pe.album;
			pc.artist = pe.artist;
			pc.title = pe.title;
			pc.filename = pe.filenameFragment;
		}];
	}

	NSIndexSet *refreshRow = [NSIndexSet indexSetWithIndex:pe.index];
	NSIndexSet *refreshColumns = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [[self.tableView tableColumns] count])];

	[self.tableView reloadDataForRowIndexes:refreshRow columnIndexes:refreshColumns];
}

- (void)ratingUpdatedWithEntry:(PlaylistEntry *)pe rating:(CGFloat)rating {
	if(pe && !pe.deLeted) {
		__block PlayCount *pc = pe.playCountItem;

		if(!pc) {
			[self.persistentContainer.viewContext performBlockAndWait:^{
				pc = [NSEntityDescription insertNewObjectForEntityForName:@"PlayCount" inManagedObjectContext:self.persistentContainer.viewContext];
				pc.count = 0;
				pc.firstSeen = [NSDate date];
				pc.album = pe.album;
				pc.artist = pe.artist;
				pc.title = pe.title;
				pc.filename = pe.filenameFragment;
			}];
		}

		[self.persistentContainer.viewContext performBlockAndWait:^{
			pc.rating = rating;
		}];

		[self commitPersistentStore];

		NSIndexSet *refreshRow = [NSIndexSet indexSetWithIndex:pe.index];
		NSIndexSet *refreshColumns = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [[self.tableView tableColumns] count])];

		[self.tableView reloadDataForRowIndexes:refreshRow columnIndexes:refreshColumns];
	}
}

- (void)resetPlayCountForTrack:(PlaylistEntry *)pe {
	PlayCount *pc = pe.playCountItem;
	
	if(pc) {
		[self.persistentContainer.viewContext performBlockAndWait:^{
			pc.count = 0;
		}];
	}
}

- (void)removeRatingForTrack:(PlaylistEntry *)pe {
	PlayCount *pc = pe.playCountItem;

	if(pc) {
		[self.persistentContainer.viewContext performBlockAndWait:^{
			pc.rating = 0;
		}];
	}
}

- (void)updatePlaylistIndexes {
	NSArray *arranged = [self arrangedObjects];
	NSUInteger n = [arranged count];
	BOOL updated = NO;
	for(NSUInteger i = 0; i < n; i++) {
		PlaylistEntry *pe = arranged[i];
		if(pe.index != i) { // Make sure we don't get into some kind of crazy observing loop...
			pe.index = i;
			updated = YES;
		}
	}
	if(updated) {
		[self commitPersistentStore];
	}
}

- (void)updateTotalTime {
	double tt = 0;
	ldiv_t hoursAndMinutes;
	ldiv_t daysAndHours;
	ldiv_t weeksAndDays;

	for(PlaylistEntry *pe in [self arrangedObjects]) {
		if(pe && !pe.deLeted && pe.length) {
			if(!isnan([pe.length doubleValue])) tt += [pe.length doubleValue];
		}
	}

	long sec = (long)(tt);
	hoursAndMinutes = ldiv(sec / 60, 60);
    
    NSString *week = NSLocalizedString(@"%1d week(s)", @"week for total");
    NSString *day = NSLocalizedString(@"%1d day(s)", @"day for total");
    NSString *hour = NSLocalizedString(@"%1d hour(s)", @"hour for total");
    NSString *min = NSLocalizedString(@"%1d minute(s)", @"minute for total");
    NSString *second = NSLocalizedString(@"%1d second(s)", @"second for total");

	if(hoursAndMinutes.quot >= 24) {
		daysAndHours = ldiv(hoursAndMinutes.quot, 24);
		if(daysAndHours.quot >= 7) {
			weeksAndDays = ldiv(daysAndHours.quot, 7);

			NSString *weekCount = [NSString localizedStringWithFormat:week, weeksAndDays.quot];
			NSString *dayCount = [NSString localizedStringWithFormat:day, weeksAndDays.rem];
			NSString *hourCount = [NSString localizedStringWithFormat:hour, daysAndHours.rem];
			NSString *minuteCount = [NSString localizedStringWithFormat:min, hoursAndMinutes.rem];
			NSString *secCount = [NSString localizedStringWithFormat:second, sec % 60];

			[self setTotalTime:[NSString stringWithFormat:
			                    NSLocalizedString(@"wdhms", "weeks, days, hours, minutes and seconds"),
			                                              weekCount, dayCount, hourCount, minuteCount, secCount]];
		} else {
			NSString *dayCount = [NSString localizedStringWithFormat:day, daysAndHours.quot];
			NSString *hourCount = [NSString localizedStringWithFormat:hour, daysAndHours.rem];
			NSString *minuteCount = [NSString localizedStringWithFormat:min, hoursAndMinutes.rem];
			NSString *secCount = [NSString localizedStringWithFormat:second, sec % 60];

			[self setTotalTime:[NSString stringWithFormat:
			                    NSLocalizedString(@"dhms", "days, hours, minutes and seconds"),
			                                              dayCount, hourCount, minuteCount, secCount]];
		}
	} else {
		if(hoursAndMinutes.quot > 0) {
			NSString *hourCount = [NSString localizedStringWithFormat:hour, hoursAndMinutes.quot];
			NSString *minuteCount = [NSString localizedStringWithFormat:min, hoursAndMinutes.rem];
			NSString *secCount = [NSString localizedStringWithFormat:second, sec % 60];

			[self setTotalTime:[NSString stringWithFormat:
			                    NSLocalizedString(@"hms", "hours, minutes and seconds"),
			                                              hourCount, minuteCount, secCount]];
		} else {
			if(hoursAndMinutes.rem > 0) {
				NSString *minuteCount = [NSString localizedStringWithFormat:min, hoursAndMinutes.rem];
				NSString *secCount = [NSString localizedStringWithFormat:second, sec % 60];

				[self setTotalTime:[NSString stringWithFormat:
				                    NSLocalizedString(@"ms", "minutes and seconds"),
				                                              minuteCount, secCount]];
			} else {
				NSString *secCount = [NSString localizedStringWithFormat:second, sec];

				[self setTotalTime:secCount];
			}
		}
	}
}

- (NSView *_Nullable)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *_Nullable)tableColumn row:(NSInteger)row {
	// What the heck
	if(row < 0 || row >= [[self arrangedObjects] count]) {
		return nil;
	}

	NSImage *cellImage = nil;
	NSString *cellText = @"";
	NSString *cellIdentifier = @"";
	NSString *cellToolTip = nil;
	NSTextAlignment cellTextAlignment = NSTextAlignmentLeft;

	PlaylistEntry *pe = [[self arrangedObjects] objectAtIndex:row];

	float fontSize = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] floatForKey:@"fontSize"];
	
	if(pe) {
		cellIdentifier = [tableColumn identifier];
		NSUInteger index = [cellIdentifiers indexOfObject:cellIdentifier];

		switch(index) {
			case 0:
				cellText = [NSString stringWithFormat:@"%lld", pe.index + 1];
				cellTextAlignment = NSTextAlignmentRight;
				break;

			case 1:
				cellImage = [statusImageTransformer transformedValue:pe.status];
				break;

			case 2:
				if([pe title]) cellText = pe.title;
				break;

			case 3:
				if([pe albumartist]) cellText = pe.albumartist;
				break;

			case 4:
				if([pe artist]) cellText = pe.artist;
				break;

			case 5:
				if([pe album]) cellText = pe.album;
				break;

			case 6:
				cellText = pe.lengthText;
				cellTextAlignment = NSTextAlignmentRight;
				cellToolTip = pe.lengthInfo;
				break;

			case 7:
				if([pe year]) cellText = pe.yearText;
				cellTextAlignment = NSTextAlignmentRight;
				break;

			case 8:
				if([pe genre]) cellText = pe.genre;
				break;

			case 9:
				if([pe track]) cellText = pe.trackText;
				cellTextAlignment = NSTextAlignmentRight;
				break;

			case 10:
				if([pe path]) cellText = pe.path;
				break;

			case 11:
				if([pe filename]) cellText = pe.filename;
				break;

			case 12:
				if([pe codec]) cellText = pe.codec;
				break;
				
			case 13:
			{
				NSString *filledStar = @"★";
				NSString *emptyStar = @"☆";
				NSUInteger rating = (NSUInteger)ceil(pe.rating);
				if(rating < 0)
					rating = 0;
				else if(rating > 5)
					rating = 5;
				cellText = [@"" stringByPaddingToLength:rating withString:filledStar startingAtIndex:0];
				cellText = [cellText stringByPaddingToLength:5 withString:emptyStar startingAtIndex:0];
				break;
			}

			case 14:
				cellText = [numberHertzToStringTransformer transformedValue:@(pe.sampleRate)];
				cellTextAlignment = NSTextAlignmentRight;
				break;

			case 15:
				cellText = [NSString stringWithFormat:@"%u", pe.bitsPerSample];
				cellTextAlignment = NSTextAlignmentRight;
				break;

			case 16:
				cellText = [NSString stringWithFormat:@"%u kbps", pe.bitrate];
				cellTextAlignment = NSTextAlignmentRight;
				break;

			case 17:
				if([pe composer]) cellText = pe.composer;
				break;

			case 18:
				cellText = pe.playCount;
				cellTextAlignment = NSTextAlignmentRight;
				break;
		}
	}

	NSString *cellTextTruncated = cellText;
	if([cellTextTruncated length] > 1024) {
		cellTextTruncated = [cellTextTruncated substringToIndex:1023];
		cellTextTruncated = [cellTextTruncated stringByAppendingString:@"…"];
	}

	NSView *view = [tableView makeViewWithIdentifier:cellIdentifier owner:nil];
	if(view && [view isKindOfClass:[NSTableCellView class]]) {
		NSTableCellView *cellView = (NSTableCellView *)view;
		NSRect frameRect = cellView.frame;
		frameRect.origin.y = 1;
		frameRect.size.height = tableView.rowHeight;
		cellView.frame = frameRect;

		if(cellView.textField) {
			cellView.textField.allowsDefaultTighteningForTruncation = YES;

			NSFont *font = [NSFont monospacedDigitSystemFontOfSize:fontSize weight:NSFontWeightRegular];

			cellView.textField.font = font;
			cellView.textField.stringValue = cellTextTruncated ?: @"";
			cellView.textField.alignment = cellTextAlignment;

			if(cellView.textField.intrinsicContentSize.width > cellView.textField.frame.size.width - 4)
				cellView.textField.toolTip = cellTextTruncated ?: @"";
			else
				cellView.textField.toolTip = [pe statusMessage];

			if(cellToolTip) {
				cellView.textField.toolTip = cellToolTip;
			}

			NSRect cellFrameRect = cellView.textField.frame;
			cellFrameRect.origin.y = 1;
			cellFrameRect.size.height = frameRect.size.height;
			cellView.textField.frame = cellFrameRect;
		}
		if(cellView.imageView) {
			cellView.imageView.image = cellImage;
			cellView.imageView.toolTip = [pe statusMessage];

			NSRect cellFrameRect = cellView.imageView.frame;
			cellFrameRect.size.height = frameRect.size.height * 14.0 / 18.0;
			cellFrameRect.origin.y = (frameRect.size.height - cellFrameRect.size.height) * 0.5;
			cellView.imageView.frame = cellFrameRect;
		}

		cellView.rowSizeStyle = NSTableViewRowSizeStyleCustom;
	}

	return view;
}

- (void)tableView:(NSTableView *)view didClickRow:(NSInteger)clickedRow column:(NSInteger)clickedColumn atPoint:(NSPoint)cellPoint {
	NSTableColumn *column = [view tableColumns][clickedColumn];
	NSString *cellIdentifier = [column identifier];
	NSUInteger index = [cellIdentifiers indexOfObject:cellIdentifier];
	if(index == 13) {
		NSInteger rating = ((CGFloat)ceil(cellPoint.x * 5.0 / 64.0));
		if(rating < 1) rating = 1;
		else if(rating > 5) rating = 5;

		PlaylistEntry *pe = [[self arrangedObjects] objectAtIndex:clickedRow];

		[self ratingUpdatedWithEntry:pe rating:rating];

		NSIndexSet *refreshRow = [NSIndexSet indexSetWithIndex:clickedRow];
		NSIndexSet *refreshColumn = [NSIndexSet indexSetWithIndex:clickedColumn];

		[self.tableView reloadDataForRowIndexes:refreshRow columnIndexes:refreshColumn];
	}
}

- (void)updateRowSize {
	[self.tableView reloadData];
	if(currentEntry != nil) [self.tableView scrollRowToVisible:currentEntry.index];
}

- (void)updateNextAfterDeleted:(PlaylistEntry *)lastEntry withDeleteIndexes:(NSIndexSet *)indexes {
	__block PlaylistEntry *pe = nil;
	NSArray *allObjects = [self arrangedObjects];
	[indexes enumerateRangesUsingBlock:^(NSRange range, BOOL *_Nonnull stop) {
		if(range.location <= lastEntry.index &&
		   range.location + range.length > lastEntry.index) {
			NSUInteger index = range.location + range.length;
			if(index < [allObjects count])
				pe = [allObjects objectAtIndex:index];
			else
				pe = nil;
		} else if(pe && range.location <= [pe index] &&
		          range.location + range.length > [pe index]) {
			NSUInteger index = range.location + range.length;
			if(index < [allObjects count])
				pe = [allObjects objectAtIndex:index];
			else
				pe = nil;
		} else if(pe && range.location > [pe index]) {
			*stop = YES;
		}
	}];
	nextEntryAfterDeleted = pe;
}

- (void)tableView:(NSTableView *)tableView didClickTableColumn:(NSTableColumn *)tableColumn {
	if([self shuffle] != ShuffleOff) [self resetShuffleList];

	NSUInteger index = [cellIdentifiers indexOfObject:[tableColumn identifier]];

	NSSortDescriptor *sortDescriptor;/* = [tableColumn sortDescriptorPrototype];*/
	NSArray *sortDescriptors;/* = sortDescriptor ? @[sortDescriptor] : @[];*/

	BOOL ascending;

	NSImage *indicatorImage = [tableView indicatorImageInTableColumn:tableColumn];
	NSImage *sortAscending = [NSImage imageNamed:@"NSAscendingSortIndicator"];
	NSImage *sortDescending = [NSImage imageNamed:@"NSDescendingSortIndicator"];

	if(indicatorImage == sortAscending) {
		[tableView setIndicatorImage:sortDescending inTableColumn:tableColumn];
		ascending = NO;
	} else {
		[tableView setIndicatorImage:sortAscending inTableColumn:tableColumn];
		ascending = YES;
	}

	/*
	 
	 - status - compare
	 1

	 - length, year, samplerate, bitspersample - numeric
	 6, 7, 14, 15

	 - playcount vs "playCount" - numeric
	 18

	 - title, albumartist, artist, album, genre, path, filename, codec, composer - strings, finderCompare
	 2, 3, 4, 5, 8, 10, 11, 12, 17

	 - rating, bitrate, numeric with text decorators - compare
	 13, 16
	 
	 - track - special sorting
	 9

	 */
	switch(index) {
		default:
		case 0:
			sortDescriptors = @[];
			break;

		case 2:
		case 3:
		case 4:
		case 5:
		case 8:
		case 10:
		case 11:
		case 12:
		case 17:
			sortDescriptor = [[NSSortDescriptor alloc] initWithKey:[tableColumn identifier] ascending:ascending selector:@selector(finderCompare:)];
			sortDescriptors = @[sortDescriptor];
			break;

		case 6:
		case 7:
		case 14:
		case 15:
			sortDescriptor = [[NSSortDescriptor alloc] initWithKey:[tableColumn identifier] ascending:ascending selector:@selector(compareTrackNumbers:)];
			sortDescriptors = @[sortDescriptor];
			break;

			// field name differs from cell name, but still numeric
		case 18:
			sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"playCount" ascending:ascending selector:@selector(compareTrackNumbers:)];
			sortDescriptors = @[sortDescriptor];
			break;

		case 1:
		case 13:
		case 16:
			sortDescriptor = [[NSSortDescriptor alloc] initWithKey:[tableColumn identifier] ascending:ascending selector:@selector(compare:)];
			sortDescriptors = @[sortDescriptor];
			break;

		case 9: {
			// Unfortunately, this makes the column header bug out. No way around it.
			sortDescriptors = @[
				[[NSSortDescriptor alloc] initWithKey:@"albumartist"
				                            ascending:ascending
				                             selector:@selector(finderCompare:)],
				[[NSSortDescriptor alloc] initWithKey:@"album"
				                            ascending:ascending
				                             selector:@selector(finderCompare:)],
				[[NSSortDescriptor alloc] initWithKey:@"disc" // Yes, this, even though it's not actually a column
				                            ascending:ascending
				                             selector:@selector(compareTrackNumbers:)],
				[[NSSortDescriptor alloc] initWithKey:@"track"
				                            ascending:ascending
				                             selector:@selector(compareTrackNumbers:)]
			];
		}
	}

	[self setSortDescriptors:sortDescriptors];
}

// This action is only needed to revert the one that follows it
- (void)moveObjectsFromIndex:(NSUInteger)fromIndex
     toArrangedObjectIndexes:(NSIndexSet *)indexSet {
	[[[self undoManager] prepareWithInvocationTarget:self]
	moveObjectsInArrangedObjectsFromIndexes:indexSet
	                                toIndex:fromIndex];
	NSString *actionName =
	[NSString stringWithFormat:@"Reordering %lu entries", (unsigned long)[indexSet count]];
	[[self undoManager] setActionName:actionName];

	[super moveObjectsFromIndex:fromIndex toArrangedObjectIndexes:indexSet];

	[playbackController playlistDidChange:self];
}

- (void)moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet *)indexSet
                                        toIndex:(NSUInteger)insertIndex {
	[[[self undoManager] prepareWithInvocationTarget:self]
	   moveObjectsFromIndex:insertIndex
	toArrangedObjectIndexes:indexSet];
	NSString *actionName =
	[NSString stringWithFormat:@"Reordering %lu entries", (unsigned long)[indexSet count]];
	[[self undoManager] setActionName:actionName];

	[super moveObjectsInArrangedObjectsFromIndexes:indexSet toIndex:insertIndex];

	[playbackController playlistDidChange:self];
}

- (id<NSPasteboardWriting>)tableView:(NSTableView *)tableView
              pasteboardWriterForRow:(NSInteger)row {
	NSPasteboardItem *item = (NSPasteboardItem *)[super tableView:tableView
	                                       pasteboardWriterForRow:row];
	if(!item) {
		item = [[NSPasteboardItem alloc] init];
	}

	PlaylistEntry *song = [[self arrangedObjects] objectAtIndex:row];

	if(song.url != nil) {
		[item setData:[song.url dataRepresentation] forType:NSPasteboardTypeFileURL];
	}

	return item;
}

- (BOOL)tableView:(NSTableView *)tv
       acceptDrop:(id<NSDraggingInfo>)info
              row:(NSInteger)row
    dropOperation:(NSTableViewDropOperation)op {
	// Check if DNDArrayController handles it.
	if([super tableView:tv acceptDrop:info row:row dropOperation:op]) return YES;

	if(row < 0) row = 0;

	// Determine the type of object that was dropped
	NSArray *supportedTypes =
	@[CogUrlsPboardType, NSPasteboardTypeFileURL, iTunesDropType];
	NSPasteboard *pboard = [info draggingPasteboard];
	NSString *bestType = [pboard availableTypeFromArray:supportedTypes];

	NSMutableArray *acceptedURLs = [[NSMutableArray alloc] init];

	// Get files from an file drawer drop
	if([bestType isEqualToString:CogUrlsPboardType]) {
		NSError *error;
		NSData *data = [pboard dataForType:CogUrlsPboardType];
		NSArray *urls;
		if(@available(macOS 11.0, *)) {
			urls = [NSKeyedUnarchiver unarchivedArrayOfObjectsOfClass:[NSURL class]
			                                                 fromData:data
			                                                    error:&error];
		} else {
			NSSet *allowed = [NSSet setWithArray:@[[NSArray class], [NSURL class]]];
			urls = [NSKeyedUnarchiver unarchivedObjectOfClasses:allowed
			                                           fromData:data
			                                              error:&error];
		}
		if(!urls) {
			DLog(@"%@", error);
		} else {
			DLog(@"URLS: %@", urls);
		}
		//[playlistLoader insertURLs: urls atIndex:row sort:YES];
		[acceptedURLs addObjectsFromArray:urls];
	}

	// Get files from a normal file drop (such as from Finder)
	if([bestType isEqualToString:NSPasteboardTypeFileURL]) {
		NSArray<Class> *classes = @[[NSURL class]];
		NSDictionary *options = @{};
		NSArray<NSURL *> *files = [pboard readObjectsForClasses:classes options:options];

		//[playlistLoader insertURLs:urls atIndex:row sort:YES];
		[acceptedURLs addObjectsFromArray:files];
	}

	// Get files from an iTunes drop
	if([bestType isEqualToString:iTunesDropType]) {
		NSDictionary *iTunesDict = [pboard propertyListForType:iTunesDropType];
		NSDictionary *tracks = [iTunesDict valueForKey:@"Tracks"];

		// Convert the iTunes URLs to URLs....MWAHAHAH!
		NSMutableArray *urls = [[NSMutableArray alloc] init];

		for(NSDictionary *trackInfo in [tracks allValues]) {
			[urls addObject:[NSURL URLWithString:[trackInfo valueForKey:@"Location"]]];
		}

		//[playlistLoader insertURLs:urls atIndex:row sort:YES];
		[acceptedURLs addObjectsFromArray:urls];
	}

	if([acceptedURLs count]) {
		if(![[self content] count]) {
			row = 0;
		}

		NSDictionary *loadEntriesData = @{ @"entries": acceptedURLs,
			                               @"index": @(row),
			                               @"sort": @(YES),
			                               @"origin": @(URLOriginInternal) };

		[self performSelectorInBackground:@selector(insertURLsInBackground:) withObject:loadEntriesData];
	}

	return YES;
}

- (NSUndoManager *)undoManager {
	return undoManager;
}

- (NSIndexSet *)disarrangeIndexes:(NSIndexSet *)indexes {
	NSMutableIndexSet *disarrangedIndexes = [[NSMutableIndexSet alloc] init];

	[indexes enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *_Nonnull stop) {
		NSUInteger arrCount = [[self arrangedObjects] count];
		NSUInteger disCount = [[self content] count];
		if(idx >= arrCount) {
			[disarrangedIndexes addIndex:idx - arrCount + disCount];
		} else {
			[disarrangedIndexes addIndex:[[self content] indexOfObject:[[self arrangedObjects] objectAtIndex:idx]]];
		}
	}];

	return disarrangedIndexes;
}

- (NSArray *)disarrangeObjects:(NSArray *)objects {
	NSMutableArray *disarrangedObjects = [[NSMutableArray alloc] init];

	for(PlaylistEntry *pe in [self content]) {
		if([objects containsObject:pe]) [disarrangedObjects addObject:pe];
	}

	return disarrangedObjects;
}

- (NSIndexSet *)rearrangeIndexes:(NSIndexSet *)indexes {
	if([[self content] count] <= [indexes lastIndex]) return indexes;

	NSMutableIndexSet *rearrangedIndexes = [[NSMutableIndexSet alloc] init];

	[indexes enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *_Nonnull stop) {
		[rearrangedIndexes addIndex:[[self arrangedObjects] indexOfObject:[[self content] objectAtIndex:idx]]];
	}];

	return rearrangedIndexes;
}

- (void)insertObjects:(NSArray *)objects atIndexes:(NSIndexSet *)indexes {
	[self clearFilterPredicate:self];
	[self insertObjects:objects atArrangedObjectIndexes:indexes];
}

- (void)untrashObjects:(NSArray *)objects atIndexes:(NSIndexSet *)indexes {
	[self clearFilterPredicate:self];
	[self untrashObjects:objects atArrangedObjectIndexes:indexes];
	[self rearrangeObjects];
}

- (void)insertObjectsUnsynced:(NSArray *)objects atArrangedObjectIndexes:(NSIndexSet *)indexes {
	[super insertObjects:objects atArrangedObjectIndexes:indexes];
	[self rearrangeObjects];

	if([self shuffle] != ShuffleOff) [self resetShuffleList];
}

- (void)insertObjects:(NSArray *)objects atArrangedObjectIndexes:(NSIndexSet *)indexes {
	// Special case, somehow someone is undoing while a search filter is in place
	__block NSUInteger index = NSNotFound;
	[indexes enumerateRangesUsingBlock:^(NSRange range, BOOL * _Nonnull stop) {
		if(range.location < index) {
			index = range.location;
		}
	}];
	NSUInteger count = [[self arrangedObjects] count];
	if(index > count) {
		// Ah, oops, bodge fix
		__block NSMutableIndexSet *replacementIndexes = [[NSMutableIndexSet alloc] init];
		[indexes enumerateRangesUsingBlock:^(NSRange range, BOOL * _Nonnull stop) {
			[replacementIndexes addIndexesInRange:NSMakeRange(range.location - index + count, range.length)];
		}];
		indexes = replacementIndexes;
	}

	[[[self undoManager] prepareWithInvocationTarget:self]
	removeObjectsAtIndexes:[self disarrangeIndexes:indexes]];
	NSString *actionName =
	[NSString stringWithFormat:@"Adding %lu entries", (unsigned long)[objects count]];
	[[self undoManager] setActionName:actionName];

	for(PlaylistEntry *pe in objects) {
		pe.deLeted = NO;
	}

	// further bodge fix
	@try {
		[super insertObjects:objects atArrangedObjectIndexes:indexes];
	}
	@catch(NSException *e) {
		// Even further bodge fix
		@try {
			count = [[self arrangedObjects] count];
			indexes = [NSMutableIndexSet indexSetWithIndexesInRange:NSMakeRange(count, [objects count])];
			[super insertObjects:objects atArrangedObjectIndexes:indexes];
		}
		@catch(NSException *e) {
			DLog(@"Exception thrown adding tracks to the playlist: %@", e);
		}
	}

	[self commitPersistentStore];

	if([self shuffle] != ShuffleOff) [self resetShuffleList];
}

- (void)untrashObjects:(NSArray *)objects atArrangedObjectIndexes:(NSIndexSet *)indexes {
	[[[self undoManager] prepareWithInvocationTarget:self]
	trashObjectsAtIndexes:[self disarrangeIndexes:indexes]];
	NSString *actionName =
	[NSString stringWithFormat:@"Restoring %lu entries from trash", (unsigned long)[objects count]];
	[[self undoManager] setActionName:actionName];

	for(PlaylistEntry *pe in objects) {
		if(pe.deLeted && pe.trashUrl) {
			NSError *error = nil;
			[[NSFileManager defaultManager] moveItemAtURL:pe.trashUrl toURL:pe.url error:&error];
		}
		pe.deLeted = NO;
		pe.trashUrl = nil;
	}

	[super insertObjects:objects atArrangedObjectIndexes:indexes];

	[self commitPersistentStore];

	if([self shuffle] != ShuffleOff) [self resetShuffleList];
}

- (void)removeObjectsAtIndexes:(NSIndexSet *)indexes {
	[self removeObjectsAtArrangedObjectIndexes:[self rearrangeIndexes:indexes]];
}

- (void)trashObjectsAtIndexes:(NSIndexSet *)indexes {
	[self trashObjectsAtArrangedObjectIndexes:[self rearrangeIndexes:indexes]];
}

- (void)removeObjectsAtArrangedObjectIndexes:(NSIndexSet *)indexes {
	NSArray *objects = [[self arrangedObjects] objectsAtIndexes:indexes];
	[[[self undoManager] prepareWithInvocationTarget:self]
	insertObjects:[self disarrangeObjects:objects]
	    atIndexes:[self disarrangeIndexes:indexes]];
	NSString *actionName =
	[NSString stringWithFormat:@"Removing %lu entries", (unsigned long)[indexes count]];
	[[self undoManager] setActionName:actionName];

	DLog(@"Removing indexes: %@", indexes);
	DLog(@"Current index: %lli", currentEntry.index);

	NSMutableIndexSet *unarrangedIndexes = [[NSMutableIndexSet alloc] init];
	for(PlaylistEntry *pe in objects) {
		[unarrangedIndexes addIndex:[pe index]];
		pe.deLeted = YES;
	}

	if([indexes containsIndex:currentEntry.index]) {
		[self updateNextAfterDeleted:currentEntry withDeleteIndexes:indexes];
	} else if(nextEntryAfterDeleted &&
	          [indexes containsIndex:nextEntryAfterDeleted.index]) {
		[self updateNextAfterDeleted:nextEntryAfterDeleted withDeleteIndexes:indexes];
	}

	if(currentEntry.index >= 0 && [unarrangedIndexes containsIndex:currentEntry.index]) {
		currentEntry.index = -currentEntry.index - 1;
		DLog(@"Current removed: %lli", currentEntry.index);
	}

	if(currentEntry.index < 0) // Need to update the negative index
	{
		NSInteger i = -currentEntry.index - 1;
		DLog(@"I is %li", i);
		NSInteger j;
		for(j = i - 1; j >= 0; j--) {
			if([unarrangedIndexes containsIndex:j]) {
				DLog(@"Removing 1");
				i--;
			}
		}
		currentEntry.index = -i - 1;
	}

	[super removeObjectsAtArrangedObjectIndexes:indexes];

	[self commitPersistentStore];

	if([self shuffle] != ShuffleOff) [self resetShuffleList];

	[playbackController playlistDidChange:self];
}

- (void)trashObjectsAtArrangedObjectIndexes:(NSIndexSet *)indexes {
	NSArray *objects = [[self arrangedObjects] objectsAtIndexes:indexes];
	[[[self undoManager] prepareWithInvocationTarget:self]
	untrashObjects:[self disarrangeObjects:objects]
	     atIndexes:[self disarrangeIndexes:indexes]];
	NSString *actionName =
	[NSString stringWithFormat:@"Trashing %lu entries", (unsigned long)[indexes count]];
	[[self undoManager] setActionName:actionName];

	DLog(@"Trashing indexes: %@", indexes);
	DLog(@"Current index: %lli", currentEntry.index);

	NSMutableIndexSet *unarrangedIndexes = [[NSMutableIndexSet alloc] init];
	for(PlaylistEntry *pe in objects) {
		[unarrangedIndexes addIndex:[pe index]];
		pe.deLeted = YES;
	}

	if([indexes containsIndex:currentEntry.index]) {
		[self updateNextAfterDeleted:currentEntry withDeleteIndexes:indexes];
		if(nextEntryAfterDeleted) {
			[playbackController playEntry:nextEntryAfterDeleted];
			nextEntryAfterDeleted = nil;
		} else {
			[playbackController stop:nil];
		}
	}

	[super removeObjectsAtArrangedObjectIndexes:indexes];

	[self commitPersistentStore];

	if([self shuffle] != ShuffleOff) [self resetShuffleList];

	[playbackController playlistDidChange:self];

	for(PlaylistEntry *pe in objects) {
		if([pe.url isFileURL]) {
			NSURL *removed = nil;
			NSError *error = nil;
			[[NSFileManager defaultManager] trashItemAtURL:pe.url resultingItemURL:&removed error:&error];
			pe.trashUrl = removed;
		}
	}
}

- (void)setSortDescriptors:(NSArray *)sortDescriptors {
	DLog(@"Current: %@, setting: %@", [self sortDescriptors], sortDescriptors);

	// Cheap hack so the index column isn't sorted
	if([sortDescriptors count] != 0) {
		if([[((NSSortDescriptor *)(sortDescriptors[0])) key] caseInsensitiveCompare:@"index"] == NSOrderedSame) {
			// Remove the sort descriptors
			[super setSortDescriptors:@[]];
			[self rearrangeObjects];

			return;
		}
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

	if([self shuffle] != ShuffleOff) [self resetShuffleList];

	[[self undoManager] setActionName:NSLocalizedString(@"PlaylistRandomizationAction", @"")];
}

- (void)unrandomizeList:(NSArray *)entries {
	[[[self undoManager] prepareWithInvocationTarget:self] randomizeList:self];
	[self setContent:entries];
}

- (IBAction)toggleShuffle:(id)sender {
	ShuffleMode shuffle = [self shuffle];

	if(shuffle == ShuffleOff) {
		[self setShuffle:ShuffleAlbums];
	} else if(shuffle == ShuffleAlbums) {
		[self setShuffle:ShuffleAll];
	} else if(shuffle == ShuffleAll) {
		[self setShuffle:ShuffleOff];
	}
}

- (IBAction)toggleRepeat:(id)sender {
	RepeatMode repeat = [self repeat];

	if(repeat == RepeatModeNoRepeat) {
		[self setRepeat:RepeatModeRepeatOne];
	} else if(repeat == RepeatModeRepeatOne) {
		[self setRepeat:RepeatModeRepeatAlbum];
	} else if(repeat == RepeatModeRepeatAlbum) {
		[self setRepeat:RepeatModeRepeatAll];
	} else if(repeat == RepeatModeRepeatAll) {
		[self setRepeat:RepeatModeNoRepeat];
	}
}

- (PlaylistEntry *)entryAtIndex:(NSInteger)i {
	if([[self arrangedObjects] count] == 0) return nil;

	RepeatMode repeat = [self repeat];

	if(i < 0 || i >= [[self arrangedObjects] count]) {
		if(repeat != RepeatModeRepeatAll) return nil;

		while(i < 0) i += [[self arrangedObjects] count];
		if(i >= [[self arrangedObjects] count]) i %= [[self arrangedObjects] count];
	}

	return [[self arrangedObjects] objectAtIndex:i];
}

- (IBAction)remove:(id)sender {
	// It's a kind of magic.
	// Plain old NSArrayController's remove: isn't working properly for some reason.
	// The method is definitely called but (overridden) removeObjectsAtArrangedObjectIndexes: isn't
	// called and no entries are removed. Putting explicit call to
	// removeObjectsAtArrangedObjectIndexes: here for now.
	// TODO: figure it out

	NSIndexSet *selected = [self selectionIndexes];
	if([selected count] > 0) {
		[self removeObjectsAtArrangedObjectIndexes:selected];
	}
}

- (IBAction)trash:(id)sender {
	// Someone asked for this, so they're getting it.
	// Trash the selection, and advance playback to the next untrashed file if necessary.

	NSIndexSet *selected = [self selectionIndexes];
	if([selected count] > 0) {
		[self trashObjectsAtArrangedObjectIndexes:selected];
	}
}

- (IBAction)removeDuplicates:(id)sender {
	NSMutableArray *originals = [[NSMutableArray alloc] init];
	NSMutableArray *duplicates = [[NSMutableArray alloc] init];

	for(PlaylistEntry *pe in [self content]) {
		if([originals containsObject:pe.url])
			[duplicates addObject:pe];
		else
			[originals addObject:pe.url];
	}

	if([duplicates count] > 0) {
		NSArray *arrangedContent = [self arrangedObjects];
		NSMutableIndexSet *duplicatesIndex = [[NSMutableIndexSet alloc] init];
		for(PlaylistEntry *pe in duplicates) {
			[duplicatesIndex addIndex:[arrangedContent indexOfObject:pe]];
		}
		[self removeObjectsAtArrangedObjectIndexes:duplicatesIndex];
	}
}

- (IBAction)removeDeadItems:(id)sender {
	NSMutableArray *deadItems = [[NSMutableArray alloc] init];

	for(PlaylistEntry *pe in [self content]) {
		NSURL *url = pe.url;
		if([url isFileURL])
			if(![[NSFileManager defaultManager] fileExistsAtPath:[url path]])
				[deadItems addObject:pe];
	}

	if([deadItems count] > 0) {
		NSArray *arrangedContent = [self arrangedObjects];
		NSMutableIndexSet *deadItemsIndex = [[NSMutableIndexSet alloc] init];
		for(PlaylistEntry *pe in deadItems) {
			[deadItemsIndex addIndex:[arrangedContent indexOfObject:pe]];
		}
		[self removeObjectsAtArrangedObjectIndexes:deadItemsIndex];
	}
}

- (PlaylistEntry *)shuffledEntryAtIndex:(NSInteger)i {
	RepeatMode repeat = [self repeat];

	while(i < 0) {
		if(repeat == RepeatModeRepeatAll) {
			[self addShuffledListToFront];
			// change i appropriately
			i += [[self arrangedObjects] count];
		} else {
			return nil;
		}
	}
	while(i >= [shuffleList count]) {
		if(repeat == RepeatModeRepeatAll) {
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
	BOOL selectionFollows = [[NSUserDefaults standardUserDefaults] boolForKey:@"selectionFollowsPlayback"];

	if(!ignoreRepeatOne && [[NSUserDefaults standardUserDefaults] boolForKey:@"alwaysStopAfterCurrent"]) {
		if(selectionFollows) {
			PlaylistEntry *nextPe = [self getNextEntry:pe ignoreRepeatOne:YES];
			[appController selectTrack:nextPe];
		}
		return nil;
	}

	if(!ignoreRepeatOne && [self repeat] == RepeatModeRepeatOne) {
		if(selectionFollows) {
			[appController selectTrack:pe];
		}
		return pe;
	}

	if([queueList count] > 0) {
		pe = queueList[0];
		[queueList removeObjectAtIndex:0];
		pe.queued = NO;
		[pe setQueuePosition:-1];

		int i;
		for(i = 0; i < [queueList count]; i++) {
			PlaylistEntry *queueItem = queueList[i];
			[queueItem setQueuePosition:i];
		}

		[self commitPersistentStore];

		if(selectionFollows) {
			[appController selectTrack:pe];
		}

		return pe;
	}

	if([self shuffle] != ShuffleOff) {
		PlaylistEntry *nextPe = [self shuffledEntryAtIndex:(pe.shuffleIndex + 1)];
		if(selectionFollows && nextPe) {
			[appController selectTrack:nextPe];
		}
		return nextPe;
	} else {
		NSInteger i;

		if(pe.deLeted) // Was a current entry, now removed.
		{
			if(nextEntryAfterDeleted)
				i = nextEntryAfterDeleted.index;
			else
				i = 0;
			nextEntryAfterDeleted = nil;
		} else {
			i = pe.index + 1;
		}

		if([self repeat] == RepeatModeRepeatAlbum) {
			PlaylistEntry *next = [self entryAtIndex:i];

			if((i > [[self arrangedObjects] count] - 1) ||
			   ([[next album] caseInsensitiveCompare:[pe album]]) || ([next album] == nil)) {
				NSArray *filtered = [self filterPlaylistOnAlbum:[pe album]];
				if([pe album] == nil || !filtered || [filtered count] < 1)
					i--;
				else
					i = [(PlaylistEntry *)filtered[0] index];
			}
		}

		PlaylistEntry *nextPe = [self entryAtIndex:i];

		if(selectionFollows) {
			[appController selectTrack:nextPe];
		}

		return nextPe;
	}
}

- (NSArray *)filterPlaylistOnAlbum:(NSString *)album {
	NSPredicate *predicate;
	if([album length] > 0)
		predicate = [NSPredicate predicateWithFormat:@"album == %@", album];
	else
		predicate = [NSPredicate predicateWithFormat:@"album == nil || album == %@", @""];

	return [[self arrangedObjects] filteredArrayUsingPredicate:predicate];
}

- (PlaylistEntry *)getPrevEntry:(PlaylistEntry *)pe {
	return [self getPrevEntry:pe ignoreRepeatOne:NO];
}

- (PlaylistEntry *)getPrevEntry:(PlaylistEntry *)pe ignoreRepeatOne:(BOOL)ignoreRepeatOne {
	BOOL selectionFollows = [[NSUserDefaults standardUserDefaults] boolForKey:@"selectionFollowsPlayback"];

	if(!ignoreRepeatOne && [[NSUserDefaults standardUserDefaults] boolForKey:@"alwaysStopAfterCurrent"]) {
		if(selectionFollows) {
			PlaylistEntry *prevPe = [self getPrevEntry:pe ignoreRepeatOne:YES];
			[appController selectTrack:prevPe];
		}
		return nil;
	}

	if(!ignoreRepeatOne && [self repeat] == RepeatModeRepeatOne) {
		if(selectionFollows) {
			[appController selectTrack:pe];
		}
		return pe;
	}

	if([self shuffle] != ShuffleOff) {
		PlaylistEntry *prevPe = [self shuffledEntryAtIndex:(pe.shuffleIndex - 1)];
		if(selectionFollows && prevPe) {
			[appController selectTrack:prevPe];
		}
		return prevPe;
	} else {
		NSInteger i;
		if(pe.index < 0) // Was a current entry, now removed.
		{
			i = -pe.index - 2;
		} else {
			i = pe.index - 1;
		}

		PlaylistEntry *prevPe = [self entryAtIndex:i];
		if(selectionFollows) {
			[appController selectTrack:prevPe];
		}

		return prevPe;
	}
}

- (BOOL)next {
	PlaylistEntry *pe;

	pe = [self getNextEntry:[self currentEntry] ignoreRepeatOne:YES];

	if(pe == nil) return NO;

	[self setCurrentEntry:pe];

	return YES;
}

- (BOOL)prev {
	PlaylistEntry *pe;

	pe = [self getPrevEntry:[self currentEntry] ignoreRepeatOne:YES];
	if(pe == nil) return NO;

	[self setCurrentEntry:pe];

	return YES;
}

- (NSArray *)shuffleAlbums {
	NSArray *newList = [self arrangedObjects];
	NSMutableArray *temp = [[NSMutableArray alloc] init];
	NSSortDescriptor *sortDescriptorTrack = [[NSSortDescriptor alloc] initWithKey:@"track"
	                                                                    ascending:YES];
	NSSortDescriptor *sortDescriptorDisc = [[NSSortDescriptor alloc] initWithKey:@"disc"
	                                                                   ascending:YES];
	NSArray *albums = [newList valueForKey:@"album"];
	albums = [[NSSet setWithArray:albums] allObjects];
	NSArray *tempList = [Shuffle shuffleList:albums];
	temp = [[NSMutableArray alloc] init];
	BOOL blankAdded = NO;
	for(NSString *album in tempList) {
		NSString *theAlbum = album;
		if((id)album == [NSNull null]) {
			if(blankAdded)
				continue;
			else
				theAlbum = @"";
			blankAdded = YES;
		} else if([album isEqualToString:@""]) {
			if(blankAdded) continue;
			blankAdded = YES;
		}
		NSArray *albumContent = [self filterPlaylistOnAlbum:theAlbum];
		NSArray *sortedContent =
		[albumContent sortedArrayUsingDescriptors:@[sortDescriptorDisc, sortDescriptorTrack]];
		if(sortedContent && [sortedContent count])
			[temp addObjectsFromArray:sortedContent];
	}
	return temp;
}

- (void)readQueueFromDataStore {
	NSPredicate *hasUrlPredicate = [NSPredicate predicateWithFormat:@"urlString != nil && urlString != %@", @""];
	NSPredicate *deletedPredicate = [NSPredicate predicateWithFormat:@"deLeted == NO || deLeted == nil"];
	NSPredicate *queuedPredicate = [NSPredicate predicateWithFormat:@"queued == YES"];

	NSSortDescriptor *sortDescriptor = [NSSortDescriptor sortDescriptorWithKey:@"queuePosition" ascending:YES];

	NSCompoundPredicate *predicate = [NSCompoundPredicate andPredicateWithSubpredicates:@[deletedPredicate, hasUrlPredicate, queuedPredicate]];
	
	NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"PlaylistEntry"];
	request.predicate = predicate;
	request.sortDescriptors = @[sortDescriptor];

	[self.persistentContainer.viewContext performBlockAndWait:^{
		NSError *error = nil;
		NSArray *results = [self.persistentContainer.viewContext executeFetchRequest:request error:&error];

		if(results && [results count] > 0) {
			[queueList removeAllObjects];
			NSIndexSet *indexSet = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [results count])];
			[queueList insertObjects:results atIndexes:indexSet];
		}
	}];
}

- (void)readShuffleListFromDataStore {
	NSPredicate *predicate = [NSPredicate predicateWithFormat:@"deLeted == NO || deLeted == nil || urlString == nil"];

	NSSortDescriptor *sortDescriptor = [NSSortDescriptor sortDescriptorWithKey:@"shuffleIndex" ascending:YES];

	NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"PlaylistEntry"];
	request.predicate = predicate;
	request.sortDescriptors = @[sortDescriptor];

	[self.persistentContainer.viewContext performBlockAndWait:^{
		NSError *error = nil;
		NSArray *results = [self.persistentContainer.viewContext executeFetchRequest:request error:&error];

		if(results && [results count] > 0) {
			[shuffleList removeAllObjects];
			NSIndexSet *indexSet = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [results count])];
			[shuffleList insertObjects:results atIndexes:indexSet];
		}
	}];
}

- (void)addShuffledListToFront {
	NSArray *newList;
	NSIndexSet *indexSet;

	if([self shuffle] == ShuffleAlbums) {
		newList = [self shuffleAlbums];
	} else {
		newList = [Shuffle shuffleList:[self arrangedObjects]];
	}

	indexSet = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [newList count])];

	[shuffleList insertObjects:newList atIndexes:indexSet];

	int i;
	for(i = 0; i < [shuffleList count]; i++) {
		[shuffleList[i] setShuffleIndex:i];
	}
	[self commitPersistentStore];
}

- (void)addShuffledListToBack {
	NSArray *newList;
	NSIndexSet *indexSet;

	if([self shuffle] == ShuffleAlbums) {
		newList = [self shuffleAlbums];
	} else {
		newList = [Shuffle shuffleList:[self arrangedObjects]];
	}

	indexSet =
	[NSIndexSet indexSetWithIndexesInRange:NSMakeRange([shuffleList count], [newList count])];

	[shuffleList insertObjects:newList atIndexes:indexSet];

	unsigned long i;
	for(i = ([shuffleList count] - [newList count]); i < [shuffleList count]; i++) {
		[shuffleList[i] setShuffleIndex:(int)i];
	}
	[self commitPersistentStore];
}

- (void)resetShuffleList {
	[shuffleList removeAllObjects];

	[self addShuffledListToFront];

	if(currentEntry && currentEntry.index >= 0) {
		if([self shuffle] == ShuffleAlbums) {
			NSString *currentAlbum = currentEntry.album;
			if(!currentAlbum) currentAlbum = @"";

			NSArray *wholeAlbum = [self filterPlaylistOnAlbum:currentAlbum];

			// First prune the shuffle list of the currently playing album
			long i, j;
			for(i = 0; i < [shuffleList count];) {
				if([wholeAlbum containsObject:shuffleList[i]]) {
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
			for(i = 0, j = [shuffleList count]; i < j; ++i) {
				[shuffleList[i] setShuffleIndex:(int)i];
			}
			[self commitPersistentStore];
		} else {
			[shuffleList insertObject:currentEntry atIndex:0];
			[currentEntry setShuffleIndex:0];

			// Need to rejigger so the current entry is at the start now...
			long i, j;
			BOOL found = NO;
			for(i = 1, j = [shuffleList count]; i < j && !found; i++) {
				if(shuffleList[i] == currentEntry) {
					found = YES;
					[shuffleList removeObjectAtIndex:i];
				} else {
					[shuffleList[i] setShuffleIndex:(int)i];
				}
			}
			[self commitPersistentStore];
		}
	}
}

- (void)setCurrentEntry:(PlaylistEntry *)pe {
	if(pe == currentEntry || kAppControllerShuttingDown) return;

	if(currentEntry) {
		currentEntry.current = NO;
		currentEntry.stopAfter = NO;
		currentEntry.currentPosition = 0.0;
		currentEntry.countAdded = NO;
	}

	if(pe) {
		pe.current = YES;
	}

	[self commitPersistentStore];

	NSMutableIndexSet *refreshSet = [[NSMutableIndexSet alloc] init];

	if(currentEntry != nil && !currentEntry.deLeted && currentEntry.index < NSNotFound) [refreshSet addIndex:currentEntry.index];
	if(pe != nil && !pe.deLeted && pe.index < NSNotFound) [refreshSet addIndex:pe.index];

	// Refresh entire row to refresh tooltips
	unsigned long columns = [[self.tableView tableColumns] count];
	[self.tableView reloadDataForRowIndexes:refreshSet columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, columns)]];

	if(pe != nil) [self.tableView scrollRowToVisible:pe.index];

	currentEntry = pe;
}

- (void)setShuffle:(ShuffleMode)s {
	[[NSUserDefaults standardUserDefaults] setInteger:s forKey:@"shuffle"];
	if(s != ShuffleOff) [self resetShuffleList];

	[playbackController playlistDidChange:self];
}

- (ShuffleMode)shuffle {
	return (ShuffleMode)[[NSUserDefaults standardUserDefaults] integerForKey:@"shuffle"];
}

- (void)setRepeat:(RepeatMode)r {
	[[NSUserDefaults standardUserDefaults] setInteger:r forKey:@"repeat"];
	[playbackController playlistDidChange:self];
}

- (RepeatMode)repeat {
	return (RepeatMode)[[NSUserDefaults standardUserDefaults] integerForKey:@"repeat"];
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
	if([[self selectedObjects] count] == 0) return;

	NSWorkspace *ws = [NSWorkspace sharedWorkspace];

	NSURL *url = [[self selectedObjects][0] url];
	if([url isFileURL]) [ws selectFile:[url path] inFileViewerRootedAtPath:[url path]];
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

- (NSMutableArray *)queueList {
	return queueList;
}

- (IBAction)emptyQueueList:(id)sender {
	[self emptyQueueListUnsynced];
}

- (void)emptyQueueListUnsynced {
	NSMutableIndexSet *refreshSet = [[NSMutableIndexSet alloc] init];

	for(PlaylistEntry *queueItem in queueList) {
		queueItem.queued = NO;
		[queueItem setQueuePosition:-1];
		[refreshSet addIndex:[queueItem index]];
	}

	[queueList removeAllObjects];

	[self commitPersistentStore];

	// Refresh entire row to refresh tooltips
	unsigned long columns = [[self.tableView tableColumns] count];
	[self.tableView reloadDataForRowIndexes:refreshSet columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, columns)]];
}

- (IBAction)toggleQueued:(id)sender {
	NSMutableIndexSet *refreshSet = [[NSMutableIndexSet alloc] init];

	for(PlaylistEntry *queueItem in [self selectedObjects]) {
		if(queueItem.queued) {
			[queueList removeObject:queueItem];

			queueItem.queued = NO;
			queueItem.queuePosition = -1;
		} else {
			queueItem.queued = YES;
			queueItem.queuePosition = (int)[queueList count];

			[queueList addObject:queueItem];
		}

		[refreshSet addIndex:[queueItem index]];

		DLog(@"TOGGLE QUEUED: %i", queueItem.queued);
	}

	for(PlaylistEntry *queueItem in queueList) {
		if(![[self selectedObjects] containsObject:queueItem])
			[refreshSet addIndex:[queueItem index]];
	}

	int i = 0;
	for(PlaylistEntry *cur in queueList) {
		cur.queuePosition = i++;
	}

	[self commitPersistentStore];

	// Refresh entire row to refresh tooltips
	unsigned long columns = [[self.tableView tableColumns] count];
	[self.tableView reloadDataForRowIndexes:refreshSet columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, columns)]];
}

- (IBAction)removeFromQueue:(id)sender {
	NSMutableIndexSet *refreshSet = [[NSMutableIndexSet alloc] init];

	for(PlaylistEntry *queueItem in [self selectedObjects]) {
		queueItem.queued = NO;
		queueItem.queuePosition = -1;

		[queueList removeObject:queueItem];

		[refreshSet addIndex:[queueItem index]];
	}

	for(PlaylistEntry *queueItem in queueList) {
		[refreshSet addIndex:[queueItem index]];
	}

	int i = 0;
	for(PlaylistEntry *cur in queueList) {
		cur.queuePosition = i++;
	}

	[self commitPersistentStore];

	// Refresh entire row to refresh tooltips
	unsigned long columns = [[self.tableView tableColumns] count];
	[self.tableView reloadDataForRowIndexes:refreshSet columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, columns)]];
}

- (IBAction)addToQueue:(id)sender {
	NSMutableIndexSet *refreshSet = [[NSMutableIndexSet alloc] init];

	for(PlaylistEntry *queueItem in [self selectedObjects]) {
		queueItem.queued = YES;
		queueItem.queuePosition = (int)[queueList count];

		[queueList addObject:queueItem];
	}

	for(PlaylistEntry *queueItem in queueList) {
		[refreshSet addIndex:[queueItem index]];
	}

	int i = 0;
	for(PlaylistEntry *cur in queueList) {
		cur.queuePosition = i++;
	}

	[self commitPersistentStore];

	// Refresh entire row to refresh tooltips
	unsigned long columns = [[self.tableView tableColumns] count];
	[self.tableView reloadDataForRowIndexes:refreshSet columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, columns)]];
}

- (IBAction)stopAfterCurrent:(id)sender {
	currentEntry.stopAfter = !currentEntry.stopAfter;
	[self commitPersistentStore];

	NSIndexSet *refreshSet = [NSIndexSet indexSetWithIndex:[currentEntry index]];

	// Refresh entire row to refresh tooltips
	unsigned long columns = [[self.tableView tableColumns] count];
	[self.tableView reloadDataForRowIndexes:refreshSet columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, columns)]];
}

- (IBAction)stopAfterSelection:(id)sender {
	NSMutableIndexSet *refreshSet = [[NSMutableIndexSet alloc] init];

	for(PlaylistEntry *pe in [self selectedObjects]) {
		pe.stopAfter = !pe.stopAfter;
		[refreshSet addIndex:pe.index];
	}

	[self commitPersistentStore];

	// Refresh entire row of all affected items to update tooltips
	unsigned long columns = [[self.tableView tableColumns] count];
	[self.tableView reloadDataForRowIndexes:refreshSet columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, columns)]];
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
	SEL action = [menuItem action];

	if(action == @selector(removeFromQueue:)) {
		for(PlaylistEntry *q in [self selectedObjects])
			if(q.queuePosition >= 0) return YES;

		return NO;
	}

	if(action == @selector(emptyQueueList:) && ([queueList count] < 1)) return NO;

	if(action == @selector(stopAfterCurrent:) && currentEntry.stopAfter) return NO;

	// if nothing is selected, gray out these
	if([[self selectedObjects] count] < 1) {
		if(action == @selector(remove:)) return NO;

		if(action == @selector(addToQueue:)) return NO;

		if(action == @selector(toggleQueued:)) return NO;

		if(action == @selector(searchByArtist:)) return NO;

		if(action == @selector(searchByAlbum:)) return NO;

		if(action == @selector(stopAfterSelection:)) return NO;
	}

	return YES;
}

// Asynchronous event inlets:
- (void)addURLsInBackground:(NSDictionary *)input {
	NSUInteger row = [[self content] count];

	NSMutableDictionary *_input = [input mutableCopy];
	[_input setObject:@(row) forKey:@"index"];

	[self insertURLsInBackground:_input];
}

static inline void dispatch_sync_reentrant(dispatch_queue_t queue, dispatch_block_t block) {
	if(dispatch_queue_get_label(queue) == dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL)) {
		block();
	} else {
		dispatch_sync(queue, block);
	}
}

- (void)insertURLsInBackground:(NSDictionary *)input {
	[self performSelectorOnMainThread:@selector(clearFilterPredicate:) withObject:self waitUntilDone:YES];
	
	NSArray *entries = [input objectForKey:@"entries"];
	NSUInteger row = [[input objectForKey:@"index"] integerValue];
	BOOL sort = [[input objectForKey:@"sort"] boolValue];
	URLOrigin origin = (URLOrigin)[[input objectForKey:@"origin"] integerValue];

	NSUInteger countNow = [[self content] count];

	dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
		[self willInsertURLs:entries origin:origin];
	});

	if(countNow && row && ![[self content] count]) {
		row = 0;
	}

	NSArray *urlsAccepted = [playlistLoader insertURLs:entries atIndex:row sort:sort];
	dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
		[self didInsertURLs:urlsAccepted origin:origin];
	});

	dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
		if([self shuffle] != ShuffleOff) [self resetShuffleList];
	});
}

// Event inlets:
- (void)willInsertURLs:(NSArray *)urls origin:(URLOrigin)origin {
	if(![urls count]) return;

	CGEventRef event = CGEventCreate(NULL /*default event source*/);
	CGEventFlags mods = CGEventGetFlags(event);
	CFRelease(event);

	BOOL modifierPressed =
	((mods & kCGEventFlagMaskCommand) != 0) & ((mods & kCGEventFlagMaskControl) != 0);
	modifierPressed |= ((mods & kCGEventFlagMaskShift) != 0);

	NSString *behavior =
	[[NSUserDefaults standardUserDefaults] valueForKey:@"openingFilesBehavior"];
	if(modifierPressed) {
		behavior =
		[[NSUserDefaults standardUserDefaults] valueForKey:@"openingFilesAlteredBehavior"];
	}

	BOOL shouldClear =
	modifierPressed; // By default, internal sources should not clear the playlist
	if(origin == URLOriginExternal) { // For external insertions, we look at the preference
		// possible settings are "clearAndPlay", "enqueue", "enqueueAndPlay"
		shouldClear = [behavior isEqualToString:@"clearAndPlay"];
	}

	if(shouldClear) {
		[self clear:self];
	}
}

- (void)didInsertURLs:(NSArray *)urls origin:(URLOrigin)origin {
	if(![urls count]) return;

	NSArray *nsurls = [urls valueForKey:@"url"];
	if(![[SandboxBroker sharedSandboxBroker] areAllPathsSafe:nsurls]) {
		[appController showPathSuggester];
	}

	CGEventRef event = CGEventCreate(NULL);
	CGEventFlags mods = CGEventGetFlags(event);
	CFRelease(event);

	BOOL modifierPressed =
	((mods & kCGEventFlagMaskCommand) != 0) & ((mods & kCGEventFlagMaskControl) != 0);
	modifierPressed |= ((mods & kCGEventFlagMaskShift) != 0);

	NSString *behavior =
	[[NSUserDefaults standardUserDefaults] valueForKey:@"openingFilesBehavior"];
	if(modifierPressed) {
		behavior =
		[[NSUserDefaults standardUserDefaults] valueForKey:@"openingFilesAlteredBehavior"];
	}

	BOOL shouldPlay = modifierPressed; // The default is NO for internal insertions
	if(origin == URLOriginExternal) { // For external insertions, we look at the preference
		shouldPlay = [behavior isEqualToString:@"clearAndPlay"] ||
		             [behavior isEqualToString:@"enqueueAndPlay"];
		;
	}

	// Auto start playback
	if(shouldPlay && [[self content] count] > 0) {
		[playbackController playEntry:urls[0]];
	}
}

- (IBAction)reloadTags:(id)sender {
	NSArray *selectedobjects = [self selectedObjects];
	if([selectedobjects count]) {
		for(PlaylistEntry *pe in selectedobjects) {
			pe.metadataLoaded = NO;
		}

		[playlistLoader performSelectorInBackground:@selector(loadInfoForEntries:) withObject:selectedobjects];
	}
}

- (IBAction)resetPlaycounts:(id)sender {
	NSArray *selectedobjects = [self selectedObjects];
	if([selectedobjects count]) {
		NSMutableIndexSet *refreshRows = [[NSMutableIndexSet alloc] init];

		for(PlaylistEntry *pe in selectedobjects) {
			[self resetPlayCountForTrack:pe];
			if(pe.index >= 0 && pe.index < NSNotFound) {
				[refreshRows addIndex:pe.index];
			}
		}
		[self commitPersistentStore];

		NSIndexSet *refreshColumns = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [[self.tableView tableColumns] count])];

		[self.tableView reloadDataForRowIndexes:refreshRows columnIndexes:refreshColumns];
	}
}

- (IBAction)removeRatings:(id)sender {
	NSArray *selectedobjects = [self selectedObjects];
	if([selectedobjects count]) {
		for(PlaylistEntry *pe in selectedobjects) {
			[self removeRatingForTrack:pe];
		}
		[self commitPersistentStore];

		NSMutableIndexSet *refreshRows = [[NSMutableIndexSet alloc] init];

		for(PlaylistEntry *pe in selectedobjects) {
			if(pe.index >= 0 && pe.index < NSNotFound) {
				[refreshRows addIndex:pe.index];
			}
		}

		// Refresh entire row to refresh tooltips
		NSIndexSet *refreshColumns = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [[self.tableView tableColumns] count])];

		[self.tableView reloadDataForRowIndexes:refreshRows columnIndexes:refreshColumns];
	}
}

- (BOOL)pathSuggesterEmpty {
	__block BOOL rval;

	NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"SandboxToken"];

	[self.persistentContainer.viewContext performBlockAndWait:^{
		NSError *error = nil;
		NSArray *results = [self.persistentContainer.viewContext executeFetchRequest:request error:&error];
		if(!results || [results count] < 1)
			rval = YES;
		else
			rval = NO;
	}];

	return rval;
}

@end
