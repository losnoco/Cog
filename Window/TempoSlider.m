//
//  TempoSlider.m
//  Cog
//
//  Created by Christopher Snowhill on 9/20/24.
//  Copyright 2024 __LoSnoCo__. All rights reserved.
//

#import "TempoSlider.h"
#import "CogAudio/Helper.h"
#import "PlaybackController.h"

static void *kTempoSliderContext = &kTempoSliderContext;

@implementation TempoSlider {
	NSTimer *currentTimer;
	BOOL wasInsideSnapRange;
	/*BOOL observersadded;*/
}

- (id)initWithFrame:(NSRect)frame {
	self = [super initWithFrame:frame];
	return self;
}

- (id)initWithCoder:(NSCoder *)coder {
	self = [super initWithCoder:coder];
	return self;
}

- (void)awakeFromNib {
	wasInsideSnapRange = NO;
	textView = [NSText new];
	[textView setFrame:NSMakeRect(0, 0, 50, 20)];
	textView.drawsBackground = NO;
	textView.editable = NO;
	textView.alignment = NSTextAlignmentCenter;

	NSViewController *viewController = [NSViewController new];
	viewController.view = textView;

	popover = [NSPopover new];
	popover.contentViewController = viewController;
	// Don't hide the popover automatically.
	popover.behavior = NSPopoverBehaviorTransient;
	popover.animates = NO;
	[popover setContentSize:textView.bounds.size];

	/*observersadded = YES;*/
}

/*- (void)dealloc {
	if(observersadded) {
	}
}*/

- (void)updateToolTip {
	const double value = ([self doubleValue] - [self minValue]) * 100.0 / ([self maxValue] - [self minValue]);
	NSString *text;
	
	const double adjustedValue = ((value * value) * (5.0 - 0.2) / 10000.0) + 0.2;
	
	double tempo;
	if(adjustedValue < 0.2) {
		tempo = 0.2;
	} else if(adjustedValue > 5.0) {
		tempo = 5.0;
	} else {
		tempo = adjustedValue;
	}

	if(tempo < 1)
		text = [NSString stringWithFormat:@"%0.2lf×", tempo];
	else
		text = [NSString stringWithFormat:@"%0.1lf×", tempo];

	[textView setString:text];
}

- (void)showToolTip {
	[self updateToolTip];

	double progress = (self.maxValue - [self doubleValue]) / (self.maxValue - self.minValue);
	CGFloat width = self.knobThickness - 1;
	// Show tooltip to the left of the Slider Knob
	CGFloat height = self.knobThickness / 2.f + (self.bounds.size.height - self.knobThickness) * progress - 1;

	NSWindow *window = self.window;
	NSPoint screenPoint = [window convertPointToScreen:NSMakePoint(width + 1, height + 1)];

	if(window.screen.frame.size.width < screenPoint.x + textView.bounds.size.width + 64) // wing it
		[popover showRelativeToRect:NSMakeRect(1, height, 2, 2) ofView:self preferredEdge:NSRectEdgeMinX];
	else
		[popover showRelativeToRect:NSMakeRect(width, height, 2, 2) ofView:self preferredEdge:NSRectEdgeMaxX];
	[self.window.parentWindow makeKeyWindow];
}

- (void)showToolTipForDuration:(NSTimeInterval)duration {
	[self showToolTip];

	[self hideToolTipAfterDelay:duration];
}

- (void)showToolTipForView:(NSView *)view closeAfter:(NSTimeInterval)duration {
	[self updateToolTip];

	[popover showRelativeToRect:view.bounds ofView:view preferredEdge:NSRectEdgeMaxY];

	[self hideToolTipAfterDelay:duration];
}

- (void)hideToolTip {
	[popover close];
}

- (void)hideToolTipAfterDelay:(NSTimeInterval)duration {
	if(currentTimer) {
		[currentTimer invalidate];
		currentTimer = nil;
	}

	if(duration > 0.0) {
		currentTimer = [NSTimer scheduledTimerWithTimeInterval:duration
		                                                target:self
		                                              selector:@selector(hideToolTip)
		                                              userInfo:nil
		                                               repeats:NO];
		[[NSRunLoop mainRunLoop] addTimer:currentTimer forMode:NSRunLoopCommonModes];
	}
}

/*- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context != kTempoSliderContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}
}*/

- (BOOL)sendAction:(SEL)theAction to:(id)theTarget {
	// Snap to 1.0× if value is close
	double snapTarget = 1.0;
	const double value = ([self doubleValue] - [self minValue]) * 100.0 / ([self maxValue] - [self minValue]);
	const double adjustedValue = ((value * value) * (5.0 - 0.2) / 10000.0) + 0.2;
	double snapProgress = (adjustedValue - snapTarget);

	BOOL speedLock = [[NSUserDefaults standardUserDefaults] boolForKey:@"speedLock"];
	if(speedLock) {
		[_PitchSlider setDoubleValue:[self doubleValue]];
	}

	if(fabs(snapProgress) < 0.01) {
		const double inverseValue = sqrtf((snapTarget - 0.2) * 10000.0 / (5.0 - 0.2));
		[self setDoubleValue:inverseValue];
		if(speedLock) {
			[_PitchSlider setDoubleValue:inverseValue];
		}
		if(!wasInsideSnapRange) {
			[[NSHapticFeedbackManager defaultPerformer] performFeedbackPattern:NSHapticFeedbackPatternGeneric performanceTime:NSHapticFeedbackPerformanceTimeDefault];
		}
		wasInsideSnapRange = YES;
	} else {
		wasInsideSnapRange = NO;
	}

	[self showToolTipForDuration:1.0];

	return [super sendAction:theAction to:theTarget];
}

- (void)scrollWheel:(NSEvent *)theEvent {
	double change = [theEvent deltaY];

	[self setDoubleValue:[self doubleValue] + change];

	[[self target] changeTempo:self];
	
	BOOL speedLock = [[NSUserDefaults standardUserDefaults] boolForKey:@"speedLock"];
	if(speedLock) {
		[_PitchSlider setDoubleValue:[self doubleValue]];

		[[self target] changePitch:self];
	}

	[self showToolTipForDuration:1.0];
}

@end
