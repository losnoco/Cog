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

@implementation OutputNode

- (void)setup
{
	amountPlayed = 0;

	output = [[OutputCoreAudio alloc] initWithController:self];
	
	[output setup];
}

- (void)seek:(double)time
{
	amountPlayed = time*format.mBytesPerFrame*(format.mSampleRate/1000.0);

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
	previousNode = [[controller bufferChain] finalNode];
	
	n = [super readData:ptr amount:amount];
//	NSLog(@"N: %i %i", n, endOfStream);
	if (endOfStream == YES)
	{
		DBLog(@"End of stream reached: %i", endOfStream);

		amountPlayed = 0;
		[controller endOfInputPlayed]; //Updates shouldContinue appropriately?
		DBLog(@"End of stream reached: %i", endOfStream);
//		return (n + [self readData:ptr amount:(amount-n)]);
	}
	
	amountPlayed += n;

	[pool release];
	
	return n;
}


- (double)amountPlayed
{

	return (amountPlayed/format.mBytesPerFrame)/(format.mSampleRate/1000.0);
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
