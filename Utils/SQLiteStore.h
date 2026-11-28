//
//  NSTableViewDataSource+sqlite.h
//  Cog
//
//  Created by Christopher Snowhill on 12/22/21.
//

#ifndef NSTableViewDataSource_sqlite_h
#import "PlaylistEntry.h"
#import <Cocoa/Cocoa.h>
#import <sqlite3.h>

@interface SQLiteStore : NSObject {
	@private
	NSString *g_databasePath;
	@private
	sqlite3 *g_database;
	@private
	sqlite3_stmt *stmt[22];
	@private
	NSMutableArray *databaseMirror;
	NSMutableDictionary *artTable;
	NSMutableDictionary *stringTable;
}

+ (SQLiteStore *)sharedStore;
+ (BOOL)databaseStarted;

- (id)init;
- (void)dealloc;

- (void)shutdown;

- (PlaylistEntry *)playlistGetItem:(int64_t)index;
- (int64_t)playlistGetCount;
- (PlaylistEntry *)playlistGetCachedItem:(int64_t)index;
- (int64_t)playlistGetCountCached;

- (int64_t)queueGetEntry:(int64_t)queueIndex;
- (int64_t)queueGetCount;

@end

#define NSTableViewDataSource_sqlite_h

#endif /* NSTableViewDataSource_sqlite_h */
