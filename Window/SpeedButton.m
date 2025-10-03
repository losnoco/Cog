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
}

- (void)awakeFromNib {
	popover = [NSPopover new];
	popover.behavior = NSPopoverBehaviorTransient;
	[popover setContentSize:_popView.bounds.size];
}

- (void)mouseDown:(NSEvent *)theEvent {
	[popover close];

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
