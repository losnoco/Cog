//
//  SpectrumViewPM.m
//  Cog
//
//  Created by Christopher Snowhill on 3/11/25.
//

#import "ProjectMView.h"

#import "NSView+Visibility.h"

#include <OpenGL/gl3.h>

#import <projectM-4/projectM.h>

#import <projectM-4/playlist.h>

#import "Logging.h"

static void *kProjectMViewContext = &kProjectMViewContext;

extern NSString *CogPlaybackDidBeginNotificiation;
extern NSString *CogPlaybackDidPauseNotificiation;
extern NSString *CogPlaybackDidResumeNotificiation;
extern NSString *CogPlaybackDidStopNotificiation;

@implementation ProjectMView {
	VisualizationController *visController;
	projectm_handle pm;
	projectm_playlist_handle pl;

	float visAudio[4096];
	
	NSTimer *timer;
}

- (id)initWithFrame:(NSRect)frame {
	NSOpenGLPixelFormatAttribute attr[] = {
		NSOpenGLPFAAllowOfflineRenderers,
		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
		kCGLPFASupportsAutomaticGraphicsSwitching,
		NSOpenGLPFAColorSize, 24,
		NSOpenGLPFAAlphaSize, 8,
		NSOpenGLPFAClosestPolicy,
		NSOpenGLPFADoubleBuffer,
		NSOpenGLPFAAccelerated,
		0
	};
	
	NSOpenGLPixelFormat *fmt = [[NSOpenGLPixelFormat alloc] initWithAttributes:attr];
	if(!fmt) {
		return nil;
	}

	self = [super initWithFrame:frame pixelFormat:fmt];
	if(self) {
		self.wantsBestResolutionOpenGLSurface = YES;

		visController = [NSClassFromString(@"VisualizationController") sharedController];
		
		timer = [NSTimer timerWithTimeInterval:(1.0 / 60.0) repeats:YES block:^(NSTimer * _Nonnull timer) {
			self.needsDisplay = YES;
		}];
		[[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
	}

	return self;
}

- (void)dealloc {
	[timer invalidate];
	timer = nil;

	if(pl) {
		projectm_playlist_destroy(pl);
	}
	if(pm) {
		projectm_destroy(pm);
	}
}

static void presetFailed(const char *preset_filename, const char *message, void *user_data) {
	NSLog(@"ProjectM preset failed: %s - for reason: %s", preset_filename, message);
}

- (void)initProjectM {
	if(!pm && !pl) {
		NSBundle* me = [NSBundle bundleForClass: self.class];
		NSLog(@"main bundle: %@", [me bundlePath]);
		NSString* presetsPath = [me pathForResource:@"presets" ofType:nil];
		NSLog(@"presets path %@", presetsPath);

		NSString* texturesPath = [me pathForResource:@"textures" ofType:nil];
		NSLog(@"textures path %@", texturesPath);

		pm = projectm_create();
		if(!pm) {
			return;
		}

		char *_texturesPath = projectm_alloc_string((int)(strlen([texturesPath UTF8String]) + 1));
		if(!_texturesPath) {
			return;
		}
		strcpy(_texturesPath, [texturesPath UTF8String]);
		const char *texturesPaths[1] = { _texturesPath };

		projectm_set_texture_search_paths(pm, texturesPaths, 1);

		projectm_free_string(_texturesPath);

		NSRect rect = [self.window convertRectToBacking:self.bounds];
		projectm_set_window_size(pm, rect.size.width, rect.size.height);

		projectm_set_fps(pm, 60);
		projectm_set_mesh_size(pm, 48, 32);
		projectm_set_aspect_correction(pm, true);
		projectm_set_preset_locked(pm, false);

		projectm_set_preset_duration(pm, 30.0);
		projectm_set_soft_cut_duration(pm, 3.0);
		projectm_set_hard_cut_enabled(pm, false);
		projectm_set_hard_cut_duration(pm, 20.0);
		projectm_set_hard_cut_sensitivity(pm, 1.0);
		projectm_set_beat_sensitivity(pm, 1.0);

		pl = projectm_playlist_create(pm);
		if(!pl) {
			return;
		}

		projectm_playlist_set_preset_switch_failed_event_callback(pl, presetFailed, NULL);

		char *_presetsPath = projectm_alloc_string((int)(strlen([presetsPath UTF8String]) + 1));
		if(!_presetsPath) {
			return;
		}
		strcpy(_presetsPath, [presetsPath UTF8String]);

		projectm_playlist_add_path(pl, _presetsPath, true, false);

		projectm_free_string(_presetsPath);

		projectm_playlist_sort(pl, 0, projectm_playlist_size(pl), SORT_PREDICATE_FILENAME_ONLY, SORT_ORDER_ASCENDING);

		projectm_playlist_set_shuffle(pl, true);
	}
}


- (void)drawRect:(NSRect)dirtyRect {
	[[self openGLContext] makeCurrentContext];

	[self initProjectM];

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	NSRect rect = [self.window convertRectToBacking:self.bounds];
	projectm_set_window_size(pm, rect.size.width, rect.size.height);

	[self->visController copyVisPCM:&visAudio[0] visFFT:nil latencyOffset:0];

	size_t maxSamples = projectm_pcm_get_max_samples();
	if(maxSamples > 4096) maxSamples = 4096;
		
	projectm_pcm_add_float(pm, &visAudio[0], (unsigned int)maxSamples, 1);
	projectm_opengl_render_frame(pm);
	
	[[self openGLContext] flushBuffer];
	[NSOpenGLContext clearCurrentContext];
}

@end
