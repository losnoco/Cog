//
//  InputNode.m
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "InputNode.h"
#import "BufferChain.h"
#import "Plugin.h"
#import "CoreAudioUtils.h"

@implementation InputNode

- (BOOL)openURL:(NSURL *)url withSource:(id<CogSource>)source outputFormat:(AudioStreamBasicDescription)of
{
	outputFormat = of;
	
	decoder = [AudioDecoder audioDecoderForURL:url];
	[decoder retain];

	[self registerObservers];

	if (decoder == nil)
		return NO;

	if (![decoder open:source])
	{
		NSLog(@"Couldn't open decoder...");
		return NO;
	}
	
	shouldContinue = YES;
	shouldSeek = NO;
	
	return YES;
}

- (void)registerObservers
{
	[decoder addObserver:self
			  forKeyPath:@"properties" 
				 options:(NSKeyValueObservingOptionNew)
				 context:NULL];

	[decoder addObserver:self
			  forKeyPath:@"metadata" 
				 options:(NSKeyValueObservingOptionNew)
				 context:NULL];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
					  ofObject:(id)object 
                        change:(NSDictionary *)change
                       context:(void *)context
{
	if ([keyPath isEqual:@"properties"]) {
		//Setup converter!
		//Inform something of properties change
	}
	else if ([keyPath isEqual:@"metadata"]) {
		//Inform something of metadata change
	}
}

- (void)process
{
	int amountRead = 0, amountInBuffer = 0;
	void *inputBuffer = malloc(CHUNK_SIZE);
	
	while ([self shouldContinue] == YES && [self endOfStream] == NO)
	{
		if (shouldSeek == YES)
		{
			NSLog(@"SEEKING!");
			[decoder seekToTime:seekTime];
			NSLog(@"Har");
			shouldSeek = NO;
			NSLog(@"Seeked! Resetting Buffer");
			
			[self resetBuffer];
			
			NSLog(@"Reset buffer!");
			initialBufferFilled = NO;
		}

		if (amountInBuffer < CHUNK_SIZE) {
			amountRead = [decoder fillBuffer:((char *)inputBuffer) + amountInBuffer ofSize:CHUNK_SIZE - amountInBuffer];
			amountInBuffer += amountRead;
		
			if (amountRead <= 0)
			{
				if (initialBufferFilled == NO) {
					[controller initialBufferFilled];
				}
				
				endOfStream = YES;
				[controller endOfInputReached];
				break; //eof
			}
		
			[self writeData:inputBuffer amount:amountInBuffer];
			amountInBuffer = 0;
		}
	}
	
	[decoder close];
	
	free(inputBuffer);
}

- (void)seek:(double)time
{
	seekTime = time;
	shouldSeek = YES;
	NSLog(@"Should seek!");
	[semaphore signal];
}

- (void)dealloc
{
	[decoder removeObserver:self forKeyPath:@"properties"];
	[decoder removeObserver:self forKeyPath:@"metadata"];

	[decoder release];
	
	[super dealloc];
}

- (NSDictionary *) properties
{
	return [decoder properties];
}

@end
