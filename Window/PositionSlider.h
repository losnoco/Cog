//
//  PositionSlider.h
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "TrackingSlider.h"
#import <Cocoa/Cocoa.h>

@class TimeField;

@interface PositionSlider : TrackingSlider

@property(nonatomic) IBOutlet TimeField *positionTextField;

@end
