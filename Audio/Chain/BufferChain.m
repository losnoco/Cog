//
//  BufferChain.m
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "BufferChain.h"
#import "OutputNode.h"
#import "AudioSource.h"
#import "CoreAudioUtils.h"

@implementation BufferChain

- (id)initWithController:(id)c
{
	self = [super init];
	if (self)
	{
		controller = c;
		streamURL = nil;
		userInfo = nil;

		inputNode = nil;
		converterNode = nil;
	}
	
	return self;
}

- (void)buildChain
{
	[inputNode release];
	[converterNode release];
	
	inputNode = [[InputNode alloc] initWithController:self previous:nil];
	converterNode = [[ConverterNode alloc] initWithController:self previous:inputNode];

	finalNode = converterNode;
}

- (BOOL)open:(NSURL *)url withOutputFormat:(AudioStreamBasicDescription)outputFormat
{	
	[self setStreamURL:url];

	[self buildChain];
	
	id<CogSource> source = [AudioSource audioSourceForURL:url];
	
	if (![source open:url])
	{
		NSLog(@"Couldn't open source...");
		return NO;
	}
	

	if (![inputNode openURL:url withSource:source])
		return NO;

	AudioStreamBasicDescription inputFormat;
	inputFormat = propertiesToASBD([inputNode properties]);
	
	[converterNode setupWithInputFormat:inputFormat outputFormat:outputFormat ];
	
	return YES;
}

- (void)launchThreads
{
	DBLog(@"LAUNCHING THREAD FOR INPUT");
	[inputNode launchThread];
	DBLog(@"LAUNCHING THREAD FOR CONVERTER");
	[converterNode launchThread];
}

- (void)setUserInfo:(id)i
{
	[i retain];
	[userInfo release];
	userInfo = i;
}

- (id)userInfo
{
	return userInfo;
}

- (void)dealloc
{
	[userInfo release];
	
	[inputNode release];
	
	[converterNode release];
	
	[super dealloc];
}

- (void)seek:(double)time
{
	NSLog(@"SEEKING IN BUFFERCHIAN");
	[inputNode seek:time];

	[[converterNode readLock] lock];
	[[converterNode writeLock] lock];
	
	[[inputNode readLock] lock];
	[[inputNode writeLock] lock];

	//Signal so its waiting when we unlock
	[[converterNode semaphore] signal];
	[[inputNode semaphore] signal];
	
	[converterNode resetBuffer];
	[inputNode resetBuffer];
	
	[[inputNode writeLock] unlock];
	[[inputNode readLock] unlock];
	
	[[converterNode writeLock] unlock];
	[[converterNode readLock] unlock];
}

- (void)endOfInputReached
{
	[controller endOfInputReached:self];
}


- (id)finalNode
{
	return finalNode;
}

- (NSURL *)streamURL
{
	return streamURL;
}

- (void)setStreamURL:(NSURL *)url
{
	[url retain];
	[streamURL release];

	streamURL = url;
}

- (void)setShouldContinue:(BOOL)s
{
	[inputNode setShouldContinue:s];
	[converterNode setShouldContinue:s];
}

@end
