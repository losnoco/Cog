//
//  SoundController.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/7/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "SoundController.h"
#import "Status.h"

@implementation SoundController

- (id)initWithDelegate:(id)d
{
	DBLog(@"Initializing\n");

	self = [super init];
	if (self)
	{
		//things
		output = NULL;
		bufferChain = NULL;
		
		chainQueue = [[NSMutableArray alloc] init];
		
		delegate = d;
	}
	
	return self;
}

- (void)play:(NSString *)filename
{
	DBLog(@"OPENING FILE: %s\n", filename);

	if (output)
	{
		[output release];
	}
	output = [[OutputNode alloc] initWithController:self previous:nil];
	[output setup];
	
	NSEnumerator *enumerator = [chainQueue objectEnumerator];
	id anObject;
	while (anObject = [enumerator nextObject])
	{
		[anObject setShouldContinue:NO];
	}
	[chainQueue removeAllObjects];
	
	if (bufferChain)
	{
		[bufferChain setShouldContinue:NO];
		[bufferChain release];
	}
	bufferChain = [[BufferChain alloc] initWithController:self];
	[bufferChain open:filename];
		
	[self setShouldContinue:YES];
	DBLog(@"DETACHING THREADS");
	
	[output launchThread];
	[bufferChain launchThreads];
	
	[self setPlaybackStatus:kCogStatusPlaying];
}

- (void)stop
{
	//Set shouldoContinue to NO on allll things
	[self setShouldContinue:NO];
	[self setPlaybackStatus:kCogStatusStopped];
}

- (void)pause
{
	[output pause];

	[self setPlaybackStatus:kCogStatusPaused];	
}

- (void)resume
{
	[output resume];

	[self setPlaybackStatus:kCogStatusPlaying];	
}

- (void)seekToTime:(double)time
{
	//Need to reset everything's buffers, and then seek?
	/*HACK TO TEST HOW WELL THIS WOULD WORK*/
	[bufferChain seek:time];
	[output seek:time];
	
	
	/*END HACK*/
}

- (void)setVolume:(double)v
{
	[output setVolume:v];
}

- (void)setNextSong:(NSString *)s
{
	//Need to lock things, and set it...this may be calling from any threads...also, if its nil then that signals end of playlist
	[s retain];
	[nextSong release];
	nextSong = s;
}


- (void)setShouldContinue:(BOOL)s
{
	[bufferChain setShouldContinue:s];
	[output setShouldContinue:s];
}

- (double)amountPlayed
{
	return [output amountPlayed];
}

- (void)endOfInputReached
{
	[delegate delegateRequestNextSong:[chainQueue count]];

	NSLog(@"END OF INPUT REACHED");

	if (nextSong == nil)
		return;
	
	BufferChain *newChain = [[BufferChain alloc] initWithController:self];

	[newChain open:nextSong];

	[newChain setShouldContinue:YES];
	[newChain launchThreads];
	
	[chainQueue insertObject:newChain atIndex:[chainQueue count]];

	[newChain release];
}

- (void)endOfInputPlayed
{
	if ([chainQueue count] <= 0)
	{
		//End of playlist
		NSLog(@"STOPPED");
		[self stop];
		
		return;
	}
//	NSLog(@"SWAPPING BUFFERS");
	[bufferChain release];
	
	NSLog(@"END OF INPUT PLAYED");
	bufferChain = [chainQueue objectAtIndex:0];
	[bufferChain retain];
	
	[chainQueue removeObjectAtIndex:0];

	[delegate delegateNotifySongChanged];
	[output setEndOfStream:NO];
}

- (void)setPlaybackStatus:(int)s
{	
	[delegate performSelectorOnMainThread:@selector(delegateNotifyStatusUpdate:) withObject:[NSNumber numberWithInt:s] waitUntilDone:NO];
}

- (BufferChain *)bufferChain
{
	return bufferChain;
}

- (OutputNode *) output
{
	return output;
}

@end
