//
//  Semaphore.m
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "Semaphore.h"


@implementation Semaphore

-(id)init
{
	self = [super init];
	if (self)
	{
		semaphore_create(mach_task_self(), &semaphore, SYNC_POLICY_FIFO, 0);
	}
	
	return self;
}

-(void)signal
{
	semaphore_signal_all(semaphore);
}

-(void)timedWait:(int)seconds
{
	mach_timespec_t timeout = {seconds, 0};
	
	semaphore_timedwait(semaphore, timeout);
}

-(void)wait
{
	mach_timespec_t t = {2.0, 0.0}; //2 second timeout
	semaphore_timedwait(semaphore, t);
}

@end
