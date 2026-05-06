//
//  HLSVariant.m
//  HLS
//
//  Created by Claude on 2026-05-05.
//  Copyright 2026 __LoSnoCo__. All rights reserved.
//

#import "HLSVariant.h"

@implementation HLSVariant

- (instancetype)init {
	self = [super init];
	if (self) {
		_bandwidth = 0;
		_averageBandwidth = 0;
		_codecs = nil;
		_resolution = nil;
		_url = nil;
		_playlistURL = nil;
		_audioGroups = @{};
		_subtitleGroups = @{};
	}
	return self;
}

@end
