//
//  OutputController.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "OutputNode.h"
#import "OutputCoreAudio.h"

@implementation OutputNode

- (void)setup
{
	output = [[OutputCoreAudio alloc] initWithController:self];
	
	[output setup];
}

- (void)process
{
	[output start];
}

- (int)readData:(void *)ptr amount:(int)amount
{
	int n;
	
	previousNode = [[controller bufferChain] finalNode];
	
	n = [super readData:ptr amount:amount];
	if ((n == 0) && (endOfInput == YES))
	{
		endOfInput = NO;
		shouldContinue = YES;
		NSLog(@"DONE IN");
		return 0;
	}
	
	void *tempPtr;
	
	if (([[[[controller bufferChain] finalNode] buffer] lengthAvailableToReadReturningPointer:&tempPtr] == 0) && ([[[controller bufferChain] finalNode] endOfInput] == YES))
	{
		NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];		
		
		NSLog(@"END OF OUTPUT INPUT?!");
		[controller endOfInputPlayed];
		endOfInput = YES;

		[pool release];

		return n + [self readData:&ptr[n] amount:(amount-n)];
		

	}
	
	return n;
}

- (AudioStreamBasicDescription) format
{
	return format;
}

- (void)setFormat:(AudioStreamBasicDescription *)f
{
	format = *f;
}

- (void)setVolume:(double) v
{
	[output setVolume:v];
}

- (void)setShouldContinue:(BOOL)s
{
	[super setShouldContinue:s];
	
	if (s == NO)
		[output stop];
}
@end
