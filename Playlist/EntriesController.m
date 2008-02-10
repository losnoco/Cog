//
//  EntriesController.m
//  Cog
//
//  Created by Vincent Spader on 2/10/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "EntriesController.h"

#define UNDO_STACK_LIMIT 0

@implementation EntriesController

- (id)init
{
	self = [super init];
	
	if (self)
	{
		playlistEntries = [[NSMutableArray alloc] init];
		undoManager = [[NSUndoManager alloc] init];

		[undoManager setLevelsOfUndo:UNDO_STACK_LIMIT];
	}
	
	return self;
}

- (void)dealloc
{
	[playlistEntries release];
	[undoManager release];
	
	[super dealloc];
}

- (NSUndoManager *)undoManager
{
	return undoManager;
}

- (NSMutableArray *)entries
{
	return playlistEntries;
}

- (void)setEntries:(NSMutableArray *)array
{
	if (array == playlistEntries)
		return;
		
	[array retain];
	[playlistEntries release];
	playlistEntries = array;
}

- (void)insertObject:(PlaylistEntry *)pe inEntriesAtIndex:(int)index
{
	[[[self undoManager] prepareWithInvocationTarget:self] removeObjectFromEntriesAtIndex:index];
	[playlistEntries insertObject:pe atIndex:index];
}

- (void)removeObjectFromEntriesAtIndex:(int)index
{
	[[[self undoManager] prepareWithInvocationTarget:self] insertObject:[playlistEntries objectAtIndex:index] inEntriesAtIndex:index];
	
	[playlistEntries removeObjectAtIndex:index];
}


@end
