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

- (void)play:(PlaylistEntry *)pe
{
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

	while (![bufferChain open:pe])
	{
		[bufferChain release];

		[self requestNextEntry:pe];

		pe = nextEntry;
		if (pe == nil)
		{
			return;
		}
		
		[self notifySongChanged:pe];
		bufferChain = [[BufferChain alloc] initWithController:self];
	}

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

- (void)setNextEntry:(PlaylistEntry *)pe
{
	[pe retain];
	[nextEntry release];
	nextEntry = pe;
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


- (void)requestNextEntry:(PlaylistEntry *)pe
{
	[delegate performSelectorOnMainThread:@selector(delegateRequestNextEntry:) withObject:pe waitUntilDone:YES];
}

- (void)notifySongChanged:(PlaylistEntry *)pe
{
	[delegate performSelectorOnMainThread:@selector(delegateNotifySongChanged:) withObject:pe waitUntilDone:NO];
}

- (void)endOfInputReached:(id)sender
{
	BufferChain *newChain = nil;

	nextEntry = [sender playlistEntry];
	
	do {
		[newChain release];
		[self requestNextEntry:nextEntry];

		if (nextEntry == nil)
		{
			return;
		}
		
		newChain = [[BufferChain alloc] initWithController:self];
	} while (![newChain open:nextEntry]);
	
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
		DBLog(@"STOPPED");
		[self stop];
		
		return;
	}
//	NSLog(@"SWAPPING BUFFERS");
	[bufferChain release];
	
	DBLog(@"END OF INPUT PLAYED");
	bufferChain = [chainQueue objectAtIndex:0];
	[bufferChain retain];
	
	[chainQueue removeObjectAtIndex:0];

	[self notifySongChanged:[bufferChain playlistEntry]];
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
