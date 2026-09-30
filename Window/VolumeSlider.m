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

@interface VolumeSlider ()
- (double)displayedVolume;
- (void)setDisplayedVolume:(double)volume;
- (void)snapToVolumeStep;
@end

@implementation VolumeSlider {
	NSTimer *currentTimer;
	BOOL wasInsideSnapRange;
	BOOL observersadded;
	double scrollDeltaRemainder;
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
	[textView setString:[NSString stringWithFormat:@"%.0lf%%", round([self displayedVolume])]];
}

- (double)displayedVolume {
	const double value = [self doubleValue];
	return (MAX_VOLUME == 100) ? value : linearToLogarithmic(value, MAX_VOLUME);
}

- (void)setDisplayedVolume:(double)volume {
	volume = MAX(0.0, MIN(volume, MAX_VOLUME));
	const double value = (MAX_VOLUME == 100) ? volume : logarithmicToLinear(volume, MAX_VOLUME);
	[self setDoubleValue:value];
}

- (void)snapToVolumeStep {
	[self setDisplayedVolume:round([self displayedVolume])];
}

- (void)showToolTip {
	[self updateToolTip];

	double range = self.maxValue - self.minValue;
	double progress = range == 0 ? 0 : ([self doubleValue] - self.minValue) / range;
	CGFloat knobCenter = self.knobThickness / 2.f + (self.bounds.size.width - self.knobThickness) * progress;
	NSRect anchor = NSMakeRect(knobCenter - 1, NSMidY(self.bounds) - 1, 2, 2);

	[popover showRelativeToRect:anchor ofView:self preferredEdge:NSRectEdgeMaxY];
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
		[self setDisplayedVolume:100.0];
		if(!wasInsideSnapRange) {
			[[NSHapticFeedbackManager defaultPerformer] performFeedbackPattern:NSHapticFeedbackPatternGeneric performanceTime:NSHapticFeedbackPerformanceTimeDefault];
		}
		wasInsideSnapRange = YES;
	} else {
		[self snapToVolumeStep];
		wasInsideSnapRange = NO;
	}

	[self showToolTip];

	return [super sendAction:theAction to:theTarget];
}

- (void)scrollWheel:(NSEvent *)theEvent {
	scrollDeltaRemainder += [theEvent deltaY];
	double steps = trunc(scrollDeltaRemainder);

	if(steps != 0) {
		scrollDeltaRemainder -= steps;
		[self setDisplayedVolume:round([self displayedVolume]) + steps];
		[[self target] changeVolume:self];
		[self showToolTipForDuration:1.0];
	}

	if(theEvent.phase == NSEventPhaseEnded || theEvent.phase == NSEventPhaseCancelled) {
		scrollDeltaRemainder = 0;
	}
}

@end
