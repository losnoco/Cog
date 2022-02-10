//
//  PlaylistController.h
//  Cog
//
//  Created by Vincent Spader on 3/18/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "DNDArrayController.h"
#import <Cocoa/Cocoa.h>
#import <Foundation/NSUndoManager.h>

@class PlaylistLoader;
@class PlaylistEntry;
@class SpotlightWindowController;
@class PlaybackController;

typedef NS_ENUM(NSInteger, RepeatMode) {
	RepeatModeNoRepeat = 0,
	RepeatModeRepeatOne,
	RepeatModeRepeatAlbum,
	RepeatModeRepeatAll
};

static inline BOOL IsRepeatOneSet() {
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

	NSValueTransformer *statusImageTransformer;

	NSMutableArray *shuffleList;
	NSMutableArray *queueList;

	NSString *totalTime;

	PlaylistEntry *currentEntry;

	PlaylistEntry *nextEntryAfterDeleted;

	NSUndoManager *undoManager;
}

@property(nonatomic, retain) PlaylistEntry *_Nullable currentEntry;
@property(retain) NSString *_Nullable totalTime;

// Private Methods
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

// queue methods
- (IBAction)toggleQueued:(id _Nullable)sender;
- (IBAction)emptyQueueList:(id _Nullable)sender;
- (void)emptyQueueListUnsynced;
- (NSMutableArray *_Nullable)queueList;

// reload metadata of selection
- (IBAction)reloadTags:(id _Nullable)sender;

- (void)moveObjectsInArrangedObjectsFromIndexes:(NSIndexSet *_Nullable)indexSet
                                        toIndex:(NSUInteger)insertIndex;

- (void)insertObjectsUnsynced:(NSArray *_Nullable)objects atArrangedObjectIndexes:(NSIndexSet *_Nullable)indexes;

@end
