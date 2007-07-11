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
	}
	
	return self;
}

- (void)buildChain
{
	[inputNode release];
	
	inputNode = [[InputNode alloc] initWithController:self previous:nil];

	finalNode = inputNode;
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
	

	if (![inputNode openURL:url withSource:source outputFormat:outputFormat])
		return NO;
	
	return YES;
}

- (void)launchThreads
{
	[inputNode launchThread];
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
	
	[super dealloc];
}

- (void)seek:(double)time
{
	[inputNode seek:time];
}

- (void)endOfInputReached
{
	[controller endOfInputReached:self];
}

- (void)initialBufferFilled
{
	[controller launchOutputThread];
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
}

@end
