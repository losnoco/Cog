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
    
    paused = YES;

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
    paused = NO;
	[output start];
}

- (void)pause
{
    paused = YES;
	[output pause];
}

- (void)resume
{
    paused = NO;
	[output resume];
}

- (void)reset
{
    [output setup];
    if (!paused)
        [output start];
}

- (int)readData:(void *)ptr amount:(int)amount
{
    @autoreleasepool {
        int n;
        [self setPreviousNode:[[controller bufferChain] finalNode]];
	
        n = [super readData:ptr amount:amount];
        amountPlayed += n;
    
        if (endOfStream == YES)
        {
            amountPlayed = 0;
            [controller endOfInputPlayed]; //Updates shouldContinue appropriately?
        }

/*	if (n == 0) {
		DLog(@"Output Buffer dry!");
	}
*/	
	
        return n;
    }
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

- (BOOL)isPaused
{
    return paused;
}
@end
