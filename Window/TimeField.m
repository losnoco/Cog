//
//  TimeField.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "TimeField.h"


@implementation TimeField

- (void)update
{
	NSString *text;
	if (showTimeRemaining == NO)
	{
		int sec = value;
		text = [NSString stringWithFormat:NSLocalizedString(@"TimeElapsed", @""), sec/60, sec%60];
	}
	else
	{
		int sec = maxValue - value;
		if (sec < 0)
			sec = 0;
		text = [NSString stringWithFormat:NSLocalizedString(@"TimeRemaining", @""), sec/60, sec%60];
	}
	[self setStringValue:text];
}

- (void)mouseDown:(NSEvent *)theEvent
{
	showTimeRemaining = !showTimeRemaining;
	[self update];
}

- (void)setMaxDoubleValue:(double)v
{
	maxValue = v;
}

- (void)setDoubleValue:(double)v
{
	value = v;
	[self update];
}

@end
