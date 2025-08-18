//
//  SimpleBuffer.m
//  CogAudio
//
//  Created by Christopher Snowhill on 8/18/25.
//

#import "SimpleBuffer.h"

#import "Logging.h"

@implementation SimpleBuffer {
	BOOL paused, stopping;
	NSRecursiveLock *mutex;
}

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p latency:(double)latency {
	self = [super initWithController:c previous:p latency:latency];
	if(self) {
		mutex = [[NSRecursiveLock alloc] init];
	}
	return self;
}

- (void)dealloc {
	DLog(@"SimpleBuffer dealloc");
	[self setShouldContinue:NO];
	[self cleanUp];
	[super cleanUp];
}

- (BOOL)setup {
	return YES;
}

- (void)cleanUp {
	paused = NO;
	stopping = YES;
}

- (void)resetBuffer {
	paused = YES;
	[mutex lock];
	[buffer reset];
	paused = NO;
	[mutex unlock];
}

- (BOOL)paused {
	return paused;
}

- (void)setPreviousNode:(id)p {
	if(previousNode != p) {
		paused = YES;
		[mutex lock];
		previousNode = p;
		paused = NO;
		[mutex unlock];
	}
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
				if(previousNode && [previousNode endOfStream] == YES) {
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
		}
	}
}

- (AudioChunk *)convert {
	if(stopping)
		return nil;

	[mutex lock];

	if(stopping || !previousNode || ([[previousNode buffer] isEmpty] && [previousNode endOfStream] == YES) || [self shouldContinue] == NO) {
		[mutex unlock];
		return nil;
	}

	AudioChunk *chunk = [self readChunk:512];

	[mutex unlock];
	return chunk;
}

@end

