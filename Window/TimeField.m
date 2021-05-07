//
//  TimeField.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "TimeField.h"

#import "SecondsFormatter.h"


static NSString *kTimerModeKey = @"timerShowTimeRemaining";


@implementation TimeField {
    BOOL showTimeRemaining;
    NSDictionary *fontAttributes;
    SecondsFormatter *secondsFormatter;
}

- (instancetype)initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        [self commonInit];
    }
    
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame
{
    if (self = [super initWithFrame:frame])
    {
        [self commonInit];
    }
    
    return self;
}

- (void)commonInit
{
    secondsFormatter = [[SecondsFormatter alloc] init];
}
    
- (void)awakeFromNib
{
    showTimeRemaining = [[NSUserDefaults standardUserDefaults] boolForKey:kTimerModeKey];
}

- (void)update
{
    NSString *text;
    if (showTimeRemaining == NO)
    {
		NSTimeInterval sec = self.currentTime;
        text = [secondsFormatter stringForTimeInterval:sec];
    }
    else
    {
		NSTimeInterval sec = self.currentTime - self.duration;
		// NOTE: The floating point standard has support for negative zero.
		// We use that to enforce the sign prefix.
		if (sec == 0.0) { sec = -0.0; }
        text = [secondsFormatter stringForTimeInterval:sec];
    }
    NSAttributedString *string = [[NSAttributedString alloc] initWithString:text
                                                                 attributes:fontAttributes];
    [self setAttributedStringValue: string];
}

- (void)mouseDown:(NSEvent *)theEvent
{
    showTimeRemaining = !showTimeRemaining;
    [[NSUserDefaults standardUserDefaults] setBool:showTimeRemaining forKey:kTimerModeKey];
    [self update];
}

- (void)setCurrentTime:(NSTimeInterval)currentTime
{
    _currentTime = currentTime;
    [self update];
}

@end
