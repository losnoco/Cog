//
//  VirtualRingBuffer.m
//  PlayBufferedSoundFile
//
/*
 Copyright (c) 2002, Kurt Revis.  All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of Snoize nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#import "VirtualRingBuffer.h"

#import <Foundation/Foundation.h>
#import <stdlib.h>
#import <mm_malloc.h>

#import "Logging.h"

@interface block_chunk : NSObject {
    void * blockPointer;
    void * theBlock;
    size_t blockSize;
}

@property void * blockPointer;
@property void * theBlock;
@property size_t blockSize;
@end

@implementation block_chunk
@synthesize blockPointer;
@synthesize theBlock;
@synthesize blockSize;
@end

@interface VirtualBufferHolder : NSObject {
    NSMutableArray * blocks;
    NSMutableArray * blocksUsed;
    NSMutableDictionary * blockRefCounts;
}

+ (VirtualBufferHolder *) sharedInstance;

- (void *) allocateBlock:(size_t)size;
- (void) freeBlock:(void *)block;
@end


@implementation VirtualRingBuffer


- (id)initWithLength:(UInt32)length
{
    if (![super init])
        return nil;

    // We need to allocate entire VM pages, so round the specified length up to the next page if necessary.
    bufferLength = (UInt32) round_page(length);

    buffer = [[VirtualBufferHolder sharedInstance] allocateBlock:bufferLength];
    if (!buffer)
    {
        self = nil;
        return nil;
    }

    atomic_init(&readPointer, 0);
    atomic_init(&writePointer, 0);
    atomic_init(&bufferFilled, 0);

    accessLock = [[NSRecursiveLock alloc] init];
    
    return self;
}

- (void)dealloc
{
    if (buffer)
        [[VirtualBufferHolder sharedInstance] freeBlock:buffer];
}

- (void)empty
{
    // Assumption:
    // No one is reading or writing from the buffer, in any thread, when this method is called.
    [accessLock lock];
    atomic_init(&readPointer, 0);
    atomic_init(&writePointer, 0);
    atomic_init(&bufferFilled, 0);
    [accessLock unlock];
}

- (BOOL)isEmpty
{
    return (atomic_load_explicit(&bufferFilled, memory_order_relaxed) == 0);
}


- (UInt32)bufferedLength
{
	return atomic_load_explicit(&bufferFilled, memory_order_relaxed);
}

//
// Theory of operation:
//
// This class keeps a pointer to the next byte to be read (readPointer) and a pointer to the next byte to be written (writePointer).
// readPointer is only advanced in the reading thread (except for one case: when the buffer first has data written to it).
// writePointer is only advanced in the writing thread.
//
// Since loading and storing word length data is atomic, each pointer can safely be modified in one thread while the other thread
// uses it, IF each thread is careful to make a local copy of the "opposite" pointer when necessary.
// 

//
// Read operations
//

- (UInt32)lengthAvailableToReadReturningPointer:(void **)returnedReadPointer
{
    // Assumptions:
    // returnedReadPointer != NULL
    [accessLock lock];

    UInt32 length;
    // Read this pointer exactly once, so we're safe in case it is changed in another thread
    int localReadPointer = atomic_load_explicit(&readPointer, memory_order_relaxed);
    int localBufferFilled = atomic_load_explicit(&bufferFilled, memory_order_relaxed);
    
    length = bufferLength - localReadPointer;
    if (length > localBufferFilled)
        length = localBufferFilled;

    // Depending on out-of-order execution and memory storage, either one of these may be NULL when the buffer is empty. So we must check both.

    *returnedReadPointer = buffer + localReadPointer;
    
    if (!length)
        [accessLock unlock];
    
    return length;
}

- (void)didReadLength:(UInt32)length
{
    // Assumptions:
    // [self lengthAvailableToReadReturningPointer:] currently returns a value >= length
    // length > 0
    

    if (atomic_fetch_add(&readPointer, length) + length >= bufferLength)
        atomic_fetch_sub(&readPointer, bufferLength);

    atomic_fetch_sub(&bufferFilled, length);
    
    [accessLock unlock];
}


//
// Write operations
//

- (UInt32)lengthAvailableToWriteReturningPointer:(void **)returnedWritePointer
{
    // Assumptions:
    // returnedWritePointer != NULL
    [accessLock lock];
    
    UInt32 length;
    // Read this pointer exactly once, so we're safe in case it is changed in another thread
    int localWritePointer = atomic_load_explicit(&writePointer, memory_order_relaxed);
    int localBufferFilled = atomic_load_explicit(&bufferFilled, memory_order_relaxed);

    length = bufferLength - localBufferFilled;
    if (length > bufferLength - localWritePointer)
        length = bufferLength - localWritePointer;

    *returnedWritePointer = buffer + localWritePointer;
    
    if (!length)
        [accessLock unlock];
    
    return length;
}

- (void)didWriteLength:(UInt32)length
{
    // Assumptions:
    // [self lengthAvailableToWriteReturningPointer:] currently returns a value >= length
    // length > 0
    
    if (atomic_fetch_add(&writePointer, length) + length >= bufferLength)
        atomic_fetch_sub(&writePointer, bufferLength);
    
    atomic_fetch_add(&bufferFilled, length);

    [accessLock unlock];
}

@end

@implementation VirtualBufferHolder

static VirtualBufferHolder * g_instance = nil;

+ (VirtualBufferHolder *) sharedInstance
{
    @synchronized (g_instance) {
        if (!g_instance) {
            g_instance = [[VirtualBufferHolder alloc] init];
        }
        return g_instance;
    }
}

- (id) init {
    self = [super init];
    
    if (self) {
        blocks = [[NSMutableArray alloc] init];
        blocksUsed = [[NSMutableArray alloc] init];
        blockRefCounts = [[NSMutableDictionary alloc] init];
    }
    
    return self;
}

- (void *)allocateBlock:(size_t)size {
    @synchronized(blocks) {
    tryagain:
        for (block_chunk * chunk in blocks) {
            if (chunk.blockSize == size) {
                [blocksUsed addObject:chunk];
                [blocks removeObject:chunk];
                NSInteger refCount = [[blockRefCounts objectForKey:[NSNumber numberWithLongLong:(uintptr_t)chunk.theBlock]] integerValue];
                [blockRefCounts setObject:[NSNumber numberWithInteger:refCount + 1] forKey:[NSNumber numberWithLongLong:(uintptr_t)chunk.theBlock]];
                return chunk.blockPointer;
            }
        }
        if (![blocks count]) {
            void * theBlock = _mm_malloc(32 * 1024 * 1024, 1024);
            if (!theBlock) return NULL;
            
            @synchronized (blocks) {
                block_chunk * chunk = [[block_chunk alloc] init];
                
                chunk.theBlock = theBlock;
                chunk.blockPointer = theBlock;
                chunk.blockSize = 4 * 1024 * 1024;
                
                [blocks addObject:chunk];
                
                chunk = [[block_chunk alloc] init];
                
                chunk.theBlock = theBlock;
                chunk.blockPointer = theBlock + 4 * 1024 * 1024;
                chunk.blockSize = 4 * 1024 * 1024;
                
                [blocks addObject:chunk];
                
                for (size_t i = 8 * 1024 * 1024; i < 32 * 1024 * 1024; i += 1024 * 1024) {
                    chunk = [[block_chunk alloc] init];
                    
                    chunk.theBlock = theBlock;
                    chunk.blockPointer = theBlock + i;
                    chunk.blockSize = 1024 * 1024;
                    
                    [blocks addObject:chunk];
                }
            }
            goto tryagain;
        }
    }
    
    return NULL;
}

- (void) freeBlock:(void *)block {
    @synchronized(blocks) {
        for (block_chunk * chunk in blocksUsed) {
            if (chunk.blockPointer == block) {
                [blocks addObject:chunk];
                [blocksUsed removeObject:chunk];
                NSInteger refCount = [[blockRefCounts objectForKey:[NSNumber numberWithLongLong:(uintptr_t)chunk.theBlock]] integerValue];
                if (refCount <= 1) {
                    [blockRefCounts removeObjectForKey:[NSNumber numberWithLongLong:(uintptr_t)chunk.theBlock]];
                    NSArray * blocksCopy = [blocks copy];
                    for (block_chunk * removeChunk in blocksCopy) {
                        if (removeChunk.theBlock == chunk.theBlock) {
                            [blocks removeObject:removeChunk];
                        }
                    }
                    _mm_free(chunk.theBlock);
                }
                else {
                    [blockRefCounts setObject:[NSNumber numberWithInteger:refCount - 1] forKey:[NSNumber numberWithLongLong:(uintptr_t)chunk.theBlock]];
                }
                return;
            }
        }
    }
}

@end
