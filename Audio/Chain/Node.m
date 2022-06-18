//
//  Node.m
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "Node.h"

#import "BufferChain.h"
#import "Logging.h"

#import "OutputCoreAudio.h"

#import <pthread.h>

#import <mach/mach_time.h>

// This workgroup attribute needs to be initialized.
static os_workgroup_attr_s attr = OS_WORKGROUP_ATTR_INITIALIZER_DEFAULT;

// One nanosecond in seconds.
static const double kOneNanosecond = 1.0e9;

// The I/O interval time in seconds.
static const double kIOIntervalTime = 0.020;

// The clock identifier that specifies interval timestamps.
static const os_clockid_t clockId = OS_CLOCK_MACH_ABSOLUTE_TIME;

// Enables time-contraint policy and priority suitable for low-latency,
// glitch-resistant audio.
BOOL SetPriorityRealtimeAudio(mach_port_t mach_thread_id) {
	kern_return_t result;

	// Increase thread priority to real-time.

	// Please note that the thread_policy_set() calls may fail in
	// rare cases if the kernel decides the system is under heavy load
	// and is unable to handle boosting the thread priority.
	// In these cases we just return early and go on with life.

	// Make thread fixed priority.
	thread_extended_policy_data_t policy;
	policy.timeshare = 0; // Set to 1 for a non-fixed thread.
	result = thread_policy_set(mach_thread_id,
	                           THREAD_EXTENDED_POLICY,
	                           (thread_policy_t)&policy,
	                           THREAD_EXTENDED_POLICY_COUNT);
	if(result != KERN_SUCCESS) {
		DLog(@"thread_policy_set extended policy failure: %d", result);
		return NO;
	}

	// Set to relatively high priority.
	thread_precedence_policy_data_t precedence;
	precedence.importance = 63;
	result = thread_policy_set(mach_thread_id,
	                           THREAD_PRECEDENCE_POLICY,
	                           (thread_policy_t)&precedence,
	                           THREAD_PRECEDENCE_POLICY_COUNT);
	if(result != KERN_SUCCESS) {
		DLog(@"thread_policy_set precedence policy failure: %d", result);
		return NO;
	}

	// Most important, set real-time constraints.

	// Define the guaranteed and max fraction of time for the audio thread.
	// These "duty cycle" values can range from 0 to 1.  A value of 0.5
	// means the scheduler would give half the time to the thread.
	// These values have empirically been found to yield good behavior.
	// Good means that audio performance is high and other threads won't starve.
	const double kGuaranteedAudioDutyCycle = 0.75;
	const double kMaxAudioDutyCycle = 0.85;

	// Define constants determining how much time the audio thread can
	// use in a given time quantum.  All times are in milliseconds.

	// About 128 frames @44.1KHz
	const double kTimeQuantum = 2.9;

	// Time guaranteed each quantum.
	const double kAudioTimeNeeded = kGuaranteedAudioDutyCycle * kTimeQuantum;

	// Maximum time each quantum.
	const double kMaxTimeAllowed = kMaxAudioDutyCycle * kTimeQuantum;

	// Get the conversion factor from milliseconds to absolute time
	// which is what the time-constraints call needs.
	mach_timebase_info_data_t tb_info;
	mach_timebase_info(&tb_info);
	double ms_to_abs_time =
	((double)tb_info.denom / (double)tb_info.numer) * 1000000;

	thread_time_constraint_policy_data_t time_constraints;
	time_constraints.period = kTimeQuantum * ms_to_abs_time;
	time_constraints.computation = kAudioTimeNeeded * ms_to_abs_time;
	time_constraints.constraint = kMaxTimeAllowed * ms_to_abs_time;
	time_constraints.preemptible = 0;

	result = thread_policy_set(mach_thread_id,
	                           THREAD_TIME_CONSTRAINT_POLICY,
	                           (thread_policy_t)&time_constraints,
	                           THREAD_TIME_CONSTRAINT_POLICY_COUNT);
	if(result != KERN_SUCCESS) {
		DLog(@"thread_policy_set constraint policy failure: %d", result);
		return NO;
	}
	
	return YES;
}

@implementation Node

- (id)initWithController:(id)c previous:(id)p {
	self = [super init];
	if(self) {
		buffer = [[ChunkList alloc] initWithMaximumDuration:3.0];
		semaphore = [[Semaphore alloc] init];

		accessLock = [[NSRecursiveLock alloc] init];

		initialBufferFilled = NO;

		controller = c;
		endOfStream = NO;
		shouldContinue = YES;

		nodeChannelConfig = 0;
		nodeLossless = NO;

		if(@available(macOS 11, *)) {
			// Get the mach time info.
			struct mach_timebase_info timeBaseInfo;
			mach_timebase_info(&timeBaseInfo);

			// The frequency of the clock is: (timeBaseInfo.denom / timeBaseInfo.numer) * kOneNanosecond
			const double nanoSecFrequency = (double)(timeBaseInfo.denom) / (double)(timeBaseInfo.numer);
			const double frequency = nanoSecFrequency * kOneNanosecond;

			// Convert the interval time in seconds to mach time length.
			intervalMachLength = (int64_t)(kIOIntervalTime * frequency);
		}

		[self setPreviousNode:p];
	}

	return self;
}

- (AudioStreamBasicDescription)nodeFormat {
	return nodeFormat;
}

- (uint32_t)nodeChannelConfig {
	return nodeChannelConfig;
}

- (BOOL)nodeLossless {
	return nodeLossless;
}

- (void)writeData:(const void *)ptr amount:(size_t)amount {
	[accessLock lock];

	AudioChunk *chunk = [[AudioChunk alloc] init];
	[chunk setFormat:nodeFormat];
	if(nodeChannelConfig) {
		[chunk setChannelConfig:nodeChannelConfig];
	}
	[chunk setLossless:nodeLossless];
	[chunk assignSamples:ptr frameCount:amount / nodeFormat.mBytesPerPacket];

	const double chunkDuration = [chunk duration];
	double durationLeft = [buffer maxDuration] - [buffer listDuration];

	while(shouldContinue == YES && chunkDuration > durationLeft) {
		if(durationLeft < chunkDuration) {
			if(initialBufferFilled == NO) {
				initialBufferFilled = YES;
				if([controller respondsToSelector:@selector(initialBufferFilled:)])
					[controller performSelector:@selector(initialBufferFilled:) withObject:self];
			}
		}

		if(durationLeft < chunkDuration || shouldReset) {
			[accessLock unlock];
			[semaphore wait];
			[accessLock lock];
		}

		durationLeft = [buffer maxDuration] - [buffer listDuration];
	}

	[buffer addChunk:chunk];

	[accessLock unlock];
}

// Should be overwriten by subclass.
- (void)process {
}

- (void)threadEntry:(id)arg {
	@autoreleasepool {
		if([self followWorkgroup]) {
			[self process];
			[self leaveWorkgroup];
		}
	}
}

- (BOOL)followWorkgroup {
	if(@available(macOS 11, *)) {
		if(!wg) {
			if(!workgroup) {
				workgroup = AudioWorkIntervalCreate([[NSString stringWithFormat:@"%@ Work Interval", [self className]] UTF8String], clockId, &attr);
				isRealtimeError = !SetPriorityRealtimeAudio(pthread_mach_thread_np(pthread_self()));
				isRealtime = !isRealtimeError;
			}
			wg = workgroup;
			if(wg && !isRealtimeError) {
				int result = os_workgroup_join(wg, &wgToken);
				isDeadlineError = NO;
				if(result == 0) return YES;
				if(result == EALREADY) {
					DLog(@"Thread already in workgroup");
					return NO;
				} else {
					DLog(@"Cannot join workgroup, error %d", result);
					isRealtimeError = YES;
					return NO;
				}
			}
		}
		return wg != nil && !isRealtimeError;
	} else {
		if(!isRealtime && !isRealtimeError) {
			isRealtimeError = SetPriorityRealtimeAudio(pthread_mach_thread_np(pthread_self()));
			isRealtime = !isRealtimeError;
		}
		return YES;
	}
}

- (void)leaveWorkgroup {
	if(@available(macOS 11, *)) {
		if(wg && wgToken.sig && !isRealtimeError) {
			os_workgroup_leave(wg, &wgToken);
			bzero(&wgToken, sizeof(wgToken));
			wg = nil;
		}
	}
}

- (void)startWorkslice {
	if(@available(macOS 11, *)) {
		if(wg && !isRealtimeError && !isDeadlineError) {
			const uint64_t currentTime = mach_absolute_time();
			const uint64_t deadline = currentTime + intervalMachLength;
			int result = os_workgroup_interval_start(wg, currentTime, deadline, nil);
			if(result != 0) {
				DLog(@"Deadline error = %d", result);
				isDeadlineError = YES;
			}
		}
	}
}

- (void)endWorkslice {
	if(@available(macOS 11, *)) {
		if(wg && !isRealtimeError && !isDeadlineError) {
			int result = os_workgroup_interval_finish(wg, nil);
			if(result != 0) {
				DLog(@"Deadline end error = %d", result);
				isDeadlineError = YES;
			}
		}
	}
}

- (BOOL)peekFormat:(nonnull AudioStreamBasicDescription *)format channelConfig:(nonnull uint32_t *)config {
	[accessLock lock];

	BOOL ret = [[previousNode buffer] peekFormat:format channelConfig:config];

	[accessLock unlock];

	return ret;
}

- (AudioChunk *)readChunk:(size_t)maxFrames {
	[accessLock lock];

	if([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) {
		endOfStream = YES;
		[accessLock unlock];
		return [[AudioChunk alloc] init];
	}

	if([previousNode shouldReset] == YES) {
		@autoreleasepool {
			[buffer reset];
		}

		shouldReset = YES;
		[previousNode setShouldReset:NO];

		[[previousNode semaphore] signal];
	}

	AudioChunk *ret;

	@autoreleasepool {
		ret = [[previousNode buffer] removeSamples:maxFrames];
	}

	[accessLock unlock];

	if([ret frameCount]) {
		[[previousNode semaphore] signal];
	}

	return ret;
}

- (void)launchThread {
	[NSThread detachNewThreadSelector:@selector(threadEntry:) toTarget:self withObject:nil];
}

- (void)setPreviousNode:(id)p {
	previousNode = p;
}

- (id)previousNode {
	return previousNode;
}

- (BOOL)shouldContinue {
	return shouldContinue;
}

- (void)setShouldContinue:(BOOL)s {
	shouldContinue = s;
}

- (ChunkList *)buffer {
	return buffer;
}

- (void)resetBuffer {
	shouldReset = YES; // Will reset on next write.
	if(previousNode == nil) {
		@autoreleasepool {
			[accessLock lock];
			[buffer reset];
			[accessLock unlock];
		}
	}
}

- (Semaphore *)semaphore {
	return semaphore;
}

- (BOOL)endOfStream {
	return endOfStream;
}

- (void)setEndOfStream:(BOOL)e {
	endOfStream = e;
}

- (void)setShouldReset:(BOOL)s {
	shouldReset = s;
}
- (BOOL)shouldReset {
	return shouldReset;
}

// Buffering nodes should implement this
- (double)secondsBuffered {
	return 0.0;
}

@end
