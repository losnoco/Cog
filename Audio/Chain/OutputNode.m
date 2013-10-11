//
//  OutputNode.m
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "OutputNode.h"
#import "OutputCoreAudio.h"
#import "AudioPlayer.h"
#import "BufferChain.h"

#import "Logging.h"

@implementation OutputNode

- (void)setup
{
	amountPlayed = 0;

	output = [[OutputCoreAudio alloc] initWithController:self];
	
	[output setup];
}

- (void)seek:(double)time
{
//	[output pause];

	amountPlayed = time*format.mBytesPerFrame*(format.mSampleRate);
}

- (void)process
{
	[output start];
}

- (void)pause
{
	[output pause];
}

- (void)resume
{
	[output resume];
}

- (int)readData:(void *)ptr amount:(int)amount
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	int n;
	[self setPreviousNode:[[controller bufferChain] finalNode]];
	
	n = [super readData:ptr amount:amount];
	if (endOfStream == YES)
	{
		amountPlayed = 0;
		[controller endOfInputPlayed]; //Updates shouldContinue appropriately?
	}

/*	if (n == 0) {
		DLog(@"Output Buffer dry!");
	}
*/	
	amountPlayed += n;

	[pool release];
	
	return n;
}


- (double)amountPlayed
{
	return (amountPlayed/format.mBytesPerFrame)/(format.mSampleRate);
}

- (AudioStreamBasicDescription) format
{
	return format;
}

- (void)setFormat:(AudioStreamBasicDescription *)f
{
	format = *f;
}

- (void)close
{
	[output stop];
}

- (void)dealloc
{
	[output release];

	[super dealloc];
}

- (void)setVolume:(double) v
{
	[output setVolume:v];
}

- (void)setShouldContinue:(BOOL)s
{
	[super setShouldContinue:s];
	
//	if (s == NO)
//		[output stop];
}
@end
