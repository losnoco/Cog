//
//  PositionSlider.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "PositionSlider.h"
#import "TimeField.h"

@implementation PositionSlider {
	BOOL updatingMaxValue;
}

- (void)setDoubleValue:(double)value {
	if(!updatingMaxValue) {
		self.positionTextField.currentTime = (long)value;
	}

	[super setDoubleValue:value];
}

- (void)setMaxValue:(double)value {
	if(isnan(value) || isinf(value)) value = 0.0; // Clip invalid values from bad file playlist entries

	self.positionTextField.duration = (long)value;

	updatingMaxValue = YES;
	[super setMaxValue:value];
	updatingMaxValue = NO;
}

- (void)mouseDragged:(NSEvent *)theEvent {
	self.positionTextField.currentTime = (long)[self doubleValue];

	[super mouseDragged:theEvent];
}

@end
