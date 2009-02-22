//
//  PositionSlider.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "PositionSlider.h"
#import "TimeField.h"

@implementation PositionSlider

- (void)setDoubleValue:(double)value
{
	[positionTextField setDoubleValue:value];
	
	[super setDoubleValue:value];
}

- (void)setMaxValue:(double)value
{
	[positionTextField setMaxDoubleValue:value];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
	[positionTextField setDoubleValue:[self doubleValue]];
	
	[super mouseDragged:theEvent];
}

@end
