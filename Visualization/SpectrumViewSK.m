//
//  SpectrumViewSK.m
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import "SpectrumViewSK.h"

#import "NSView+Visibility.h"

#import <Metal/Metal.h>

#import <Accelerate/Accelerate.h>

#import "analyzer.h"

#import "Logging.h"

#define LOWER_BOUND -80

static void *kSpectrumViewSKContext = &kSpectrumViewSKContext;

extern NSString *CogPlaybackDidBeginNotificiation;
extern NSString *CogPlaybackDidPauseNotificiation;
extern NSString *CogPlaybackDidResumeNotificiation;
extern NSString *CogPlaybackDidStopNotificiation;

@implementation SpectrumViewSK {
	VisualizationController *visController;
	NSTimer *timer;
	BOOL paused;
	BOOL stopped;
	BOOL isSetup;
	BOOL isListening;
	BOOL bandsReset;
	BOOL cameraControlEnabled;
	BOOL observersAdded;
	BOOL isOccluded;

	NSColor *backgroundColor;
	ddb_analyzer_t _analyzer;
	ddb_analyzer_draw_data_t _draw_data;

	SCNVector3 cameraPosition2d;
	SCNVector3 cameraEulerAngles2d;
	SCNVector3 cameraPosition3d;
	SCNVector3 cameraEulerAngles3d;

	float visFFT[2048];

	UInt64 visSamplesLastPosted;
	double visLatencyOffset;
}

+ (SpectrumViewSK *_Nullable)createGuardWithFrame:(NSRect)frame {
	if (![NSUserDefaults.standardUserDefaults boolForKey:@"spectrumSceneKit"])
		return nil;

	do {
		if(@available(macOS 11.0, *)) {
			// macOS 11 and newer seems to be safe
			break;
		} else if(@available(macOS 10.15, *)) {
			// macOS 10.15.7 has a SceneKit bug with PBR noise
			return nil;
		} else {
			// macOS 10.12 through 10.14.x seem to be safe too
			break;
		}
	} while(0);

	return [[SpectrumViewSK alloc] initWithFrame:frame];
}

@synthesize isListening;

- (id)initWithFrame:(NSRect)frame {
	id<MTLDevice> device = MTLCreateSystemDefaultDevice();

	if(!device) return nil;

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
	if(self.isListening && (![self visibleInWindow] || isOccluded || paused || stopped)) {
		[self stopTimer];
		self.isListening = NO;
	} else if(!self.isListening && ([self visibleInWindow] && !isOccluded && !stopped && !paused)) {
		[self startTimer];
		self.isListening = YES;
	}
}

- (void)setOccluded:(BOOL)occluded {
	isOccluded = occluded;
	[self updateVisListening];
}

- (void)windowChangedOcclusionState:(NSNotification *)notification {
	if([notification object] == self.window) {
		BOOL curOccluded = !self.window;
		if(!curOccluded) {
			curOccluded = !(self.window.occlusionState & NSWindowOcclusionStateVisible);
		}
		if(curOccluded != isOccluded) {
			[self setOccluded:curOccluded];
		}
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

	isSetup = YES;
}

- (void)addObservers {
	if(!observersAdded) {
		NSUserDefaultsController *sharedUserDefaultsController = [NSUserDefaultsController sharedUserDefaultsController];
		[sharedUserDefaultsController addObserver:self forKeyPath:@"values.spectrumProjectionMode" options:0 context:kSpectrumViewSKContext];
		[sharedUserDefaultsController addObserver:self forKeyPath:@"values.spectrumBarColor" options:0 context:kSpectrumViewSKContext];
		[sharedUserDefaultsController addObserver:self forKeyPath:@"values.spectrumDotColor" options:0 context:kSpectrumViewSKContext];

		[self addObserver:self forKeyPath:@"self.window.visible" options:0 context:kSpectrumViewSKContext];

		NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
		[defaultCenter addObserver:self
		                  selector:@selector(playbackDidBegin:)
		                      name:CogPlaybackDidBeginNotificiation
		                    object:nil];
		[defaultCenter addObserver:self
		                  selector:@selector(playbackDidPause:)
		                      name:CogPlaybackDidPauseNotificiation
		                    object:nil];
		[defaultCenter addObserver:self
		                  selector:@selector(playbackDidResume:)
		                      name:CogPlaybackDidResumeNotificiation
		                    object:nil];
		[defaultCenter addObserver:self
		                  selector:@selector(playbackDidStop:)
		                      name:CogPlaybackDidStopNotificiation
		                    object:nil];

		[defaultCenter addObserver:self
		                  selector:@selector(windowChangedOcclusionState:)
		                      name:NSWindowDidChangeOcclusionStateNotification
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
		NSUserDefaultsController *sharedUserDefaultsController = [NSUserDefaultsController sharedUserDefaultsController];
		[sharedUserDefaultsController removeObserver:self forKeyPath:@"values.spectrumProjectionMode" context:kSpectrumViewSKContext];
		[sharedUserDefaultsController removeObserver:self forKeyPath:@"values.spectrumBarColor" context:kSpectrumViewSKContext];
		[sharedUserDefaultsController removeObserver:self forKeyPath:@"values.spectrumDotColor" context:kSpectrumViewSKContext];

		[self removeObserver:self forKeyPath:@"self.window.visible" context:kSpectrumViewSKContext];

		NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
		[defaultCenter removeObserver:self
		                         name:CogPlaybackDidBeginNotificiation
		                       object:nil];
		[defaultCenter removeObserver:self
		                         name:CogPlaybackDidPauseNotificiation
		                       object:nil];
		[defaultCenter removeObserver:self
		                         name:CogPlaybackDidResumeNotificiation
		                       object:nil];
		[defaultCenter removeObserver:self
		                         name:CogPlaybackDidStopNotificiation
		                       object:nil];

		[defaultCenter removeObserver:self
		                         name:NSWindowDidChangeOcclusionStateNotification
		                       object:nil];

		observersAdded = NO;
	}
}

- (void)repaint {
	if(!isSetup) return;

	[self updateVisListening];

	if(stopped) {
		[self drawBaseBands];
		return;
	}

	UInt64 samplesPosted = [self->visController samplesPosted];
	if (samplesPosted != visSamplesLastPosted) {
		visSamplesLastPosted = samplesPosted;
		visLatencyOffset = 0.0;
	}

	[self->visController copyVisPCM:nil visFFT:&visFFT[0] latencyOffset:visLatencyOffset];

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
	visLatencyOffset -= 1.0 / 60.0;
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
	bandsReset = NO;
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
		{
			const int barBase = i * barStep;
			const int barEnd = (barBase + maxBars) <= _draw_data.bar_count ? (barBase + maxBars) : _draw_data.bar_count;
			{
				const int stride = sizeof(ddb_analyzer_draw_bar_t) / sizeof(Float32);
				const int barCount = barEnd - barBase + 1;
				vDSP_maxv(&bar[barBase].bar_height, stride, &maxValue, barCount);
				vDSP_maxv(&bar[barBase].peak_ypos, stride, &maxMax, barCount);
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
