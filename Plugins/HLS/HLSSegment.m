//
//  HLSSegment.m
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import "HLSSegment.h"

@implementation HLSSegment

- (instancetype)init {
	self = [super init];
	if (self) {
		_duration = 0.0;
		_sequenceNumber = 0;
		_encrypted = NO;
		_discontinuity = NO;
	}
	return self;
}

@end
