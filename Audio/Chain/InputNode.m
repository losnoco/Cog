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

static void *kInputNodeContext = &kInputNodeContext;

@synthesize threadExited;
@synthesize exitAtTheEndOfTheStream;

- (id)initWithController:(id)c previous:(id)p {
	self = [super initWithController:c previous:p];
	if(self) {
		exitAtTheEndOfTheStream = [[Semaphore alloc] init];
		threadExited = NO;
	}

	return self;
}

- (BOOL)openWithSource:(id<CogSource>)source {
	[self removeObservers];

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
	[self removeObservers];

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
	if(!observersAdded) {
		DLog(@"REGISTERING OBSERVERS");
		[decoder addObserver:self
		          forKeyPath:@"properties"
		             options:(NSKeyValueObservingOptionNew)
		             context:kInputNodeContext];

		[decoder addObserver:self
		          forKeyPath:@"metadata"
		             options:(NSKeyValueObservingOptionNew)
		             context:kInputNodeContext];

		observersAdded = YES;
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context {
	if(context == kInputNodeContext) {
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
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)process {
	BOOL shouldClose = YES;
	BOOL seekError = NO;

	BOOL isError = NO;

	BOOL signalReset = NO;

	if([decoder respondsToSelector:@selector(isSilence)]) {
		if([decoder isSilence]) {
			isError = YES;
		}
	}

	[controller setError:isError];

	while([self shouldContinue] == YES && [self endOfStream] == NO) {
		if(shouldSeek == YES) {
			BufferChain *bufferChain = controller;
			AudioPlayer *audioPlayer = [bufferChain controller];
			OutputNode *outputNode = [audioPlayer output];

			DLog(@"SEEKING! Resetting Buffer");
			[self resetBuffer];
			[outputNode resetDSPs];

			DLog(@"Reset buffer!");

			DLog(@"SEEKING!");
			@autoreleasepool {
				seekError = [decoder seek:seekFrame] < 0;
			}

			shouldSeek = NO;
			DLog(@"Seeked! Resetting Buffer");
			initialBufferFilled = NO;

			if(seekError) {
				[controller setError:YES];
			}

			signalReset = YES;
		}

		AudioChunk *chunk;
		@autoreleasepool {
			chunk = [decoder readAudio];
		}

		if(chunk && [chunk frameCount]) {
			@autoreleasepool {
				if(signalReset) {
					chunk.resetForward = YES;
					signalReset = NO;
				}
				[self writeChunk:chunk];
				chunk = nil;
			}
		} else {
			if(chunk) {
				@autoreleasepool {
					chunk = nil;
				}
			}
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
			DLog("InputNode: After wait, should seek = %d", shouldSeek);
			if(shouldSeek) {
				endOfStream = NO;
				shouldClose = NO;
				continue;
			} else {
				break;
			}
		}
	}

	if(shouldClose)
		[decoder close];

	[exitAtTheEndOfTheStream signal];
	threadExited = YES;

	DLog("Input node thread stopping");
}

- (void)seek:(long)frame {
	seekFrame = frame;
	shouldSeek = YES;
	DLog(@"Should seek!");
	[self resetBuffer];
	[writeSemaphore signal];

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
		[decoder removeObserver:self forKeyPath:@"properties" context:kInputNodeContext];
		[decoder removeObserver:self forKeyPath:@"metadata" context:kInputNodeContext];
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
	[super cleanUp];
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
