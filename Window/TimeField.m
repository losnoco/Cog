//
//  TimeField.m
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "TimeField.h"
#import "PlaybackController.h"

static NSString *kTimerModeKey = @"timerShowTimeRemaining";

NSString *formatTimer(long minutes, long seconds, unichar prefix, int padding) {
	NSString *paddingChar = [NSString stringWithFormat:@"%C", (unichar)0x2007]; // Digit-width space
	NSString *paddingString = [@"" stringByPaddingToLength:padding withString:paddingChar startingAtIndex:0];
	return [NSString localizedStringWithFormat:@"%@%C%lu:%02lu", paddingString, prefix, minutes, seconds];
}

@implementation TimeFieldCell

- (NSRect)drawingRectForBounds:(NSRect)rect {
	NSRect drawingRect = [super drawingRectForBounds:rect];
	NSFont *font = self.font;
	if(!font) return drawingRect;
	// The bezel is as tall as the field would be with its control size's
	// own font, at the top of the field.
	NSTextFieldCell *plain = [[NSTextFieldCell alloc] initTextCell:@"0"];
	plain.bezeled = self.bezeled;
	plain.bezelStyle = self.bezelStyle;
	plain.controlSize = self.controlSize;
	plain.font = [NSFont systemFontOfSize:[NSFont systemFontSizeForControlSize:self.controlSize]];
	CGFloat bezelHeight = MIN(plain.cellSize.height, NSHeight(rect));
	// The text is laid out from the top of the drawing rect; move it so
	// that the digits, from the baseline up, sit in the bezel's middle.
	NSLayoutManager *layoutManager = [NSLayoutManager new];
	CGFloat baseline = NSMinY(rect) + bezelHeight / 2.0 + font.capHeight / 2.0;
	drawingRect.origin.y = baseline - [layoutManager defaultBaselineOffsetForFont:font];
	drawingRect.size.height = [layoutManager defaultLineHeightForFont:font];
	return drawingRect;
}

@end

@implementation TimeField {
	BOOL showTimeRemaining;
	BOOL allowZeroTime;
}

- (void)awakeFromNib {
	CGFloat fontSize = 13.0;

	showTimeRemaining = [[NSUserDefaults standardUserDefaults] boolForKey:kTimerModeKey];
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(playbackWillResetPosition:)
	                                             name:CogPlaybackWillResetPositionNotification
	                                           object:nil];

	self.font = [NSFont monospacedDigitSystemFontOfSize:fontSize
	                                             weight:NSFontWeightRegular];

	[self update];
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)playbackWillResetPosition:(NSNotification *)notification {
	allowZeroTime = YES;
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
	BOOL zeroTimeAllowed = allowZeroTime;
	allowZeroTime = NO;

	if(currentTime == 0 && _currentTime > 0 && _duration == 0 && !zeroTimeAllowed) {
		return;
	}

	_currentTime = currentTime;
	[self update];
}

@end
