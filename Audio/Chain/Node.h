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

#import <os/workgroup.h>

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
	BOOL isRealtime, isRealtimeError, isDeadlineError; // If was successfully set realtime, or if error

	AudioStreamBasicDescription nodeFormat;
	uint32_t nodeChannelConfig;
	BOOL nodeLossless;

	int64_t intervalMachLength;
	os_workgroup_interval_t workgroup, wg;
	os_workgroup_join_token_s wgToken;
}
- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p;

- (void)writeData:(const void *_Nonnull)ptr amount:(size_t)a;
- (AudioChunk *_Nonnull)readChunk:(size_t)maxFrames;

- (BOOL)peekFormat:(AudioStreamBasicDescription *_Nonnull)format channelConfig:(uint32_t *_Nonnull)config;

- (void)process; // Should be overwriten by subclass
- (void)threadEntry:(id _Nullable)arg;

- (BOOL)followWorkgroup;
- (void)leaveWorkgroup;
- (void)startWorkslice;
- (void)endWorkslice;

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
