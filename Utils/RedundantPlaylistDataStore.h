//
//  RedundantPlaylistDataStore.h
//  Cog
//
//  Created by Christopher Snowhill on 2/16/22.
//

// This is designed primarily for the PlaylistEntry info loader, to prevent
// memory overrun due to redundant blobs of data being duplicated repeatedly
// until the list is fully loaded. This instance will be discarded after the
// info is loaded, freeing up hopefully way less memory afterward than now.

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface RedundantPlaylistDataStore : NSObject {
	NSMutableArray *stringStore;
	NSMutableArray *artStore;
}

- (id)init;
- (NSDictionary *)coalesceEntryInfo:(NSDictionary *)pe;

@end

NS_ASSUME_NONNULL_END
