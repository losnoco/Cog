//
//  InputNode.m
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "InputNode.h"
#import "AudioPlayer.h"
#import "BufferChain.h"
#import "CoreAudioUtils.h"
#import "OutputNode.h"
#import "Plugin.h"

#import "NSDictionary+Merge.h"

#import "Logging.h"

@implementation InputNode
@synthesize exitAtTheEndOfTheStream;

- (id)initWithController:(id)c previous:(id)p {
	self = [super initWithController:c previous:p];
	if(self) {
		exitAtTheEndOfTheStream = [[Semaphore alloc] init];
	}

	return self;
}

- (BOOL)openWithSource:(id<CogSource>)source {
	decoder = [AudioDecoder audioDecoderForSource:source];

	if(decoder == nil)
		return NO;

	[self registerObservers];

	if(![decoder open:source]) {
		ALog(@"Couldn't open decoder...");
		return NO;
	}

	NSDictionary *properties = [decoder properties];
	int bitsPerSample = [[properties objectForKey:@"bitsPerSample"] intValue];
	int channels = [[properties objectForKey:@"channels"] intValue];

	bytesPerFrame = ((bitsPerSample + 7) / 8) * channels;

	nodeFormat = propertiesToASBD(properties);
	if([properties valueForKey:@"channelConfig"])
		nodeChannelConfig = [[properties valueForKey:@"channelConfig"] unsignedIntValue];
	nodeLossless = [[properties valueForKey:@"encoding"] isEqualToString:@"lossless"];

	shouldContinue = YES;
	shouldSeek = NO;

	return YES;
}

- (BOOL)openWithDecoder:(id<CogDecoder>)d {
	DLog(@"Opening with old decoder: %@", d);
	decoder = d;

	NSDictionary *properties = [decoder properties];
	int bitsPerSample = [[properties objectForKey:@"bitsPerSample"] intValue];
	int channels = [[properties objectForKey:@"channels"] intValue];

	bytesPerFrame = ((bitsPerSample + 7) / 8) * channels;

	nodeFormat = propertiesToASBD(properties);
	if([properties valueForKey:@"channelConfig"])
		nodeChannelConfig = [[properties valueForKey:@"channelConfig"] unsignedIntValue];
	nodeLossless = [[properties valueForKey:@"encoding"] isEqualToString:@"lossless"];

	[self registerObservers];

	shouldContinue = YES;
	shouldSeek = NO;

	DLog(@"DONES: %@", decoder);
	return YES;
}

- (void)registerObservers {
	DLog(@"REGISTERING OBSERVERS");
	[decoder addObserver:self
	          forKeyPath:@"properties"
	             options:(NSKeyValueObservingOptionNew)
	             context:NULL];

	[decoder addObserver:self
	          forKeyPath:@"metadata"
	             options:(NSKeyValueObservingOptionNew)
	             context:NULL];

	observersAdded = YES;
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
	DLog(@"SOMETHING CHANGED!");
	if([keyPath isEqual:@"properties"]) {
		DLog(@"Input format changed");
		// Converter may need resetting, it'll do that when it reaches the new chunks
		NSDictionary *properties = [decoder properties];

		int bitsPerSample = [[properties objectForKey:@"bitsPerSample"] intValue];
		int channels = [[properties objectForKey:@"channels"] intValue];

		bytesPerFrame = ((bitsPerSample + 7) / 8) * channels;

		nodeFormat = propertiesToASBD(properties);
		nodeChannelConfig = [[properties valueForKey:@"channelConfig"] unsignedIntValue];
		nodeLossless = [[properties valueForKey:@"encoding"] isEqualToString:@"lossless"];
	} else if([keyPath isEqual:@"metadata"]) {
		// Inform something of metadata change
		NSDictionary *entryProperties = [decoder properties];
		if(entryProperties == nil)
			return;

		NSDictionary *entryInfo = [NSDictionary dictionaryByMerging:entryProperties with:[decoder metadata]];

		[controller pushInfo:entryInfo];
	}
}

- (void)process {
	int amountInBuffer = 0;
	int bytesInBuffer = 0;
	void *inputBuffer = malloc(CHUNK_SIZE * 8 * 18); // Maximum 18 channels, dunno what we'll receive

	BOOL shouldClose = YES;
	BOOL seekError = NO;

	BOOL isError = NO;

	if([decoder respondsToSelector:@selector(isSilence)]) {
		if([decoder isSilence]) {
			isError = YES;
		}
	}

	[controller setError:isError];

	while([self shouldContinue] == YES && [self endOfStream] == NO) {
		if(shouldSeek == YES) {
			[self leaveWorkgroup];
			BufferChain *bufferChain = [[controller controller] bufferChain];
			ConverterNode *converter = [bufferChain converter];
			DLog(@"SEEKING! Resetting Buffer");

			amountInBuffer = 0;
			// This resets the converter's buffer
			[self resetBuffer];
			[converter resetBuffer];
			[converter inputFormatDidChange:[bufferChain inputFormat] inputConfig:[bufferChain inputConfig]];

			DLog(@"Reset buffer!");

			DLog(@"SEEKING!");
			seekError = [decoder seek:seekFrame] < 0;

			shouldSeek = NO;
			DLog(@"Seeked! Resetting Buffer");
			initialBufferFilled = NO;

			if(seekError) {
				[controller setError:YES];
			}
		}

		if(amountInBuffer < CHUNK_SIZE) {
			[self followWorkgroup];

			int framesToRead = CHUNK_SIZE - amountInBuffer;
			int framesRead;
			[self startWorkslice];
			@autoreleasepool {
				framesRead = [decoder readAudio:((char *)inputBuffer) + bytesInBuffer frames:framesToRead];
			}
			[self endWorkslice];

			if(framesRead > 0 && !seekError) {
				amountInBuffer += framesRead;
				bytesInBuffer += framesRead * bytesPerFrame;
				[self writeData:inputBuffer amount:bytesInBuffer];
				amountInBuffer = 0;
				bytesInBuffer = 0;
			} else {
				DLog(@"End of stream? %@", [self properties]);

				endOfStream = YES;
				shouldClose = [controller endOfInputReached]; // Lets us know if we should keep going or not (occassionally, for track changes within a file)
				DLog(@"closing? is %i", shouldClose);

				// Move this here, so that the above endOfInputReached has a chance to queue another track before starting output
				// Technically, the output should still play out its buffer first before checking if it should stop
				if(initialBufferFilled == NO) {
					[controller initialBufferFilled:self];
				}

				// wait before exiting, as we might still get seeking request
				DLog("InputNode: Before wait")
				[exitAtTheEndOfTheStream waitIndefinitely];
				DLog("InputNode: After wait, should seek = %d", shouldSeek) if(shouldSeek) {
					endOfStream = NO;
					shouldClose = NO;
					continue;
				}
				else {
					break;
				}
			}
		}
	}

	if(shouldClose)
		[decoder close];

	free(inputBuffer);

	[exitAtTheEndOfTheStream signal];

	DLog("Input node thread stopping");
}

- (void)seek:(long)frame {
	seekFrame = frame;
	shouldSeek = YES;
	DLog(@"Should seek!");
	[semaphore signal];

	if(endOfStream) {
		[exitAtTheEndOfTheStream signal];
	}
}

- (BOOL)setTrack:(NSURL *)track {
	if([decoder respondsToSelector:@selector(setTrack:)] && [decoder setTrack:track]) {
		DLog(@"SET TRACK!");

		return YES;
	}

	return NO;
}

- (void)removeObservers {
	if(observersAdded) {
		[decoder removeObserver:self forKeyPath:@"properties"];
		[decoder removeObserver:self forKeyPath:@"metadata"];
		observersAdded = NO;
	}
}

- (void)setShouldContinue:(BOOL)s {
	[super setShouldContinue:s];
	if(!s)
		[self removeObservers];
}

- (void)dealloc {
	DLog(@"Input Node dealloc");
	[self removeObservers];
}

- (NSDictionary *)properties {
	return [decoder properties];
}

- (id<CogDecoder>)decoder {
	return decoder;
}

- (double)secondsBuffered {
	return [buffer listDuration];
}

@end
