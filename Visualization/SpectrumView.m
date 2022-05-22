//
//  SpectrumView.m
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import "SpectrumView.h"

#import "analyzer.h"

#define LOWER_BOUND -80

extern NSString *CogPlaybackDidBeginNotficiation;
extern NSString *CogPlaybackDidPauseNotficiation;
extern NSString *CogPlaybackDidResumeNotficiation;
extern NSString *CogPlaybackDidStopNotficiation;

@interface SpectrumView () {
	VisualizationController *visController;
	NSTimer *timer;
	BOOL paused;
	BOOL stopped;
	BOOL isListening;
	BOOL bandsReset;

	NSColor *backgroundColor;
	ddb_analyzer_t _analyzer;
	ddb_analyzer_draw_data_t _draw_data;
}
@end

@implementation SpectrumView

@synthesize isListening;

- (id)initWithFrame:(NSRect)frame {
	NSDictionary *sceneOptions = @{
		SCNPreferredRenderingAPIKey: @(SCNRenderingAPIMetal),
		SCNPreferLowPowerDeviceKey: @(YES)
	};

	self = [super initWithFrame:frame options:sceneOptions];
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

	[self setBackgroundColor:[NSColor clearColor]];

	SCNScene *theScene = [SCNScene sceneNamed:@"Scenes.scnassets/Spectrum.scn"];
	[self setScene:theScene];

	bandsReset = NO;
	[self drawBaseBands];

	//[self colorsDidChange:nil];

	BOOL freqMode = [[NSUserDefaults standardUserDefaults] boolForKey:@"spectrumFreqMode"];

	ddb_analyzer_init(&_analyzer);
	_analyzer.db_lower_bound = LOWER_BOUND;
	_analyzer.min_freq = 10;
	_analyzer.max_freq = 22000;
	_analyzer.peak_hold = 10;
	_analyzer.view_width = 11;
	_analyzer.fractional_bars = 1;
	_analyzer.octave_bars_step = 2;
	_analyzer.max_of_stereo_data = 1;
	_analyzer.freq_is_log = 0;
	_analyzer.mode = freqMode ? DDB_ANALYZER_MODE_FREQUENCIES : DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;

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
	ddb_analyzer_dealloc(&_analyzer);
	ddb_analyzer_draw_data_dealloc(&_draw_data);

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
	[self updateVisListening];

	if(stopped) {
		[self drawBaseBands];
		return;
	}

	float visAudio[4096], visFFT[2048];

	[self->visController copyVisPCM:&visAudio[0] visFFT:&visFFT[0]];

	ddb_analyzer_process(&_analyzer, [self->visController readSampleRate] / 2.0, 1, visFFT, 2048);
	ddb_analyzer_tick(&_analyzer);
	ddb_analyzer_get_draw_data(&_analyzer, 11.0, 1.0, &_draw_data);

	[self drawAnalyzer];
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

- (void)drawBaseBands {
	if(bandsReset) return;

	SCNScene *scene = [self scene];
	SCNNode *rootNode = [scene rootNode];
	NSArray<SCNNode *> *nodes = [rootNode childNodes];

	for(int i = 0; i < 11; ++i) {
		SCNNode *node = nodes[i + 1];
		SCNNode *dotNode = nodes[i + 1 + 11];
		SCNVector3 position = node.position;
		position.y = 0.0;
		node.scale = SCNVector3Make(1.0, 0.0, 1.0);
		node.position = position;

		position = dotNode.position;
		position.y = 0;
		dotNode.position = position;
	}

	bandsReset = YES;
}

- (void)drawAnalyzerOctaveBands {
	const int maxBars = (int)(ceilf((float)(_draw_data.bar_count) / 11.0));
	const int barStep = (int)(floorf((float)(_draw_data.bar_count) / 11.0));

	ddb_analyzer_draw_bar_t *bar = _draw_data.bars;

	SCNScene *scene = [self scene];
	SCNNode *rootNode = [scene rootNode];
	NSArray<SCNNode *> *nodes = [rootNode childNodes];

	for(int i = 0; i < 11; ++i) {
		float maxValue = 0.0;
		float maxMax = 0.0;
		for(int j = 0; j < maxBars; ++j) {
			const int barBase = i * barStep;
			const int barIndex = barBase + j;
			if(barIndex < _draw_data.bar_count) {
				if(bar[barIndex].bar_height > maxValue) {
					maxValue = bar[barIndex].bar_height;
				}
				if(bar[barIndex].peak_ypos > maxMax) {
					maxMax = bar[barIndex].peak_ypos;
				}
			}
		}
		SCNNode *node = nodes[i + 1];
		SCNNode *dotNode = nodes[i + 1 + 11];
		SCNVector3 position = node.position;
		position.y = maxValue * 0.5;
		node.scale = SCNVector3Make(1.0, maxValue, 1.0);
		node.position = position;

		position = dotNode.position;
		position.y = maxMax;
		dotNode.position = position;
	}

	bandsReset = NO;
}

- (void)drawAnalyzer {
	[self drawAnalyzerOctaveBands];
}

- (void)mouseDown:(NSEvent *)event {
	BOOL freqMode = ![[NSUserDefaults standardUserDefaults] boolForKey:@"spectrumFreqMode"];
	[[NSUserDefaults standardUserDefaults] setBool:freqMode forKey:@"spectrumFreqMode"];

	_analyzer.mode = freqMode ? DDB_ANALYZER_MODE_FREQUENCIES : DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
	_analyzer.mode_did_change = 1;

	[self repaint];
}

@end
