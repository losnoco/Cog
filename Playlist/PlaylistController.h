//
//  PlaylistController.h
//  Cog
//
//  Created by Vincent Spader on 3/18/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DNDArrayController.h"
#import "PlaylistEntry.h"

@interface PlaylistController : DNDArrayController {
	NSArray *acceptableFileTypes;
	NSArray *acceptablePlaylistTypes;
	
	NSString *playlistFilename;
	NSString *totalTimeDisplay;
	
	NSMutableArray *shuffleList;

	PlaylistEntry *currentEntry;
	
	int shuffleIndex;
	
	BOOL shuffle;
	BOOL repeat;
}

//All these return the number of things actually added
//Private Methods
- (int)addPath:(NSString *)path;
- (int)insertPath:(NSString *)path atIndex:(int)index;
- (int)insertFile:(NSString *)filename atIndex:(int)index;
- (int)addFile:(NSString *)filename;
- (void)updateIndexesFromRow:(int) row;
- (void)updateTotalTime;


//PUBLIC METHODS
- (int)addPaths:(NSArray *)paths sort:(BOOL)sort;
- (int)insertPaths:(NSArray *)paths atIndex:(int)index sort:(BOOL)sort;

- (NSArray *)acceptableFileTypes;

- (void)setShuffle:(BOOL)s;
- (BOOL)shuffle;
- (void)setRepeat:(BOOL)r;
- (BOOL)repeat;

- (IBAction)takeShuffleFromObject:(id)sender;
- (IBAction)takeRepeatFromObject:(id)sender;

- (void)setTotalTimeDisplay:(NSString *)ttd;
- (NSString *)totalTimeDisplay;

//FUN PLAYLIST MANAGEMENT STUFF!
- (id)currentEntry;
- (void)setCurrentEntry:(PlaylistEntry *)pe;

- (BOOL)next;
- (BOOL)prev;

- (PlaylistEntry *)entryAtOffset:(int)offset;
- (void)addShuffledListToBack;
- (void)addShuffledListToFront;
- (void)resetShuffleList;

//load/save playlist
- (void)loadPlaylist:(NSString *)filename;
- (void)savePlaylist:(NSString *)filename;

- (NSString *)playlistFilename;
- (void)setPlaylistFilename:(NSString *)pf;
- (NSArray *)acceptablePlaylistTypes;

@end
