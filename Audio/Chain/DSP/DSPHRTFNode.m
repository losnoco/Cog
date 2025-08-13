//
//  DSPHRTFNode.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/11/25.
//

#import <Foundation/Foundation.h>

#import <CoreMotion/CoreMotion.h>

#import "Logging.h"

#import "DSPHRTFNode.h"

#import "lpc.h"

#import "HeadphoneFilter.h"

#include <AvailabilityMacros.h>

#if defined(MAC_OS_VERSION_14_0) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_VERSION_14_0
#define MOTION_MANAGER 1
#endif

static void * kDSPHRTFNodeContext = &kDSPHRTFNodeContext;

static NSString *CogPlaybackDidResetHeadTracking = @"CogPlaybackDigResetHeadTracking";

#ifdef MOTION_MANAGER
static simd_float4x4 convertMatrix(CMRotationMatrix r) {
	simd_float4x4 matrix = {
		simd_make_float4(r.m33, -r.m31, r.m32, 0.0f),
		simd_make_float4(r.m13, -r.m11, r.m12, 0.0f),
		simd_make_float4(r.m23, -r.m21, r.m22, 0.0f),
		simd_make_float4(0.0f, 0.0f, 0.0f, 1.0f)
	};
	return matrix;
}

static NSLock *motionManagerLock = nil;
API_AVAILABLE(macos(14.0)) static CMHeadphoneMotionManager *motionManager = nil;
static DSPHRTFNode __weak *registeredMotionListener = nil;
#endif

static void registerMotionListener(DSPHRTFNode *listener) {
#ifdef MOTION_MANAGER
	if(@available(macOS 14, *)) {
		[motionManagerLock lock];
		if([motionManager isDeviceMotionActive]) {
			[motionManager stopDeviceMotionUpdates];
		}
		if([motionManager isDeviceMotionAvailable]) {
			registeredMotionListener = listener;
			[motionManager startDeviceMotionUpdatesToQueue:[NSOperationQueue mainQueue] withHandler:^(CMDeviceMotion * _Nullable motion, NSError * _Nullable error) {
				if(motion) {
					[motionManagerLock lock];
					if(registeredMotionListener)
						[registeredMotionListener reportMotion:convertMatrix(motion.attitude.rotationMatrix)];
					[motionManagerLock unlock];
				}
			}];
		}
		[motionManagerLock unlock];
	}
#endif
}

static void unregisterMotionListener(void) {
#ifdef MOTION_MANAGER
	if(@available(macOS 14, *)) {
		[motionManagerLock lock];
		if([motionManager isDeviceMotionActive]) {
			[motionManager stopDeviceMotionUpdates];
		}
		registeredMotionListener = nil;
		[motionManagerLock unlock];
	}
#endif
}

@implementation DSPHRTFNode {
	BOOL enableHrtf;
	BOOL enableHeadTracking;
	BOOL lastEnableHeadTracking;

	HeadphoneFilter *hrtf;

	BOOL stopping, paused;
	BOOL resetFilter;
	NSRecursiveLock *mutex;

	size_t needPrefill;

	BOOL observersapplied;

	AudioStreamBasicDescription lastInputFormat;
	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription outputFormat;

	uint32_t lastInputChannelConfig, inputChannelConfig;
	uint32_t outputChannelConfig;

	BOOL referenceMatrixSet;
	BOOL rotationMatrixUpdated;
	simd_float4x4 rotationMatrix;
	simd_float4x4 referenceMatrix;

	float prefillBuffer[4096 * 32];
	float outBuffer[4096 * 2];

	void *extrapolate_buffer;
	size_t extrapolate_buffer_size;
}

+ (void)initialize {
#ifdef MOTION_MANAGER
	motionManagerLock = [[NSLock alloc] init];

	if(@available(macOS 14, *)) {
		CMAuthorizationStatus status = [CMHeadphoneMotionManager authorizationStatus];
		if(status == CMAuthorizationStatusDenied) {
			ALog(@"Headphone motion not authorized");
			return;
		} else if(status == CMAuthorizationStatusAuthorized) {
			ALog(@"Headphone motion authorized");
		} else if(status == CMAuthorizationStatusRestricted) {
			ALog(@"Headphone motion restricted");
		} else if(status == CMAuthorizationStatusNotDetermined) {
			ALog(@"Headphone motion status not determined; will prompt for access");
		}

		motionManager = [[CMHeadphoneMotionManager alloc] init];
	}
#endif
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency {
	self = [super initWithController:c previous:p latency:latency];
	if(self) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		enableHrtf = [defaults boolForKey:@"enableHrtf"];
		enableHeadTracking = [defaults boolForKey:@"enableHeadTracking"];

		rotationMatrix = matrix_identity_float4x4;

		mutex = [[NSRecursiveLock alloc] init];

		[self addObservers];
	}
	return self;
}

- (void)dealloc {
	DLog(@"HRTF dealloc");
	[self setShouldContinue:NO];
	[self cleanUp];
	[self removeObservers];
	[super cleanUp];
	if(extrapolate_buffer) {
		free(extrapolate_buffer);
		extrapolate_buffer = NULL;
		extrapolate_buffer_size = 0;
	}
}

- (void)addObservers {
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.enableHrtf" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPHRTFNodeContext];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.enableHeadTracking" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kDSPHRTFNodeContext];

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(resetReferencePosition:) name:CogPlaybackDidResetHeadTracking object:nil];

	observersapplied = YES;
}

- (void)removeObservers {
	if(observersapplied) {
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.enableHrtf" context:kDSPHRTFNodeContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.enableHeadTracking" context:kDSPHRTFNodeContext];

		[[NSNotificationCenter defaultCenter] removeObserver:self name:CogPlaybackDidResetHeadTracking object:nil];

		observersapplied = NO;
	}
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context != kDSPHRTFNodeContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}

	if([keyPath isEqualToString:@"values.enableHrtf"] ||
	   [keyPath isEqualToString:@"values.enableHeadTracking"]) {
		NSUserDefaults *defaults = [[NSUserDefaultsController sharedUserDefaultsController] defaults];
		enableHrtf = [defaults boolForKey:@"enableHrtf"];
		enableHeadTracking = [defaults boolForKey:@"enableHeadTracking"];
		resetFilter = YES;
	}
}

- (BOOL)fullInit {
	[mutex lock];
	if(enableHrtf) {
		NSURL *presetUrl = [[NSBundle mainBundle] URLForResource:@"SADIE_D02-96000" withExtension:@"mhr"];

		rotationMatrixUpdated = NO;

		simd_float4x4 matrix;
		if(!referenceMatrixSet || !enableHeadTracking) {
			referenceMatrixSet = NO;
			matrix = matrix_identity_float4x4;
			self->referenceMatrix = matrix;
			if(enableHeadTracking) {
				lastEnableHeadTracking = YES;
				registerMotionListener(self);
			} else if(lastEnableHeadTracking) {
				lastEnableHeadTracking = NO;
				unregisterMotionListener();
			}
		} else {
			simd_float4x4 mirrorTransform = {
				simd_make_float4(-1.0, 0.0, 0.0, 0.0),
				simd_make_float4(0.0, 1.0, 0.0, 0.0),
				simd_make_float4(0.0, 0.0, 1.0, 0.0),
				simd_make_float4(0.0, 0.0, 0.0, 1.0)
			};

			matrix = simd_mul(mirrorTransform, rotationMatrix);
			matrix = simd_mul(matrix, referenceMatrix);
		}

		hrtf = [[HeadphoneFilter alloc] initWithImpulseFile:presetUrl forSampleRate:inputFormat.mSampleRate withInputChannels:inputFormat.mChannelsPerFrame withConfig:inputChannelConfig withMatrix:matrix];
		if(!hrtf) {
			[mutex unlock];
			return NO;
		}

		outputFormat = inputFormat;
		outputFormat.mChannelsPerFrame = 2;
		outputFormat.mBytesPerFrame = sizeof(float) * outputFormat.mChannelsPerFrame;
		outputFormat.mBytesPerPacket = outputFormat.mBytesPerFrame * outputFormat.mFramesPerPacket;
		outputChannelConfig = AudioChannelSideLeft | AudioChannelSideRight;

		resetFilter = NO;

		needPrefill = [hrtf needPrefill];
	} else {
		if(lastEnableHeadTracking) {
			lastEnableHeadTracking = NO;
			unregisterMotionListener();
		}
		referenceMatrixSet = NO;

		hrtf = nil;
	}

	[mutex unlock];
	return YES;
}

- (void)fullShutdown {
	[mutex lock];
	hrtf = nil;
	if(lastEnableHeadTracking) {
		lastEnableHeadTracking = NO;
		unregisterMotionListener();
	}
	resetFilter = NO;
	[mutex unlock];
}

- (BOOL)setup {
	if(stopping)
		return NO;
	[self fullShutdown];
	return [self fullInit];
}

- (void)cleanUp {
	stopping = YES;
	[self fullShutdown];
}

- (void)resetBuffer {
	paused = YES;
	[mutex lock];
	[buffer reset];
	[self fullShutdown];
	paused = NO;
	[mutex unlock];
}

- (BOOL)paused {
	return paused;
}

- (void)process {
	while([self shouldContinue] == YES) {
		if(paused || endOfStream) {
			usleep(500);
			continue;
		}
		@autoreleasepool {
			AudioChunk *chunk = nil;
			chunk = [self convert];
			if(!chunk || ![chunk frameCount]) {
				if([previousNode endOfStream] == YES) {
					usleep(500);
					endOfStream = YES;
					continue;
				}
				if(paused) {
					continue;
				}
				usleep(500);
			} else {
				[self writeChunk:chunk];
				chunk = nil;
			}
			if(resetFilter || (!enableHrtf && hrtf)) {
				[self fullShutdown];
			}
		}
	}
}

- (AudioChunk *)convert {
	if(stopping)
		return nil;

	[mutex lock];

	if(stopping || ([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) || [self shouldContinue] == NO) {
		[mutex unlock];
		return nil;
	}

	if(![self peekFormat:&inputFormat channelConfig:&inputChannelConfig]) {
		[mutex unlock];
		return nil;
	}

	if(!inputFormat.mSampleRate ||
	   !inputFormat.mBitsPerChannel ||
	   !inputFormat.mChannelsPerFrame ||
	   !inputFormat.mBytesPerFrame ||
	   !inputFormat.mFramesPerPacket ||
	   !inputFormat.mBytesPerPacket) {
		[mutex unlock];
		return nil;
	}

	if((enableHrtf && !hrtf) ||
	   memcmp(&inputFormat, &lastInputFormat, sizeof(inputFormat)) != 0 ||
	   inputChannelConfig != lastInputChannelConfig) {
		lastInputFormat = inputFormat;
		lastInputChannelConfig = inputChannelConfig;
		[self fullShutdown];
		if(enableHrtf && ![self setup]) {
			[mutex unlock];
			return nil;
		}
	}

	if(!hrtf) {
		[mutex unlock];
		return [self readChunk:4096];
	}

	AudioChunk *chunk = [self readChunkAsFloat32:4096];
	if(!chunk || ![chunk frameCount]) {
		[mutex unlock];
		return nil;
	}

	if(rotationMatrixUpdated) {
		rotationMatrixUpdated = NO;
		simd_float4x4 mirrorTransform = {
			simd_make_float4(-1.0, 0.0, 0.0, 0.0),
			simd_make_float4(0.0, 1.0, 0.0, 0.0),
			simd_make_float4(0.0, 0.0, 1.0, 0.0),
			simd_make_float4(0.0, 0.0, 0.0, 1.0)
		};

		simd_float4x4 matrix = simd_mul(mirrorTransform, rotationMatrix);
		matrix = simd_mul(matrix, referenceMatrix);

		[hrtf reloadWithMatrix:matrix];
	}

	double streamTimestamp = [chunk streamTimestamp];

	size_t frameCount = [chunk frameCount];
	NSData *sampleData = [chunk removeSamples:frameCount];

	if(needPrefill) {
		size_t maxToUse = 4096 - needPrefill;
		if(maxToUse > frameCount) {
			maxToUse = frameCount;
		}
		size_t channels = inputFormat.mChannelsPerFrame;
		memcpy(&prefillBuffer[needPrefill * channels], [sampleData bytes], maxToUse * sizeof(float) * channels);
		lpc_extrapolate_bkwd(&prefillBuffer[needPrefill * channels], maxToUse, maxToUse, (int)channels, LPC_ORDER, needPrefill, &extrapolate_buffer, &extrapolate_buffer_size);
		[hrtf process:&prefillBuffer[0] sampleCount:(int)needPrefill toBuffer:&outBuffer[0]];
		needPrefill = 0;
	}

	[hrtf process:(const float *)[sampleData bytes] sampleCount:(int)frameCount toBuffer:&outBuffer[0]];

	AudioChunk *outputChunk = [[AudioChunk alloc] init];
	[outputChunk setFormat:outputFormat];
	if(outputChannelConfig) {
		[outputChunk setChannelConfig:outputChannelConfig];
	}
	if([chunk isHDCD]) [outputChunk setHDCD];
	[outputChunk setStreamTimestamp:streamTimestamp];
	[outputChunk setStreamTimeRatio:[chunk streamTimeRatio]];
	[outputChunk assignSamples:&outBuffer[0] frameCount:frameCount];

	[mutex unlock];
	return outputChunk;
}

- (void)reportMotion:(simd_float4x4)matrix {
	rotationMatrix = matrix;
	if(!referenceMatrixSet) {
		referenceMatrix = simd_inverse(matrix);
		referenceMatrixSet = YES;
	}
	rotationMatrixUpdated = YES;
}

- (void)resetReferencePosition:(NSNotification *)notification {
	referenceMatrixSet = NO;
}

@end
