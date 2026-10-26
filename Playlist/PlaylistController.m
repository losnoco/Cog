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
#import "Shuffle.h"
#import "SpotlightWindowController.h"

#import "NSString+CogSort.h"
#import "NSString+FinderCompare.h"

#import "Logging.h"

#import "Cog-Swift.h"

#import "AppController.h"
#import "SandboxBroker.h"

#define UNDO_STACK_LIMIT 0

extern BOOL kAppControllerShuttingDown;

NSPersistentContainer *kPersistentContainer = nil;

// validateMenuItem: belongs to NSMenuItemValidation now; without saying so,
// it matches the deprecated informal declaration on NSObject.
@interface PlaylistController () <NSMenuItemValidation>
@end

@implementation PlaylistController

@synthesize currentEntry;
@synthesize totalTime;
@synthesize currentStatus;

static NSArray *cellIdentifiers = nil;


static void *playlistControllerContext = &playlistControllerContext;

+ (void)initialize {
	cellIdentifiers = @[@"index", @"status", @"title", @"albumartist", @"artist",
		                @"album", @"length", @"date", @"genre", @"track", @"path",
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

	NSValueTransformer *repeatModeImageTransformer = [RepeatModeImageTransformer new];
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

	NSValueTransformer *shuffleImageTransformer = [ShuffleImageTransformer new];
	[NSValueTransformer setValueTransformer:shuffleImageTransformer
	                                forName:@"ShuffleImageTransformer"];

	NSValueTransformer *statusImageTransformer = [StatusImageTransformer new];
	[NSValueTransformer setValueTransformer:statusImageTransformer
	                                forName:@"StatusImageTransformer"];

	NSValueTransformer *toggleQueueTitleTransformer = [ToggleQueueTitleTransformer new];
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

	undoManager = [NSUndoManager new];

	[undoManager setLevelsOfUndo:UNDO_STACK_LIMIT];

	[self initDefaults];

	// Spotlight's results controller is one of these too: it shares the
	// playlist's store rather than opening another.
	if(kPersistentContainer) {
		_persistentContainer = kPersistentContainer;
		__weak PlaylistController *weakSelf = self;
		_playbackOrder = [[PlaybackOrder alloc] initWithDefaults:[NSUserDefaults standardUserDefaults]
		                                                    save:^{
			                                                    [weakSelf commitPersistentStoreAsync];
		                                                    }];
		return self;
	}

	// The model CogPlaylist compiles, whose entities are its classes; the store
	// stays where it always was.
	_persistentContainer = [[NSPersistentContainer alloc] initWithName:@"DataModel" managedObjectModel:PlaylistStore.model];
	[self.persistentContainer loadPersistentStoresWithCompletionHandler:^(NSPersistentStoreDescription *description, NSError *error) {
		if(error != nil) {
			ALog(@"Failed to load Core Data stack: %@", error);
			abort();
		}
	}];

	kPersistentContainer = self.persistentContainer;

	self.persistentContainer.viewContext.mergePolicy = NSMergeByPropertyObjectTrumpMergePolicy;

	// Where entries keep their album art; it takes in the stored pictures as
	// the playlist loads.
	ArtworkStore.shared = [[ArtworkStore alloc] initWithContext:self.persistentContainer.viewContext];

	__weak PlaylistController *weakSelf = self;
	_playbackOrder = [[PlaybackOrder alloc] initWithDefaults:[NSUserDefaults standardUserDefaults]
	                                                    save:^{
		                                                    [weakSelf commitPersistentStoreAsync];
	                                                    }];

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
	[playbackController addObserver:self
	                     forKeyPath:@"progressMetadata"
	                        options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionOld)
	                        context:playlistControllerContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.fontSize" options:(NSKeyValueObservingOptionNew | NSKeyValueObservingOptionInitial) context:playlistControllerContext];

	observersRegistered = YES;

	__weak PlaylistController *weakSelf = self;
	self.playbackOrder.onFollow = ^(PlaylistEntry *pe) {
		PlaylistController *strongSelf = weakSelf;
		if(strongSelf) [strongSelf->appController selectTrack:pe];
	};

	[self.tableView setDraggingSourceOperationMask:NSDragOperationCopy forLocal:NO];
}

- (void)deinit {
	if(observersRegistered) {
		[self removeObserver:self forKeyPath:@"arrangedObjects" context:playlistControllerContext];
		[playbackController removeObserver:self forKeyPath:@"progressOverall" context:playlistControllerContext];
		[playbackController removeObserver:self forKeyPath:@"progressMetadata" context:playlistControllerContext];
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
		} else if([keyPath isEqualToString:@"progressOverall"] ||
		          [keyPath isEqualToString:@"progressMetadata"]) {
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
	NSProgress *progress = playbackController.progressOverall;
	if(progress) {
		if(playbackController.progressJob) {
			description = [NSString stringWithFormat:@"%@ - %@", progress.localizedDescription, playbackController.progressJob.localizedDescription];
		} else {
			description = progress.localizedDescription;
		}
	} else {
		// Tags loading in the background show while nothing else does
		progress = playbackController.progressMetadata;
		description = progress.localizedDescription;
	}

	if(description) {
		[self setTotalTime:nil];
		description = [description stringByAppendingFormat:@" - %.2f%% complete", progress.fractionCompleted * 100.0];
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

- (void)commitPersistentStoreAsync {
	[self.persistentContainer.viewContext performBlock:^{
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
	
	[self commitPersistentStoreAsync];
	
	[self reloadRowsForEntries:@[pe]];
}

- (void)firstSawTrack:(PlaylistEntry *)pe {
	[self firstSawTrackWithoutReload:pe];

	[self reloadRowsForEntries:@[pe]];
}

- (void)firstSawTrackWithoutReload:(PlaylistEntry *)pe {
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
}

// firstSawTrackWithoutReload: for many entries at once: the play counts
// they may have are fetched together, where each entry would fetch up to
// three times on its own.
- (void)firstSawTracksWithoutReload:(NSArray<PlaylistEntry *> *)entries {
	if(![entries count]) return;

	NSManagedObjectContext *moc = self.persistentContainer.viewContext;

	NSMutableSet *titles = [NSMutableSet new];
	NSMutableSet *filenames = [NSMutableSet new];
	for(PlaylistEntry *pe in entries) {
		if(pe.title) [titles addObject:pe.title];
		if(pe.filenameFragment) [filenames addObject:pe.filenameFragment];
		if(pe.filename) [filenames addObject:pe.filename];
	}

	// Tags as playCountItem matches them: album, artist and title.
	NSArray * (^tagKey)(NSString *, NSString *, NSString *) = ^NSArray *(NSString *album, NSString *artist, NSString *title) {
		return @[album ?: [NSNull null], artist ?: [NSNull null], title ?: [NSNull null]];
	};

	NSMutableSet *knownTags = [NSMutableSet new];
	NSMutableSet *knownFilenames = [NSMutableSet new];

	[moc performBlockAndWait:^{
		NSError *error = nil;
		if([titles count]) {
			NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"PlayCount"];
			request.predicate = [NSPredicate predicateWithFormat:@"title IN %@", titles];
			for(PlayCount *pc in [moc executeFetchRequest:request error:&error]) {
				[knownTags addObject:tagKey(pc.album, pc.artist, pc.title)];
			}
		}
		if([filenames count]) {
			NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"PlayCount"];
			request.predicate = [NSPredicate predicateWithFormat:@"filename IN %@", filenames];
			for(PlayCount *pc in [moc executeFetchRequest:request error:&error]) {
				if(pc.filename) [knownFilenames addObject:pc.filename];
			}
		}
	}];

	for(PlaylistEntry *pe in entries) {
		if(!pe.title) {
			// No title to look for together.
			[self firstSawTrackWithoutReload:pe];
			continue;
		}
		NSArray *key = tagKey(pe.album, pe.artist, pe.title);
		if([knownTags containsObject:key]) continue;
		if((pe.filenameFragment && [knownFilenames containsObject:pe.filenameFragment]) ||
		   (pe.filename && [knownFilenames containsObject:pe.filename])) {
			// Counted under its file name only, which playCountItem tags.
			[self firstSawTrackWithoutReload:pe];
			[knownTags addObject:key];
			continue;
		}
		[moc performBlockAndWait:^{
			PlayCount *pc = [NSEntityDescription insertNewObjectForEntityForName:@"PlayCount" inManagedObjectContext:moc];
			pc.count = 0;
			pc.firstSeen = [NSDate date];
			pc.album = pe.album;
			pc.artist = pe.artist;
			pc.title = pe.title;
			pc.filename = pe.filenameFragment;
		}];
		[knownTags addObject:key];
		if(pe.filenameFragment) [knownFilenames addObject:pe.filenameFragment];
	}
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

		[self commitPersistentStoreAsync];

		[self reloadRowsForEntries:@[pe]];
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
	// A search shows only some entries; their places in the playlist stay.
	if([self filterPredicate]) return;
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
		[self commitPersistentStoreAsync];
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
				if([pe date]) cellText = pe.date;
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

		NSFont *font = [NSFont monospacedDigitSystemFontOfSize:fontSize weight:NSFontWeightRegular];

		if(cellView.textField) {
			cellView.textField.allowsDefaultTighteningForTruncation = YES;

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
			if(@available(macOS 11.0, *)) {
				cellView.imageView.symbolConfiguration =
				[NSImageSymbolConfiguration configurationWithPointSize:font.pointSize weight:NSFontWeightRegular];
			}
			cellView.imageView.image = cellImage;
			cellView.imageView.toolTip = [pe statusMessage];
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
	NSInteger row = [self rowForEntry:currentEntry];
	if(row != NSNotFound) [self.tableView scrollRowToVisible:row];
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

	 - length, samplerate, bitspersample - numeric
	 6, 14, 15

	 - playcount vs "playCount" - numeric
	 18

	 - title, albumartist, artist, album, date, genre, path, filename, codec, composer - strings, finderCompare
	 2, 3, 4, 5, 7, 8, 10, 11, 12, 17

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
		case 7:
		case 8:
		case 10:
		case 11:
		case 12:
		case 17:
			sortDescriptor = [[NSSortDescriptor alloc] initWithKey:[tableColumn identifier] ascending:ascending selector:@selector(finderCompare:)];
			sortDescriptors = @[sortDescriptor];
			break;

		case 6:
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
		item = [NSPasteboardItem new];
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

	NSMutableArray *acceptedURLs = [NSMutableArray new];

	// Get files from an file drawer drop
	if([bestType isEqualToString:CogUrlsPboardType]) {
		NSError *error;
		NSData *data = [pboard dataForType:CogUrlsPboardType];
		NSArray *urls;
		urls = [NSKeyedUnarchiver unarchivedArrayOfObjectsOfClass:[NSURL class]
		                                                 fromData:data
		                                                    error:&error];
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
		NSMutableArray *urls = [NSMutableArray new];

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
			                               @"sort": @YES,
			                               @"origin": @(URLOriginInternal) };

		[self performSelectorInBackground:@selector(insertURLsInBackground:) withObject:loadEntriesData];
	}

	return YES;
}

- (NSUndoManager *)undoManager {
	return undoManager;
}

- (NSIndexSet *)disarrangeIndexes:(NSIndexSet *)indexes {
	NSMutableIndexSet *disarrangedIndexes = [NSMutableIndexSet new];

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
	NSMutableArray *disarrangedObjects = [NSMutableArray new];

	for(PlaylistEntry *pe in [self content]) {
		if([objects containsObject:pe]) [disarrangedObjects addObject:pe];
	}

	return disarrangedObjects;
}

- (NSIndexSet *)rearrangeIndexes:(NSIndexSet *)indexes {
	if([[self content] count] <= [indexes lastIndex]) return indexes;

	NSMutableIndexSet *rearrangedIndexes = [NSMutableIndexSet new];

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
	for(PlaylistEntry *pe in objects) {
		pe.playlist = self.shownPlaylist;
	}
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
		__block NSMutableIndexSet *replacementIndexes = [NSMutableIndexSet new];
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
		pe.playlist = self.shownPlaylist;
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

	[self commitPersistentStoreAsync];

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
			NSURL *destURL = pe.url;

			if(pe.urlBookmark) {
				BOOL isStale = NO;
				NSURL *resolvedFolder = [NSURL URLByResolvingBookmarkData:pe.urlBookmark
				                                                 options:NSURLBookmarkResolutionWithSecurityScope
				                                           relativeToURL:nil
				                                     bookmarkDataIsStale:&isStale
				                                                   error:&error];
				if(resolvedFolder) {
					[resolvedFolder startAccessingSecurityScopedResource];
					error = nil;
					[[NSFileManager defaultManager] moveItemAtURL:pe.trashUrl toURL:destURL error:&error];
					[resolvedFolder stopAccessingSecurityScopedResource];
					if(error) {
						ALog(@"Error restoring %@ to %@: %@", pe.trashUrl, destURL, [error localizedDescription]);
					}
				} else {
					ALog(@"Could not resolve trash bookmark for %@, falling back: %@", destURL, [error localizedDescription]);
					const void *handle = [[SandboxBroker sharedSandboxBroker] beginFolderAccess:destURL];
					error = nil;
					[[NSFileManager defaultManager] moveItemAtURL:pe.trashUrl toURL:destURL error:&error];
					[[SandboxBroker sharedSandboxBroker] endFolderAccess:handle];
					if(error) {
						ALog(@"Error restoring %@ to %@: %@", pe.trashUrl, destURL, [error localizedDescription]);
					}
				}
			} else {
				[[NSFileManager defaultManager] moveItemAtURL:pe.trashUrl toURL:destURL error:&error];
				if(error) {
					ALog(@"Error restoring %@ to %@: %@", pe.trashUrl, destURL, [error localizedDescription]);
				}
			}
		}
		pe.deLeted = NO;
		pe.trashUrl = nil;
		pe.urlBookmark = nil;
	}

	[super insertObjects:objects atArrangedObjectIndexes:indexes];

	[self commitPersistentStoreAsync];

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

	// Where playback goes on from, if the current entry is among them.
	[self.playbackOrder willRemoveEntries:objects current:currentEntry];
	for(PlaylistEntry *pe in objects) {
		pe.deLeted = YES;
	}

	[super removeObjectsAtArrangedObjectIndexes:indexes];

	[self commitPersistentStoreAsync];

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

	BOOL trashingCurrent = currentEntry && [objects containsObject:currentEntry];
	[self.playbackOrder willRemoveEntries:objects current:currentEntry];
	for(PlaylistEntry *pe in objects) {
		pe.deLeted = YES;
	}

	if(trashingCurrent) {
		PlaylistEntry *next = self.playbackOrder.nextEntryAfterDeleted;
		if(next) {
			[playbackController playEntry:next];
			self.playbackOrder.nextEntryAfterDeleted = nil;
		} else {
			[playbackController stop:nil];
		}
	}

	[super removeObjectsAtArrangedObjectIndexes:indexes];

	if([self shuffle] != ShuffleOff) [self resetShuffleList];

	[playbackController playlistDidChange:self];

	// Collect file URLs before dispatching to background, since PlaylistEntry
	// is a CoreData managed object and must not be accessed off its context queue.
	// Also request sandbox permission for any folder not yet covered, then begin
	// folder access and capture the token bookmark for later restoration.
	NSMutableArray<NSURL *> *fileURLs = [NSMutableArray array];
	for(PlaylistEntry *pe in objects) {
		if([pe.url isFileURL]) [fileURLs addObject:pe.url];
	}
	if([fileURLs count] > 0 && ![[SandboxBroker sharedSandboxBroker] areAllPathsSafe:fileURLs]) {
		for(NSURL *url in fileURLs) {
			[[SandboxBroker sharedSandboxBroker] requestFolderForFile:url];
		}
	}

	NSMutableArray<NSValue *> *sbHandles = [NSMutableArray array];
	NSMutableArray<NSDictionary *> *trashWork = [NSMutableArray array];
	for(PlaylistEntry *pe in objects) {
		if([pe.url isFileURL]) {
			const void *handle = [[SandboxBroker sharedSandboxBroker] beginFolderAccess:pe.url];
			[sbHandles addObject:[NSValue valueWithPointer:handle]];

			NSData *bookmark = [[SandboxBroker sharedSandboxBroker] bookmarkDataForHandle:handle];

			NSMutableDictionary *item = [@{@"url": pe.url, @"objectID": pe.objectID} mutableCopy];
			if(bookmark) item[@"bookmark"] = bookmark;
			[trashWork addObject:[item copy]];
		}
	}

	if([trashWork count] > 0) {
		NSPersistentContainer *container = self.persistentContainer;
		dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
			NSMutableDictionary<NSManagedObjectID *, NSURL *> *trashedURLs = [NSMutableDictionary dictionary];
			NSMutableDictionary<NSManagedObjectID *, NSData *> *bookmarks = [NSMutableDictionary dictionary];

			for(NSDictionary *item in trashWork) {
				NSURL *fileURL = item[@"url"];
				NSManagedObjectID *objectID = item[@"objectID"];
				NSURL *removed = nil;
				NSError *error = nil;
				[[NSFileManager defaultManager] trashItemAtURL:fileURL resultingItemURL:&removed error:&error];
				if(removed) {
					trashedURLs[objectID] = removed;
					NSData *bookmark = item[@"bookmark"];
					if(bookmark) bookmarks[objectID] = bookmark;
				}
				if(error) {
					ALog(@"Error trashing file %@: %@", fileURL, [error localizedDescription]);
				}
			}

			// Update CoreData on its context queue and persist everything
			[container.viewContext performBlock:^{
				for(NSManagedObjectID *objectID in trashedURLs) {
					NSError *error = nil;
					PlaylistEntry *pe = (PlaylistEntry *)[container.viewContext existingObjectWithID:objectID error:&error];
					if(pe && !error) {
						pe.trashUrl = trashedURLs[objectID];
						pe.urlBookmark = bookmarks[objectID];
					}
				}

				NSError *saveError = nil;
				[container.viewContext save:&saveError];
				if(saveError) {
					ALog(@"Error saving trash URLs: %@", [saveError localizedDescription]);
				}
			}];

			// Release sandbox access handles after background work completes.
			dispatch_async(dispatch_get_main_queue(), ^{
				for(NSValue *handleValue in sbHandles) {
					[[SandboxBroker sharedSandboxBroker] endFolderAccess:[handleValue pointerValue]];
				}
			});
		});
	} else {
		// No files to trash, but still need to persist the playlist changes
		[self commitPersistentStoreAsync];
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

BOOL showTrashConsent(NSWindow *window) {
	BOOL askedConsent = [[NSUserDefaults standardUserDefaults] boolForKey:@"trashAskedConsent"];
	if(!askedConsent) {
		[window orderFront:window];

		NSAlert *alert = [NSAlert new];
		[alert setMessageText:NSLocalizedString(@"TrashConsentTitle", @"Title for a file trash operation consent dialog")];
		[alert setInformativeText:NSLocalizedString(@"TrashConsentText", @"Message body for the consent dialog asking if the user really wants to reversibly trash some files from their playlist")];
		[alert addButtonWithTitle:NSLocalizedString(@"ConsentNo", @"The negative answer")];
		[alert addButtonWithTitle:NSLocalizedString(@"ConsentYes",@"The affirmative answer")];
		[alert addButtonWithTitle:NSLocalizedString(@"ConsentAlways",@"The always affirmative answer")];
		
		NSModalResponse returnCode = [alert runModal];
		if(returnCode == NSAlertThirdButtonReturn) {
			[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"trashAskedConsent"];
			return YES;
		} else if(returnCode == NSAlertSecondButtonReturn) {
			return YES;
		} else {
			return NO;
		}
	} else {
		return YES;
	}
}

- (IBAction)trash:(id)sender {
	// Someone asked for this, so they're getting it.
	// Trash the selection, and advance playback to the next untrashed file if necessary.

	NSIndexSet *selected = [self selectionIndexes];
	if([selected count] > 0) {
		if(showTrashConsent([appController mainWindow])) {
			[self trashObjectsAtArrangedObjectIndexes:selected];
		}
	}
}

- (IBAction)removeDuplicates:(id)sender {
	NSMutableArray *originals = [NSMutableArray new];
	NSMutableArray *duplicates = [NSMutableArray new];

	for(PlaylistEntry *pe in [self content]) {
		if([originals containsObject:pe.url])
			[duplicates addObject:pe];
		else
			[originals addObject:pe.url];
	}

	if([duplicates count] > 0) {
		NSArray *arrangedContent = [self arrangedObjects];
		NSMutableIndexSet *duplicatesIndex = [NSMutableIndexSet new];
		for(PlaylistEntry *pe in duplicates) {
			[duplicatesIndex addIndex:[arrangedContent indexOfObject:pe]];
		}
		[self removeObjectsAtArrangedObjectIndexes:duplicatesIndex];
	}
}

- (IBAction)removeDeadItems:(id)sender {
	NSMutableArray *deadItems = [NSMutableArray new];

	for(PlaylistEntry *pe in [self content]) {
		NSURL *url = pe.url;
		if([url isFileURL])
			if(![[NSFileManager defaultManager] fileExistsAtPath:[url path]])
				[deadItems addObject:pe];
	}

	if([deadItems count] > 0) {
		NSArray *arrangedContent = [self arrangedObjects];
		NSMutableIndexSet *deadItemsIndex = [NSMutableIndexSet new];
		for(PlaylistEntry *pe in deadItems) {
			[deadItemsIndex addIndex:[arrangedContent indexOfObject:pe]];
		}
		[self removeObjectsAtArrangedObjectIndexes:deadItemsIndex];
	}
}

- (PlaylistEntry *)getNextEntry:(PlaylistEntry *)pe {
	return [self getNextEntry:pe ignoreRepeatOne:NO];
}

- (PlaylistEntry *)getNextEntry:(PlaylistEntry *)pe ignoreRepeatOne:(BOOL)ignoreRepeatOne {
	return [self.playbackOrder nextEntryAfter:pe ignoreRepeatOne:ignoreRepeatOne startingIn:self.shownPlaylist];
}

- (PlaylistEntry *)getPrevEntry:(PlaylistEntry *)pe {
	return [self getPrevEntry:pe ignoreRepeatOne:NO];
}

- (PlaylistEntry *)getPrevEntry:(PlaylistEntry *)pe ignoreRepeatOne:(BOOL)ignoreRepeatOne {
	return [self.playbackOrder previousEntryBefore:pe ignoreRepeatOne:ignoreRepeatOne];
}

- (PlaylistEntry *)firstEntryToPlay {
	return [self.playbackOrder firstEntryInPlaylist:self.shownPlaylist];
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

- (void)readQueueFromDataStore {
	[self.playbackOrder loadQueueInContext:self.persistentContainer.viewContext];
}

- (void)resetShuffleList {
	[self.playbackOrder resetShuffleInPlaylist:self.shownPlaylist current:currentEntry];
}

- (Playlist *)playingPlaylist {
	return currentEntry.playlist;
}

- (void)stopPlayback {
	[playbackController stop:self];
}

- (void)playingPlaylistDidChange {
	[playbackController playlistDidChange:self];
}

- (void)showPlaylist:(Playlist *)playlist {
	if(!playlist || playlist == _shownPlaylist) return;

	if(_shownPlaylist.uuid) {
		if(!viewStates) viewStates = [NSMutableDictionary new];
		viewStates[_shownPlaylist.uuid.UUIDString] = @{
			@"selection": [self selectedObjects],
			@"scroll": [NSValue valueWithPoint:self.tableView.enclosingScrollView.contentView.bounds.origin]
		};
	}

	[self setFilterPredicate:nil];
	// A column sort is already the playlist's order (its indexes follow the
	// table); keep the content in that order and show the next unsorted.
	if([[self sortDescriptors] count]) {
		[self setContent:[[self arrangedObjects] mutableCopy]];
		[super setSortDescriptors:@[]];
		for(NSTableColumn *column in [self.tableView tableColumns]) {
			[self.tableView setIndicatorImage:nil inTableColumn:column];
		}
	}
	// Undo works on rows of the playlist it was done in. Clearing it also
	// resets registration, so not while launch has it off.
	if([[self undoManager] isUndoRegistrationEnabled]) [[self undoManager] removeAllActions];

	[self willChangeValueForKey:@"shownPlaylist"];
	_shownPlaylist = playlist;
	[self didChangeValueForKey:@"shownPlaylist"];
	if(playlist.uuid) [[NSUserDefaults standardUserDefaults] setObject:playlist.uuid.UUIDString forKey:@"shownPlaylist"];

	[self setContent:[playlist.orderedEntries mutableCopy]];

	NSDictionary *state = playlist.uuid ? viewStates[playlist.uuid.UUIDString] : nil;
	[self setSelectedObjects:state[@"selection"] ?: @[]];
	if(state[@"scroll"]) {
		[self.tableView scrollPoint:[state[@"scroll"] pointValue]];
	} else {
		[self.tableView scrollRowToVisible:0];
	}
	[self commitPersistentStoreAsync];
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

	[self commitPersistentStoreAsync];

	PlaylistEntry *previous = currentEntry;
	[self willChangeValueForKey:@"playingPlaylist"];
	currentEntry = pe;
	[self didChangeValueForKey:@"playingPlaylist"];

	[self reloadRowsForEntries:@[previous ?: pe, pe ?: previous]];

	NSInteger row = [self rowForEntry:pe];
	if(row != NSNotFound) [self.tableView scrollRowToVisible:row];
}

- (void)setShuffle:(ShuffleMode)s {
	[[NSUserDefaults standardUserDefaults] setInteger:s forKey:@"shuffle"];
	// A new order for what plays, or for what will.
	if(s != ShuffleOff) [self.playbackOrder resetShuffleInPlaylist:(currentEntry.playlist ?: self.shownPlaylist) current:currentEntry];

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

- (NSArray<PlaylistEntry *> *)queueList {
	return self.playbackOrder.queue;
}

/// The table row showing `pe`, or NSNotFound when it has none: removed, in
/// another playlist, or hidden by the search. An entry's index is its row
/// unless the table is searched or sorted.
- (NSInteger)rowForEntry:(PlaylistEntry *)pe {
	if(!pe || pe.deLeted || pe.playlist != self.shownPlaylist) return NSNotFound;
	NSArray *arranged = [self arrangedObjects];
	NSInteger row = pe.index;
	if(row >= 0 && row < [arranged count] && arranged[row] == pe) return row;
	return [arranged indexOfObjectIdenticalTo:pe];
}

/// Redraws the rows of `entries` that the table shows, whole, for their
/// tooltips.
- (void)reloadRowsForEntries:(id<NSFastEnumeration>)entries {
	NSMutableIndexSet *refreshSet = [NSMutableIndexSet new];
	for(PlaylistEntry *pe in entries) {
		NSInteger row = [self rowForEntry:pe];
		if(row != NSNotFound) [refreshSet addIndex:row];
	}
	if(![refreshSet count]) return;

	unsigned long columns = [[self.tableView tableColumns] count];
	[self.tableView reloadDataForRowIndexes:refreshSet columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, columns)]];
}

- (IBAction)emptyQueueList:(id)sender {
	[self emptyQueueListUnsynced];
}

- (void)emptyQueueListUnsynced {
	NSArray *queued = self.playbackOrder.queue;
	[self.playbackOrder emptyQueue];
	[self reloadRowsForEntries:queued];
}

- (void)addEntriesToQueue:(NSArray<PlaylistEntry *> *)entries {
	[self.playbackOrder addToQueue:entries];
	[self reloadRowsForEntries:self.playbackOrder.queue];
}

- (void)toggleQueuedForEntries:(NSArray *)entries {
	// Every queued row renumbers, besides the toggled ones.
	NSMutableSet *affected = [NSMutableSet setWithArray:entries];
	[affected addObjectsFromArray:self.playbackOrder.queue];
	[self.playbackOrder toggleQueued:entries];
	[self reloadRowsForEntries:affected];
}

- (IBAction)toggleQueued:(id)sender {
	[self toggleQueuedForEntries:[self selectedObjects]];
}

- (BOOL)toggleQueueForEntryAtIndex:(NSInteger)i {
	if(i < 0 || i >= [[self arrangedObjects] count]) return NO;

	PlaylistEntry *pe = [[self arrangedObjects] objectAtIndex:i];
	[self toggleQueuedForEntries:@[pe]];

	return YES;
}

- (BOOL)removeEntryAtIndex:(NSInteger)i {
	if(i < 0 || i >= [[self arrangedObjects] count]) return NO;

	[self removeObjectsAtArrangedObjectIndexes:[NSIndexSet indexSetWithIndex:i]];

	return YES;
}

- (IBAction)removeFromQueue:(id)sender {
	NSMutableSet *affected = [NSMutableSet setWithArray:[self selectedObjects]];
	[affected addObjectsFromArray:self.playbackOrder.queue];
	[self.playbackOrder removeFromQueue:[self selectedObjects]];
	[self reloadRowsForEntries:affected];
}

- (IBAction)addToQueue:(id)sender {
	[self addEntriesToQueue:[self selectedObjects]];
}

- (IBAction)stopAfterCurrent:(id)sender {
	currentEntry.stopAfter = !currentEntry.stopAfter;
	[self commitPersistentStoreAsync];

	if(currentEntry) [self reloadRowsForEntries:@[currentEntry]];
}

- (IBAction)stopAfterSelection:(id)sender {
	for(PlaylistEntry *pe in [self selectedObjects]) {
		pe.stopAfter = !pe.stopAfter;
	}

	[self commitPersistentStoreAsync];

	[self reloadRowsForEntries:[self selectedObjects]];
}

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem {
	SEL action = [menuItem action];

	if(action == @selector(removeFromQueue:)) {
		for(PlaylistEntry *q in [self selectedObjects])
			if(q.queuePosition >= 0) return YES;

		return NO;
	}

	if(action == @selector(emptyQueueList:) && ([self.playbackOrder.queue count] < 1)) return NO;

	if(action == @selector(stopAfterCurrent:) && currentEntry.stopAfter) return NO;

	// The last playlist stays.
	if(action == NSSelectorFromString(@"deletePlaylist:") &&
	   [[Playlist playlistsInContext:self.persistentContainer.viewContext] count] < 2) return NO;

	// if nothing is selected, gray out these
	if([[self selectedObjects] count] < 1) {
		if(action == @selector(remove:)) return NO;

		if(action == @selector(addToQueue:)) return NO;

		if(action == @selector(toggleQueued:)) return NO;

		if(action == @selector(searchByArtist:)) return NO;

		if(action == @selector(searchByAlbum:)) return NO;

		if(action == @selector(stopAfterSelection:)) return NO;

		if(action == NSSelectorFromString(@"newPlaylistFromSelection:")) return NO;

		if(action == NSSelectorFromString(@"addSelectionToPlaylist:")) return NO;
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
		for(PlaylistEntry *pe in selectedobjects) {
			[self resetPlayCountForTrack:pe];
		}
		[self commitPersistentStoreAsync];

		[self reloadRowsForEntries:selectedobjects];
	}
}

- (IBAction)removeRatings:(id)sender {
	NSArray *selectedobjects = [self selectedObjects];
	if([selectedobjects count]) {
		for(PlaylistEntry *pe in selectedobjects) {
			[self removeRatingForTrack:pe];
		}
		[self commitPersistentStoreAsync];

		[self reloadRowsForEntries:selectedobjects];
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

- (IBAction)saveSelectionAsPlaylist:(id)sender {
	[appController savePlaylistFromSelection:sender];
}

@end
