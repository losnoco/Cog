//
//  HLSSegment.m
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Rewritten by Christopher Snowhill on 2026-05-06
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import "HLSSegment.h"

@implementation HLSSegment

- (instancetype)init {
	self = [super init];
	if(self) {
		_duration = 0.0;
		_sequenceNumber = 0;
		_discontinuitySequence = 0;
		_discontinuity = NO;
		_encrypted = NO;
	}
	return self;
}

@end
