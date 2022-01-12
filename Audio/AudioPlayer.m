
//  AudioController.m
//  Cog
//
//  Created by Vincent Spader on 8/7/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "AudioPlayer.h"
#import "BufferChain.h"
#import "OutputNode.h"
#import "Status.h"
#import "Helper.h"
#import "PluginController.h"


#import "Logging.h"

@implementation AudioPlayer

- (id)init
{
	self = [super init];
	if (self)
	{
		output = NULL;
		bufferChain = nil;
		outputLaunched = NO;
		endOfInputReached = NO;

        chainQueue = [[NSMutableArray alloc] init];
	}
	
	return self;
}

- (void)setDelegate:(id)d
{
	delegate = d;
}

- (id)delegate {
	return delegate;
}

- (void)play:(NSURL *)url
{
	[self play:url withUserInfo:nil withRGInfo:nil startPaused:NO andSeekTo:0.0];
}

- (void)play:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi
{
    [self play:url withUserInfo:userInfo withRGInfo:rgi startPaused:NO andSeekTo:0.0];
}

- (void)play:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi startPaused:(BOOL)paused
{
    [self play:url withUserInfo:userInfo withRGInfo:rgi startPaused:paused andSeekTo:0.0];
}

- (void)play:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi startPaused:(BOOL)paused andSeekTo:(double)time
{
	@synchronized(chainQueue) {
        for (id anObject in chainQueue)
		{
			[anObject setShouldContinue:NO];
		}
		[chainQueue removeAllObjects];
		endOfInputReached = NO;
		if (bufferChain)
		{
			[bufferChain setShouldContinue:NO];

            bufferChain = nil;
		}
	}
    output = [[OutputNode alloc] initWithController:self previous:nil];
    [output setup];
    [output setVolume: volume];

	bufferChain = [[BufferChain alloc] initWithController:self];
	[self notifyStreamChanged:userInfo];
    
    currentStream = url;
    currentUserInfo = userInfo;
    currentRGInfo = rgi;
	
	while (![bufferChain open:url withOutputFormat:[output format] withRGInfo:rgi])
	{
		bufferChain = nil;
		
		[self requestNextStream: userInfo];

		url = nextStream;
		if (url == nil)
		{
			return;
		}
        
		userInfo = nextStreamUserInfo;
        rgi = nextStreamRGInfo;
	
        currentStream = url;
        currentUserInfo = userInfo;
        currentRGInfo = rgi;
    
		[self notifyStreamChanged:userInfo];
		
		bufferChain = [[BufferChain alloc] initWithController:self];
	}

	[bufferChain setUserInfo:userInfo];

    if (time > 0.0)
    {
        [output seek:time];
        [bufferChain seek:time];
    }
    
	[self setShouldContinue:YES];
	
	outputLaunched = NO;
    startedPaused = paused;
    initialBufferFilled = NO;

	[bufferChain launchThreads];
    
    if (paused)
        [self setPlaybackStatus:CogStatusPaused waitUntilDone:YES];
    
    self->paused = paused;
}

- (void)play:(id<CogDecoder>)decoder startPaused:(BOOL)paused
{
    @synchronized(chainQueue) {
        for (id anObject in chainQueue)
        {
            [anObject setShouldContinue:NO];
        }
        [chainQueue removeAllObjects];
        endOfInputReached = NO;
        if (bufferChain)
        {
            [bufferChain setShouldContinue:NO];

            bufferChain = nil;
        }
    }
    output = [[OutputNode alloc] initWithController:self previous:nil];
    [output setup];
    [output setVolume: volume];

    bufferChain = [[BufferChain alloc] initWithController:self];
    [self notifyStreamChanged:currentUserInfo];
    
    NSURL *url = currentStream;
    id userInfo = currentUserInfo;
    NSDictionary *rgi = currentRGInfo;
    
    while (![bufferChain openWithDecoder:decoder withOutputFormat:[output format] withRGInfo:rgi])
    {
        bufferChain = nil;
        
        [self requestNextStream: userInfo];

        url = nextStream;
        if (url == nil)
        {
            return;
        }
        
        userInfo = nextStreamUserInfo;
        rgi = nextStreamRGInfo;
    
        currentStream = url;
        currentUserInfo = userInfo;
        currentRGInfo = rgi;
    
        [self notifyStreamChanged:userInfo];
        
        bufferChain = [[BufferChain alloc] initWithController:self];
    }

    [bufferChain setUserInfo:userInfo];

    [self setShouldContinue:YES];
    
    outputLaunched = NO;
    startedPaused = paused;
    initialBufferFilled = NO;

    [bufferChain launchThreads];
    
    if (paused)
        [self setPlaybackStatus:CogStatusPaused waitUntilDone:YES];
    
    self->paused = paused;
}

- (void)stop
{
	//Set shouldoContinue to NO on all things
	[self setShouldContinue:NO];
	[self setPlaybackStatus:CogStatusStopped waitUntilDone:YES];
}

- (void)pause
{
	[output pause];

	[self setPlaybackStatus:CogStatusPaused waitUntilDone:YES];
    
    paused = YES;
}

- (void)resume
{
    if (startedPaused)
    {
        startedPaused = NO;
        if (initialBufferFilled)
            [self launchOutputThread];
    }
    
	[output resume];

	[self setPlaybackStatus:CogStatusPlaying waitUntilDone:YES];
    
    paused = NO;
}

- (void)seekToTime:(double)time
{
	//Need to reset everything's buffers, and then seek?
	/*HACK TO TEST HOW WELL THIS WOULD WORK*/
	[output seek:time];
	[bufferChain seek:time];
	/*END HACK*/
}

- (void)setVolume:(double)v
{
	volume = v;
	
	[output setVolume:v];
}

- (double)volume
{
	return volume;
}


//This is called by the delegate DURING a requestNextStream request.
- (void)setNextStream:(NSURL *)url
{
	[self setNextStream:url withUserInfo:nil withRGInfo:nil];
}

- (void)setNextStream:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi
{
	nextStream = url;
	
	nextStreamUserInfo = userInfo;

    nextStreamRGInfo = rgi;
}

// Called when the playlist changed before we actually started playing a requested stream. We will re-request.
- (void)resetNextStreams
{
	@synchronized (chainQueue) {
		for (id anObject in chainQueue) {
			[anObject setShouldContinue:NO];
		}
		[chainQueue removeAllObjects];

		if (endOfInputReached) {
			[self endOfInputReached:bufferChain];
		} 
	}
}

- (void)setShouldContinue:(BOOL)s
{
	if (bufferChain)
		[bufferChain setShouldContinue:s];
		
	if (output)
		[output setShouldContinue:s];
}

- (double)amountPlayed
{
	return [output amountPlayed];
}

- (void)launchOutputThread
{
    initialBufferFilled = YES;
	if (outputLaunched == NO && startedPaused == NO) {
		[self setPlaybackStatus:CogStatusPlaying];	
		[output launchThread];
		outputLaunched = YES;
	}
}

- (void)requestNextStream:(id)userInfo
{
	[self sendDelegateMethod:@selector(audioPlayer:willEndStream:) withObject:userInfo waitUntilDone:YES];
}

- (void)notifyStreamChanged:(id)userInfo
{
	[self sendDelegateMethod:@selector(audioPlayer:didBeginStream:) withObject:userInfo waitUntilDone:YES];
}

- (void)addChainToQueue:(BufferChain *)newChain
{	
	[newChain setUserInfo: nextStreamUserInfo];
	
	[newChain setShouldContinue:YES];
	[newChain launchThreads];

    [chainQueue insertObject:newChain atIndex:[chainQueue count]];
}

- (BOOL)endOfInputReached:(BufferChain *)sender //Sender is a BufferChain
{
    // Stop single or series of short tracks from queueing forever
    {
        unsigned long queueCount;

        @synchronized (chainQueue) {
            queueCount = [chainQueue count];
        }

        while (queueCount >= 5)
        {
            usleep(10000);
            @synchronized (chainQueue) {
                queueCount = [chainQueue count];
            }
        }
    }

    return [self endOfInputReachedInternal:sender];
}

- (BOOL)endOfInputReachedInternal:(BufferChain *)sender //Sender is a BufferChain
{
    BufferChain *newChain = nil;
    
	@synchronized (chainQueue) {
        // No point in constructing new chain for the next playlist entry
        // if there's already one at the head of chainQueue... r-r-right?
        for (BufferChain *chain in chainQueue)
        {
            if ([chain isRunning])
            {
                return YES;
            }
        }

        // We don't want to do this, it may happen with a lot of short files
        //if ([chainQueue count] >= 5)
        //{
        //    return YES;
        //}

		nextStreamUserInfo = [sender userInfo];
		
        nextStreamRGInfo = [sender rgInfo];
    }

    // This call can sometimes lead to invoking a chainQueue block on another thread
	[self requestNextStream: nextStreamUserInfo];
    
    if (!nextStream)
        return YES;

    @synchronized (chainQueue) {
		newChain = [[BufferChain alloc] initWithController:self];
	
		endOfInputReached = YES;
		
		BufferChain *lastChain = [chainQueue lastObject];
		if (lastChain == nil) {
			lastChain = bufferChain;
		}
        
		if ([[nextStream scheme] isEqualToString:[[lastChain streamURL] scheme]]
			&& (([nextStream host] == nil &&
                 [[lastChain streamURL] host] == nil)
                || [[nextStream host] isEqualToString:[[lastChain streamURL] host]])
			&& [[nextStream path] isEqualToString:[[lastChain streamURL] path]])
		{
			if ([lastChain setTrack:nextStream] 
				&& [newChain openWithInput:[lastChain inputNode] withOutputFormat:[output format] withRGInfo:nextStreamRGInfo])
			{
				[newChain setStreamURL:nextStream];
				[newChain setUserInfo:nextStreamUserInfo];

				[self addChainToQueue:newChain];
				DLog(@"TRACK SET!!! %@", newChain);
				//Keep on-playin
                newChain = nil;
				
				return NO;
			}
		}
        
        lastChain = nil;
		
		while (![newChain open:nextStream withOutputFormat:[output format] withRGInfo:nextStreamRGInfo])
		{
			if (nextStream == nil)
			{
                newChain = nil;
				return YES;
			}
			
            newChain = nil;
			[self requestNextStream: nextStreamUserInfo];

			newChain = [[BufferChain alloc] initWithController:self];
		}
		
		[self addChainToQueue:newChain];
        
        newChain = nil;

        // I'm stupid and can't hold too much stuff in my head all at once, so writing it here.
        //
        // Once we get here:
        // - buffer chain for previous stream finished reading
        // - there are (probably) some bytes of the previous stream in the output buffer which haven't been played
        //   (by output node) yet
        // - self.bufferChain == previous playlist entry's buffer chain
        // - self.nextStream == next playlist entry's URL
        // - self.nextStreamUserInfo == next playlist entry
        // - head of chainQueue is the buffer chain for the next entry (which has launched its threads already)
	}
	
	return YES;
}

- (void)endOfInputPlayed
{
    // Once we get here:
    // - the buffer chain for the next playlist entry (started in endOfInputReached) have been working for some time
    //   already, so that there is some decoded and converted data to play
    // - the buffer chain for the next entry is the first item in chainQueue

	@synchronized(chainQueue) {
		endOfInputReached = NO;
		
		if ([chainQueue count] <= 0)
		{
			//End of playlist
			[self stop];
			
			bufferChain = nil;
			
			return;
		}
		
        bufferChain = nil;
        bufferChain = [chainQueue objectAtIndex:0];

        [chainQueue removeObjectAtIndex:0];
		DLog(@"New!!! %@ %@", bufferChain, [[bufferChain inputNode] decoder]);
	}
	
	[self notifyStreamChanged:[bufferChain userInfo]];
	[output setEndOfStream:NO];
}

- (void)sendDelegateMethod:(SEL)selector withObject:(id)obj waitUntilDone:(BOOL)wait
{
	NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[delegate methodSignatureForSelector:selector]];
    [invocation setTarget:delegate];
	[invocation setSelector:selector];
    [invocation setArgument:(void*)&self atIndex:2];
	[invocation setArgument:&obj	     atIndex:3];
    [invocation retainArguments];
	
	[invocation performSelectorOnMainThread:@selector(invoke) withObject:nil waitUntilDone:wait];
}

- (void)sendDelegateMethod:(SEL)selector withObject:(id)obj withObject:(id)obj2 waitUntilDone:(BOOL)wait
{
	NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[delegate methodSignatureForSelector:selector]];
    [invocation setTarget:delegate];
    [invocation setSelector:selector];
    [invocation setArgument:(void*)&self atIndex:2];
	[invocation setArgument:&obj	     atIndex:3];
	[invocation setArgument:&obj2	     atIndex:4];
    [invocation retainArguments];
	
	[invocation performSelectorOnMainThread:@selector(invoke) withObject:nil waitUntilDone:wait];
}


- (void)setPlaybackStatus:(int)status waitUntilDone:(BOOL)wait
{	
	[self sendDelegateMethod:@selector(audioPlayer:didChangeStatus:userInfo:) withObject:[NSNumber numberWithInt:status] withObject:[bufferChain userInfo] waitUntilDone:wait];
}

- (void)setPlaybackStatus:(int)status
{	
	[self setPlaybackStatus:status waitUntilDone:NO];
}

- (BufferChain *)bufferChain
{
	return bufferChain;
}

- (OutputNode *) output
{
	return output;
}

+ (NSArray *)containerTypes
{
	return [[[PluginController sharedPluginController] containers] allKeys];
}

+ (NSArray *)fileTypes
{
	PluginController *pluginController = [PluginController sharedPluginController];
	
	NSArray *containerTypes = [[pluginController containers] allKeys];
	NSArray *decoderTypes = [[pluginController decodersByExtension] allKeys];
	NSArray *metdataReaderTypes = [[pluginController metadataReaders] allKeys];
	NSArray *propertiesReaderTypes = [[pluginController propertiesReadersByExtension] allKeys];
	
	NSMutableSet *types = [NSMutableSet set];
	
	[types addObjectsFromArray:containerTypes];
	[types addObjectsFromArray:decoderTypes];
	[types addObjectsFromArray:metdataReaderTypes];
	[types addObjectsFromArray:propertiesReaderTypes];
	
	return [types allObjects];
}

+ (NSArray *)schemes
{
	PluginController *pluginController = [PluginController sharedPluginController];
	
	return [[pluginController sources] allKeys];
}

- (double)volumeUp:(double)amount
{
	double newVolume = linearToLogarithmic(logarithmicToLinear(volume + amount));
	if (newVolume > MAX_VOLUME)
		newVolume = MAX_VOLUME;
	
	[self setVolume:newVolume];
	
	// the playbackController needs to know the new volume, so it can update the
	// volumeSlider accordingly.
	return newVolume;
}

- (double)volumeDown:(double)amount
{
	double newVolume;
	if (amount > volume)
		newVolume = 0.0;
	else
		newVolume = linearToLogarithmic(logarithmicToLinear(volume - amount));
	
	[self setVolume:newVolume];
	return newVolume;
}


@end
