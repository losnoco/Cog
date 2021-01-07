//
//  VolumeSlider.m
//  Cog
//
//  Created by Vincent Spader on 2/8/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "VolumeSlider.h"
#import "PlaybackController.h"
#import "CogAudio/Helper.h"

@implementation VolumeSlider {
    NSTimer *currentTimer;
    BOOL wasInsideSnapRange;
}

- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    return self;
}

- (id)initWithCoder:(NSCoder *)coder
{
    self = [super initWithCoder:coder];
    return self;
}

- (void) awakeFromNib {
    wasInsideSnapRange = NO;
    textView = [[NSText alloc] init];
    [textView setFrame:NSMakeRect(0, 0, 50, 20)];
    textView.drawsBackground = NO;
    textView.editable = NO;
    textView.alignment = NSTextAlignmentCenter;

    NSViewController * viewController = [[NSViewController alloc] init];
    viewController.view = textView;

    popover = [[NSPopover alloc] init];
    popover.contentViewController = viewController;
    // Don't hide the popover automatically.
    popover.behavior = NSPopoverBehaviorTransient;
    popover.animates = NO;
    [popover setContentSize:textView.bounds.size];
}

- (void)updateToolTip
{
    double value = [self doubleValue];
    double volume = linearToLogarithmic(value);

    NSString *text = [NSString stringWithFormat:@"%0.lf%%", volume];

    [textView setString:text];
}

- (void)showToolTip
{
    [self updateToolTip];

    double progress = (self.maxValue - [self doubleValue]) / (self.maxValue - self.minValue);
    CGFloat width = self.knobThickness - 1;
    // Show tooltip to the left of the Slider Knob
    CGFloat height = self.knobThickness / 2.f + (self.bounds.size.height - self.knobThickness) * progress - 1;
    
    [popover showRelativeToRect:NSMakeRect(width, height, 2, 2) ofView:self preferredEdge:NSRectEdgeMaxX];
    [self.window.parentWindow makeKeyWindow];
}

- (void)showToolTipForDuration:(NSTimeInterval)duration
{
    [self showToolTip];

    [self hideToolTipAfterDelay:duration];
}

- (void)showToolTipForView:(NSView *)view closeAfter:(NSTimeInterval)duration
{
    [self updateToolTip];

    [popover showRelativeToRect:view.bounds ofView:view preferredEdge:NSRectEdgeMaxY];

    [self hideToolTipAfterDelay:duration];
}

- (void)hideToolTip
{
    [popover close];
}

- (void) hideToolTipAfterDelay:(NSTimeInterval)duration
{
    if (currentTimer)
    {
        [currentTimer invalidate];
        currentTimer = nil;
    }

    if (duration > 0.0) {
        currentTimer = [NSTimer scheduledTimerWithTimeInterval:duration
                                                        target:self
                                                      selector:@selector(hideToolTip)
                                                      userInfo:nil
                                                       repeats:NO];
        [[NSRunLoop mainRunLoop] addTimer:currentTimer forMode:NSRunLoopCommonModes];
    }
}

- (BOOL)sendAction:(SEL)theAction to:(id)theTarget
{
    // Snap to 100% if value is close
    double snapTarget = logarithmicToLinear(100.0);
    double snapProgress = ([self doubleValue] - snapTarget) / (self.maxValue - self.minValue);

    if (fabs(snapProgress) < 0.005)
    {
        [self setDoubleValue:snapTarget];
        if (!wasInsideSnapRange)
        {
            if (@available(macOS 10.11, *))
            {
                [[NSHapticFeedbackManager defaultPerformer] performFeedbackPattern:NSHapticFeedbackPatternGeneric
                                                                   performanceTime:NSHapticFeedbackPerformanceTimeDefault];
            }
        }
        wasInsideSnapRange = YES;
    }
    else
    {
        wasInsideSnapRange = NO;
    }

    [self showToolTip];

    return [super sendAction:theAction to:theTarget];
}

- (void)scrollWheel:(NSEvent *)theEvent
{
    double change = [theEvent deltaY];

    [self setDoubleValue:[self doubleValue] + change];

    [[self target] changeVolume:self];

    [self showToolTipForDuration:1.0];
}

@end
