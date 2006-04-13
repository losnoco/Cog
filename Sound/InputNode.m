//
//  InputNode.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import "InputNode.h"


@implementation InputNode

- (BOOL)open:(NSString *)filename
{
	NSLog(@"Opening: %@", filename);
	soundFile = [SoundFile open:filename];
	if (soundFile == nil)
		return NO;
/*	while (soundFile == nil)
	{
		NSString *nextSong = [controller invalidSoundFile];
		if (nextSong == nil)
			return NO;

		soundFile = [SoundFile open:nextSong];
	}
*/	
	[soundFile getFormat:&format];
	
	shouldContinue = YES;
	shouldSeek = NO;
	
	return YES;
}

- (void)process
{
	const int chunk_size = CHUNK_SIZE;
	char *buf;
	int amountRead;
	
	DBLog(@"Playing file.\n");
	buf = malloc(chunk_size);
	
	while ([self shouldContinue] == YES && [self endOfStream] == NO)
	{
		if (shouldSeek == YES)
		{
			NSLog(@"Actually seeking");
			[soundFile seekToTime:seekTime];
			shouldSeek = NO;
		}
		
		amountRead = [soundFile fillBuffer:buf ofSize: chunk_size];
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
	[soundFile close];
}

- (void)seek:(double)time
{
	NSLog(@"SEEKING WEEE");
	seekTime = time;
	shouldSeek = YES;
}

- (AudioStreamBasicDescription) format
{
	return format;
}

@end
