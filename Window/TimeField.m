//
//  TimeField.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "TimeField.h"

static NSString *kTimerModeKey = @"timerShowTimeRemaining";

NSString * timeStringForTimeInterval(NSTimeInterval timeInterval, BOOL enforceMinusSign) {
	const int64_t signed_total_seconds = (int64_t)timeInterval;
	const bool need_minus_sign = enforceMinusSign || signed_total_seconds < 0;
	const int64_t total_seconds = (need_minus_sign ? -1 : 1) * signed_total_seconds;
	const int64_t seconds = total_seconds % 60;
	const int64_t total_minutes = (total_seconds - seconds) / 60;
	const int64_t minutes = total_minutes % 60;
	const int64_t total_hours = (total_minutes - minutes) / 60;
	const int64_t hours = total_hours % 24;
	const int64_t days = (total_hours - hours) / 24;
	
	NSString *timeString = nil;
	
	if (days > 0) {
		timeString =
		[NSString localizedStringWithFormat:@"%s" "%" PRIi64 ":" "%02" PRIi64 ":" "%02" PRIi64 ":" "%02" PRIi64,
		 need_minus_sign ? "-" : "",
		 days,
		 hours,
		 minutes,
		 seconds];
	}
	else if (hours > 0) {
		timeString =
		[NSString localizedStringWithFormat:@"%s" "%" PRIi64 ":" "%02" PRIi64 ":" "%02" PRIi64,
		 need_minus_sign ? "-" : "",
		 hours,
		 minutes,
		 seconds];
	}
	else if (minutes > 0) {
		timeString =
		[NSString localizedStringWithFormat:@"%s" "%" PRIi64 ":" "%02" PRIi64,
		 need_minus_sign ? "-" : "",
		 minutes,
		 seconds];
	}
	else {
		timeString =
		[NSString localizedStringWithFormat:@"%s" "0" ":" "%02" PRIi64,
		 need_minus_sign ? "-" : "",
		 seconds];
	}

	return timeString;
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
        text = timeStringForTimeInterval(sec, NO);
    }
    else
    {
		NSTimeInterval sec = self.currentTime - self.duration;
		text = timeStringForTimeInterval(sec, YES);
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
