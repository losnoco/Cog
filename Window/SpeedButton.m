//
//  SpeedButton.m
//  Cog
//
//  Created by Christopher Snowhill on 9/20/24.
//  Copyright 2024 __LoSnoCo__. All rights reserved.
//

#import "SpeedButton.h"
#import "PlaybackController.h"

static double reverseSpeedScale(double input, double min, double max) {
	input = sqrtf((input - 0.2) * 10000.0 / (5.0 - 0.2));
	return (input * (max - min) / 100.0) + min;
}

@implementation SpeedButton {
	NSPopover *popover;
	NSViewController *viewController;

	// The two-slider layout from the nib, restored when leaving varispeed.
	NSSize stretchSize;
	NSRect tempoSliderFrame;
	NSRect tempoLabelFrame;
	NSRect resetButtonFrame;
	NSRect noticeButtonFrame;
	NSString *tempoLabelTitle;
}

- (void)awakeFromNib {
	popover = [NSPopover new];
	popover.behavior = NSPopoverBehaviorTransient;

	stretchSize = _popView.bounds.size;
	tempoSliderFrame = _TempoSlider.frame;
	tempoLabelFrame = _TempoLabel.frame;
	resetButtonFrame = _ResetButton.frame;
	noticeButtonFrame = _NoticeButton.frame;
	tempoLabelTitle = _TempoLabel.stringValue;
}

// Varispeed has no pitch of its own, so it gets a single speed slider; the
// stretchers keep separate pitch and tempo sliders and the lock.
- (void)layoutForEngine {
	const BOOL varispeed = [PlaybackController isVarispeed];

	_PitchSlider.hidden = varispeed;
	_PitchLabel.hidden = varispeed;
	_LockButton.hidden = varispeed;

	NSSize size = stretchSize;
	if(varispeed) {
		size.width = 40;
		const CGFloat center = size.width / 2;
		_TempoSlider.frame = NSOffsetRect(tempoSliderFrame, center - NSMidX(tempoSliderFrame), 0);
		_TempoLabel.frame = NSOffsetRect(tempoLabelFrame, center - NSMidX(tempoLabelFrame), 0);
		_ResetButton.frame = NSMakeRect(2, resetButtonFrame.origin.y, size.width - 4, resetButtonFrame.size.height);
		_NoticeButton.frame = NSMakeRect(0, noticeButtonFrame.origin.y, size.width, noticeButtonFrame.size.height);
		_TempoLabel.stringValue = @"⏱";
		_TempoLabel.toolTip = NSLocalizedString(@"Speed", @"Tooltip for the single varispeed slider");
	} else {
		_TempoSlider.frame = tempoSliderFrame;
		_TempoLabel.frame = tempoLabelFrame;
		_ResetButton.frame = resetButtonFrame;
		_NoticeButton.frame = noticeButtonFrame;
		_TempoLabel.stringValue = tempoLabelTitle;
		_TempoLabel.toolTip = nil;
	}

	[_popView setFrameSize:size];
	[popover setContentSize:size];
}

- (void)mouseDown:(NSEvent *)theEvent {
	[popover close];

	[self layoutForEngine];

	popover.contentViewController = nil;
	viewController = [NSViewController new];
	viewController.view = _popView;
	popover.contentViewController = viewController;

	[popover showRelativeToRect:self.bounds ofView:self preferredEdge:NSRectEdgeMaxY];

	[super mouseDown:theEvent];
}

- (IBAction)pressLock:(id)sender {
	BOOL speedLock = [[NSUserDefaults standardUserDefaults] boolForKey:@"speedLock"];
	speedLock = !speedLock;
	[_LockButton setTitle:speedLock ? @"🔒" : @"🔓"];
	[[NSUserDefaults standardUserDefaults] setBool:speedLock forKey:@"speedLock"];

	if(speedLock) {
		const double pitchValue = ([_PitchSlider doubleValue] - [_PitchSlider minValue]) / ([_PitchSlider maxValue] - [_PitchSlider minValue]);
		const double tempoValue = ([_TempoSlider doubleValue] - [_TempoSlider minValue]) / ([_TempoSlider maxValue] - [_TempoSlider minValue]);
		const double averageValue = (pitchValue + tempoValue) * 0.5;
		[_PitchSlider setDoubleValue:(averageValue * ([_PitchSlider maxValue] - [_PitchSlider minValue])) + [_PitchSlider minValue]];
		[_TempoSlider setDoubleValue:(averageValue * ([_TempoSlider maxValue] - [_TempoSlider minValue])) + [_TempoSlider minValue]];

		[[_PitchSlider target] changePitch:_PitchSlider];
		[[_TempoSlider target] changeTempo:_TempoSlider];
	}
}

- (IBAction)pressReset:(id)sender {
	[_PitchSlider setDoubleValue:reverseSpeedScale(1.0, [_PitchSlider minValue], [_PitchSlider maxValue])];
	[_TempoSlider setDoubleValue:reverseSpeedScale(1.0, [_TempoSlider minValue], [_TempoSlider maxValue])];

	[[_PitchSlider target] changePitch:_PitchSlider];
	[[_TempoSlider target] changeTempo:_TempoSlider];
}

@end
