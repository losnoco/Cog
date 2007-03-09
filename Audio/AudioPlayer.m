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
	bufferChain = [[BufferChain alloc] initWithController:self];

	while (![bufferChain open:url withOutputFormat:[output format]])
	{
		[bufferChain release];

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
	DBLog(@"DETACHING THREADS");
	
	[output launchThread];
	[bufferChain launchThreads];
	
	[self setPlaybackStatus:kCogStatusPlaying];
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
	[bufferChain seek:time];
	[output seek:time];
	
	
	/*END HACK*/
}

- (void)setVolume:(double)v
{
	[output setVolume:v];
}

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
	[bufferChain setShouldContinue:s];
	[output setShouldContinue:s];
}

- (double)amountPlayed
{
	return [output amountPlayed];
}




- (void)requestNextStream:(id)userInfo
{
	[self sendDelegateMethod:@selector(audioPlayer:requestNextStream:) withObject:userInfo waitUntilDone:YES];
}

- (void)notifyStreamChanged:(id)userInfo
{
	[self sendDelegateMethod:@selector(audioPlayer:streamChanged:) withObject:userInfo waitUntilDone:NO];
}



- (void)endOfInputReached:(BufferChain *)sender //Sender is a BufferChain
{
	BufferChain *newChain = nil;

	nextStreamUserInfo = [sender userInfo];
	[nextStreamUserInfo retain]; //Retained because when setNextStream is called, it will be released!!!
	
	do {
		[newChain release];
		[self requestNextStream: nextStreamUserInfo];

		if (nextStream == nil)
		{
			return;
		}
		
		newChain = [[BufferChain alloc] initWithController:self];
	} while (![newChain open:nextStream withOutputFormat:[output format]]);
	
	[newChain setUserInfo: nextStreamUserInfo];
	
	[newChain setShouldContinue:YES];
	[newChain launchThreads];
	
	[chainQueue insertObject:newChain atIndex:[chainQueue count]];

	[newChain release];
}

- (void)endOfInputPlayed
{
	if ([chainQueue count] <= 0)
	{
		//End of playlist
		DBLog(@"STOPPED");
		[self stop];
		
		return;
	}
//	NSLog(@"SWAPPING BUFFERS");
	[bufferChain release];
	
	DBLog(@"END OF INPUT PLAYED");
	bufferChain = [chainQueue objectAtIndex:0];
	[bufferChain retain];
	
	[chainQueue removeObjectAtIndex:0];

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

+ (NSArray *)fileTypes
{
	PluginController *pluginController = [PluginController sharedPluginController];
	
	NSArray *decoderTypes = [[pluginController decoders] allKeys];
	NSArray *metdataReaderTypes = [[pluginController metadataReaders] allKeys];
	NSArray *propertiesReaderTypes = [[pluginController propertiesReaders] allKeys];
	
	NSMutableSet *types = [NSMutableSet set];
	
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
