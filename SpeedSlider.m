//
//  SpeedSlider.m
//  Cog
//
//  Created by Christopher Snowhill on 9/20/24.
//  Copyright 2024 __LoSnoCo__. All rights reserved.
//

#import "SpeedSlider.h"
#import "CogAudio/Helper.h"
#import "PlaybackController.h"

static void *kSpeedSliderContext = &kSpeedSliderContext;

@implementation SpeedSlider {
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
	textView = [[NSText alloc] init];
	[textView setFrame:NSMakeRect(0, 0, 50, 20)];
	textView.drawsBackground = NO;
	textView.editable = NO;
	textView.alignment = NSTextAlignmentCenter;

	NSViewController *viewController = [[NSViewController alloc] init];
	viewController.view = textView;

	popover = [[NSPopover alloc] init];
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
	const double value = [self doubleValue];
	NSString *text;
	
	double speed;
	if(value < 0.2) {
		speed = 0.2;
	} else if(value > 5.0) {
		speed = 5.0;
	} else {
		speed = value;
	}

	if(speed < 1)
		text = [NSString stringWithFormat:@"%0.2lf×", speed];
	else
		text = [NSString stringWithFormat:@"%0.1lf×", speed];

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
	if(context != kSpeedSliderContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}
}*/

- (BOOL)sendAction:(SEL)theAction to:(id)theTarget {
	// Snap to 1.0× if value is close
	double snapTarget = 1.0;
	double snapProgress = ([self doubleValue] - snapTarget) / (self.maxValue - self.minValue);

	if(fabs(snapProgress) < 0.005) {
		[self setDoubleValue:snapTarget];
		if(!wasInsideSnapRange) {
			[[NSHapticFeedbackManager defaultPerformer] performFeedbackPattern:NSHapticFeedbackPatternGeneric performanceTime:NSHapticFeedbackPerformanceTimeDefault];
		}
		wasInsideSnapRange = YES;
	} else {
		wasInsideSnapRange = NO;
	}

	[self showToolTip];

	return [super sendAction:theAction to:theTarget];
}

- (void)scrollWheel:(NSEvent *)theEvent {
	double change = [theEvent deltaY];

	[self setDoubleValue:[self doubleValue] + change];

	[[self target] changeSpeed:self];

	[self showToolTipForDuration:1.0];
}

@end
