//
//  Semaphore.h
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <mach/mach.h>

@interface Semaphore : NSObject {
	semaphore_t semaphore;
}

-(id)init;
-(void)signal;
-(void)timedWait:(int)seconds;
-(void)wait;
-(void)waitIndefinitely;

@end
