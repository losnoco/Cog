//
//  Node.m
//  CogNew
//
//  Created by Zaphod Beeblebrox on 1/4/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import "Node.h"

@implementation Node

- (id)initWithController:(id)c previous:(id)p
{
	self = [super init];
	if (self)
	{
		buffer = [[VirtualRingBuffer alloc] initWithLength:BUFFER_SIZE];
		semaphore = [[Semaphore alloc] init];
		
		controller = c;
		previousNode = p;
		endOfStream = NO;
		shouldContinue = YES;
	}
	
	return self;
}

- (int)writeData:(void *)ptr amount:(int)amount
{
	void *writePtr;
	int amountToCopy, availOutput;
	int amountLeft = amount;
	
	while (shouldContinue == YES && amountLeft > 0)
	{
		availOutput = [buffer lengthAvailableToWriteReturningPointer:&writePtr];
		
		if (availOutput == 0)
		{
			[semaphore wait];
		}
		else
		{
			amountToCopy = availOutput;
			if (amountToCopy > amountLeft)
				amountToCopy = amountLeft;
			
			memcpy(writePtr, &((char *)ptr)[amount - amountLeft], amountToCopy);
			if (amountToCopy > 0)
			{
				[buffer didWriteLength:amountToCopy];
			}
			
			amountLeft -= amountToCopy;
		}
	}
	
	return (amount - amountLeft);
}

//Should be overwriten by subclass.
- (void)process
{
	DBLog(@"WRONG PROCESS");
}

- (void)threadEntry:(id)arg
{
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
	DBLog(@"In thread entry");
	[self retain];

	[self process];

	[self release];

	[pool release];
}

- (int)readData:(void *)ptr amount:(int)amount
{
	void *readPtr;
	int amountToCopy;
	int availInput;
	
	availInput = [[previousNode buffer] lengthAvailableToReadReturningPointer:&readPtr];
	
	if (availInput <= amount && [previousNode endOfStream] == YES)
	{
//		NSLog(@"RELEASING: %i %i %i", availInput, [previousNode endOfStream], shouldContinue);
//		[previousNode release]; 
		//If it is the outputNode, [soundController newInputChain];
		//else
		endOfStream = YES;
	}	

	amountToCopy = availInput;
	if (amountToCopy > amount)
	{
		amountToCopy = amount;
	}
	
	memcpy(ptr, readPtr, amountToCopy);
	
	if (amountToCopy > 0)
	{
		[[previousNode buffer] didReadLength:amountToCopy];
		
		[[previousNode semaphore] signal];
	}
	
	return amountToCopy;
}

- (void)launchThread
{
	DBLog(@"THREAD LAUNCHED");
	[NSThread detachNewThreadSelector:@selector(threadEntry:) toTarget:self withObject:nil];
}

- (id)previousNode
{
	return previousNode;
}

- (BOOL)shouldContinue
{
	return shouldContinue;
}

- (void)setShouldContinue:(BOOL)s
{
	shouldContinue = s;
}

- (VirtualRingBuffer *)buffer
{
	return buffer;
}

- (Semaphore *)semaphore
{
	return semaphore;
}

- (BOOL)endOfStream
{
	return endOfStream;
}

- (void)setEndOfStream:(BOOL)e
{
	endOfStream = e;
}

@end
