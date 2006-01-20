//
//  InputController.m
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
	
	endOfInput = NO;
}

- (void)process
{
	const int chunk_size = CHUNK_SIZE;
	char buf[chunk_size];
	int amountRead;
	
	DBLog(@"Playing file.\n");
	
	while ([self shouldContinue] == YES)
	{
		amountRead = [soundFile fillBuffer:buf ofSize: chunk_size];
		if (amountRead <= 0)
		{
			endOfInput = YES;
			NSLog(@"END OF INPUT WAS REACHED");
			[controller endOfInputReached];
			shouldContinue = NO;
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
