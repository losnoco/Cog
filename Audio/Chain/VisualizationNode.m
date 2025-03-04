//
//  VisualizationNode.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/12/25.
//

#import <Foundation/Foundation.h>

#import <AudioToolbox/AudioToolbox.h>

#import <Accelerate/Accelerate.h>

#import "Downmix.h"

#import <CogAudio/VisualizationController.h>

#import "BufferChain.h"

#import "Logging.h"

#import "rsstate.h"

#import "VisualizationNode.h"

@interface VisualizationCollection : NSObject {
	NSMutableArray *collection;
}

+ (VisualizationCollection *)sharedCollection;

- (id)init;

- (BOOL)pushVisualizer:(VisualizationNode *)visualization;
- (void)popVisualizer:(VisualizationNode *)visualization;

@end

@implementation VisualizationCollection

static VisualizationCollection *theCollection = nil;

+ (VisualizationCollection *)sharedCollection {
	@synchronized (theCollection) {
		if(!theCollection) {
			theCollection = [[VisualizationCollection alloc] init];
		}
		return theCollection;
	}
}

- (id)init {
	self = [super init];
	if(self) {
		collection = [[NSMutableArray alloc] init];
	}
	return self;
}

- (BOOL)pushVisualizer:(VisualizationNode *)visualization {
	@synchronized (collection) {
		[collection addObject:visualization];
		return [collection count] > 1;
	}
}

- (void)popVisualizer:(VisualizationNode *)visualization {
	@synchronized (collection) {
		[collection removeObject:visualization];
		if([collection count]) {
			VisualizationNode *next = [collection objectAtIndex:0];
			[next replayPreroll];
		}
	}
}

@end

@implementation VisualizationNode {
	void *rs;
	double lastVisRate;

	BOOL processEntered;
	BOOL stopping;
	BOOL paused;
	BOOL replay;

	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription visFormat; // Mono format for vis

	uint32_t inputChannelConfig;
	uint32_t visChannelConfig;

	size_t resamplerRemain;

	DownmixProcessor *downmixer;

	VisualizationController *visController;

	float visAudio[512];
	float resamplerInput[8192];
	float visTemp[8192];

	BOOL registered;
	BOOL prerolling;
	NSMutableData *prerollBuffer;
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency {
	self = [super init];
	if(self) {
		buffer = [[ChunkList alloc] initWithMaximumDuration:latency];

		writeSemaphore = [[Semaphore alloc] init];
		readSemaphore = [[Semaphore alloc] init];

		accessLock = [[NSLock alloc] init];

		initialBufferFilled = NO;

		controller = c;
		endOfStream = NO;
		shouldContinue = YES;

		nodeChannelConfig = 0;
		nodeLossless = NO;

		durationPrebuffer = latency * 0.25;

		visController = [VisualizationController sharedController];

		registered = NO;
		prerolling = NO;
		replay = NO;
		prerollBuffer = [[NSMutableData alloc] init];

		inWrite = NO;
		inPeek = NO;
		inRead = NO;
		inMerge = NO;

		[self setPreviousNode:p];
	}

	return self;
}

- (void)dealloc {
	DLog(@"Visualization node dealloc");
	[self cleanUp];
	[self pop];
	[super cleanUp];
}

- (void)pop {
	if(registered) {
		[[VisualizationCollection sharedCollection] popVisualizer:self];
		registered = NO;
	}
}

// Visualization thread should be fairly high priority, too
- (void)threadEntry:(id _Nullable)arg {
	@autoreleasepool {
		NSThread *currentThread = [NSThread currentThread];
		[currentThread setThreadPriority:0.75];
		[currentThread setQualityOfService:NSQualityOfServiceUserInitiated];
		[self process];
	}
}

- (void)resetBuffer {
	paused = YES;
	while(processEntered) {
		usleep(500);
	}
	[buffer reset];
	[self fullShutdown];
	paused = NO;
}

- (double)secondsBuffered {
	return [buffer listDuration];
}

- (BOOL)setup {
	if(fabs(inputFormat.mSampleRate - 44100.0) > 1e-6) {
		rs = rsstate_new(1, inputFormat.mSampleRate, 44100.0);
		if(!rs) {
			return NO;
		}
		resamplerRemain = 0;
	}

	visFormat = inputFormat;
	visFormat.mChannelsPerFrame = 1;
	visFormat.mBytesPerFrame = sizeof(float);
	visFormat.mBytesPerPacket = visFormat.mBytesPerFrame * visFormat.mFramesPerPacket;
	visChannelConfig = AudioChannelFrontCenter;

	downmixer = [[DownmixProcessor alloc] initWithInputFormat:inputFormat inputConfig:inputChannelConfig andOutputFormat:visFormat outputConfig:visChannelConfig];
	if(!downmixer) {
		return NO;
	}

	return YES;
}

- (void)cleanUp {
	stopping = YES;
	while(processEntered) {
		usleep(500);
	}
	[self fullShutdown];
}

- (void)fullShutdown {
	if(rs) {
		rsstate_delete(rs);
		rs = NULL;
	}
	downmixer = nil;
}

- (BOOL)paused {
	return paused;
}

- (void)process {
	while([self shouldContinue] == YES) {
		if(paused) {
			usleep(500);
			continue;
		}
		@autoreleasepool {
			if(replay) {
				size_t length = [prerollBuffer length];
				if(length) {
					[visController postVisPCM:(const float *)[prerollBuffer bytes] amount:(int)(length / sizeof(float))];
					[prerollBuffer replaceBytesInRange:NSMakeRange(0, length) withBytes:NULL length:0];
				}
				replay = NO;
				prerolling = NO;
			}
			AudioChunk *chunk = nil;
			chunk = [self readAndMergeChunksAsFloat32:512];
			if(!chunk || ![chunk frameCount]) {
				if([previousNode endOfStream] == YES) {
					break;
				}
			} else {
				[self processVis:[chunk copy]];
				[self writeChunk:chunk];
				chunk = nil;
			}
		}
	}
	endOfStream = YES;
}

- (void)postVisPCM:(const float *)visTemp amount:(size_t)samples {
	if(!registered) {
		prerolling = [[VisualizationCollection sharedCollection] pushVisualizer:self];
		registered = YES;
	}
	if(prerolling) {
		[prerollBuffer appendBytes:visTemp length:(samples * sizeof(float))];
	} else {
		[visController postVisPCM:visTemp amount:samples];
	}
}

- (void)replayPreroll {
	paused = YES;
	while(processEntered) {
		usleep(500);
	}
	replay = YES;
	paused = NO;
}

- (void)processVis:(AudioChunk *)chunk {
	processEntered = YES;

	if(paused) {
		processEntered = NO;
		return;
	}

	AudioStreamBasicDescription format = [chunk format];
	uint32_t channelConfig = [chunk channelConfig];

	[visController postSampleRate:44100.0];

	if(!rs || !downmixer ||
	   memcmp(&format, &inputFormat, sizeof(format)) != 0 ||
	   channelConfig != inputChannelConfig) {
		if(rs) {
			while(!stopping) {
				int samplesFlushed;
				samplesFlushed = (int)rsstate_flush(rs, &visTemp[0], 8192);
				if(samplesFlushed > 1) {
					[self postVisPCM:visTemp amount:samplesFlushed];
				} else {
					break;
				}
			}
		}
		[self fullShutdown];
		inputFormat = format;
		inputChannelConfig = channelConfig;
		if(![self setup]) {
			processEntered = NO;
			return;
		}
	}

	size_t frameCount = [chunk frameCount];
	NSData *sampleData = [chunk removeSamples:frameCount];

	[downmixer process:[sampleData bytes] frameCount:frameCount output:&visAudio[0]];

	if(rs) {
		int samplesProcessed;
		size_t totalDone = 0;
		size_t inDone = 0;
		size_t visFrameCount = frameCount;
		do {
			if(stopping) {
				break;
			}
			int visTodo = (int)MIN(visFrameCount, resamplerRemain + visFrameCount - 8192);
			if(visTodo) {
				cblas_scopy(visTodo, &visAudio[0], 1, &resamplerInput[resamplerRemain], 1);
			}
			visTodo += resamplerRemain;
			resamplerRemain = 0;
			samplesProcessed = (int)rsstate_resample(rs, &resamplerInput[0], visTodo, &inDone, &visTemp[0], 8192);
			resamplerRemain = (int)(visTodo - inDone);
			if(resamplerRemain && inDone) {
				memmove(&resamplerInput[0], &resamplerInput[inDone], resamplerRemain * sizeof(float));
			}
			if(samplesProcessed) {
				[self postVisPCM:&visTemp[0] amount:samplesProcessed];
			}
			totalDone += inDone;
			visFrameCount -= inDone;
		} while(samplesProcessed && visFrameCount);
	} else {
		[self postVisPCM:&visAudio[0] amount:frameCount];
	}

	processEntered = NO;
}

@end
