//
//  SpectrumView.m
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import "SpectrumView.h"

#import <Accelerate/Accelerate.h>

extern NSString *CogPlaybackDidBeginNotficiation;
extern NSString *CogPlaybackDidPauseNotficiation;
extern NSString *CogPlaybackDidResumeNotficiation;
extern NSString *CogPlaybackDidStopNotficiation;

@implementation SpectrumView

@synthesize isListening;

- (id)initWithFrame:(NSRect)frame {
	self = [super initWithFrame:frame];
	if(self) {
		[self setup];
	}
	return self;
}

- (void)updateVisListening {
	if(self.isListening && (paused || stopped)) {
		[self stopTimer];
		self.isListening = NO;
	} else if(!self.isListening && (!stopped && !paused)) {
		[self startTimer];
		self.isListening = YES;
	}
}

- (void)setup {
	visController = [NSClassFromString(@"VisualizationController") sharedController];
	timer = nil;
	stopped = YES;
	paused = NO;
	isListening = NO;

	[self colorsDidChange:nil];

	vDSP_vclr(&FFTMax[0], 1, 256);

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

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self
	                                                name:NSSystemColorsDidChangeNotification
	                                              object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self
	                                                name:CogPlaybackDidBeginNotficiation
	                                              object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self
	                                                name:CogPlaybackDidPauseNotficiation
	                                              object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self
	                                                name:CogPlaybackDidResumeNotficiation
	                                              object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self
	                                                name:CogPlaybackDidStopNotficiation
	                                              object:nil];
}

- (void)repaint {
	self.needsDisplay = YES;
}

- (void)startTimer {
	[self stopTimer];
	timer = [NSTimer timerWithTimeInterval:1.0 / 60.0
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
	backgroundColor = [NSColor textBackgroundColor];

	if(@available(macOS 10.14, *)) {
		baseColor = [NSColor textColor];
		peakColor = [NSColor controlAccentColor];
		peakColor = [peakColor colorWithAlphaComponent:0.7];
	} else {
		peakColor = [NSColor textColor];
		baseColor = [peakColor colorWithAlphaComponent:0.6];
	}

	[self repaint];
}

- (void)playbackDidBegin:(NSNotification *)notification {
	stopped = NO;
	paused = NO;
	[self updateVisListening];
}

- (void)playbackDidPause:(NSNotification *)notification {
	stopped = NO;
	paused = YES;
	[self updateVisListening];
}

- (void)playbackDidResume:(NSNotification *)notification {
	stopped = NO;
	paused = NO;
	[self updateVisListening];
}

- (void)playbackDidStop:(NSNotification *)notification {
	stopped = YES;
	paused = NO;
	[self updateVisListening];
	vDSP_vclr(&FFTMax[0], 1, 256);
	[self repaint];
}

- (void)drawRect:(NSRect)dirtyRect {
	[super drawRect:dirtyRect];

	[self updateVisListening];

	[backgroundColor setFill];
	NSRectFill(dirtyRect);

	float visAudio[512], visFFT[256];

	if(!self->stopped) {
		[self->visController copyVisPCM:&visAudio[0] visFFT:&visFFT[0]];
	} else {
		memset(visFFT, 0, sizeof(visFFT));
	}

	float scale = 0.95;
	vDSP_vsmul(&FFTMax[0], 1, &scale, &FFTMax[0], 1, 256);
	vDSP_vmax(&visFFT[0], 1, &FFTMax[0], 1, &FFTMax[0], 1, 256);

	CGContextRef context = NSGraphicsContext.currentContext.CGContext;

	for(int i = 0; i < 60; ++i) {
		CGFloat y = MAX(MIN(visFFT[i], 0.25), 0.0) * 4.0 * 22.0;
		CGContextMoveToPoint(context, 2.0 + i, 2.0);
		CGContextAddLineToPoint(context, 2.0 + i, 2.0 + y);
	}
	CGContextSetStrokeColorWithColor(context, baseColor.CGColor);
	CGContextStrokePath(context);

	for(int i = 0; i < 60; ++i) {
		CGFloat y = MAX(MIN(FFTMax[i], 0.25), 0.0) * 4.0 * 22.0;
		CGContextMoveToPoint(context, 2.0 + i, 1.5 + y);
		CGContextAddLineToPoint(context, 2.0 + i, 2.5 + y);
	}
	CGContextSetStrokeColorWithColor(context, peakColor.CGColor);
	CGContextStrokePath(context);
}

@end
