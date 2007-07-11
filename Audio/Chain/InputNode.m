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

	converter = [[Converter alloc] init];
	if (converter == nil)
		return NO;
	
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
		[converter cleanUp];
		[converter setupWithInputFormat:propertiesToASBD([decoder properties]) outputFormat:outputFormat];
		//Inform something of properties change
	}
	else if ([keyPath isEqual:@"metadata"]) {
		//Inform something of metadata change
	}
}

- (void)process
{
	int amountRead = 0, amountConverted = 0, amountInBuffer = 0;
	void *inputBuffer = malloc(CHUNK_SIZE);
	
	while ([self shouldContinue] == YES && [self endOfStream] == NO)
	{
		if (shouldSeek == YES)
		{
			[decoder seekToTime:seekTime];
			shouldSeek = NO;

			[self resetBuffer];
			initialBufferFilled = NO;
		}

		if (amountInBuffer < CHUNK_SIZE) {
			amountRead = [decoder fillBuffer:((char *)inputBuffer) + amountInBuffer ofSize:CHUNK_SIZE - amountInBuffer];
			amountInBuffer += amountRead;
		}
		
		amountConverted = [converter convert:inputBuffer amount:amountInBuffer]; //Convert fills in converter buffer, til the next call
		if (amountInBuffer - amountConverted > 0) {
			memmove(inputBuffer,((char *)inputBuffer) + amountConverted, amountInBuffer - amountConverted);
		}
		amountInBuffer -= amountConverted;
		
		if ([converter outputBufferSize] <= 0)
		{
			if (initialBufferFilled == NO) {
				[controller initialBufferFilled];
			}
			
			endOfStream = YES;
			[controller endOfInputReached];
			break; //eof
		}
	
		[self writeData:[converter outputBuffer] amount:[converter outputBufferSize]];
	}
	
	[decoder close];
	[converter cleanUp];
	
	free(inputBuffer);
}

- (void)seek:(double)time
{
	seekTime = time;
	shouldSeek = YES;
	[self resetBuffer];
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
