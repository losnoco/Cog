
//  AudioController.m
//  Cog
//
//  Created by Vincent Spader on 8/7/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "AudioPlayer.h"
#import "BufferChain.h"
#import "Helper.h"
#import "OutputNode.h"
#import "PluginController.h"
#import "Status.h"

#import "Logging.h"

@implementation AudioPlayer {
	BOOL stoppedRecently;
}

- (id)init {
	self = [super init];
	if(self) {
		output = NULL;
		bufferChain = nil;
		outputLaunched = NO;
		endOfInputReached = NO;
		stoppedRecently = NO;

		// Safety
		pitch = 1.0;
		tempo = 1.0;

		chainQueue = [NSMutableArray new];

		semaphore = [Semaphore new];

		atomic_init(&resettingNow, false);
		atomic_init(&refCount, 0);
	}

	return self;
}

- (void)setDelegate:(id)d {
	delegate = d;
}

- (id)delegate {
	return delegate;
}

- (void)play:(NSURL *)url {
	[self play:url withUserInfo:nil withRGInfo:nil startPaused:NO andSeekTo:0.0];
}

- (void)play:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi {
	[self play:url withUserInfo:userInfo withRGInfo:rgi startPaused:NO andSeekTo:0.0];
}

- (void)play:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi startPaused:(BOOL)paused {
	[self play:url withUserInfo:userInfo withRGInfo:rgi startPaused:paused andSeekTo:0.0];
}

- (void)playBG:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi startPaused:(NSNumber *)paused andSeekTo:(NSNumber *)time {
	@synchronized (self) {
		[self play:url withUserInfo:userInfo withRGInfo:rgi startPaused:[paused boolValue] andSeekTo:[time doubleValue]];
	}
}

- (void)play:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi startPaused:(BOOL)paused andSeekTo:(double)time {
	[self play:url withUserInfo:userInfo withRGInfo:rgi startPaused:paused andSeekTo:time andResumeInterval:NO];
}

- (void)play:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi startPaused:(BOOL)paused andSeekTo:(double)time andResumeInterval:(BOOL)resumeInterval {
	ALog(@"Opening file for playback: %@ at seek offset %f%@", url, time, (paused) ? @", starting paused" : @"");

	[self waitUntilCallbacksExit];
	if(output) {
		[output fadeOutBackground];
	}
	if(!output) {
		output = [[OutputNode alloc] initWithController:self previous:nil];
		if(![output setupWithInterval:resumeInterval]) {
			return;
		}
	}
	[output setVolume:volume];
	@synchronized(chainQueue) {
		for(id anObject in chainQueue) {
			[anObject setShouldContinue:NO];
		}
		[chainQueue removeAllObjects];
		endOfInputReached = NO;
		if(bufferChain) {
			[bufferChain setShouldContinue:NO];

			bufferChain = nil;
		}
	}

	bufferChain = [[BufferChain alloc] initWithController:self];
	if(!resumeInterval) {
		[self notifyStreamChanged:userInfo];
	}

	while(![bufferChain open:url withOutputFormat:[output format] withUserInfo:userInfo withRGInfo:rgi resetBuffers:YES]) {
		bufferChain = nil;

		[self requestNextStream:userInfo];

		if([nextStream isEqualTo:url]) {
			return;
		}

		url = nextStream;
		if(url == nil) {
			return;
		}

		userInfo = nextStreamUserInfo;
		rgi = nextStreamRGInfo;

		[self notifyStreamChanged:userInfo];

		bufferChain = [[BufferChain alloc] initWithController:self];
	}

	if(resumeInterval || time > 0.0) {
		[output seek:time];
		[bufferChain seek:time];
	}

	[self setShouldContinue:YES];

	if(!resumeInterval) {
		outputLaunched = NO;
	}
	startedPaused = paused;
	initialBufferFilled = NO;
	previousUserInfo = userInfo;

	[bufferChain launchThreads];

	if(paused) {
		[self setPlaybackStatus:CogStatusPaused waitUntilDone:YES];
		if(time > 0.0) {
			[self updatePosition:userInfo];
		}
	} else if(resumeInterval || stoppedRecently) {
		[output faderFadeIn];
	}
}

- (void)stop {
	// Set shouldoContinue to NO on all things
	[self setShouldContinue:NO];
	[self setPlaybackStatus:CogStatusStopped waitUntilDone:YES];

	@synchronized(chainQueue) {
		for(id anObject in chainQueue) {
			[anObject setShouldContinue:NO];
		}
		[chainQueue removeAllObjects];
		endOfInputReached = NO;
		if(bufferChain) {
			bufferChain = nil;
		}
	}
	if(output) {
		[output setShouldContinue:NO];
		[output close];
	}
	output = nil;
	stoppedRecently = YES;

	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
		self->stoppedRecently = NO;
	});
}

- (void)pause {
	[output fadeOut];
	[output timeOut];

	[self setPlaybackStatus:CogStatusPaused waitUntilDone:YES];
}

- (void)resume {
	if(startedPaused) {
		startedPaused = NO;
		if(initialBufferFilled)
			[self launchOutputThread];
	}

	[output fadeIn];
	[output resume];

	[self setPlaybackStatus:CogStatusPlaying waitUntilDone:YES];
}

- (void)seekToTimeBG:(NSNumber *)time {
	[self seekToTime:[time doubleValue]];
}

- (void)seekToTime:(double)time {
	if(endOfInputReached) {
		// This is a dirty hack in case the playback has finished with the track
		// that the user thinks they're seeking into
		CogStatus status = (CogStatus)currentPlaybackStatus;
		NSURL *url;
		id userInfo;
		NSDictionary *rgi;

		@synchronized(chainQueue) {
			url = [bufferChain streamURL];
			userInfo = [bufferChain userInfo];
			rgi = [bufferChain rgInfo];
		}

		[self play:url withUserInfo:userInfo withRGInfo:rgi startPaused:(status == CogStatusPaused) andSeekTo:time andResumeInterval:YES];
	} else {
		[output fadeOutBackground];

		[output seek:time];
		[bufferChain seek:time];

		CogStatus status = (CogStatus)currentPlaybackStatus;
		BOOL paused = status == CogStatusPaused;
		id userInfo;

		@synchronized(chainQueue) {
			userInfo = [bufferChain userInfo];
		}

		if(paused) {
			[self setPlaybackStatus:CogStatusPaused waitUntilDone:YES];
		}
		[self updatePosition:userInfo];
		[output faderFadeIn];
	}
}

- (void)setVolume:(double)v {
	volume = v;

	[output setVolume:v];
}

- (double)volume {
	return volume;
}

// This is called by the delegate DURING a requestNextStream request.
- (void)setNextStream:(NSURL *)url {
	[self setNextStream:url withUserInfo:nil withRGInfo:nil];
}

- (void)setNextStream:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi {
	nextStream = url;

	nextStreamUserInfo = userInfo;

	nextStreamRGInfo = rgi;
}

// Called when the playlist changed before we actually started playing a requested stream. We will re-request.
- (void)resetNextStreams {
	[self waitUntilCallbacksExit];

	@synchronized(chainQueue) {
		for(id anObject in chainQueue) {
			[anObject setShouldContinue:NO];
		}
		[chainQueue removeAllObjects];

		if(endOfInputReached) {
			[self endOfInputReached:bufferChain];
		}
	}
}

- (void)restartPlaybackAtCurrentPosition {
	[self sendDelegateMethod:@selector(audioPlayer:restartPlaybackAtCurrentPosition:) withObject:previousUserInfo waitUntilDone:NO];
}

- (void)updatePosition:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:updatePosition:) withObject:userInfo waitUntilDone:NO];
}

- (void)pushInfo:(NSDictionary *)info toTrack:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:pushInfo:toTrack:) withObject:info withObject:userInfo waitUntilDone:NO];
}

- (void)reportPlayCountForTrack:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:reportPlayCountForTrack:) withObject:userInfo waitUntilDone:NO];
}

- (void)setShouldContinue:(BOOL)s {
	shouldContinue = s;

	if(bufferChain)
		[bufferChain setShouldContinue:s];

	if(output)
		[output setShouldContinue:s];
}

- (double)amountPlayed {
	return [output amountPlayed];
}

- (double)amountPlayedInterval {
	return [output amountPlayedInterval];
}

- (void)launchOutputThread {
	initialBufferFilled = YES;
	if(outputLaunched == NO && startedPaused == NO) {
		[self setPlaybackStatus:CogStatusPlaying];
		[output launchThread];
		outputLaunched = YES;
	}
}

- (void)requestNextStream:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:willEndStream:) withObject:userInfo waitUntilDone:YES];
}

- (void)notifyStreamChanged:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:didBeginStream:) withObject:userInfo waitUntilDone:YES];
}

- (void)notifyPlaybackStopped:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:didStopNaturally:) withObject:userInfo waitUntilDone:NO];
}

- (void)beginEqualizer:(AudioUnit)eq {
	[self sendDelegateMethod:@selector(audioPlayer:displayEqualizer:) withVoid:eq waitUntilDone:YES];
}

- (void)refreshEqualizer:(AudioUnit)eq {
	[self sendDelegateMethod:@selector(audioPlayer:refreshEqualizer:) withVoid:eq waitUntilDone:YES];
}

- (void)endEqualizer:(AudioUnit)eq {
	[self sendDelegateMethod:@selector(audioPlayer:removeEqualizer:) withVoid:eq waitUntilDone:YES];
}

- (void)addChainToQueue:(BufferChain *)newChain {
	[newChain setShouldContinue:YES];
	[newChain launchThreads];

	[chainQueue insertObject:newChain atIndex:[chainQueue count]];
}

- (BOOL)endOfInputReached:(BufferChain *)sender // Sender is a BufferChain
{
	previousUserInfo = [sender userInfo];

	BufferChain *newChain = nil;

	if(atomic_load_explicit(&resettingNow, memory_order_relaxed))
		return YES;

	atomic_fetch_add(&refCount, 1);

	@synchronized(chainQueue) {
		// No point in constructing new chain for the next playlist entry
		// if there's already one at the head of chainQueue... r-r-right?
		for(BufferChain *chain in chainQueue) {
			if([chain isRunning]) {
				if(output)
					[output setShouldPlayOutBuffer:YES];
				atomic_fetch_sub(&refCount, 1);
				return YES;
			}
		}

		// We don't want to do this, it may happen with a lot of short files
		// if ([chainQueue count] >= 5)
		//{
		//    return YES;
		//}
	}

	double duration = 0.0;

	@synchronized(chainQueue) {
		for(BufferChain *chain in chainQueue) {
			duration += [chain secondsBuffered];
		}
	}

	while(duration >= 30.0 && shouldContinue) {
		[semaphore wait];
		if(atomic_load_explicit(&resettingNow, memory_order_relaxed)) {
			if(output)
				[output setShouldPlayOutBuffer:YES];
			atomic_fetch_sub(&refCount, 1);
			return YES;
		}
		@synchronized(chainQueue) {
			duration = 0.0;
			for(BufferChain *chain in chainQueue) {
				duration += [chain secondsBuffered];
			}
		}
	}

	nextStreamUserInfo = [sender userInfo];

	nextStreamRGInfo = [sender rgInfo];

	// This call can sometimes lead to invoking a chainQueue block on another thread
	[self requestNextStream:nextStreamUserInfo];

	if(!nextStream) {
		if(output)
			[output setShouldPlayOutBuffer:YES];
		atomic_fetch_sub(&refCount, 1);
		return YES;
	}

	BufferChain *lastChain;

	@synchronized(chainQueue) {
		newChain = [[BufferChain alloc] initWithController:self];

		endOfInputReached = YES;

		lastChain = [chainQueue lastObject];
		if(lastChain == nil) {
			lastChain = bufferChain;
			if(lastChain == nil) {
				/* Perhaps this should be the default anyway, since the chain that just
				 * finished is explicitly calling us */
				lastChain = sender;
			}
		}
	}

	BOOL pathsEqual = NO;

	if(!lastChain || ![lastChain isKindOfClass:[BufferChain class]] ||
	   ![lastChain streamURL] ||
	   !nextStream || ![nextStream isKindOfClass:[NSURL class]]) {
		DLog(@"Previous chain or next stream references broken, or invalid classes");
		if(!nextStream || ![nextStream isKindOfClass:[NSURL class]]) {
			// Terminate playback
			nextStream = nil;
			atomic_fetch_sub(&refCount, 1);
			return YES;
		}
	} else if([nextStream isFileURL] && [[lastChain streamURL] isFileURL]) {
		NSString *unixPathNext = [nextStream path];
		NSString *unixPathPrev = [[lastChain streamURL] path];

		if([unixPathNext isEqualToString:unixPathPrev])
			pathsEqual = YES;
	} else if(![nextStream isFileURL] && ![[lastChain streamURL] isFileURL]) {
		@try {
			NSURL *lastURL = [lastChain streamURL];
			NSString *nextScheme = [nextStream scheme];
			NSString *lastScheme = [lastURL scheme];
			NSString *nextHost = [nextStream host];
			NSString *lastHost = [lastURL host];
			NSString *nextPath = [nextStream path];
			NSString *lastPath = [lastURL path];
			if(nextScheme && lastScheme && [nextScheme isEqualToString:lastScheme]) {
				if((!nextHost && !lastHost) ||
				   (nextHost && lastHost && [nextHost isEqualToString:lastHost])) {
					if(nextPath && lastPath && [nextPath isEqualToString:lastPath]) {
						pathsEqual = YES;
					}
				}
			}
		}
		@catch(NSException *e) {
			DLog(@"Exception thrown checking file match: %@", e);
		}
	}

	if(pathsEqual) {
		if([lastChain setTrack:nextStream] && [newChain openWithInput:[lastChain inputNode] withOutputFormat:[output format] withUserInfo:nextStreamUserInfo withRGInfo:nextStreamRGInfo resetBuffers:NO]) {
			[newChain setStreamURL:nextStream];

			@synchronized(chainQueue) {
				[self addChainToQueue:newChain];
			}
			DLog(@"TRACK SET!!! %@", newChain);
			// Keep on-playin
			newChain = nil;

			atomic_fetch_sub(&refCount, 1);
			return NO;
		}
	}

	lastChain = nil;

	NSURL *url = nextStream;

	while(shouldContinue && ![newChain open:url withOutputFormat:[output format] withUserInfo:nextStreamUserInfo withRGInfo:nextStreamRGInfo resetBuffers:NO]) {
		if(nextStream == nil) {
			newChain = nil;
			if(output)
				[output setShouldPlayOutBuffer:YES];
			atomic_fetch_sub(&refCount, 1);
			return YES;
		}

		newChain = nil;
		[self requestNextStream:nextStreamUserInfo];

		if([nextStream isEqualTo:url]) {
			newChain = nil;
			if(output)
				[output setShouldPlayOutBuffer:YES];
			atomic_fetch_sub(&refCount, 1);
			return YES;
		}

		url = nextStream;

		newChain = [[BufferChain alloc] initWithController:self];
	}

	@synchronized(chainQueue) {
		[self addChainToQueue:newChain];
	}

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

	if(output)
		[output setShouldPlayOutBuffer:YES];

	atomic_fetch_sub(&refCount, 1);
	return YES;
}

- (void)reportPlayCount {
	[self reportPlayCountForTrack:previousUserInfo];
}

- (BOOL)selectNextBuffer {
	BOOL signalStopped = NO;
	do {
		@synchronized(chainQueue) {
			endOfInputReached = NO;

			if([chainQueue count] <= 0) {
				// End of playlist
				signalStopped = YES;
				break;
			}

			[bufferChain setShouldContinue:NO];
			bufferChain = nil;
			bufferChain = [chainQueue objectAtIndex:0];

			[chainQueue removeObjectAtIndex:0];
			DLog(@"New!!! %@ %@", bufferChain, [[bufferChain inputNode] decoder]);

			[semaphore signal];
		}
	} while(0);

	if(signalStopped) {
		double latency = 0;
		if(output) latency = [output latency];
		
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, latency * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
			[self stop];

			self->bufferChain = nil;

			[self notifyPlaybackStopped:nil];
		});

		return YES;
	}

	[output setEndOfStream:NO];

	return NO;
}

- (void)endOfInputPlayed {
	// Once we get here:
	// - the buffer chain for the next playlist entry (started in endOfInputReached) have been working for some time
	//   already, so that there is some decoded and converted data to play
	// - the buffer chain for the next entry is the first item in chainQueue
	previousUserInfo = [bufferChain userInfo];
	[self notifyStreamChanged:previousUserInfo];
}

- (BOOL)chainQueueHasTracks {
	@synchronized(chainQueue) {
		return [chainQueue count] > 0;
	}
	return NO;
}

- (void)sendDelegateMethod:(SEL)selector withVoid:(void *)obj waitUntilDone:(BOOL)wait {
	NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[delegate methodSignatureForSelector:selector]];
	[invocation setTarget:delegate];
	[invocation setSelector:selector];
	[invocation setArgument:(void *)&self atIndex:2];
	[invocation setArgument:&obj atIndex:3];
	[invocation retainArguments];

	[invocation performSelectorOnMainThread:@selector(invoke) withObject:nil waitUntilDone:wait];
}

- (void)sendDelegateMethod:(SEL)selector withObject:(id)obj waitUntilDone:(BOOL)wait {
	NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[delegate methodSignatureForSelector:selector]];
	[invocation setTarget:delegate];
	[invocation setSelector:selector];
	[invocation setArgument:(void *)&self atIndex:2];
	[invocation setArgument:&obj atIndex:3];
	[invocation retainArguments];

	[invocation performSelectorOnMainThread:@selector(invoke) withObject:nil waitUntilDone:wait];
}

- (void)sendDelegateMethod:(SEL)selector withObject:(id)obj withObject:(id)obj2 waitUntilDone:(BOOL)wait {
	NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[delegate methodSignatureForSelector:selector]];
	[invocation setTarget:delegate];
	[invocation setSelector:selector];
	[invocation setArgument:(void *)&self atIndex:2];
	[invocation setArgument:&obj atIndex:3];
	[invocation setArgument:&obj2 atIndex:4];
	[invocation retainArguments];

	[invocation performSelectorOnMainThread:@selector(invoke) withObject:nil waitUntilDone:wait];
}

- (void)setPlaybackStatus:(int)status waitUntilDone:(BOOL)wait {
	currentPlaybackStatus = status;

	[self sendDelegateMethod:@selector(audioPlayer:didChangeStatus:userInfo:) withObject:@(status) withObject:[bufferChain userInfo] waitUntilDone:wait];
}

- (void)sustainHDCD {
//	[self sendDelegateMethod:@selector(audioPlayer:sustainHDCD:) withObject:[bufferChain userInfo] waitUntilDone:NO];
}

- (void)setError:(BOOL)status {
	[self sendDelegateMethod:@selector(audioPlayer:setError:toTrack:) withObject:@(status) withObject:[bufferChain userInfo] waitUntilDone:NO];
}

- (void)setPlaybackStatus:(int)status {
	[self setPlaybackStatus:status waitUntilDone:NO];
}

- (BufferChain *)bufferChain {
	return bufferChain;
}

- (OutputNode *)output {
	return output;
}

+ (NSArray *)containerTypes {
	return [[[PluginController sharedPluginController] containers] allKeys];
}

+ (NSArray *)fileTypes {
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

+ (NSArray *)schemes {
	PluginController *pluginController = [PluginController sharedPluginController];

	return [[pluginController sources] allKeys];
}

- (double)volumeUp:(double)amount {
	BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
	const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

	double newVolume = linearToLogarithmic(logarithmicToLinear(volume + amount, MAX_VOLUME), MAX_VOLUME);
	if(newVolume > MAX_VOLUME)
		newVolume = MAX_VOLUME;

	[self setVolume:newVolume];

	// the playbackController needs to know the new volume, so it can update the
	// volumeSlider accordingly.
	return newVolume;
}

- (double)volumeDown:(double)amount {
	BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
	const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

	double newVolume;
	if(amount > volume)
		newVolume = 0.0;
	else
		newVolume = linearToLogarithmic(logarithmicToLinear(volume - amount, MAX_VOLUME), MAX_VOLUME);

	[self setVolume:newVolume];
	return newVolume;
}

- (void)waitUntilCallbacksExit {
	// This sucks! And since the thread that's inside the function can be calling
	// event dispatches, we have to pump the message queue if we're on the main
	// thread. Damn.
	if(atomic_load_explicit(&refCount, memory_order_relaxed) != 0) {
		BOOL mainThread = (dispatch_queue_get_label(dispatch_get_main_queue()) == dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL));
		atomic_store(&resettingNow, true);
		while(atomic_load_explicit(&refCount, memory_order_relaxed) != 0) {
			[semaphore signal]; // Gotta poke this periodically
			if(mainThread)
				[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
			else
				usleep(500);
		}
		atomic_store(&resettingNow, false);
	}
}

@end
