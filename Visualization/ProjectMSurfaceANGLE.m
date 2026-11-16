//
//  ProjectMSurfaceANGLE.m
//  Cog
//
//  Created by Kevin López Brante on 2026-11-16.
//
//  OpenGL ES 3.0 from ANGLE, which draws with Metal, into the view's
//  CAMetalLayer: projectM uses no deprecated OpenGL. ANGLE is built by
//  Scripts/build-angle.sh. libEGL is opened at run time and finds libGLESv2
//  beside itself in Contents/Frameworks; libprojectM links libGLESv2.
//

#import "ProjectMSurface.h"

#import <QuartzCore/CAMetalLayer.h>
#import <dlfcn.h>
#import <pthread.h>

#import <EGL/egl.h>
#import <EGL/eglext.h>
#import <EGL/eglext_angle.h>

#import "Logging.h"

// The EGL functions used, looked up once.
static struct {
	PFNEGLGETPLATFORMDISPLAYPROC GetPlatformDisplay;
	PFNEGLINITIALIZEPROC Initialize;
	PFNEGLCHOOSECONFIGPROC ChooseConfig;
	PFNEGLCREATECONTEXTPROC CreateContext;
	PFNEGLDESTROYCONTEXTPROC DestroyContext;
	PFNEGLCREATEWINDOWSURFACEPROC CreateWindowSurface;
	PFNEGLDESTROYSURFACEPROC DestroySurface;
	PFNEGLMAKECURRENTPROC MakeCurrent;
	PFNEGLSWAPBUFFERSPROC SwapBuffers;
	PFNEGLSWAPINTERVALPROC SwapInterval;
	PFNEGLQUERYSURFACEPROC QuerySurface;
	PFNEGLGETPROCADDRESSPROC GetProcAddress;
	PFNEGLGETERRORPROC GetError;
} egl;

static EGLDisplay display = EGL_NO_DISPLAY;
static EGLConfig config;
static ProjectMGL anglesGL;

/// Opens libEGL and ANGLE's Metal display, once. False when ANGLE can't
/// run here.
static BOOL loadANGLE(void) {
	static BOOL loaded = NO;
	static dispatch_once_t once;
	dispatch_once(&once, ^{
		NSString *path = [NSBundle.mainBundle.privateFrameworksPath stringByAppendingPathComponent:@"libEGL.dylib"];
		void *library = dlopen(path.fileSystemRepresentation, RTLD_NOW | RTLD_LOCAL);
		if(!library) {
			ALog(@"projectM: %s", dlerror());
			return;
		}
#define LOAD(name)                                                    \
	egl.name = (__typeof__(egl.name))dlsym(library, "egl" #name);     \
	if(!egl.name) {                                                   \
		ALog(@"projectM: libEGL lacks egl%s", #name);                 \
		return;                                                       \
	}
		LOAD(GetPlatformDisplay)
		LOAD(Initialize)
		LOAD(ChooseConfig)
		LOAD(CreateContext)
		LOAD(DestroyContext)
		LOAD(CreateWindowSurface)
		LOAD(DestroySurface)
		LOAD(MakeCurrent)
		LOAD(SwapBuffers)
		LOAD(SwapInterval)
		LOAD(QuerySurface)
		LOAD(GetProcAddress)
		LOAD(GetError)
#undef LOAD

		const EGLAttrib displayAttributes[] = {
			EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE,
			EGL_NONE
		};
		display = egl.GetPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE, (void *)EGL_DEFAULT_DISPLAY, displayAttributes);
		if(display == EGL_NO_DISPLAY || !egl.Initialize(display, NULL, NULL)) {
			ALog(@"projectM: no ANGLE Metal display (0x%x)", egl.GetError());
			return;
		}
		const EGLint configAttributes[] = {
			EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
			EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
			EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
			EGL_NONE
		};
		EGLint count = 0;
		if(!egl.ChooseConfig(display, configAttributes, &config, 1, &count) || count < 1) {
			ALog(@"projectM: no ANGLE config (0x%x)", egl.GetError());
			return;
		}

#define GL(member, name)                                                          \
	anglesGL.member = (__typeof__(anglesGL.member))egl.GetProcAddress("gl" #name); \
	if(!anglesGL.member) {                                                        \
		ALog(@"projectM: ANGLE lacks gl%s", #name);                               \
		return;                                                                   \
	}
		GL(Viewport, Viewport)
		GL(Disable, Disable)
		GL(GenTextures, GenTextures)
		GL(DeleteTextures, DeleteTextures)
		GL(BindTexture, BindTexture)
		GL(TexImage2D, TexImage2D)
		GL(TexParameteri, TexParameteri)
		GL(GenFramebuffers, GenFramebuffers)
		GL(DeleteFramebuffers, DeleteFramebuffers)
		GL(BindFramebuffer, BindFramebuffer)
		GL(FramebufferTexture2D, FramebufferTexture2D)
		GL(BlitFramebuffer, BlitFramebuffer)
#undef GL
		loaded = YES;
	});
	return loaded;
}

@interface ProjectMSurfaceANGLE : NSObject <ProjectMSurface>
@end

@implementation ProjectMSurfaceANGLE {
	pthread_mutex_t mutex;
	EGLContext context;
	EGLSurface surface;
	CAMetalLayer *layer;
}

- (instancetype)initWithContext:(EGLContext)context {
	self = [super init];
	if(self) {
		pthread_mutex_init(&mutex, NULL);
		self->context = context;
		surface = EGL_NO_SURFACE;
		layer = [CAMetalLayer layer];
		layer.opaque = YES;
		layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
	}
	return self;
}

- (void)dealloc {
	[self lock];
	egl.MakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	if(surface != EGL_NO_SURFACE) egl.DestroySurface(display, surface);
	egl.DestroyContext(display, context);
	[self unlock];
	pthread_mutex_destroy(&mutex);
}

- (const ProjectMGL *)gl {
	return &anglesGL;
}

- (CALayer *)makeBackingLayer {
	return layer;
}

- (void)attachToView:(NSView *)view {
	if(surface == EGL_NO_SURFACE) {
		// ANGLE draws into the layer itself; it stays with the view when the
		// view moves between windows, so one surface lasts.
		surface = egl.CreateWindowSurface(display, config, (__bridge EGLNativeWindowType)layer, NULL);
		if(surface == EGL_NO_SURFACE) {
			ALog(@"projectM: no ANGLE window surface (0x%x)", egl.GetError());
		}
	}
	[self updateForView:view];
}

- (void)updateForView:(NSView *)view {
	CGFloat scale = view.window.backingScaleFactor ?: layer.contentsScale;
	layer.contentsScale = scale;
	NSSize size = [view convertRectToBacking:view.bounds].size;
	layer.drawableSize = CGSizeMake(MAX(size.width, 1), MAX(size.height, 1));
}

- (void)detachFromView {
}

- (void)lock {
	pthread_mutex_lock(&mutex);
}

- (void)unlock {
	pthread_mutex_unlock(&mutex);
}

- (BOOL)makeCurrent {
	if(surface == EGL_NO_SURFACE) return NO;
	if(!egl.MakeCurrent(display, surface, surface, context)) {
		ALog(@"projectM: eglMakeCurrent failed (0x%x)", egl.GetError());
		return NO;
	}
	egl.SwapInterval(display, 1);
	return YES;
}

- (void)clearCurrent {
	egl.MakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

- (void)present {
	egl.SwapBuffers(display, surface);
}

- (void)getDrawableWidth:(size_t *)width height:(size_t *)height {
	EGLint w = 0, h = 0;
	if(surface != EGL_NO_SURFACE) {
		egl.QuerySurface(display, surface, EGL_WIDTH, &w);
		egl.QuerySurface(display, surface, EGL_HEIGHT, &h);
	}
	if(w <= 0 || h <= 0) {
		w = (EGLint)layer.drawableSize.width;
		h = (EGLint)layer.drawableSize.height;
	}
	*width = (size_t)MAX(w, 1);
	*height = (size_t)MAX(h, 1);
}

@end

id<ProjectMSurface> ProjectMMakeSurface(void) {
	if(!loadANGLE()) return nil;
	const EGLint contextAttributes[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 0,
		EGL_NONE
	};
	EGLContext context = egl.CreateContext(display, config, EGL_NO_CONTEXT, contextAttributes);
	if(context == EGL_NO_CONTEXT) {
		ALog(@"projectM: no ANGLE context (0x%x)", egl.GetError());
		return nil;
	}
	return [[ProjectMSurfaceANGLE alloc] initWithContext:context];
}
