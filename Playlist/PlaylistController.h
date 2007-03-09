//
//  PlaylistController.h
//  Cog
//
//  Created by Vincent Spader on 3/18/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DNDArrayController.h"

@class PlaylistLoader;
@class PlaylistEntry;

@interface PlaylistController : DNDArrayController {
	IBOutlet PlaylistLoader *playlistLoader;

	NSString *totalTimeDisplay;
	
	NSMutableArray *shuffleList;

	PlaylistEntry *currentEntry;
	
	BOOL shuffle;
	BOOL repeat;
	
	int selectedRow;
}

//Private Methods
- (void)updateIndexesFromRow:(int) row;
- (void)updateTotalTime;


//PUBLIC METHODS
- (void)setShuffle:(BOOL)s;
- (BOOL)shuffle;
- (void)setRepeat:(BOOL)r;
- (BOOL)repeat;

- (IBAction)takeShuffleFromObject:(id)sender;
- (IBAction)takeRepeatFromObject:(id)sender;

- (IBAction)sortByPath;
- (IBAction)randomizeList;

- (IBAction)showEntryInFinder:(id)sender;

- (void)setTotalTimeDisplay:(NSString *)ttd;
- (NSString *)totalTimeDisplay;

//FUN PLAYLIST MANAGEMENT STUFF!
- (id)currentEntry;
- (void)setCurrentEntry:(PlaylistEntry *)pe;

- (BOOL)next;
- (BOOL)prev;

- (void)addShuffledListToBack;
- (void)addShuffledListToFront;
- (void)resetShuffleList;

- (void)handlePlaylistViewHeaderNotification:(NSNotification*)notif;

@end
