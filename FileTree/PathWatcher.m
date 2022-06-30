//
//  PathWatcher.m
//  Cog
//
//  Created by Vincent Spader on 2/17/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "PathWatcher.h"

static void myFSEventCallback(
ConstFSEventStreamRef streamRef,
void *clientCallBackInfo,
size_t numEvents,
void *eventPaths,
const FSEventStreamEventFlags eventFlags[],
const FSEventStreamEventId eventIds[]) {
	int i;
	char **paths = eventPaths;
	PathWatcher *pathWatcher = (__bridge PathWatcher *)clientCallBackInfo;

	printf("Callback called\n");
	for(i = 0; i < numEvents; i++) {
		NSString *pathString = [[NSString alloc] initWithUTF8String:paths[i]];
		[[pathWatcher delegate] pathDidChange:pathString];
	}
}

@implementation PathWatcher

- (void)cleanUp {
	if(stream) {
		FSEventStreamStop(stream);
		FSEventStreamInvalidate(stream);
		FSEventStreamRelease(stream);
		stream = NULL;
	}
	if(context) {
		free(context);
		context = NULL;
	}
}

- (void)setPath:(NSString *)path {
	[self cleanUp];

	if(!path) return;

	// Create FSEvent stream
	NSArray *pathsToWatch = @[path];

	context = (FSEventStreamContext *)malloc(sizeof(FSEventStreamContext));
	context->version = 0;
	context->info = (__bridge void *)self;
	context->retain = NULL;
	context->release = NULL;

	// Create the stream, passing in a callback
	stream = FSEventStreamCreate(NULL,
	                             &myFSEventCallback,
	                             context,
	                             (__bridge CFArrayRef)pathsToWatch,
	                             kFSEventStreamEventIdSinceNow, // Or a previous event ID
	                             1.0, // latency in seconds
	                             kFSEventStreamCreateFlagNone // Watch this and all its subdirectories
	);

	FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

	FSEventStreamStart(stream);
}

- (void)setDelegate:(id)d {
	delegate = d;
}
- (id)delegate {
	return delegate;
}

- (void)dealloc {
	[self cleanUp];
}

@end
