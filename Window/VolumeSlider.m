//
//  VolumeSlider.m
//  Cog
//
//  Created by Vincent Spader on 2/8/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "VolumeSlider.h"
#import "CogAudio/Helper.h"
#import "PlaybackController.h"

static void *kVolumeSliderContext = &kVolumeSliderContext;

@implementation VolumeSlider {
	NSTimer *currentTimer;
	BOOL wasInsideSnapRange;
	BOOL observersadded;
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
	BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
	MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

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

	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.volumeLimit" options:0 context:kVolumeSliderContext];
	observersadded = YES;
}

- (void)dealloc {
	if(observersadded) {
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.volumeLimit" context:kVolumeSliderContext];
	}
}

- (void)updateToolTip {
	const double value = [self doubleValue];
	// Sets volume to be the slider value if limit is set to 100% or the actual volume otherwise.
	const double volume = (MAX_VOLUME == 100) ? value : linearToLogarithmic(value, MAX_VOLUME);
	NSString *text;

	// If volume becomes less than 1%, display two decimal digits of precision (e.g. 0.34%).
	if(volume < 1)
		text = [NSString stringWithFormat:@"%0.2lf%%", volume];
	// Else if volume becomes less than 10%, display one decimal digit of precision (e.g. 3.4%).
	else if(volume < 10)
		text = [NSString stringWithFormat:@"%0.1lf%%", volume];
	// Else display no decimal digits.
	else
		text = [NSString stringWithFormat:@"%0.lf%%", volume];

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

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context != kVolumeSliderContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}

	if([keyPath isEqualToString:@"values.volumeLimit"]) {
		BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
		const double new_MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

		if(MAX_VOLUME != new_MAX_VOLUME) {
			double currentLevel = linearToLogarithmic([self doubleValue], MAX_VOLUME);
			[self setDoubleValue:logarithmicToLinear(currentLevel, new_MAX_VOLUME)];
		}
		MAX_VOLUME = new_MAX_VOLUME;
	}
}

- (BOOL)sendAction:(SEL)theAction to:(id)theTarget {
	// Snap to 100% if value is close
	double snapTarget = logarithmicToLinear(100.0, MAX_VOLUME);
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

	[[self target] changeVolume:self];

	[self showToolTipForDuration:1.0];
}

@end
