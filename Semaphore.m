//
//  Semaphore.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
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
	semaphore_signal(semaphore);
}

-(void)timedWait:(int)seconds
{
	mach_timespec_t timeout = {seconds, 0};
	
	semaphore_timedwait(semaphore, timeout);
}

-(void)wait
{
	semaphore_wait(semaphore);
}

@end
