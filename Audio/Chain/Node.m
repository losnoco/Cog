//
//  Node.m
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "Node.h"

#import "Logging.h"
#import "BufferChain.h"

@implementation Node

- (id)initWithController:(id)c previous:(id)p
{
	self = [super init];
	if (self)
	{
		buffer = [[VirtualRingBuffer alloc] initWithLength:BUFFER_SIZE];
		semaphore = [[Semaphore alloc] init];
		readLock = [[NSLock alloc] init];
		writeLock = [[NSLock alloc] init];
		
		initialBufferFilled = NO;
		
		controller = c;
		endOfStream = NO;
		shouldContinue = YES;

		[self setPreviousNode:p];
	}
	
	return self;
}

- (int)writeData:(void *)ptr amount:(int)amount
{
	void *writePtr;
	int amountToCopy, availOutput;
	int amountLeft = amount;
	
	[writeLock lock];
	while (shouldContinue == YES && amountLeft > 0)
	{
		availOutput = [buffer lengthAvailableToWriteReturningPointer:&writePtr];
		if (availOutput == 0) {
			if (initialBufferFilled == NO) {
				initialBufferFilled = YES;
				if ([controller respondsToSelector:@selector(initialBufferFilled:)])
					[controller performSelector:@selector(initialBufferFilled:) withObject:self];
			}
		}
		
		if (availOutput == 0 || shouldReset)
		{
			[writeLock unlock];
			[semaphore wait];
			[writeLock lock];
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
	[writeLock unlock];
	
	return (amount - amountLeft);
}

//Should be overwriten by subclass.
- (void)process
{
}

- (void)threadEntry:(id)arg
{
    @autoreleasepool {
        [self process];
    }
}

- (int)readData:(void *)ptr amount:(int)amount
{
	void *readPtr;
	int amountToCopy;
	int availInput;
    
    if ([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES)
    {
        endOfStream = YES;
        return 0;
    }
	
	[readLock lock];
	availInput = [[previousNode buffer] lengthAvailableToReadReturningPointer:&readPtr];
	
/*	if (availInput <= 0) {
		DLog(@"BUFFER RAN DRY!");
	}
	else if (availInput < amount) {
		DLog(@"BUFFER IN DANGER");
	}
*/

	if ([previousNode shouldReset] == YES) {
		[writeLock lock];

		[buffer empty];

		shouldReset = YES;
		[previousNode setShouldReset: NO];
		[writeLock unlock];

		[[previousNode semaphore] signal];
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
	[readLock unlock];
	
	return amountToCopy;
}

- (void)launchThread
{
	[NSThread detachNewThreadSelector:@selector(threadEntry:) toTarget:self withObject:nil];
}

- (void)setPreviousNode:(id)p
{
	previousNode = p;
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

- (void)resetBuffer
{
	shouldReset = YES; //Will reset on next write.
	if (previousNode == nil) {
		[readLock lock];
		[buffer empty];
		[readLock unlock];
	}
}

- (NSLock *)readLock
{
	return readLock;
}

- (NSLock *)writeLock
{
	return writeLock;
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

- (void)setShouldReset:(BOOL)s
{
	shouldReset = s;
}
- (BOOL)shouldReset
{
	return shouldReset;
}


@end
