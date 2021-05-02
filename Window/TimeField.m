//
//  TimeField.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "TimeField.h"

static NSString *kTimerModeKey = @"timerShowTimeRemaining";

NSString * formatTimer(NSTimeInterval minutes, NSTimeInterval seconds, char *prefix) {
	return [NSString localizedStringWithFormat:@"%s%02u:%02u", prefix, (unsigned)minutes, (unsigned)seconds];
}

@implementation TimeField {
    BOOL showTimeRemaining;
    NSDictionary *fontAttributes;
}

- (void)awakeFromNib
{
    showTimeRemaining = [[NSUserDefaults standardUserDefaults] boolForKey:kTimerModeKey];
    if (@available(macOS 10.11, *)) {
        fontAttributes =
            @{NSFontAttributeName : [NSFont monospacedDigitSystemFontOfSize:13
																	 weight:NSFontWeightRegular]};
        [self update];
    }
}

- (void)update
{
    NSString *text;
    if (showTimeRemaining == NO)
    {
		NSTimeInterval sec = self.currentTime;
        text = formatTimer(sec / 60, fmod(sec, 60), ""); // No prefix.
    }
    else
    {
		NSTimeInterval sec = MAX(0.0, self.duration - self.currentTime);
		text = formatTimer(sec / 60,  fmod(sec, 60), "-"); // Hyphen-minus.
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
