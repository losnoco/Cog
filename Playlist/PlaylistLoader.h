//
//  PlaylistLoader.h
//  Cog
//
//  Created by Vincent Spader on 3/05/07.
//  Copyright 2007 Vincent Spader All rights reserved.
//

#import "PlaylistController.h"
#import "PlaylistView.h"
#import <Cocoa/Cocoa.h>

#import <stdatomic.h>

@class PlaylistController;
@class PlaybackController;
@class PlaylistEntry;

typedef enum {
	kPlaylistM3u,
	kPlaylistPls,
	kPlaylistXml,
} PlaylistType;

@interface PlaylistLoader : NSObject {
	IBOutlet PlaylistController *playlistController;
	IBOutlet NSScrollView *playlistView;
	IBOutlet PlaybackController *playbackController;

	NSOperationQueue *containerQueue;
	NSOperationQueue *queue;

	BOOL metadataLoadInProgress;

	NSMutableDictionary *queuedURLs;
}

- (void)initDefaults;

// Clear playlist
- (void)clear:(id)sender;

// Load arrays of urls...
- (NSArray *)addURLs:(NSArray *)urls sort:(BOOL)sort;
- (NSArray *)addURL:(NSURL *)url;
- (NSArray *)insertURLs:(NSArray *)urls atIndex:(NSInteger)index sort:(BOOL)sort;

- (NSArray *)addDatabase;

- (BOOL)addDataStore;

// Save playlist, auto-determines type based on extension. Uses m3u if it cannot be determined.
- (BOOL)save:(NSString *)filename onlySelection:(BOOL)selection;
- (BOOL)save:(NSString *)filename asType:(PlaylistType)type onlySelection:(BOOL)selection;
- (BOOL)saveM3u:(NSString *)filename onlySelection:(BOOL)selection;
- (BOOL)savePls:(NSString *)filename onlySelection:(BOOL)selection;
- (BOOL)saveXml:(NSString *)filename onlySelection:(BOOL)selection;

// Read info for a playlist entry
//- (NSDictionary *)readEntryInfo:(PlaylistEntry *)pe;

- (void)loadInfoForEntries:(NSArray *)entries;

// To be dispatched on main thread only
- (void)syncLoadInfoForEntries:(NSArray *)entries;

- (NSArray *)acceptableFileTypes;
- (NSArray *)acceptablePlaylistTypes; // Only m3u and pls saving
- (NSArray *)acceptableContainerTypes;

// Events (passed to playlist controler):
- (void)willInsertURLs:(NSArray *)urls origin:(URLOrigin)origin;
- (void)didInsertURLs:(NSArray *)entries origin:(URLOrigin)origin;
- (void)addURLsInBackground:(NSDictionary *)input;
- (void)insertURLsInBackground:(NSDictionary *)input;

@end
