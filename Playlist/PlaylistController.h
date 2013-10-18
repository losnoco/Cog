//
//  PlaylistController.h
//  Cog
//
//  Created by Vincent Spader on 3/18/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Foundation/NSUndoManager.h>
#import "DNDArrayController.h"

@class PlaylistLoader;
@class PlaylistEntry;
@class SpotlightWindowController;
@class PlaybackController;

typedef enum {
	RepeatNone = 0,
	RepeatOne,
	RepeatAlbum,
	RepeatAll
} RepeatMode;

typedef enum {
	ShuffleOff = 0,
	ShuffleAlbums,
	ShuffleAll
} ShuffleMode;
	

typedef enum {
	URLOriginInternal = 0,
	URLOriginExternal,
} URLOrigin;

@interface PlaylistController : DNDArrayController {
	IBOutlet PlaylistLoader *playlistLoader;
	IBOutlet SpotlightWindowController *spotlightWindowController;
	IBOutlet PlaybackController *playbackController;
	
	NSMutableArray *shuffleList;
	NSMutableArray *queueList;
	
	NSString *totalTime;
	
	PlaylistEntry *currentEntry;
    
    NSUndoManager *undoManager;
}

@property(nonatomic, retain) PlaylistEntry *currentEntry;
@property(retain) NSString *totalTime;

//Private Methods
- (void)updateTotalTime;
- (void)updatePlaylistIndexes;
- (IBAction)stopAfterCurrent:(id)sender;


//PUBLIC METHODS
- (void)setShuffle:(ShuffleMode)s;
- (ShuffleMode)shuffle;
- (void)setRepeat:(RepeatMode)r;
- (RepeatMode)repeat;
- (NSArray *)filterPlaylistOnAlbum:(NSString *)album;

- (PlaylistEntry *)getNextEntry:(PlaylistEntry *)pe;
- (PlaylistEntry *)getPrevEntry:(PlaylistEntry *)pe;

/* Methods for undoing various actions */
- (NSUndoManager *)undoManager;

- (IBAction)toggleShuffle:(id)sender;

- (IBAction)toggleRepeat:(id)sender;

- (IBAction)randomizeList:(id)sender;

- (IBAction)removeDuplicates:(id)sender;
- (IBAction)removeDeadItems:(id)sender;

- (IBAction)showEntryInFinder:(id)sender;
- (IBAction)clearFilterPredicate:(id)sender;
- (IBAction)clear:(id)sender;

//- (IBAction)showTagEditor:(id)sender;

// Spotlight
- (IBAction)searchByArtist:(id)sender;
- (IBAction)searchByAlbum:(id)sender;

//FUN PLAYLIST MANAGEMENT STUFF!
- (BOOL)next;
- (BOOL)prev;

- (void)addShuffledListToBack;
- (void)addShuffledListToFront;
- (void)resetShuffleList;

- (PlaylistEntry *)shuffledEntryAtIndex:(int)i;
- (PlaylistEntry *)entryAtIndex:(int)i;

// Event inlets:
- (void)willInsertURLs:(NSArray*)urls origin:(URLOrigin)origin;
- (void)didInsertURLs:(NSArray*)urls origin:(URLOrigin)origin;

// queue methods
- (IBAction)toggleQueued:(id)sender;
- (IBAction)emptyQueueList:(id)sender;
- (NSMutableArray *)queueList;

@end
