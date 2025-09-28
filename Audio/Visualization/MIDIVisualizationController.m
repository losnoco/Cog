//
//  MIDIVisualizationController.m
//  CogAudio
//
//  Created by Christopher Snowhill on 9/27/25.
//

#import <Foundation/Foundation.h>

#import <CogAudio/VisualizationController.h>

#import <CogAudio/MIDIVisualizationController.h>

// PlaylistEntry hack
extern NSURL *_getPlaylistEntryURL(id object);

static NSString *CogPlaybackDidPrebufferNotification = @"CogPlaybackDidPrebufferNotification";

static NSString *CogPlaybackDidBeginNotificiation = @"CogPlaybackDidBeginNotificiation";
static NSString *CogPlaybackDidStopNotificiation = @"CogPlaybackDidStopNotificiation";

static void *kSCViewContext = &kSCViewContext;

@interface MIDIFileEventContainer : NSObject
	@property (nonatomic) NSURL *url;
	@property (nonatomic) NSMutableArray<MIDIEvent *> *events;
@end

@implementation MIDIFileEventContainer
@end

@implementation MIDIVisualizationController {
	VisualizationController *controller;

	NSTimer *cleanupTimer;

	BOOL observersAdded;
	BOOL prebuffered;

	NSURL *currentTrack;
	NSMutableArray<MIDIFileEventContainer *> *files;
}

- (id)initWithController:(id)c {
	self = [super init];
	if(self) {
		controller = c;
		files = [NSMutableArray new];

		cleanupTimer = [NSTimer timerWithTimeInterval:30.0
											   target:self
											 selector:@selector(cleanup:)
											 userInfo:nil
											  repeats:YES];
		[[NSRunLoop currentRunLoop] addTimer:cleanupTimer
									 forMode:NSRunLoopCommonModes];
		//prebuffered = NO;

		[self addObservers];
	}
	return self;
}

- (void)dealloc {
	[self removeObservers];
	[cleanupTimer invalidate];
	cleanupTimer = nil;
}

- (void)cleanup:(id)sender {
	@synchronized (self) {
		for(size_t idx = 0, fileCount = [files count]; idx < fileCount;) {
			MIDIFileEventContainer *file = files[idx];
			NSArray<MIDIEvent *> *events = file.events;
			uint64_t count = [events count];
			if(count) {
				uint64_t timestampEnd = events.lastObject.timestamp;
				uint64_t duration = timestampEnd - events.firstObject.timestamp;
				const uint64_t maxDuration = 120 * 1000;
				if(duration > maxDuration) {
					size_t i = 0;
					uint64_t wantedStart = timestampEnd - maxDuration;
					for(MIDIEvent *event in events) {
						if(event.timestamp > wantedStart) break;
						++i;
					}
					if(i < count)
						files[idx].events = [[events subarrayWithRange:NSMakeRange(i, count - i)] mutableCopy];
					else if(idx > 0)
						files[idx].events = [NSMutableArray new];
					else {
						[files removeObjectAtIndex:0];
						--fileCount;
						continue;
					}
				}
			}
			++idx;
		}
	}
}

- (void)addObservers {
	if(!observersAdded) {
		NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
		[defaultCenter addObserver:self
						  selector:@selector(playbackDidBegin:)
							  name:CogPlaybackDidBeginNotificiation
							object:nil];
		[defaultCenter addObserver:self
						  selector:@selector(playbackDidStop:)
							  name:CogPlaybackDidStopNotificiation
							object:nil];

		[defaultCenter addObserver:self
						  selector:@selector(playbackPrebuffered:)
							  name:CogPlaybackDidPrebufferNotification
							object:nil];

		observersAdded = YES;
	}
}

- (void)removeObservers {
	if(observersAdded) {
		NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
		[defaultCenter removeObserver:self
								 name:CogPlaybackDidBeginNotificiation
							   object:nil];
		[defaultCenter removeObserver:self
								 name:CogPlaybackDidStopNotificiation
							   object:nil];

		[defaultCenter removeObserver:self
								 name:CogPlaybackDidPrebufferNotification
							   object:nil];

		observersAdded = NO;
	}
}

- (void)playbackDidBegin:(NSNotification *)notification {
	@synchronized (self) {
		if(currentTrack) {
			[self removeTrack:currentTrack];
			NSURL *url = _getPlaylistEntryURL(notification.object);
			if([files count]) {
				MIDIFileEventContainer *file = files[0];
				if([file.url isEqualTo:url])
					currentTrack = files[0].url;
				else
					currentTrack = nil;
			} else {
				currentTrack = nil;
			}
		}
	}
}

- (void)playbackDidStop:(NSNotification *)notification {
	prebuffered = NO;
	@synchronized (self) {
		[files removeAllObjects];
		currentTrack = nil;
	}
}

- (void)playbackPrebuffered:(NSNotification *)notification {
	prebuffered = YES;
	@synchronized (self) {
		if(!currentTrack && [files count]) {
			currentTrack = files[0].url;
		}
	}
}

- (NSURL *)currentTrack {
	@synchronized (self) {
		return currentTrack;
	}
}

- (void)removeTrack:(NSURL *)url {
	@synchronized (self) {
		assert([url isEqualTo:files[0].url]);
		[files removeObjectAtIndex:0];
	}
}

- (void)addTrack:(NSURL *)url {
	NSMutableArray *events = [NSMutableArray new];
	MIDIFileEventContainer *file = [MIDIFileEventContainer new];
	file.url = url;
	file.events = events;
	[files addObject:file];
}

- (uint64_t)currentTimestamp {
	if(!prebuffered) return 0;

	@synchronized (self) {
		if(!currentTrack) return 0;
	}

	uint64_t currentTimestamp = 0;
	double latency = [controller getFullLatency];
	uint64_t mslatency = floor(latency * 1000.0);
	@synchronized (self) {
		if([files count]) {
			NSArray<MIDIEvent *> *events = files[0].events;
			if(events && [events count]) {
				MIDIEvent *event = [events lastObject];
				currentTimestamp = event.timestamp;
				if(currentTimestamp > mslatency)
					currentTimestamp -= mslatency;
				else
					currentTimestamp = 0;
			}
		}
	}
	return currentTimestamp;
}

- (NSArray<MIDIEvent *> *)eventsForTimestamp:(uint64_t)timestamp {
	if(!prebuffered || !currentTrack) return nil;

	@synchronized (self) {
		NSMutableArray *events = [files count] ? files[0].events : nil;
		if(!events || ![events count]) return nil;

		size_t i = 0;
		for(MIDIEvent *event in events) {
			if(event.timestamp > timestamp)
				break;
			++i;
		}

		if(!i)
			return nil;

		size_t count = [events count];
		NSArray *ret = [events subarrayWithRange:NSMakeRange(0, i)];
		if(i < count)
			files[0].events = [[events subarrayWithRange:NSMakeRange(i, count - i)] mutableCopy];
		else
			files[0].events = [NSMutableArray new];

		return ret;
	}
}

- (NSMutableArray<MIDIEvent *> *)findEventsForUrl:(NSURL *)url withTimestamp:(uint64_t)timestamp {
	for(MIDIFileEventContainer *file in files) {
		if([file.url isEqualTo:url]) {
			NSMutableArray *events = file.events;
			if(![events count])
				return events;
			MIDIEvent *event = [events count] ? [events lastObject] : nil;
			if(event) {
				if(timestamp >= event.timestamp) {
					return events;
				}
			}
		}
	}
	return nil;
}

- (void)postEvent:(MIDIEvent *)event {
	@synchronized (self) {
		NSMutableArray *events = [self findEventsForUrl:event.url withTimestamp:event.timestamp];
		if(!events) {
			[self addTrack:event.url];
			events = [files lastObject].events;
		}

		[events addObject:event];
	}
}

@end

@implementation MIDIEvent
@end

@implementation SCVisEvent

- (id)initWithUrl:(NSURL *)url whichPort:(uint32_t)which state:(const void *)state stateSize:(size_t)size timestamp:(uint64_t)timestamp {
	self = [super init];
	if(self) {
		self.url = url;
		self.timestamp = timestamp;
		self.which = which;
		self.state = [NSData dataWithBytes:state length:size];
	}
	return self;
}

@end
