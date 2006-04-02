//
//  BufferChain.m
//  CogNew
//
//  Created by Zaphod Beeblebrox on 1/4/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
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
	}
	
	return self;
}

- (void)buildChain
{
	[inputNode release];
	[converterNode release];
	
	inputNode = [[InputNode alloc] initWithController:soundController previous:nil];
	converterNode = [[ConverterNode alloc] initWithController:soundController previous:inputNode];

	finalNode = converterNode;
}

- (BOOL)open:(NSString *)filename
{
	[self buildChain];

	[inputNode open:filename];
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
	[inputNode release];
	[converterNode release];
	
	[super dealloc];
}


- (id)finalNode
{
	return finalNode;
}

- (void)setShouldContinue:(BOOL)s
{
	[inputNode setShouldContinue:s];
	[converterNode setShouldContinue:s];
}

@end
