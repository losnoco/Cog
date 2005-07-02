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
	
	NSMutableArray *shuffleList;
	NSMutableArray *history;

	PlaylistEntry *currentEntry;
	
	PlaylistEntry *nextEntry;
	PlaylistEntry *prevEntry;
	
	BOOL shuffle;
	BOOL repeat;
}

//All these return the number of things actually added
//PRIVATE ONES
/*- (int)addPath:(NSString *)path;
- (int)insertPath:(NSString *)path atIndex:(int)index;
- (int)insertFile:(NSString *)filename atIndex:(int)index;
- (int)addFile:(NSString *)filename;
*/
//ONLY PUBLIC ONES
- (int)addPaths:(NSArray *)paths sort:(BOOL)sort;
- (int)insertPaths:(NSArray *)paths atIndex:(int)index sort:(BOOL)sort;

- (NSArray *)acceptableFileTypes;

- (void)setShuffle:(BOOL)s;
- (BOOL)shuffle;
- (void)setRepeat:(BOOL)r;
- (BOOL)repeat;

- (IBAction)takeShuffleFromObject:(id)sender;
- (IBAction)takeRepeatFromObject:(id)sender;

//FUN PLAYLIST MANAGEMENT STUFF!
- (void)setCurrentEntry:(PlaylistEntry *)pe addToHistory:(BOOL)h;
- (id)currentEntry;
- (void)setCurrentEntry:(id)pe;

- (void)reset;

- (void)generateShuffleList;

- (void)next;
- (void)prev;
- (PlaylistEntry *)prevEntry;
- (PlaylistEntry *)nextEntry;

//load/save playlist
- (void)loadPlaylist:(NSString *)filename;
- (void)savePlaylist:(NSString *)filename;

- (NSString *)playlistFilename;
- (void)setPlaylistFilename:(NSString *)pf;
- (NSArray *)acceptablePlaylistTypes;

//private playlist management stuff..ssshhhh
- (void)getNextEntry;
- (void)getPrevEntry;
- (void)setPrevEntry:(PlaylistEntry *)pe;
- (void)setNextEntry:(PlaylistEntry *)pe;

//private
- (void)updateIndexesFromRow:(int) row;

@end
