//
//  PathWatcher.h
//  Cog
//
//  Created by Vincent Spader on 2/17/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <CoreServices/CoreServices.h>

@interface PathWatcher : NSObject {
	FSEventStreamRef stream;
	FSEventStreamContext *context;

	IBOutlet id delegate;
}

- (void)setDelegate:(id)d;
- (id)delegate;

- (void)setPath:(NSString *)path; // Set the path to watch
- (void)cleanUp;
@end

@protocol PathWatcherDelegate

- (void)pathDidChange:(NSString *)path;

@end
