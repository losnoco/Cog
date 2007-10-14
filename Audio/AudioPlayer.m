//
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
#import "PluginController.h"


@implementation AudioPlayer

- (id)init
{
	self = [super init];
	if (self)
	{
		output = NULL;
		bufferChain = NULL;
		outputLaunched = NO;
		
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
	[self play:url withUserInfo:nil];
}


- (void)play:(NSURL *)url withUserInfo:(id)userInfo
{
	if (output)
	{
		[output release];
	}
	output = [[OutputNode alloc] initWithController:self previous:nil];
	[output setup];
	
	@synchronized(chainQueue) {
		NSEnumerator *enumerator = [chainQueue objectEnumerator];
		id anObject;
		while (anObject = [enumerator nextObject])
		{
			[anObject setShouldContinue:NO];
		}
		[chainQueue removeAllObjects];
		
		if (bufferChain)
		{
			[bufferChain setShouldContinue:NO];

			[bufferChain release];
		}
	}
	
	bufferChain = [[BufferChain alloc] initWithController:self];

	while (![bufferChain open:url withOutputFormat:[output format]])
	{
		[bufferChain release];
		bufferChain = nil;
		
		[self requestNextStream: userInfo];

		url = nextStream;
		if (url == nil)
		{
			return;
		}
	
		userInfo = nextStreamUserInfo;
	
		[self notifyStreamChanged:userInfo];
		
		bufferChain = [[BufferChain alloc] initWithController:self];
	}
	
	[bufferChain setUserInfo:userInfo];

	[self setShouldContinue:YES];
	
	outputLaunched = NO;

	[bufferChain launchThreads];
}

- (void)stop
{
	//Set shouldoContinue to NO on allll things
	[self setShouldContinue:NO];
	[self setPlaybackStatus:kCogStatusStopped];
}

- (void)pause
{
	[output pause];

	[self setPlaybackStatus:kCogStatusPaused];	
}

- (void)resume
{
	[output resume];

	[self setPlaybackStatus:kCogStatusPlaying];	
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
	[output setVolume:v];
}

//This is called by the delegate DURING a requestNextStream request.
- (void)setNextStream:(NSURL *)url
{
	[self setNextStream:url withUserInfo:nil];
}

- (void)setNextStream:(NSURL *)url withUserInfo:(id)userInfo
{
	[url retain];
	[nextStream release];
	nextStream = url;
	
	[userInfo retain];
	[nextStreamUserInfo release];
	nextStreamUserInfo = userInfo;
	
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
	if (outputLaunched == NO) {
		[self setPlaybackStatus:kCogStatusPlaying];	
		[output launchThread];
		outputLaunched = YES;
	}
}

- (void)requestNextStream:(id)userInfo
{
	[self sendDelegateMethod:@selector(audioPlayer:requestNextStream:) withObject:userInfo waitUntilDone:YES];
}

- (void)notifyStreamChanged:(id)userInfo
{
	[self sendDelegateMethod:@selector(audioPlayer:streamChanged:) withObject:userInfo waitUntilDone:NO];
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
	BufferChain *newChain = nil;

	nextStreamUserInfo = [sender userInfo];
	[nextStreamUserInfo retain]; //Retained because when setNextStream is called, it will be released!!!
	
	[self requestNextStream: nextStreamUserInfo];
	newChain = [[BufferChain alloc] initWithController:self];
	
	BufferChain *lastChain = [chainQueue lastObject];
	if (lastChain == nil) {
		lastChain = bufferChain;
	}
	
	if ([[nextStream scheme] isEqualToString:[[lastChain streamURL] scheme]]
		&& [[nextStream host] isEqualToString:[[lastChain streamURL] host]]
		&& [[nextStream path] isEqualToString:[[lastChain streamURL] path]])
	{
		if ([lastChain setTrack:nextStream]) {
			[newChain openWithInput:[lastChain inputNode] withOutputFormat:[output format]];
			
			[newChain setStreamURL:nextStream];
			[newChain setUserInfo:nextStreamUserInfo];

			[self addChainToQueue:newChain];
			NSLog(@"TRACK SET!!! %@", newChain);
			//Keep on-playin
			[newChain release];
			
			return NO;
		}
	}
	
	while (![newChain open:nextStream withOutputFormat:[output format]]) 
	{
		if (nextStream == nil)
		{
			return YES;
		}
		
		[newChain release];
		[self requestNextStream: nextStreamUserInfo];

		newChain = [[BufferChain alloc] initWithController:self];
	}
	
	[self addChainToQueue:newChain];

	[newChain release];
	
	return YES;
}

- (void)endOfInputPlayed
{
	if ([chainQueue count] <= 0)
	{
		//End of playlist
		[self stop];
		
		[bufferChain release];
		bufferChain = nil;
		
		return;
	}
	
	[bufferChain release];
	
	@synchronized(chainQueue) {
		bufferChain = [chainQueue objectAtIndex:0];
		[bufferChain retain];
		
		NSLog(@"New!!! %@ %@", bufferChain, [[bufferChain inputNode] decoder]);
		
		[chainQueue removeObjectAtIndex:0];
	}
	
	[self notifyStreamChanged:[bufferChain userInfo]];
	[output setEndOfStream:NO];
}

- (void)sendDelegateMethod:(SEL)selector withObject:(id)obj waitUntilDone:(BOOL)wait
{
	NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[delegate methodSignatureForSelector:selector]];
	[invocation setSelector:selector];
	[invocation setArgument:&self	atIndex:2]; //Indexes start at 2, the first being self, the second being command.
	[invocation setArgument:&obj	atIndex:3];

	[self performSelectorOnMainThread:@selector(sendDelegateMethodMainThread:) withObject:invocation waitUntilDone:wait];
}


- (void)sendDelegateMethodMainThread:(id)invocation
{
	[invocation invokeWithTarget:delegate];
}

- (void)setPlaybackStatus:(int)status
{	
	[self sendDelegateMethod:@selector(audioPlayer:statusChanged:) withObject:[NSNumber numberWithInt:status] waitUntilDone:NO];
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

@end
