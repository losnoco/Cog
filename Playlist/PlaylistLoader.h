//
//  PlaylistLoader.h
//  Cog
//
//  Created by Vincent Spader on 3/05/07.
//  Copyright 2007 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class PlaylistController;

typedef enum {
	kPlaylistM3u,
	kPlaylistPls,
} PlaylistType;

@interface PlaylistLoader : NSObject {
	IBOutlet PlaylistController *playlistController;
}

//load arrays of urls...
- (void)addURLs:(NSArray *)urls sort:(BOOL)sort;
- (void)insertURLs:(NSArray *)urls atIndex:(int)index sort:(BOOL)sort;

//load playlist auto-determines type to be either pls or m3u.
- (BOOL)load:(NSString *)filename;
- (BOOL)loadM3u:(NSString *)filename;
- (BOOL)loadPls:(NSString *)filename;

//save playlist, uses current info, will fail if there is no current info.
- (BOOL)save;
//save playlist, auto-determines type based on extension. Uses m3u if it cannot be determined.
- (BOOL)save:(NSString *)filename;
- (BOOL)save:(NSString *)filename asType:(PlaylistType)type;
- (BOOL)saveM3u:(NSString *)filename;
- (BOOL)savePls:(NSString *)filename;

- (NSArray *)acceptableFileTypes;
- (NSArray *)acceptablePlaylistTypes;

- (PlaylistType)currentType;
- (void)setCurrentType:(PlaylistType)type;

- (NSString *)currentFile;
- (void)setCurrentFile:(NSString *)currentFile;

@end
