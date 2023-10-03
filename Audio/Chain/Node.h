//
//  Node.h
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "ChunkList.h"
#import <CogAudio/CogSemaphore.h>
#import <Cocoa/Cocoa.h>

#import <os/workgroup.h>

#define BUFFER_SIZE 1024 * 1024
#define CHUNK_SIZE 16 * 1024

@interface Node : NSObject {
	ChunkList *buffer;
	Semaphore *semaphore;

	NSLock *accessLock;

	id __weak previousNode;
	id __weak controller;

	BOOL shouldReset;

	BOOL shouldContinue;
	BOOL endOfStream; // All data is now in buffer
	BOOL initialBufferFilled;

	AudioStreamBasicDescription nodeFormat;
	uint32_t nodeChannelConfig;
	BOOL nodeLossless;

	double durationPrebuffer;
}
- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p;

- (void)writeData:(const void *_Nonnull)ptr amount:(size_t)a;
- (void)writeChunk:(AudioChunk *_Nonnull)chunk;
- (AudioChunk *_Nonnull)readChunk:(size_t)maxFrames;
- (AudioChunk *_Nonnull)readChunkAsFloat32:(size_t)maxFrames;

- (BOOL)peekFormat:(AudioStreamBasicDescription *_Nonnull)format channelConfig:(uint32_t *_Nonnull)config;

- (void)process; // Should be overwriten by subclass
- (void)threadEntry:(id _Nullable)arg;

- (void)launchThread;

- (void)setShouldReset:(BOOL)s;
- (BOOL)shouldReset;

- (void)setPreviousNode:(id _Nullable)p;
- (id _Nullable)previousNode;

- (BOOL)shouldContinue;
- (void)setShouldContinue:(BOOL)s;

- (ChunkList *_Nonnull)buffer;
- (void)resetBuffer; // WARNING! DANGER WILL ROBINSON!

- (AudioStreamBasicDescription)nodeFormat;
- (uint32_t)nodeChannelConfig;
- (BOOL)nodeLossless;

- (Semaphore *_Nonnull)semaphore;

//-(void)resetBuffer;

- (BOOL)endOfStream;
- (void)setEndOfStream:(BOOL)e;

- (double)secondsBuffered;

@end
