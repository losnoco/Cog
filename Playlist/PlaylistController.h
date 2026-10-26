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

#import "PlaylistControllerEnums.h"

#import "DNDArrayController.h"

#import <CogPlaylist/CogPlaylist-Swift.h>

@class AlbumArtwork;

@class PlaylistLoader;
@class PlaylistEntry;
@class SpotlightWindowController;
@class PlaybackController;
@class AppController;

@interface PlaylistController : DNDArrayController <NSTableViewDelegate> {
	IBOutlet PlaylistLoader *playlistLoader;
	IBOutlet SpotlightWindowController *spotlightWindowController;
	IBOutlet PlaybackController *playbackController;
	IBOutlet AppController *appController;

	NSValueTransformer *statusImageTransformer;
	NSValueTransformer *numberHertzToStringTransformer;

	NSString *totalTime;
	NSString *currentStatus;

	PlaylistEntry *currentEntry;

	// Each playlist's selection and scroll position, by UUID, for coming back
	// to it.
	NSMutableDictionary<NSString *, NSDictionary *> *viewStates;

	NSUndoManager *undoManager;

	BOOL observersRegistered;
}

@property(nonatomic, retain) PlaylistEntry *_Nullable currentEntry;
@property(retain) NSString *_Nullable totalTime;
@property(retain) NSString *_Nullable currentStatus;

@property(strong, nonatomic, readonly) NSPersistentContainer *_Nonnull persistentContainer;

/// What plays next, shared with the iOS app: the queue, shuffle and repeat,
/// within the playing entry's playlist.
@property(strong, nonatomic, readonly) PlaybackOrder *_Nonnull playbackOrder;
/// The playlist the table shows, which need not be the one playing.
@property(strong, nonatomic, readonly) Playlist *_Nullable shownPlaylist;

// Private Methods
- (void)commitPersistentStore;
- (void)commitPersistentStoreAsync;
- (void)updateTotalTime;
- (void)updatePlaylistIndexes;
- (IBAction)stopAfterCurrent:(id _Nullable)sender;

// PUBLIC METHODS
- (void)setShuffle:(ShuffleMode)s;
- (ShuffleMode)shuffle;
- (void)setRepeat:(RepeatMode)r;
- (RepeatMode)repeat;

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

- (void)resetShuffleList;

/// The first entry to play in the shown playlist: its first, or shuffle's.
- (PlaylistEntry *_Nullable)firstEntryToPlay;
- (PlaylistEntry *_Nullable)entryAtIndex:(NSInteger)i;

// Event inlets:
- (void)willInsertURLs:(NSArray *_Nullable)urls origin:(URLOrigin)origin;
- (void)didInsertURLs:(NSArray *_Nullable)urls origin:(URLOrigin)origin;

// Asynchronous event handler
- (void)addURLsInBackground:(NSDictionary *_Nonnull)input;
- (void)insertURLsInBackground:(NSDictionary *_Nonnull)input;

// queue methods
- (IBAction)toggleQueued:(id _Nullable)sender;
- (BOOL)toggleQueueForEntryAtIndex:(NSInteger)i;
- (BOOL)removeEntryAtIndex:(NSInteger)i;
- (IBAction)emptyQueueList:(id _Nullable)sender;
- (void)emptyQueueListUnsynced;
- (NSArray<PlaylistEntry *> *_Nonnull)queueList;

/// The row showing `pe`, or NSNotFound: removed, in another playlist, or
/// hidden by the search.
- (NSInteger)rowForEntry:(PlaylistEntry *_Nullable)pe;
- (void)reloadRowsForEntries:(id<NSFastEnumeration> _Nonnull)entries;

/// Puts `playlist` in the table; playback goes on where it was. A column
/// sort stays the order of the playlist it was made in.
- (void)showPlaylist:(Playlist *_Nonnull)playlist;
/// The playlist playback goes on in: the current entry's.
@property(nonatomic, readonly) Playlist *_Nullable playingPlaylist;
/// For the playlist commands in Swift: stops playback, or has the engine
/// ask again what plays next.
- (void)stopPlayback;
- (void)playingPlaylistDidChange;
- (void)addEntriesToQueue:(NSArray<PlaylistEntry *> *_Nonnull)entries;

// internal methods for data store init
- (void)readQueueFromDataStore;

+ (NSPersistentContainer *_Nonnull)sharedPersistentContainer;

// reload metadata of selection
- (IBAction)reloadTags:(id _Nullable)sender;

// Reset playcount of selection
- (IBAction)resetPlaycounts:(id _Nullable)sender;
- (IBAction)removeRatings:(id _Nullable)sender;

// Play statistics
- (void)updatePlayCountForTrack:(PlaylistEntry *_Nonnull)pe;
- (void)firstSawTrack:(PlaylistEntry *_Nonnull)pe;
- (void)firstSawTrackWithoutReload:(PlaylistEntry *_Nonnull)pe;
- (void)firstSawTracksWithoutReload:(NSArray<PlaylistEntry *> *_Nonnull)entries;

- (void)moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet *_Nullable)indexSet
                                        toIndex:(NSUInteger)insertIndex;

- (void)insertObjectsUnsynced:(NSArray *_Nullable)objects atArrangedObjectIndexes:(NSIndexSet *_Nullable)indexes;

- (void)tableView:(NSTableView *_Nonnull)view didClickRow:(NSInteger)clickedRow column:(NSInteger)clickedColumn atPoint:(NSPoint)cellPoint;

- (BOOL)pathSuggesterEmpty;

- (IBAction)saveSelectionAsPlaylist:(id _Nullable)sender;

@end
