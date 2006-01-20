//
//  InputChainLink.m
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
		endOfInput = NO;
	}
	
	return self;
}

- (int)writeData:(void *)ptr amount:(int)amount
{
	void *writePtr;
	int amountToCopy, availOutput;
	int amountLeft = amount;

	do
	{
		availOutput = [buffer lengthAvailableToWriteReturningPointer:&writePtr];
		while (availOutput < CHUNK_SIZE)
		{
			[semaphore wait];
			
			if (shouldContinue == NO)
			{
				return (amount - amountLeft);
			}
			
			availOutput = [buffer lengthAvailableToWriteReturningPointer:&writePtr];
		}
		amountToCopy = availOutput;
		if (amountToCopy > amountLeft)
			amountToCopy = amountLeft;
		
		memcpy(writePtr, &((char *)ptr)[amount - amountLeft], amountToCopy);
		if (amountToCopy > 0)
		{
			[buffer didWriteLength:amountToCopy];
		}
		
		amountLeft -= amountToCopy;
	} while (amountLeft > 0);
	
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
	[self process];
	
	[pool release];
}

- (int)readData:(void *)ptr amount:(int)amount
{
	void *readPtr;
	int amountToCopy;
	int availInput;
	
	availInput = [[previousNode buffer] lengthAvailableToReadReturningPointer:&readPtr];
	
	amountToCopy = availInput;
	if (availInput > amount)
	{
		amountToCopy = amount;
	}
	
	memcpy(ptr, readPtr, amountToCopy);
	
	if (amountToCopy > 0)
	{
		[[previousNode buffer] didReadLength:amountToCopy];
		[[previousNode semaphore] signal];
	}
	//Do endOfInput fun now...
	if ((amountToCopy <= 0) && ([previousNode endOfInput] == YES))
	{
		endOfInput = YES;
		shouldContinue = NO;
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

- (BOOL)endOfInput
{
	return endOfInput;
}

- (void)setEndOfInput:(BOOL)e
{
	endOfInput = e;
}

@end
