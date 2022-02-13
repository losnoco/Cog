//
//  SpectrumView.m
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import "SpectrumView.h"

extern NSString *CogPlaybackDidBeginNotficiation;
extern NSString *CogPlaybackDidPauseNotficiation;
extern NSString *CogPlaybackDidResumeNotficiation;
extern NSString *CogPlaybackDidStopNotficiation;

@implementation SpectrumView

- (void)awakeFromNib {
	visController = [NSClassFromString(@"VisualizationController") sharedController];
	timer = nil;
	theImage = [NSImage imageWithSize:NSMakeSize(64, 26)
	                          flipped:NO
	                   drawingHandler:^BOOL(NSRect dstRect) {
		                   NSColor *backColor = [NSColor textBackgroundColor];
		                   [backColor drawSwatchInRect:dstRect];
		                   return YES;
	                   }];

	stopped = YES;

	[self setImage:theImage];
	[self setImageScaling:NSImageScaleAxesIndependently];

	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(colorsDidChange:)
	                                             name:NSSystemColorsDidChangeNotification
	                                           object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(playbackDidBegin:)
	                                             name:CogPlaybackDidBeginNotficiation
	                                           object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(playbackDidPause:)
	                                             name:CogPlaybackDidPauseNotficiation
	                                           object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(playbackDidResume:)
	                                             name:CogPlaybackDidResumeNotficiation
	                                           object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
	                                         selector:@selector(playbackDidStop:)
	                                             name:CogPlaybackDidStopNotficiation
	                                           object:nil];
}

- (void)repaint {
	{
		[theImage lockFocus];

		NSColor *backColor = [NSColor textBackgroundColor];
		[backColor drawSwatchInRect:NSMakeRect(0, 0, 64, 26)];

		NSBezierPath *bezierPath = [[NSBezierPath alloc] init];

		float visAudio[512], visFFT[256];

		if(!self->stopped) {
			[self->visController copyVisPCM:&visAudio[0] visFFT:&visFFT[0]];
		} else {
			memset(visFFT, 0, sizeof(visFFT));
		}

		for(int i = 0; i < 60; ++i) {
			CGFloat y = MAX(MIN(visFFT[i], 0.25), 0.0) * 4.0 * 22.0 + 2.0;
			[bezierPath moveToPoint:NSMakePoint(2 + i, 2)];
			[bezierPath lineToPoint:NSMakePoint(2 + i, y)];
		}

		NSColor *lineColor = [NSColor textColor];
		[lineColor setStroke];

		[bezierPath stroke];

		[theImage unlockFocus];
	}

	[self setNeedsDisplay];
}

- (void)startTimer {
	[self stopTimer];
	timer = [NSTimer timerWithTimeInterval:0.02
	                                target:self
	                              selector:@selector(timerRun:)
	                              userInfo:nil
	                               repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
}

- (void)stopTimer {
	[timer invalidate];
	timer = nil;
}

- (void)timerRun:(NSTimer *)timer {
	[self repaint];
}

- (void)colorsDidChange:(NSNotification *)notification {
	[self repaint];
}

- (void)playbackDidBegin:(NSNotification *)notification {
	stopped = NO;
	[self startTimer];
}

- (void)playbackDidPause:(NSNotification *)notification {
	stopped = NO;
	[self stopTimer];
}

- (void)playbackDidResume:(NSNotification *)notification {
	stopped = NO;
	[self startTimer];
}

- (void)playbackDidStop:(NSNotification *)notification {
	[self stopTimer];
	stopped = YES;
	[self repaint];
}

- (void)drawRect:(NSRect)dirtyRect {
	[super drawRect:dirtyRect];
}

@end
