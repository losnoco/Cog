//
//  InputNode.m
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "SourceNode.h"

@implementation SourceNode

- (id)initWithSource:(id<CogSource>)s
{
	self = [super initWithController:nil previous:self];
	if (self)
	{
		source = [s retain];
	}
	return self;
}

- (void)process
{
	const int chunk_size = CHUNK_SIZE;
	char *buf;
	int amountRead;
	
	buf = malloc(chunk_size);
	
	while ([self shouldContinue] == YES)
	{
		NSLog(@"PROCESSING!!!");
		amountRead = [source read:buf amount: chunk_size];
		if (amountRead <= 0)
		{
			endOfStream = YES;
			NSLog(@"END OF SOURCE WAS REACHED");
			break; //eof
		}
		[self writeData:buf amount:amountRead];
	}
	
	free(buf);
	[source close];
	
	NSLog(@"CLOSED: %i", self);
}

- (BOOL)open:(NSURL *)url
{
	shouldContinue = YES;
	endOfStream = NO;
	
	byteCount = 0;

	return [source open:url];
}


- (int)read:(void *)buf amount:(int)amount
{
	int l;
	do {
		l = [self readData:buf amount:amount];
		if (l > 0)
			byteCount += l;
	} while (l == 0 && endOfStream == NO);
	
	NSLog(@"READ: %i", l);
	return l;
}

//Buffered streams are never seekable.
- (BOOL)seekable
{
	return NO;
}

- (BOOL)seek:(long)offset whence:(int)whence
{
	return NO;
}

- (long)tell
{
	return byteCount;
}

- (void)close
{
	NSLog(@"CLOSING");
	shouldContinue = NO;
	
	[source close];
}

- (void)dealloc
{
	[source release];
	
	[super dealloc];
}

- (NSDictionary *) properties
{
	return [source properties];
}

//Shouldnt be used. Required for protocol
- (BOOL)buffered
{
	return YES;
}

+ (NSArray *) schemes
{
	return nil;
}
//end of shouldnt be used

@end
