//
//  OutputNode.m
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "OutputNode.h"
#import "AudioPlayer.h"
#import "BufferChain.h"
#import "OutputCoreAudio.h"

#import "Logging.h"

@implementation OutputNode

- (void)setup {
	amountPlayed = 0.0;
	sampleRatio = 0.0;

	paused = YES;
	started = NO;

	output = [[OutputCoreAudio alloc] initWithController:self];

	[output setup];
}

- (void)seek:(double)time {
	//	[output pause];
	[self resetBuffer];

	amountPlayed = time;
}

- (void)process {
	paused = NO;
	[output start];
}

- (void)pause {
	paused = YES;
	[output pause];
}

- (void)resume {
	paused = NO;
	[output resume];
}

- (void)incrementAmountPlayed:(double)seconds {
	amountPlayed += seconds;
}

- (void)resetAmountPlayed {
	amountPlayed = 0;
}

- (void)endOfInputPlayed {
	[controller endOfInputPlayed];
}

- (BOOL)chainQueueHasTracks {
	return [controller chainQueueHasTracks];
}

- (double)secondsBuffered {
	return [buffer listDuration];
}

- (AudioChunk *)readChunk:(size_t)amount {
	@autoreleasepool {
		[self setPreviousNode:[[controller bufferChain] finalNode]];

		AudioChunk *ret = [super readChunk:amount];

		/*	if (n == 0) {
		        DLog(@"Output Buffer dry!");
		    }
		*/
		return ret;
	}
}

- (double)amountPlayed {
	return amountPlayed;
}

- (AudioStreamBasicDescription)format {
	return format;
}

- (void)setFormat:(AudioStreamBasicDescription *)f {
	format = *f;
	// Calculate a ratio and add to double(seconds) instead, as format may change
	// double oldSampleRatio = sampleRatio;
	sampleRatio = 1.0 / (format.mSampleRate * format.mBytesPerPacket);
	BufferChain *bufferChain = [controller bufferChain];
	if(bufferChain) {
		ConverterNode *converter = [bufferChain converter];
		if(converter) {
			// This clears the resampler buffer, but not the input buffer
			// We also have to jump the play position ahead accounting for
			// the data we are flushing
#if 0
            // We no longer need to do this, because outputchanged converter
            // now uses the RefillNode to slap the previous samples into
            // itself
            if (oldSampleRatio)
                amountPlayed += oldSampleRatio * [[converter buffer] bufferedLength];
#endif
			[converter setOutputFormat:format];
			[converter inputFormatDidChange:[bufferChain inputFormat]];
		}
	}
}

- (void)close {
	[output stop];
	output = nil;
}

- (void)setVolume:(double)v {
	[output setVolume:v];
}

- (void)setShouldContinue:(BOOL)s {
	[super setShouldContinue:s];

	//	if (s == NO)
	//		[output stop];
}

- (BOOL)isPaused {
	return paused;
}

- (void)beginEqualizer:(AudioUnit)eq {
	[controller beginEqualizer:eq];
}

- (void)refreshEqualizer:(AudioUnit)eq {
	[controller refreshEqualizer:eq];
}

- (void)endEqualizer:(AudioUnit)eq {
	[controller endEqualizer:eq];
}

- (void)sustainHDCD {
	[output sustainHDCD];
}

@end
