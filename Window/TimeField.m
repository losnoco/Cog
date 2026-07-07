//
//  TimeField.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "TimeField.h"

static NSString *kTimerModeKey = @"timerShowTimeRemaining";
static NSNotificationName CogPlaybackAllowsZeroTimeDisplayNotification = @"CogPlaybackAllowsZeroTimeDisplayNotification";
static const NSTimeInterval kTransientZeroDelay = 0.75;

NSString *formatTimer(long minutes, long seconds, unichar prefix, int padding) {
	NSString *paddingChar = [NSString stringWithFormat:@"%C", (unichar)0x2007]; // Digit-width space
	NSString *paddingString = [@"" stringByPaddingToLength:padding withString:paddingChar startingAtIndex:0];
	return [NSString localizedStringWithFormat:@"%@%C%lu:%02lu", paddingString, prefix, minutes, seconds];
}

@implementation TimeField {
	BOOL showTimeRemaining;
	BOOL allowsImmediateZeroTime;
	NSUInteger zeroTimeGeneration;
}

- (void)awakeFromNib {
	CGFloat fontSize = 13.0;

	showTimeRemaining = [[NSUserDefaults standardUserDefaults] boolForKey:kTimerModeKey];
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(allowImmediateZeroTime:)
	                                             name:CogPlaybackAllowsZeroTimeDisplayNotification
	                                           object:nil];

	self.font = [NSFont monospacedDigitSystemFontOfSize:fontSize
	                                             weight:NSFontWeightRegular];

	[self update];
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)allowImmediateZeroTime:(NSNotification *)notification {
	allowsImmediateZeroTime = YES;
	NSUInteger generation = ++zeroTimeGeneration;
	dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(NSEC_PER_SEC * kTransientZeroDelay)), dispatch_get_main_queue(), ^{
		if(generation == self->zeroTimeGeneration) {
			self->allowsImmediateZeroTime = NO;
		}
	});
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
	[self setStringValue:text];
}

- (void)mouseDown:(NSEvent *)theEvent {
	showTimeRemaining = !showTimeRemaining;
	[[NSUserDefaults standardUserDefaults] setBool:showTimeRemaining forKey:kTimerModeKey];
	[self update];
}

- (void)setCurrentTime:(NSInteger)currentTime {
	if(currentTime < 0) {
		currentTime = 0;
	}

	if(currentTime == 0 && _currentTime > 0 && _duration == 0 && !allowsImmediateZeroTime) {
		NSUInteger generation = ++zeroTimeGeneration;
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(NSEC_PER_SEC * kTransientZeroDelay)), dispatch_get_main_queue(), ^{
			if(generation == self->zeroTimeGeneration) {
				self->_currentTime = currentTime;
				[self update];
			}
		});
		return;
	}

	zeroTimeGeneration++;
	if(currentTime != 0) {
		allowsImmediateZeroTime = NO;
	}

	_currentTime = currentTime;
	[self update];
}

@end
