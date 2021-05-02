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
    self.positionTextField.currentTime = value;

    [super setDoubleValue:value];
}

- (void)setMaxValue:(double)value
{
    self.positionTextField.duration = value;

    [super setMaxValue:value];
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    self.positionTextField.currentTime = [self doubleValue];

    [super mouseDragged:theEvent];
}

@end
