//
//  TimeField.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "TimeField.h"

static NSString *kTimerModeKey = @"timerShowTimeRemaining";

NSString *formatTimer(long minutes, long seconds, unichar prefix, int padding) {
	NSString *paddingChar = [NSString stringWithFormat:@"%C", (unichar)0x2007]; // Digit-width space
	NSString *paddingString = [@"" stringByPaddingToLength:padding withString:paddingChar startingAtIndex:0];
	return [NSString localizedStringWithFormat:@"%@%C%lu:%02lu", paddingString, prefix, minutes, seconds];
}

@implementation TimeField {
	BOOL showTimeRemaining;
	NSDictionary *fontAttributes;
}

- (void)awakeFromNib {
	CGFloat fontSize = 13.0;

	showTimeRemaining = [[NSUserDefaults standardUserDefaults] boolForKey:kTimerModeKey];

	NSFont *font = [NSFont monospacedDigitSystemFontOfSize:fontSize weight:NSFontWeightRegular];

	fontAttributes = @{ NSFontAttributeName: font };

	[self update];
}

static int _log10(long minutes) {
	int ret = 1;
	while(minutes >= 10) {
		minutes /= 10;
		ret++;
	}
	return ret;
}

- (void)update {
	NSString *text;
	if(showTimeRemaining == NO) {
		long sec = self.currentTime;
		long sectotal = self.duration;
		int minutedigits = _log10(sec / 60);
		int otherminutedigits = _log10(sectotal / 60) + 1; // Plus hyphen
		int padding = MAX(0, otherminutedigits - minutedigits);
		text = formatTimer(sec / 60, sec % 60, 0x200B, padding); // Zero-width space
	} else {
		long sec = MAX(0, self.duration - self.currentTime);
		long sectotal = self.duration;
		int minutedigits = _log10(sec / 60) + 1; // Plus hyphen
		int otherminutedigits = _log10(sectotal / 60) + 1; // Also plus hyphen
		int padding = MAX(0, otherminutedigits - minutedigits);
		text = formatTimer(sec / 60, sec % 60, 0x2212, padding); // Minus
	}
	NSAttributedString *string = [[NSAttributedString alloc] initWithString:text
	                                                             attributes:fontAttributes];
	[self setAttributedStringValue:string];
}

- (void)mouseDown:(NSEvent *)theEvent {
	showTimeRemaining = !showTimeRemaining;
	[[NSUserDefaults standardUserDefaults] setBool:showTimeRemaining forKey:kTimerModeKey];
	[self update];
}

- (void)setCurrentTime:(NSInteger)currentTime {
	_currentTime = currentTime;
	[self update];
}

@end
