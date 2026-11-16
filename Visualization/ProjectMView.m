//
//  ProjectMView.m
//  Cog
//
//  Created by Kevin López Brante on 2026-11-15.
//

#define GL_SILENCE_DEPRECATION

// projectM draws only with OpenGL, which AppKit deprecates.
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#import "ProjectMView.h"

#import <CoreVideo/CoreVideo.h>
#import <OpenGL/gl3.h>

#import <CogAudio/VisualizationController.h>

#import <projectM-4/playlist.h>
#import <projectM-4/projectM.h>

#import "Logging.h"

// projectM keeps its own window of recent audio; a frame at 60 Hz needs
// 735, and a late one more.
static const int kMaxFramesPerDraw = 4096;

@interface ProjectMView ()
- (void)renderFrameAtTime:(const CVTimeStamp *)outputTime;
@end

static CVReturn displayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *now, const CVTimeStamp *outputTime,
                                    CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *context) {
	@autoreleasepool {
		[(__bridge ProjectMView *)context renderFrameAtTime:outputTime];
	}
	return kCVReturnSuccess;
}

static void presetSwitched(bool isHardCut, unsigned int index, void *userData);

@implementation ProjectMView {
	NSOpenGLContext *context;
	NSOpenGLPixelFormat *pixelFormat;
	CVDisplayLinkRef displayLink;

	// Touched only with the context locked.
	projectm_handle projectM;
	projectm_playlist_handle playlist;
	BOOL playlistNeedsLoading;
	NSMutableArray<void (^)(void)> *pendingCommands;
	size_t drawableWidth, drawableHeight;
	int64_t lastVideoTime;
	float *audio;
	// The preset showing, to keep it across a playlist reload.
	NSString *currentPresetPath;

	// Render thread only. The frame drawn at a lower resolution is copied
	// here before it is scaled up into the window.
	GLuint scaleFramebuffer, scaleTexture;
	size_t scaleWidth, scaleHeight;
	// When the next frame is due, in display link seconds, and the frame
	// rate projectM was last told.
	double nextFrameTime;
	int wantedFPS, appliedFPS;
}

+ (ProjectMView *)createWithFrame:(NSRect)frame {
	NSOpenGLPixelFormatAttribute attributes[] = {
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion4_1Core,
		NSOpenGLPFAAccelerated,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFADepthSize, 0,
		0
	};
	NSOpenGLPixelFormat *pixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
	if(!pixelFormat) {
		ALog(@"projectM: no OpenGL 4.1 Core pixel format");
		return nil;
	}
	NSOpenGLContext *context = [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil];
	if(!context) {
		ALog(@"projectM: no OpenGL 4.1 Core context");
		return nil;
	}
	return [[ProjectMView alloc] initWithFrame:frame pixelFormat:pixelFormat context:context];
}

- (instancetype)initWithFrame:(NSRect)frame pixelFormat:(NSOpenGLPixelFormat *)pixelFormat context:(NSOpenGLContext *)context {
	self = [super initWithFrame:frame];
	if(self) {
		self->pixelFormat = pixelFormat;
		self->context = context;
		GLint swapInterval = 1;
		[context setValues:&swapInterval forParameter:NSOpenGLContextParameterSwapInterval];
		pendingCommands = [NSMutableArray new];
		audio = calloc(kMaxFramesPerDraw * 2, sizeof(float));
		_presetFolders = @[];
		_textureFolders = @[];
		_shuffle = YES;
		_presetDuration = 30.0;
		_crossfadeDuration = 3.0;
		_hardCutThreshold = 2.0;
		_meshSize = NSMakeSize(32, 24);
		_excludedThemes = @[];
		_renderScale = 1;
		self.wantsBestResolutionOpenGLSurface = YES;
		self.wantsLayer = YES;
		[[NSNotificationCenter defaultCenter] addObserver:self
		                                         selector:@selector(surfaceNeedsUpdate:)
		                                             name:NSViewGlobalFrameDidChangeNotification
		                                           object:self];
	}
	return self;
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[self shutdown];
	free(audio);
}

#pragma mark - Surface

- (BOOL)isOpaque {
	return YES;
}

- (void)viewDidMoveToWindow {
	[super viewDidMoveToWindow];
	if(self.window) {
		[self attachContext];
	} else if(context) {
		// Docking moves the view between windows; the surface follows it.
		[self updateDisplayLink];
		CGLLockContext(context.CGLContextObj);
		[context clearDrawable];
		CGLUnlockContext(context.CGLContextObj);
		return;
	}
	[self updateDisplayLink];
}

- (void)viewDidChangeBackingProperties {
	[super viewDidChangeBackingProperties];
	[self updateDrawableSize];
}

- (void)setFrameSize:(NSSize)newSize {
	[super setFrameSize:newSize];
	[self updateDrawableSize];
}

- (void)surfaceNeedsUpdate:(NSNotification *)notification {
	[self updateDrawableSize];
}

- (void)attachContext {
	CGLLockContext(context.CGLContextObj);
	context.view = self;
	CGLUnlockContext(context.CGLContextObj);
	[self updateDrawableSize];
}

- (void)updateDrawableSize {
	// NSView's initializer may size the view before the context exists.
	if(!context) return;
	NSSize size = [self convertRectToBacking:self.bounds].size;
	CGLLockContext(context.CGLContextObj);
	[context update];
	drawableWidth = (size_t)MAX(size.width, 1);
	drawableHeight = (size_t)MAX(size.height, 1);
	CGLUnlockContext(context.CGLContextObj);
	if(!_isRunning) {
		[self drawOnce];
	}
}

- (void)drawRect:(NSRect)dirtyRect {
	// The display link draws; a resize while frozen redraws the last state.
	if(!_isRunning) {
		[self drawOnce];
	}
}

#pragma mark - Running

- (void)setIsRunning:(BOOL)isRunning {
	if(_isRunning == isRunning) return;
	_isRunning = isRunning;
	if(isRunning) {
		[[VisualizationController sharedController] addStereoConsumer];
	} else {
		[[VisualizationController sharedController] removeStereoConsumer];
	}
	[self updateDisplayLink];
}

- (void)updateDisplayLink {
	BOOL wanted = _isRunning && self.window != nil;
	if(wanted && !displayLink) {
		if(CVDisplayLinkCreateWithActiveCGDisplays(&displayLink) != kCVReturnSuccess) {
			displayLink = NULL;
			return;
		}
		CVDisplayLinkSetOutputCallback(displayLink, displayLinkCallback, (__bridge void *)self);
		CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(displayLink, context.CGLContextObj, pixelFormat.CGLPixelFormatObj);
		lastVideoTime = 0;
		nextFrameTime = 0;
		CVDisplayLinkStart(displayLink);
	} else if(!wanted && displayLink) {
		// Stop waits for a callback in flight, which takes the context lock.
		CVDisplayLinkStop(displayLink);
		CVDisplayLinkRelease(displayLink);
		displayLink = NULL;
	}
}

- (void)shutdown {
	self.isRunning = NO;
	[self updateDisplayLink];
	CGLLockContext(context.CGLContextObj);
	[context makeCurrentContext];
	[self deleteScaleBuffer];
	appliedFPS = 0;
	if(playlist) {
		projectm_playlist_destroy(playlist);
		playlist = NULL;
	}
	if(projectM) {
		projectm_destroy(projectM);
		projectM = NULL;
	}
	[NSOpenGLContext clearCurrentContext];
	CGLUnlockContext(context.CGLContextObj);
}

#pragma mark - Drawing (display link thread, or main while frozen)

- (void)drawOnce {
	if(!self.window || !context.view) return;
	[self renderFrameAtTime:NULL];
}

- (BOOL)makeProjectM {
	projectM = projectm_create();
	if(!projectM) {
		ALog(@"projectM: projectm_create failed");
		return NO;
	}
	projectm_set_fps(projectM, 60);
	projectm_set_preset_duration(projectM, _presetDuration);
	projectm_set_preset_locked(projectM, _presetLocked);
	projectm_set_soft_cut_duration(projectM, _crossfadeDuration);
	projectm_set_hard_cut_enabled(projectM, _hardCutsEnabled);
	projectm_set_hard_cut_sensitivity(projectM, (float)_hardCutThreshold);
	projectm_set_mesh_size(projectM, (size_t)_meshSize.width, (size_t)_meshSize.height);
	projectm_set_aspect_correction(projectM, true);
	playlist = projectm_playlist_create(projectM);
	projectm_playlist_set_preset_switched_event_callback(playlist, presetSwitched, (__bridge void *)self);
	playlistNeedsLoading = YES;
	return YES;
}

- (void)loadPlaylist {
	playlistNeedsLoading = NO;
	NSMutableArray<NSString *> *texturePaths = [NSMutableArray new];
	for(NSURL *url in _textureFolders) {
		[texturePaths addObject:url.path];
	}
	const char *paths[MAX(texturePaths.count, 1)];
	for(NSUInteger i = 0; i < texturePaths.count; i++) {
		paths[i] = texturePaths[i].fileSystemRepresentation;
	}
	projectm_set_texture_search_paths(projectM, paths, texturePaths.count);

	// A theme is a top-level folder of a preset folder, so these match it
	// at any depth; the filter only applies to presets added after it.
	NSMutableArray<NSString *> *filters = [NSMutableArray new];
	for(NSString *theme in _excludedThemes) {
		[filters addObject:[NSString stringWithFormat:@"-**/%@/**", theme]];
	}
	const char *filterList[MAX(filters.count, 1)];
	for(NSUInteger i = 0; i < filters.count; i++) {
		filterList[i] = filters[i].UTF8String;
	}
	projectm_playlist_set_filter(playlist, filterList, filters.count);

	projectm_playlist_clear(playlist);
	for(NSURL *url in _presetFolders) {
		projectm_playlist_add_path(playlist, url.fileSystemRepresentation, true, false);
	}
	// By folder, then name, so the list reads as the folders do.
	projectm_playlist_sort(playlist, 0, projectm_playlist_size(playlist), SORT_PREDICATE_FULL_PATH, SORT_ORDER_ASCENDING);
	projectm_playlist_set_shuffle(playlist, _shuffle);
	uint32_t size = projectm_playlist_size(playlist);
	if(size == 0) return;

	// The preset showing before the reload, or the one last seen.
	NSString *wanted = currentPresetPath ?: _startingPresetPath;
	if(wanted) {
		char **items = projectm_playlist_items(playlist, 0, size);
		for(uint32_t i = 0; items && items[i]; i++) {
			if(strcmp(items[i], wanted.fileSystemRepresentation) == 0) {
				projectm_playlist_free_string_array(items);
				projectm_playlist_set_position(playlist, i, true);
				return;
			}
		}
		if(items) projectm_playlist_free_string_array(items);
	}
	projectm_playlist_play_next(playlist, true);
}

/// Whether a display refresh at `outputTime` should draw, at the frame rate
/// asked for: the frame due soonest is drawn at the refresh nearest it.
- (BOOL)isFrameDueAt:(const CVTimeStamp *)outputTime {
	if(!outputTime || !outputTime->videoTimeScale) return YES;
	double now = (double)outputTime->videoTime / outputTime->videoTimeScale;
	double refresh = outputTime->videoRefreshPeriod ? (double)outputTime->videoRefreshPeriod / outputTime->videoTimeScale : 1.0 / 60.0;
	double limit = _frameRateLimit;
	// Presets time some motion by frames, so projectM must know the rate.
	wantedFPS = (int)lround(limit > 0 ? MIN(limit, 1.0 / refresh) : 1.0 / refresh);
	if(limit <= 0 || 1.0 / limit <= refresh) {
		nextFrameTime = 0;
		return YES;
	}
	double interval = 1.0 / limit;
	if(nextFrameTime > 0 && now + refresh / 2 < nextFrameTime) {
		return NO;
	}
	// Fell behind, or the first frame: start the cadence again from now.
	nextFrameTime = (nextFrameTime > 0 && now - nextFrameTime < interval) ? nextFrameTime + interval : now + interval;
	return YES;
}

- (void)deleteScaleBuffer {
	if(scaleFramebuffer) glDeleteFramebuffers(1, &scaleFramebuffer);
	if(scaleTexture) glDeleteTextures(1, &scaleTexture);
	scaleFramebuffer = 0;
	scaleTexture = 0;
	scaleWidth = scaleHeight = 0;
}

/// Scales the frame projectM drew into the lower left `width` × `height` of
/// the window's framebuffer up to the whole of it. A framebuffer can't be
/// blitted onto itself where the areas overlap, so the frame goes by way of
/// a texture of its own size.
- (void)scaleUpFrom:(size_t)width height:(size_t)height {
	if(width != scaleWidth || height != scaleHeight || !scaleFramebuffer) {
		[self deleteScaleBuffer];
		glGenTextures(1, &scaleTexture);
		glBindTexture(GL_TEXTURE_2D, scaleTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glBindTexture(GL_TEXTURE_2D, 0);
		glGenFramebuffers(1, &scaleFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, scaleFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, scaleTexture, 0);
		scaleWidth = width;
		scaleHeight = height;
	}
	// Blits obey the scissor test, which a preset may leave on.
	glDisable(GL_SCISSOR_TEST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, scaleFramebuffer);
	glBlitFramebuffer(0, 0, (GLint)width, (GLint)height, 0, 0, (GLint)width, (GLint)height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, scaleFramebuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, (GLint)width, (GLint)height, 0, 0, (GLint)drawableWidth, (GLint)drawableHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

- (void)renderFrameAtTime:(const CVTimeStamp *)outputTime {
	if(![self isFrameDueAt:outputTime]) return;
	CGLContextObj cgl = context.CGLContextObj;
	CGLLockContext(cgl);
	[context makeCurrentContext];

	if(!projectM && ![self makeProjectM]) {
		CGLUnlockContext(cgl);
		return;
	}

	NSArray<void (^)(void)> *commands;
	@synchronized(pendingCommands) {
		commands = [pendingCommands copy];
		[pendingCommands removeAllObjects];
	}
	if(playlistNeedsLoading) {
		[self loadPlaylist];
	}
	for(void (^command)(void) in commands) {
		command();
	}

	if(wantedFPS && wantedFPS != appliedFPS) {
		appliedFPS = wantedFPS;
		projectm_set_fps(projectM, wantedFPS);
	}

	// At a lower resolution, projectM draws into the lower left corner.
	NSInteger scale = MAX(_renderScale, 1);
	size_t width = MAX(drawableWidth / scale, 1);
	size_t height = MAX(drawableHeight / scale, 1);
	projectm_set_window_size(projectM, width, height);

	// The audio heard since the last frame.
	if(outputTime) {
		int frames = 735;
		if(lastVideoTime && outputTime->videoTimeScale) {
			double elapsed = (double)(outputTime->videoTime - lastVideoTime) / outputTime->videoTimeScale;
			frames = (int)(elapsed * 44100.0 + 0.5);
		}
		lastVideoTime = outputTime->videoTime;
		if(frames > kMaxFramesPerDraw) frames = kMaxFramesPerDraw;
		if(frames > 0) {
			int copied = [[VisualizationController sharedController] copyHeardStereoPCM:audio frames:frames latencyOffset:0];
			if(copied > 0) {
				projectm_pcm_add_float(projectM, audio, (unsigned int)copied, PROJECTM_STEREO);
			}
		}
	}

	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	projectm_opengl_render_frame(projectM);
	if(width != drawableWidth || height != drawableHeight) {
		[self scaleUpFrom:width height:height];
	} else if(scaleFramebuffer) {
		[self deleteScaleBuffer];
	}
	[context flushBuffer];

	[NSOpenGLContext clearCurrentContext];
	CGLUnlockContext(cgl);
}

#pragma mark - Presets

/// Runs `command` with projectM ready and the context current: on the next
/// frame, or at once while frozen.
- (void)perform:(void (^)(void))command {
	@synchronized(pendingCommands) {
		[pendingCommands addObject:command];
	}
	if(!_isRunning) {
		dispatch_async(dispatch_get_main_queue(), ^{
			[self drawOnce];
		});
	}
}

- (void)setPresetFolders:(NSArray<NSURL *> *)presetFolders {
	_presetFolders = [presetFolders copy];
	[self perform:^{
		self->playlistNeedsLoading = YES;
	}];
}

- (void)setTextureFolders:(NSArray<NSURL *> *)textureFolders {
	_textureFolders = [textureFolders copy];
	[self perform:^{
		self->playlistNeedsLoading = YES;
	}];
}

- (void)setShuffle:(BOOL)shuffle {
	_shuffle = shuffle;
	[self perform:^{
		projectm_playlist_set_shuffle(self->playlist, shuffle);
	}];
}

- (void)setPresetDuration:(double)presetDuration {
	_presetDuration = presetDuration;
	[self perform:^{
		projectm_set_preset_duration(self->projectM, presetDuration);
	}];
}

- (void)setPresetLocked:(BOOL)presetLocked {
	_presetLocked = presetLocked;
	[self perform:^{
		projectm_set_preset_locked(self->projectM, presetLocked);
	}];
}

- (void)setCrossfadeDuration:(double)crossfadeDuration {
	_crossfadeDuration = crossfadeDuration;
	[self perform:^{
		projectm_set_soft_cut_duration(self->projectM, crossfadeDuration);
	}];
}

- (void)setHardCutsEnabled:(BOOL)hardCutsEnabled {
	_hardCutsEnabled = hardCutsEnabled;
	[self perform:^{
		projectm_set_hard_cut_enabled(self->projectM, hardCutsEnabled);
	}];
}

- (void)setHardCutThreshold:(double)hardCutThreshold {
	_hardCutThreshold = hardCutThreshold;
	[self perform:^{
		projectm_set_hard_cut_sensitivity(self->projectM, (float)hardCutThreshold);
	}];
}

- (void)setMeshSize:(NSSize)meshSize {
	_meshSize = meshSize;
	[self perform:^{
		projectm_set_mesh_size(self->projectM, (size_t)meshSize.width, (size_t)meshSize.height);
	}];
}

- (void)setRenderScale:(NSInteger)renderScale {
	_renderScale = MAX(renderScale, 1);
	if(!_isRunning) {
		[self perform:^{}];
	}
}

- (void)setFrameRateLimit:(double)frameRateLimit {
	_frameRateLimit = MAX(frameRateLimit, 0);
	[self perform:^{
		self->nextFrameTime = 0;
	}];
}

- (void)setExcludedThemes:(NSArray<NSString *> *)excludedThemes {
	if([_excludedThemes isEqualToArray:excludedThemes]) return;
	_excludedThemes = [excludedThemes copy];
	[self perform:^{
		self->playlistNeedsLoading = YES;
	}];
}

- (void)fetchPresets:(void (^)(NSArray<NSString *> *, NSInteger))completion {
	[self perform:^{
		NSMutableArray<NSString *> *paths = [NSMutableArray new];
		uint32_t size = projectm_playlist_size(self->playlist);
		char **items = size ? projectm_playlist_items(self->playlist, 0, size) : NULL;
		for(uint32_t i = 0; items && items[i]; i++) {
			NSString *path = [NSString stringWithUTF8String:items[i]];
			[paths addObject:path ?: @""];
		}
		if(items) projectm_playlist_free_string_array(items);
		NSInteger current = size ? (NSInteger)projectm_playlist_get_position(self->playlist) : -1;
		dispatch_async(dispatch_get_main_queue(), ^{
			completion(paths, current);
		});
	}];
}

- (void)selectPresetAtIndex:(NSUInteger)index {
	[self perform:^{
		if(index < projectm_playlist_size(self->playlist)) {
			projectm_playlist_set_position(self->playlist, (uint32_t)index, false);
		}
	}];
}

- (void)nextPreset {
	[self perform:^{
		if(projectm_playlist_size(self->playlist) > 0) {
			projectm_playlist_play_next(self->playlist, false);
		}
	}];
}

- (void)previousPreset {
	[self perform:^{
		if(projectm_playlist_size(self->playlist) > 0) {
			projectm_playlist_play_previous(self->playlist, false);
		}
	}];
}

- (void)presetSwitchedToIndex:(unsigned int)index {
	char *item = projectm_playlist_item(playlist, index);
	if(!item) return;
	NSString *path = [NSString stringWithUTF8String:item];
	projectm_playlist_free_string(item);
	currentPresetPath = path;
	void (^presetDidChange)(NSString *) = self.presetDidChange;
	if(path && presetDidChange) {
		dispatch_async(dispatch_get_main_queue(), ^{
			presetDidChange(path);
		});
	}
}

@end

static void presetSwitched(bool isHardCut, unsigned int index, void *userData) {
	[(__bridge ProjectMView *)userData presetSwitchedToIndex:index];
}
