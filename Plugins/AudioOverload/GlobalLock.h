//
//  GlobalLock.h
//  AudioOverload
//
//  Created by Vincent Spader on 2/28/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#include <pthread.h>

// Because NSLock bitches when we unlock from a different thread.
@interface GlobalLock : NSObject<NSLocking> {
	pthread_mutex_t mutex;
}

@end
