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
@class EntriesController;
@class SpotlightWindowController;

typedef enum {
	RepeatNone = 0,
	RepeatOne,
	RepeatAlbum,
	RepeatAll
} RepeatMode;

@interface PlaylistController : DNDArrayController {
	IBOutlet PlaylistLoader *playlistLoader;
	IBOutlet EntriesController *entriesController;
	IBOutlet SpotlightWindowController *spotlightWindowController;
	
	NSMutableArray *shuffleList;
	NSMutableArray *queueList;
	
	NSString *totalTime;
	
	PlaylistEntry *currentEntry;
	
	BOOL shuffle;
	RepeatMode repeat;
}

@property(retain) PlaylistEntry *currentEntry;
@property(retain) NSString *totalTime;

//Private Methods
- (void)updateTotalTime;
- (void)updatePlaylistIndexes;
- (IBAction)stopAfterCurrent:(id)sender;


//PUBLIC METHODS
- (void)setShuffle:(BOOL)s;
- (BOOL)shuffle;
- (void)setRepeat:(RepeatMode)r;
- (RepeatMode)repeat;
- (NSArray *)filterPlaylistOnAlbum:(NSString *)album;

- (PlaylistEntry *)getNextEntry:(PlaylistEntry *)pe;
- (PlaylistEntry *)getPrevEntry:(PlaylistEntry *)pe;

/* Methods for undoing various actions */
- (NSUndoManager *)undoManager;

- (IBAction)takeShuffleFromObject:(id)sender;

- (IBAction)toggleRepeat:(id)sender;

- (IBAction)sortByPath;
- (IBAction)randomizeList;

- (IBAction)showEntryInFinder:(id)sender;
- (IBAction)clearFilterPredicate:(id)sender;
- (IBAction)clear:(id)sender;

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

// queue methods
- (IBAction)toggleQueued:(id)sender;
- (IBAction)emptyQueueList:(id)sender;
- (NSMutableArray *)queueList;

@end
