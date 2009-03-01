//
//  GlobalLock.m
//  AudioOverload
//
//  Created by Vincent Spader on 2/28/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "GlobalLock.h"


@implementation GlobalLock

- (id)init
{
	self = [super init];
	if (self)
	{
		if (0 != pthread_mutex_init(&mutex, NULL)) {
			return nil;
		}
	}
	
	return self;
}

- (void)dealloc 
{
	pthread_mutex_destroy(&mutex);
	
	[super dealloc];
}

- (void)lock
{
	pthread_mutex_lock(&mutex);
}

- (void)unlock
{
	pthread_mutex_unlock(&mutex);
}

@end
