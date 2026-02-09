//
//  PlaylistController.h
//  Cog
//
//  Created by Vincent Spader on 3/18/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <CoreData/CoreData.h>
#import <Foundation/NSUndoManager.h>

#import "DNDArrayController.h"

@class AlbumArtwork;

@class PlaylistLoader;
@class PlaylistEntry;
@class SpotlightWindowController;
@class PlaybackController;
@class AppController;

typedef NS_ENUM(NSInteger, RepeatMode) {
	RepeatModeNoRepeat = 0,
	RepeatModeRepeatOne,
	RepeatModeRepeatAlbum,
	RepeatModeRepeatAll
};

static inline BOOL IsRepeatOneSet(void) {
	return [[NSUserDefaults standardUserDefaults] integerForKey:@"repeat"] == RepeatModeRepeatOne;
}

typedef enum { ShuffleOff = 0,
	           ShuffleAlbums,
	           ShuffleAll } ShuffleMode;

typedef NS_ENUM(NSInteger, URLOrigin) {
	URLOriginInternal = 0,
	URLOriginExternal
};

@interface PlaylistController : DNDArrayController <NSTableViewDelegate> {
	IBOutlet PlaylistLoader *playlistLoader;
	IBOutlet SpotlightWindowController *spotlightWindowController;
	IBOutlet PlaybackController *playbackController;
	IBOutlet AppController *appController;

	NSValueTransformer *statusImageTransformer;
	NSValueTransformer *numberHertzToStringTransformer;

	NSMutableArray *shuffleList;
	NSMutableArray *queueList;

	NSString *totalTime;
	NSString *currentStatus;

	PlaylistEntry *currentEntry;

	PlaylistEntry *nextEntryAfterDeleted;

	NSUndoManager *undoManager;

	BOOL observersRegistered;
}

@property(nonatomic, retain) PlaylistEntry *_Nullable currentEntry;
@property(retain) NSString *_Nullable totalTime;
@property(retain) NSString *_Nullable currentStatus;

@property(strong, nonatomic, readonly) NSPersistentContainer *_Nonnull persistentContainer;
@property(strong, nonatomic, readonly) NSMutableDictionary<NSString *, AlbumArtwork *> *_Nonnull persistentArtStorage;

// Private Methods
- (void)commitPersistentStore;
- (void)updateTotalTime;
- (void)updatePlaylistIndexes;
- (IBAction)stopAfterCurrent:(id _Nullable)sender;

// PUBLIC METHODS
- (void)setShuffle:(ShuffleMode)s;
- (ShuffleMode)shuffle;
- (void)setRepeat:(RepeatMode)r;
- (RepeatMode)repeat;
- (NSArray *_Nullable)filterPlaylistOnAlbum:(NSString *_Nullable)album;

- (PlaylistEntry *_Nullable)getNextEntry:(PlaylistEntry *_Nullable)pe;
- (PlaylistEntry *_Nullable)getPrevEntry:(PlaylistEntry *_Nullable)pe;

/* Methods for undoing various actions */
- (NSUndoManager *_Nullable)undoManager;

- (IBAction)toggleShuffle:(id _Nullable)sender;

- (IBAction)toggleRepeat:(id _Nullable)sender;

- (IBAction)randomizeList:(id _Nullable)sender;

- (IBAction)removeDuplicates:(id _Nullable)sender;
- (IBAction)removeDeadItems:(id _Nullable)sender;

- (IBAction)remove:(id _Nullable)sender;
- (IBAction)trash:(id _Nullable)sender;

- (IBAction)showEntryInFinder:(id _Nullable)sender;
- (IBAction)clearFilterPredicate:(id _Nullable)sender;
- (IBAction)clear:(id _Nullable)sender;

//- (IBAction)showTagEditor:(id)sender;

// Spotlight
- (IBAction)searchByArtist:(id _Nullable)sender;
- (IBAction)searchByAlbum:(id _Nullable)sender;

// FUN PLAYLIST MANAGEMENT STUFF!
- (BOOL)next;
- (BOOL)prev;

- (void)addShuffledListToBack;
- (void)addShuffledListToFront;
- (void)resetShuffleList;

- (PlaylistEntry *_Nullable)shuffledEntryAtIndex:(NSInteger)i;
- (PlaylistEntry *_Nullable)entryAtIndex:(NSInteger)i;

// Event inlets:
- (void)willInsertURLs:(NSArray *_Nullable)urls origin:(URLOrigin)origin;
- (void)didInsertURLs:(NSArray *_Nullable)urls origin:(URLOrigin)origin;

// Asynchronous event handler
- (void)addURLsInBackground:(NSDictionary *_Nonnull)input;
- (void)insertURLsInBackground:(NSDictionary *_Nonnull)input;

// queue methods
- (IBAction)toggleQueued:(id _Nullable)sender;
- (IBAction)emptyQueueList:(id _Nullable)sender;
- (void)emptyQueueListUnsynced;
- (NSMutableArray *_Nullable)queueList;

// internal methods for data store init
- (void)readQueueFromDataStore;
- (void)readShuffleListFromDataStore;

+ (NSPersistentContainer *_Nonnull)sharedPersistentContainer;

// reload metadata of selection
- (IBAction)reloadTags:(id _Nullable)sender;

// Reset playcount of selection
- (IBAction)resetPlaycounts:(id _Nullable)sender;
- (IBAction)removeRatings:(id _Nullable)sender;

// Play statistics
- (void)updatePlayCountForTrack:(PlaylistEntry *_Nonnull)pe;
- (void)firstSawTrack:(PlaylistEntry *_Nonnull)pe;

- (void)moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet *_Nullable)indexSet
                                        toIndex:(NSUInteger)insertIndex;

- (void)insertObjectsUnsynced:(NSArray *_Nullable)objects atArrangedObjectIndexes:(NSIndexSet *_Nullable)indexes;

- (void)tableView:(NSTableView *_Nonnull)view didClickRow:(NSInteger)clickedRow column:(NSInteger)clickedColumn atPoint:(NSPoint)cellPoint;

- (BOOL)pathSuggesterEmpty;

- (IBAction)saveSelectionAsPlaylist:(id _Nullable)sender;

@end
