//
//  TimeField.h
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface TimeField : NSTextField {
	BOOL showTimeRemaining;
	
	double value;
	double maxValue;
}

- (void)setMaxDoubleValue:(double)v;
- (void)setDoubleValue:(double)v;

@end
