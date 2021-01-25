//
//  TimeField.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "TimeField.h"

static NSString *kTimerModeKey = @"timerShowTimeRemaining";

NSString * formatTimer(long minutes, long seconds, unichar prefix) {
    return [NSString localizedStringWithFormat:@"%C%lu:%02lu", prefix, minutes, seconds];;
}

@implementation TimeField {
    BOOL showTimeRemaining;
    NSDictionary *fontAttributes;
}

- (void)awakeFromNib
{
    showTimeRemaining = [[NSUserDefaults standardUserDefaults] boolForKey:kTimerModeKey];
    if (@available(macOS 10.15, *)) {
        fontAttributes =
            @{NSFontAttributeName : [NSFont monospacedSystemFontOfSize:13
                                                                weight:NSFontWeightMedium]};
        [self update];
    }
}

- (void)update
{
    NSString *text;
    if (showTimeRemaining == NO)
    {
        long sec = self.currentTime;
        text = formatTimer(sec / 60, sec % 60, 0x2007); // Digit-width space
    }
    else
    {
        long sec = MAX(0, self.duration - self.currentTime);
        text = formatTimer(sec / 60, sec % 60, 0x2012); // Hyphen
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

- (void)setCurrentTime:(NSInteger)currentTime
{
    _currentTime = currentTime;
    [self update];
}

@end
