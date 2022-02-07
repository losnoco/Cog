//
//  Node.h
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "ChunkList.h"
#import "Semaphore.h"
#import <Cocoa/Cocoa.h>

#define BUFFER_SIZE 1024 * 1024
#define CHUNK_SIZE 16 * 1024

@interface Node : NSObject {
	ChunkList *buffer;
	Semaphore *semaphore;

	NSRecursiveLock *accessLock;

	id __weak previousNode;
	id __weak controller;

	BOOL shouldReset;

	BOOL shouldContinue;
	BOOL endOfStream; // All data is now in buffer
	BOOL initialBufferFilled;

	AudioStreamBasicDescription nodeFormat;
	uint32_t nodeChannelConfig;
	BOOL nodeLossless;
}
- (id)initWithController:(id)c previous:(id)p;

- (void)writeData:(const void *)ptr amount:(size_t)a;
- (AudioChunk *)readChunk:(size_t)maxFrames;

- (void)process; // Should be overwriten by subclass
- (void)threadEntry:(id)arg;

- (void)launchThread;

- (void)setShouldReset:(BOOL)s;
- (BOOL)shouldReset;

- (void)setPreviousNode:(id)p;
- (id)previousNode;

- (BOOL)shouldContinue;
- (void)setShouldContinue:(BOOL)s;

- (ChunkList *)buffer;
- (void)resetBuffer; // WARNING! DANGER WILL ROBINSON!

- (AudioStreamBasicDescription)nodeFormat;
- (uint32_t)nodeChannelConfig;
- (BOOL)nodeLossless;

- (Semaphore *)semaphore;

//-(void)resetBuffer;

- (BOOL)endOfStream;
- (void)setEndOfStream:(BOOL)e;

- (double)secondsBuffered;

@end
