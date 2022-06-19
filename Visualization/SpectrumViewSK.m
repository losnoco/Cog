//
//  SpectrumViewSK.m
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import "SpectrumViewSK.h"

#import "NSView+Visibility.h"

#import <Metal/Metal.h>

#import "analyzer.h"

#import "Logging.h"

@import Firebase;

#define LOWER_BOUND -80

static void *kSpectrumViewSKContext = &kSpectrumViewSKContext;

extern NSString *CogPlaybackDidBeginNotficiation;
extern NSString *CogPlaybackDidPauseNotficiation;
extern NSString *CogPlaybackDidResumeNotficiation;
extern NSString *CogPlaybackDidStopNotficiation;

@interface SpectrumViewSK () {
	VisualizationController *visController;
	NSTimer *timer;
	BOOL paused;
	BOOL stopped;
	BOOL isListening;
	BOOL isWorking;
	BOOL bandsReset;
	BOOL cameraControlEnabled;
	BOOL observersAdded;

	NSColor *backgroundColor;
	ddb_analyzer_t _analyzer;
	ddb_analyzer_draw_data_t _draw_data;

	SCNVector3 cameraPosition2d;
	SCNVector3 cameraEulerAngles2d;
	SCNVector3 cameraPosition3d;
	SCNVector3 cameraEulerAngles3d;
}
@end

@implementation SpectrumViewSK

+ (SpectrumViewSK *)createGuardWithFrame:(NSRect)frame {
	if (![[NSUserDefaults standardUserDefaults] boolForKey:@"sceneKitCrashed"]) {
		return [[SpectrumViewSK alloc] initWithFrame:frame];
	}
	return nil;
}

@synthesize isListening;
@synthesize isWorking;

- (id)initWithFrame:(NSRect)frame {
	id<MTLDevice> device = MTLCreateSystemDefaultDevice();

	if(!device) return nil;

	DLog(@"SceneKit visualizer accessed device named: %@", device.name);
	[[FIRCrashlytics crashlytics] logWithFormat:@"SceneKit visualizer accessed device named: %@", device.name];

	if([device.name containsString:@"AMD"]) {
		if([device.name containsString:@"FirePro D"] ||
		   [device.name containsString:@" M2"] ||
		   [device.name containsString:@" M3"] ||
		   [device.name containsString:@" 460 "] ||
		   [device.name containsString:@" 470 "] ||
		   [device.name containsString:@" 480 "] ||
		   [device.name containsString:@" 550 "] ||
		   [device.name containsString:@" 560 "] ||
		   [device.name containsString:@" 570 "] ||
		   [device.name containsString:@" 580 "] ||
		   [device.name containsString:@" 590 "])
			return nil;
	}

	NSDictionary *sceneOptions = @{
		SCNPreferredRenderingAPIKey: @(SCNRenderingAPIMetal),
		SCNPreferredDeviceKey: device,
		SCNPreferLowPowerDeviceKey: @(NO)
	};

	self = [super initWithFrame:frame options:sceneOptions];
	if(self) {
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

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey, id> *)change
                       context:(void *)context {
	if(context == kSpectrumViewSKContext) {
		if([keyPath isEqualToString:@"self.window.visible"]) {
			[self updateVisListening];
		} else {
			[self updateControls];
		}
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)updateControls {
	BOOL projectionMode = cameraControlEnabled ? NO : [[NSUserDefaults standardUserDefaults] boolForKey:@"spectrumProjectionMode"];
	SCNNode *rootNode = [[self scene] rootNode];
	SCNNode *cameraNode = [rootNode childNodeWithName:@"camera" recursively:NO];
	SCNCamera *camera = [cameraNode camera];
	if (projectionMode) {
		cameraNode.eulerAngles = cameraEulerAngles2d;
		cameraNode.position = cameraPosition2d;
		camera.usesOrthographicProjection = YES;
		camera.orthographicScale = 0.6;
	} else {
		cameraNode.eulerAngles = cameraEulerAngles3d;
		cameraNode.position = cameraPosition3d;
		camera.usesOrthographicProjection = NO;
		camera.orthographicScale = 1.0;
	}

	NSValueTransformer *colorToValueTransformer = [NSValueTransformer valueTransformerForName:@"ColorToValueTransformer"];

	NSColor *barColor = [colorToValueTransformer transformedValue:[[NSUserDefaults standardUserDefaults] dataForKey:@"spectrumBarColor"]];
	NSColor *dotColor = [colorToValueTransformer transformedValue:[[NSUserDefaults standardUserDefaults] dataForKey:@"spectrumDotColor"]];

	{
		SCNNode *barNode = [rootNode childNodeWithName:@"cylinder0" recursively:NO];
		SCNGeometry *geometry = [barNode geometry];
		NSArray<SCNMaterial *> *materials = [geometry materials];
		SCNMaterial *material = materials[0];
		material.diffuse.contents = barColor;
		material.emission.contents = barColor;
	}

	{
		SCNNode *dotNode = [rootNode childNodeWithName:@"sphere0" recursively:NO];
		SCNGeometry *geometry = [dotNode geometry];
		NSArray<SCNMaterial *> *materials = [geometry materials];
		SCNMaterial *material = materials[0];
		material.diffuse.contents = dotColor;
		material.emission.contents = dotColor;
	}
}

- (void)enableCameraControl {
	[self setAllowsCameraControl:YES];
	cameraControlEnabled = YES;
	[self updateControls];
}

- (void)setup {
	visController = [NSClassFromString(@"VisualizationController") sharedController];
	timer = nil;
	stopped = YES;
	paused = NO;
	isListening = NO;
	cameraControlEnabled = NO;
	isWorking = NO;

	[self setBackgroundColor:[NSColor clearColor]];

	SCNScene *theScene = [SCNScene sceneNamed:@"Scenes.scnassets/Spectrum.scn"];
	[self setScene:theScene];

	[self setAntialiasingMode:SCNAntialiasingModeMultisampling8X];

	SCNNode *rootNode = [[self scene] rootNode];
	SCNNode *cameraNode = [rootNode childNodeWithName:@"camera" recursively:NO];
	cameraPosition2d = SCNVector3Make(0.0, 0.5, 1.0);
	cameraEulerAngles2d = SCNVector3Zero;
	// Save initial camera position from SceneKit file.
	cameraPosition3d = cameraNode.position;
	cameraEulerAngles3d = cameraNode.eulerAngles;
	[self updateControls];

	bandsReset = NO;
	[self drawBaseBands];

	//[self colorsDidChange:nil];

	BOOL freqMode = [[NSUserDefaults standardUserDefaults] boolForKey:@"spectrumFreqMode"];

	ddb_analyzer_init(&_analyzer);
	_analyzer.db_lower_bound = LOWER_BOUND;
	_analyzer.min_freq = 10;
	_analyzer.max_freq = 22000;
	_analyzer.peak_hold = 10;
	_analyzer.view_width = 1000;
	_analyzer.fractional_bars = 1;
	_analyzer.octave_bars_step = 2;
	_analyzer.max_of_stereo_data = 1;
	_analyzer.freq_is_log = 0;
	_analyzer.mode = freqMode ? DDB_ANALYZER_MODE_FREQUENCIES : DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;

	[self addObservers];
}

- (void)addObservers {
	if(!observersAdded) {
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.spectrumProjectionMode" options:0 context:kSpectrumViewSKContext];
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.spectrumBarColor" options:0 context:kSpectrumViewSKContext];
		[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.spectrumDotColor" options:0 context:kSpectrumViewSKContext];

		[self addObserver:self forKeyPath:@"self.window.visible" options:0 context:kSpectrumViewSKContext];

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
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.spectrumProjectionMode" context:kSpectrumViewSKContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.spectrumBarColor" context:kSpectrumViewSKContext];
		[[NSUserDefaultsController sharedUserDefaultsController] removeObserver:self forKeyPath:@"values.spectrumDotColor" context:kSpectrumViewSKContext];

		[self removeObserver:self forKeyPath:@"self.window.visible" context:kSpectrumViewSKContext];

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
	[self updateVisListening];
	
	if(!isWorking) {
		isWorking = YES;
		[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"sceneKitCrashed"];
	}

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
		position.y = 0.072;
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
		position.y = maxMax + 0.072;
		dotNode.position = position;
	}

	bandsReset = NO;
}

- (void)drawAnalyzer {
	[self drawAnalyzerOctaveBands];
}

- (void)mouseDown:(NSEvent *)event {
	if(cameraControlEnabled) return;

	BOOL freqMode = ![[NSUserDefaults standardUserDefaults] boolForKey:@"spectrumFreqMode"];
	[[NSUserDefaults standardUserDefaults] setBool:freqMode forKey:@"spectrumFreqMode"];

	_analyzer.mode = freqMode ? DDB_ANALYZER_MODE_FREQUENCIES : DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS;
	_analyzer.mode_did_change = 1;

	[self repaint];
}

@end
