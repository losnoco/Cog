//
//  PlaylistLoader.h
//  Cog
//
//  Created by Vincent Spader on 3/05/07.
//  Copyright 2007 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PlaylistController.h"
#import "PlaylistView.h"

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
    
	NSOperationQueue *queue;
}

- (void)initDefaults;

// Clear playlist
- (void)clear:(id)sender;

// Load arrays of urls...
- (NSArray*)addURLs:(NSArray *)urls sort:(BOOL)sort;
- (NSArray*)addURL:(NSURL *)url;
- (NSArray*)insertURLs:(NSArray *)urls atIndex:(int)index sort:(BOOL)sort;

// Save playlist, auto-determines type based on extension. Uses m3u if it cannot be determined.
- (BOOL)save:(NSString *)filename;
- (BOOL)save:(NSString *)filename asType:(PlaylistType)type;
- (BOOL)saveM3u:(NSString *)filename;
- (BOOL)savePls:(NSString *)filename;
- (BOOL)saveXml:(NSString *)filename;

// Read info for a playlist entry
//- (NSDictionary *)readEntryInfo:(PlaylistEntry *)pe;

- (void)loadInfoForEntries:(NSArray *)entries;

- (NSArray *)acceptableFileTypes;
- (NSArray *)acceptablePlaylistTypes; //Only m3u and pls saving
- (NSArray *)acceptableContainerTypes;

// Events (passed to playlist controler):
- (void)willInsertURLs:(NSArray*)urls origin:(URLOrigin)origin;
- (void)didInsertURLs:(NSArray*)entries origin:(URLOrigin)origin;

@end
