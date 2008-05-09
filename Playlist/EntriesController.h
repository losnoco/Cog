//
//  EntriesController.h
//  Cog
//
//  Created by Vincent Spader on 2/10/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PlaylistEntry.h"

@interface EntriesController : NSObject {
	NSUndoManager *undoManager;
	NSMutableArray *playlistEntries;
}

- (NSUndoManager *)undoManager;
- (NSMutableArray *)entries;
- (void)setEntries:(NSMutableArray *)array;
- (void)insertObject:(PlaylistEntry *)pe inEntriesAtIndex:(int)index;
- (void)removeObjectFromEntriesAtIndex:(int)index;


@end
