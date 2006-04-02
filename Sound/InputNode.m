//
//  InputNode.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "InputNode.h"


@implementation InputNode

- (void)open:(NSString *)filename
{
	soundFile = [SoundFile open:filename];
	
	[soundFile getFormat:&format];
	
	shouldContinue = YES;
}

- (void)process
{
	const int chunk_size = CHUNK_SIZE;
	char buf[chunk_size];
	int amountRead;
	
	DBLog(@"Playing file.\n");
	
	while ([self shouldContinue] == YES && [self endOfStream] == NO)
	{
		amountRead = [soundFile fillBuffer:buf ofSize: chunk_size];
		if (amountRead <= 0)
		{
			endOfStream = YES;
			NSLog(@"END OF INPUT WAS REACHED");
			[controller endOfInputReached];
			[soundFile close];
			return; //eof
		}
		[self writeData:buf amount:amountRead];
	}
	
	[soundFile close];
}

- (AudioStreamBasicDescription) format
{
	return format;
}

@end
