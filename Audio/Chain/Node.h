//
//  Node.h
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "VirtualRingBuffer.h"
#import "Semaphore.h"

#define BUFFER_SIZE 1024 * 1024
#define CHUNK_SIZE 16 * 1024

@interface Node : NSObject {
	VirtualRingBuffer *buffer;
	Semaphore *semaphore;
	NSLock *readLock;
	NSLock *writeLock;
	
	id __weak previousNode;
	id __weak controller;
	
	BOOL shouldReset;
	
	BOOL shouldContinue;	
	BOOL endOfStream; //All data is now in buffer
	BOOL initialBufferFilled;
}
- (id)initWithController:(id)c previous:(id)p;

- (int)writeData:(void *)ptr amount:(int)a;
- (int)readData:(void *)ptr amount:(int)a;

- (void)process; //Should be overwriten by subclass
- (void)threadEntry:(id)arg;

- (void)launchThread;

- (void)setShouldReset:(BOOL)s;
- (BOOL)shouldReset;

- (NSLock *)readLock;
- (NSLock *)writeLock;

- (void)setPreviousNode:(id)p;
- (id)previousNode;

- (BOOL)shouldContinue;
- (void)setShouldContinue:(BOOL)s;

- (VirtualRingBuffer *)buffer;
- (void)resetBuffer; //WARNING! DANGER WILL ROBINSON!

- (Semaphore *)semaphore;

-(void)resetBuffer;

- (BOOL)endOfStream;
- (void)setEndOfStream:(BOOL)e;

@end
