//
//  CueSheetTrack.m
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CueSheetTrack.h"


@implementation CueSheetTrack

- (void)initWithTrack:(NSString *)t start:(double)s end:(double)e
{
	self = [super init];
	if (self)
	{
		track = [t copy];
		start = s;
		end = e;
	}
	
	return self;
}

- (NSString *)track
{
	return track;
}

- (double)start
{
	return start;
}

- (double)end
{
	return end;
}

@end
