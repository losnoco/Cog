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
    self.positionTextField.currentTime = (long)value;

    [super setDoubleValue:value];
}

- (void)setMaxValue:(double)value
{
    self.positionTextField.duration = (long)value;

    [super setMaxValue:value];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    self.positionTextField.currentTime = (long)[self doubleValue];

    [super mouseDragged:theEvent];
}

@end
