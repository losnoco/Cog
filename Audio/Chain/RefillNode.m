//
//  RefillNode.m
//  Cog
//
//  Created by Christopher Snowhill on 1/13/22.
//  Copyright 2022 __LoSnoCo__. All rights reserved.
//

#import "RefillNode.h"
#import "Plugin.h"

#import "Logging.h"

@implementation RefillNode

- (id)initWithController:(id)c previous:(id)p {
	self = [super init];
	if(self) {
		// This special node should be able to handle up to four buffers
		buffer = [[ChunkList alloc] initWithMaximumDuration:12.0];
		semaphore = [[Semaphore alloc] init];

		initialBufferFilled = NO;

		controller = c;
		endOfStream = NO;
		shouldContinue = YES;

		nodeLossless = NO;

		[self setPreviousNode:p];
	}

	return self;
}

- (void)dealloc {
	DLog(@"Refill Node dealloc");
}

- (void)setFormat:(AudioStreamBasicDescription)format {
	nodeFormat = format;
}

@end
