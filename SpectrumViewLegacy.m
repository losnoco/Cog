//
//  SpectrumViewLegacy.m
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import "SpectrumViewLegacy.h"

#import "NSView+Visibility.h"

#import "analyzer.h"

#define LOWER_BOUND -80

static void *kSpectrumViewLegacyContext = &kSpectrumViewLegacyContext;

extern NSString *CogPlaybackDidBeginNotficiation;
extern NSString *CogPlaybackDidPauseNotficiation;
extern NSString *CogPlaybackDidResumeNotficiation;
extern NSString *CogPlaybackDidStopNotficiation;

@interface SpectrumViewLegacy () {
	VisualizationController *visController;
	NSTimer *timer;
	float saLowerBound;
	BOOL paused;
	BOOL stopped;
	BOOL isListening;
	BOOL observersAdded;
	BOOL isFullView;

	NSRect initFrame;

	NSDictionary *textAttrs;
	NSColor *baseColor;
	NSColor *peakColor;
	NSColor *backgroundColor;
	NSColor *borderColor;
	ddb_analyzer_t _analyzer;
	ddb_analyzer_draw_data_t _draw_data;
}
@end

@implementation SpectrumViewLegacy

@synthesize isListening;

- (id)initWithFrame:(NSRect)frame {
	self = [super initWithFrame:frame];
	if(self) {
		initFrame = frame;
		[self setup];
	}
	return self;
}

- (void)updateVisListening {
	if(self.isListening && (![self visibleInWindow] || paused || stopped)) {
		[self stopTimer];
		self.isListening = NO;
	} else if(!self.isListening && ([self visibleInWindow] && !stopped && !paused)) {
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

	saLowerBound = LOWER_BOUND;

	[self colorsDidChange:nil];

	BOOL freqMode = [[NSUserDefaults standardUserDefaults] boolForKey:@"spectrumFreqMode"];

	ddb_analyzer_init(&_analyzer);
	_analyzer.db_lower_bound = LOWER_BOUND;
	_analyzer.min_freq = 10;
	_analyzer.max_freq = 22000;
	_analyzer.peak_hold = 10;
	_analyzer.view_width = 64;
	_analyzer.fractional_bars = 1;
	_analyzer.octave_bars_step = 2;
	_analyzer.max_of_stereo_data = 1;
	_analyzer.freq_is_log = 0;
	_analyzer.mode = freqMode ? DDB_ANALYZER_MODE_FREQUENCIES : DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;

	[self addObservers];
}

- (void)enableFullView {
	isFullView = YES;
	_analyzer.freq_is_log = 1;
	[self repaint];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id> *)change
                       context:(void *)context {
	if(context == kSpectrumViewLegacyContext) {
		if([keyPath isEqualToString:@"self.window.visible"]) {
			[self updateVisListening];
		} else {
			[self colorsDidChange:nil];
		}
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)addObservers {
	if(!observersAdded) {
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.spectrumBarColor" options:0 context:kSpectrumViewLegacyContext];
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.spectrumDotColor" options:0 context:kSpectrumViewLegacyContext];

		[self addObserver:self forKeyPath:@"self.window.visible" options:0 context:kSpectrumViewLegacyContext];

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
		observersAdded = YES;
	}
}

- (void)dealloc {
	ddb_analyzer_dealloc(&_analyzer);
	ddb_analyzer_draw_data_dealloc(&_draw_data);

	[self removeObservers];
}

- (void)removeObservers {
	if(observersAdded) {
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.spectrumBarColor" context:kSpectrumViewLegacyContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.spectrumDotColor" context:kSpectrumViewLegacyContext];

		[self removeObserver:self forKeyPath:@"self.window.visible" context:kSpectrumViewLegacyContext];

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
		observersAdded = NO;
	}
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
	backgroundColor = [backgroundColor colorWithAlphaComponent:0.0];
	borderColor = [NSColor systemGrayColor];

	NSValueTransformer *colorToValueTransformer = [NSValueTransformer valueTransformerForName:@"ColorToValueTransformer"];

	baseColor = [colorToValueTransformer transformedValue:[[NSUserDefaults standardUserDefaults] dataForKey:@"spectrumBarColor"]];
	peakColor = [colorToValueTransformer transformedValue:[[NSUserDefaults standardUserDefaults] dataForKey:@"spectrumDotColor"]];

	NSMutableParagraphStyle *paragraphStyle = [NSMutableParagraphStyle new];
	paragraphStyle.alignment = NSTextAlignmentLeft;

	textAttrs = @{
		NSFontAttributeName: [NSFont fontWithName:@"HelveticaNeue" size:10],
		NSParagraphStyleAttributeName: paragraphStyle,
		NSForegroundColorAttributeName: borderColor
	};

	[self repaint];
}

- (void)startPlayback {
	[self playbackDidBegin:nil];
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
	[self repaint];
}

- (void)drawSaGrid {
	CGContextRef context = NSGraphicsContext.currentContext.CGContext;

	// horz lines, db scale
	CGContextSetStrokeColorWithColor(context, borderColor.CGColor);
	CGFloat lower = -floor(saLowerBound);
	for(int db = 10; db < lower; db += 10) {
		CGFloat y = (CGFloat)(db / lower) * NSHeight(self.bounds);
		if(y >= NSHeight(self.bounds)) {
			break;
		}

		CGPoint points[] = {
			CGPointMake(0, y),
			CGPointMake(NSWidth(self.bounds) - 1, y)
		};
		CGContextAddLines(context, points, 2);
	}
	CGFloat dash[2] = { 1, 2 };
	CGContextSetLineDash(context, 0, dash, 2);
	CGContextStrokePath(context);
	CGContextSetLineDash(context, 0, NULL, 0);

	// db text
	for(int db = 10; db < lower; db += 10) {
		CGFloat y = (CGFloat)(db / lower) * NSHeight(self.bounds);
		if(y >= NSHeight(self.bounds)) {
			break;
		}

		NSString *string = [NSString stringWithFormat:@"%d dB", -db];
		[string drawAtPoint:NSMakePoint(0, NSHeight(self.bounds) - y - 12) withAttributes:textAttrs];
	}
}

- (void)drawFrequencyLabels {
	// octaves text
	for(int i = 0; i < _draw_data.label_freq_count; i++) {
		if(_draw_data.label_freq_positions < 0) {
			continue;
		}
		NSString *string = [NSString stringWithUTF8String:_draw_data.label_freq_texts[i]];
		CGFloat x = _draw_data.label_freq_positions[i];
		[string drawAtPoint:NSMakePoint(x, NSHeight(self.bounds) - 12) withAttributes:textAttrs];
	}
}

- (void)drawAnalyzerDescreteFrequencies {
	CGContextRef context = NSGraphicsContext.currentContext.CGContext;
	ddb_analyzer_draw_bar_t *bar = _draw_data.bars;
	for(int i = 0; i < _draw_data.bar_count; i++, bar++) {
		CGContextMoveToPoint(context, bar->xpos, 0);
		CGContextAddLineToPoint(context, bar->xpos, bar->bar_height);
	}
	CGContextSetStrokeColorWithColor(context, baseColor.CGColor);
	CGContextStrokePath(context);

	bar = _draw_data.bars;
	for(int i = 0; i < _draw_data.bar_count; i++, bar++) {
		CGContextMoveToPoint(context, bar->xpos - 0.5, bar->peak_ypos);
		CGContextAddLineToPoint(context, bar->xpos + 0.5, bar->peak_ypos);
	}
	CGContextSetStrokeColorWithColor(context, peakColor.CGColor);
	CGContextStrokePath(context);
}

- (void)drawAnalyzerOctaveBands {
	CGContextRef context = NSGraphicsContext.currentContext.CGContext;
	ddb_analyzer_draw_bar_t *bar = _draw_data.bars;
	for(int i = 0; i < _draw_data.bar_count; i++, bar++) {
		CGContextAddRect(context, CGRectMake(bar->xpos, 0, _draw_data.bar_width, bar->bar_height));
	}
	CGContextSetFillColorWithColor(context, baseColor.CGColor);
	CGContextFillPath(context);

	bar = _draw_data.bars;
	for(int i = 0; i < _draw_data.bar_count; i++, bar++) {
		CGContextAddRect(context, CGRectMake(bar->xpos, bar->peak_ypos, _draw_data.bar_width, 1.0));
	}
	CGContextSetFillColorWithColor(context, peakColor.CGColor);
	CGContextFillPath(context);
}

- (void)drawAnalyzer {
	if(_analyzer.mode == DDB_ANALYZER_MODE_FREQUENCIES) {
		[self drawAnalyzerDescreteFrequencies];
	} else {
		[self drawAnalyzerOctaveBands];
	}
}

- (void)drawRect:(NSRect)dirtyRect {
	[super drawRect:dirtyRect];

	[self updateVisListening];

	[backgroundColor setFill];
	NSRectFill(dirtyRect);

	if(!isFullView) {
		CGContextRef context = NSGraphicsContext.currentContext.CGContext;
		CGContextMoveToPoint(context, 0.0, 0.0);
		CGContextAddLineToPoint(context, initFrame.size.width, 0.0);
		CGContextAddLineToPoint(context, initFrame.size.width, initFrame.size.height);
		CGContextAddLineToPoint(context, 0.0, initFrame.size.height);
		CGContextAddLineToPoint(context, 0.0, 0.0);
		CGContextSetStrokeColorWithColor(context, borderColor.CGColor);
		CGContextStrokePath(context);
	}

	if(stopped) return;

	if(isFullView) {
		_analyzer.view_width = self.bounds.size.width;
	}

	float visAudio[4096], visFFT[2048];

	[self->visController copyVisPCM:&visAudio[0] visFFT:&visFFT[0]];

	ddb_analyzer_process(&_analyzer, [self->visController readSampleRate] / 2.0, 1, visFFT, 2048);
	ddb_analyzer_tick(&_analyzer);
	ddb_analyzer_get_draw_data(&_analyzer, self.bounds.size.width, self.bounds.size.height, &_draw_data);

	if(isFullView) {
		[self drawSaGrid];
		[self drawFrequencyLabels];
	}

	[self drawAnalyzer];
}

- (void)mouseDown:(NSEvent *)event {
	BOOL freqMode = ![[NSUserDefaults standardUserDefaults] boolForKey:@"spectrumFreqMode"];
	[[NSUserDefaults standardUserDefaults] setBool:freqMode forKey:@"spectrumFreqMode"];

	_analyzer.mode = freqMode ? DDB_ANALYZER_MODE_FREQUENCIES : DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
	_analyzer.mode_did_change = 1;

	[self repaint];
}

@end
