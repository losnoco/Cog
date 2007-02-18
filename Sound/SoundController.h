//
//  SoundController.h
//  Cog
//
//  Created by Vincent Spader on 8/7/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "BufferChain.h"
#import "OutputNode.h"
#import "PlaylistEntry.h"

@class BufferChain;
@class OutputNode;

@interface SoundController : NSObject {	
	BufferChain *bufferChain;
	OutputNode *output;

	NSMutableArray *chainQueue;
	
	PlaylistEntry *nextEntry; //Updated whenever the playlist changes?
	
	id delegate;
}

- (OutputNode *) output;
- (BufferChain *) bufferChain;
- (id)initWithDelegate:(id)d;

- (void)play:(PlaylistEntry *)pe;
- (void)stop;
- (void)pause;
- (void)resume;

- (void)seekToTime:(double)time;
- (void)setVolume:(double)v;

- (double)amountPlayed;


- (void)setNextEntry:(PlaylistEntry *)pe;
- (void)setPlaybackStatus:(int)s;


@end
