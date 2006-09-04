//
//  BufferChain.m
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "BufferChain.h"
#import "OutputNode.h"

@implementation BufferChain

- (id)initWithController:(id)c
{
	self = [super init];
	if (self)
	{
		soundController = c;
		playlistEntry = nil;
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

- (BOOL)open:(PlaylistEntry *)pe
{
	[pe retain];
	[playlistEntry release];
	NSLog(@"THEY ARE THE SAME?!");
	playlistEntry = pe;
	
	[self buildChain];
	NSLog(@"Filename in bufferchain: %@, %i %i", [pe filename], playlistEntry, pe);
	if (![inputNode open:[playlistEntry filename]])
		return NO;
	
	[converterNode setupWithInputFormat:(AudioStreamBasicDescription)[inputNode format] outputFormat:[[soundController output] format] ];
	
	return YES;
}

- (void)launchThreads
{
	DBLog(@"LAUNCHING THREAD FOR INPUT");
	[inputNode launchThread];
	DBLog(@"LAUNCHING THREAD FOR CONVERTER");
	[converterNode launchThread];
}

- (void)dealloc
{
	NSLog(@"Releasing playlistEntry: %i", [playlistEntry retainCount]);
	[playlistEntry release];
	
	[inputNode release];
	
	[converterNode release];
	
	[super dealloc];
}

- (void)seek:(double)time
{
	NSLog(@"SEEKING IN BUFFERCHIAN");
	[inputNode seek:time];

	[converterNode resetBuffer];
	[inputNode resetBuffer];
}

- (void)endOfInputReached
{
	[soundController endOfInputReached:self];
}


- (id)finalNode
{
	return finalNode;
}

- (PlaylistEntry *)playlistEntry
{
	return playlistEntry;
}

- (void)setShouldContinue:(BOOL)s
{
	[inputNode setShouldContinue:s];
	[converterNode setShouldContinue:s];
}

@end
