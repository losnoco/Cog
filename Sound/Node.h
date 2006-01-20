//
//  InputChainLink.h
//  CogNew
//
//  Created by Zaphod Beeblebrox on 1/4/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "VirtualRingBuffer.h"
#import "Semaphore.h"

#define BUFFER_SIZE 512 * 1024
#define CHUNK_SIZE 16 * 1024

@interface Node : NSObject {
	VirtualRingBuffer *buffer;
	Semaphore *semaphore;
	
	id previousNode;
	id controller;
	
	BOOL shouldContinue;	
	BOOL endOfInput; //All data is now in buffer
}
- (id)initWithPrevious:(id)p;

- (int)writeData:(void *)ptr amount:(int)a;
- (int)readData:(void *)ptr amount:(int)a;

- (void)process; //Should be overwriten by subclass
- (void)threadEntry:(id)arg;

- (void)launchThread;

- (id)previousNode;

- (BOOL)shouldContinue;
- (void)setShouldContinue:(BOOL)s;

- (VirtualRingBuffer *)buffer;

- (Semaphore *)semaphore;

- (BOOL)endOfInput;
- (void)setEndOfInput:(BOOL)e;

@end
