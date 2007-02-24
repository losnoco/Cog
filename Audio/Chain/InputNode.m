//
//  InputNode.m
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "InputNode.h"
#import "BufferChain.h"

@implementation InputNode

- (BOOL)open:(NSURL *)url
{
	NSLog(@"Opening: %@", url);
	decoder = [AudioDecoder audioDecoderForURL:url];
	[decoder retain];
	
	NSLog(@"Got decoder...%@", decoder);
	if (decoder == nil)
		return NO;

	if (![decoder open:url])
		return NO;

/*	while (decoder == nil)
	{
		NSURL *nextStream = [controller invalidDecoder];
		if (nextStream == nil)
			return NO;

		decoder = [AudioDecoder audioDecoderForURL:nextStream];
		[decoder open:nextStream];
	}
*/	
	shouldContinue = YES;
	shouldSeek = NO;
	
	return YES;
}

- (void)process
{
	const int chunk_size = CHUNK_SIZE;
	char *buf;
	int amountRead;
	
	NSLog(@"Playing file: %i", self);
	buf = malloc(chunk_size);
	
	while ([self shouldContinue] == YES && [self endOfStream] == NO)
	{
		if (shouldSeek == YES)
		{
			NSLog(@"Actually seeking");
			[decoder seekToTime:seekTime];
			shouldSeek = NO;
		}
		
		amountRead = [decoder fillBuffer:buf ofSize: chunk_size];
		if (amountRead <= 0)
		{
			endOfStream = YES;
			DBLog(@"END OF INPUT WAS REACHED");
			[controller endOfInputReached];
			break; //eof
		}
		[self writeData:buf amount:amountRead];
	}
	
	free(buf);
	[decoder close];
	
	NSLog(@"CLOSED: %i", self);
}

- (void)seek:(double)time
{
	NSLog(@"SEEKING WEEE");
	seekTime = time;
	shouldSeek = YES;
}

- (void)dealloc
{
	[decoder release];
	
	[super dealloc];
}

- (NSDictionary *) properties
{
	return [decoder properties];
}

@end
