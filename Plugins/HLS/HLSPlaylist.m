//
//  HLSPlaylist.m
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Rewritten by Christopher Snowhill on 2026-05-06
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import "HLSPlaylist.h"
#import "HLSSegment.h"
#import "HLSVariant.h"

@implementation HLSPlaylist

- (instancetype)init {
	self = [super init];
	if(self) {
		_isMasterPlaylist = NO;
		_isLiveStream = YES;
		_hasEndList = NO;
		_version = 1;
		_targetDuration = 10;
		_mediaSequence = 0;
		_discontinuitySequence = 0;
		_segments = @[];
		_variants = @[];
		_type = HLSPlaylistTypeUnspecified;
	}
	return self;
}

@end
