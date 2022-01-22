//
//  NSTableViewDataSource+sqlite.h
//  Cog
//
//  Created by Christopher Snowhill on 12/22/21.
//

#ifndef NSTableViewDataSource_sqlite_h
#import <Cocoa/Cocoa.h>
#import <sqlite3.h>
#import "PlaylistEntry.h"

@interface SQLiteStore : NSObject
{
@private NSString *g_databasePath;
@private sqlite3 *g_database;
@private sqlite3_stmt *stmt[39];
@private NSMutableArray *databaseMirror;
}

@property (nonatomic, readwrite) NSString *databasePath;
@property (nonatomic, assign, readwrite) sqlite3 *database;

+ (SQLiteStore *)sharedStore;

- (id)init;
- (void)dealloc;

- (void) trackUpdate:(PlaylistEntry *)track;

- (void)playlistInsertTracks:(NSArray *)tracks atIndex:(int64_t)index progressCall:(void(^)(double progress))callback;
- (void)playlistInsertTracks:(NSArray *)tracks atObjectIndexes:(NSIndexSet *)indexes progressCall:(void(^)(double progress))callback;
- (void)playlistRemoveTracks:(int64_t)index forCount:(int64_t)count progressCall:(void(^)(double progress))callback;
- (void)playlistRemoveTracksAtIndexes:(NSIndexSet *)indexes progressCall:(void(^)(double progress))callback;
- (PlaylistEntry *)playlistGetItem:(int64_t)index;
- (PlaylistEntry *)playlistGetCachedItem:(int64_t)index;
- (int64_t)playlistGetCount;
#if 0
- (void)playlistMoveObjectsInArrangedObjectsFromIndexes:(NSIndexSet *)indexSet toIndex:(NSUInteger)insertIndex;
#endif

- (void)syncPlaylistEntries:(NSArray *)entries progressCall:(void(^)(double progress))callback;

- (void)queueAddItem:(int64_t)playlistIndex;
- (void)queueAddItems:(NSArray *)playlistIndexes;
- (void)queueRemoveItem:(int64_t)queueIndex;
- (void)queueRemovePlaylistItems:(NSArray *)playlistIndexes;
- (int64_t)queueGetEntry:(int64_t)queueIndex;
- (int64_t)queueGetCount;
- (void)queueEmpty;

@end

#define NSTableViewDataSource_sqlite_h


#endif /* NSTableViewDataSource_sqlite_h */
