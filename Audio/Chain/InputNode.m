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

- (BOOL)openURL:(NSURL *)url withSource:(id<CogSource>)source
{
	decoder = [AudioDecoder audioDecoderForURL:url];
	[decoder retain];

	if (decoder == nil)
		return NO;

	[self registerObservers];

	if (![decoder open:source])
	{
		NSLog(@"Couldn't open decoder...");
		return NO;
	}
	
	shouldContinue = YES;
	shouldSeek = NO;
	
	return YES;
}

- (BOOL)openWithDecoder:(id<CogDecoder>) d
{
	NSLog(@"Opening with old decoder: %@", d);
	decoder = d;
	[decoder retain];
	
	[self registerObservers];
	
	shouldContinue = YES;
	shouldSeek = NO;
	
	NSLog(@"DONES: %@", decoder);
	return YES;
}


- (void)registerObservers
{
	NSLog(@"REGISTERING OBSERVERS");
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
	NSLog(@"SOMETHING CHANGED!");
	if ([keyPath isEqual:@"properties"]) {
		//Setup converter!
		//Inform something of properties change
		[controller inputFormatDidChange: propertiesToASBD([decoder properties])];
	}
	else if ([keyPath isEqual:@"metadata"]) {
		//Inform something of metadata change
	}
}

- (void)process
{
	int amountRead = 0, amountInBuffer = 0;
	void *inputBuffer = malloc(CHUNK_SIZE);
	
	BOOL shouldClose = YES;
	
	while ([self shouldContinue] == YES && [self endOfStream] == NO)
	{
		if (shouldSeek == YES)
		{
			NSLog(@"SEEKING!");
			[decoder seekToTime:seekTime];
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
					[controller initialBufferFilled:self];
				}
				
				NSLog(@"End of stream? %@", [self properties]);
				endOfStream = YES;
				shouldClose = [controller endOfInputReached]; //Lets us know if we should keep going or not (occassionally, for track changes within a file)
				NSLog(@"closing? is %i", shouldClose);
				break; 
			}
		
			[self writeData:inputBuffer amount:amountInBuffer];
			amountInBuffer = 0;
		}
	}
	if (shouldClose)
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

- (BOOL)setTrack:(NSURL *)track
{
	if ([decoder respondsToSelector:@selector(setTrack:)] && [decoder setTrack:track]) {
		NSLog(@"SET TRACK!");
		
		return YES;
	}
	
	return NO;
}

- (void)dealloc
{
	NSLog(@"Input Node dealloc");

	[decoder removeObserver:self forKeyPath:@"properties"];
	[decoder removeObserver:self forKeyPath:@"metadata"];

	[decoder release];
	
	[super dealloc];
}

- (NSDictionary *) properties
{
	return [decoder properties];
}

- (id<CogDecoder>) decoder
{
	return decoder;
}

@end
