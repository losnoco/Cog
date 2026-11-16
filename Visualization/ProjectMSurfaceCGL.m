//
//  ProjectMSurfaceCGL.m
//  Cog
//
//  Created by Kevin López Brante on 2026-11-16.
//
//  Intel Macs: desktop OpenGL 4.1 through an NSOpenGLContext on the view.
//  Apple Silicon draws with ANGLE instead (ProjectMSurfaceANGLE.m).
//

#if defined(__x86_64__)

#define GL_SILENCE_DEPRECATION

// AppKit deprecates OpenGL, which projectM's desktop build needs.
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#import "ProjectMSurface.h"

#import <OpenGL/gl3.h>

#import "Logging.h"

static const ProjectMGL desktopGL = {
	.Viewport = glViewport,
	.Disable = glDisable,
	.GenTextures = glGenTextures,
	.DeleteTextures = glDeleteTextures,
	.BindTexture = glBindTexture,
	.TexImage2D = glTexImage2D,
	.TexParameteri = glTexParameteri,
	.GenFramebuffers = glGenFramebuffers,
	.DeleteFramebuffers = glDeleteFramebuffers,
	.BindFramebuffer = glBindFramebuffer,
	.FramebufferTexture2D = glFramebufferTexture2D,
	.BlitFramebuffer = glBlitFramebuffer,
};

@interface ProjectMSurfaceCGL : NSObject <ProjectMSurface>
@end

@implementation ProjectMSurfaceCGL {
	NSOpenGLContext *context;
	size_t width, height;
	__weak NSView *view;
}

- (instancetype)initWithContext:(NSOpenGLContext *)context {
	self = [super init];
	if(self) {
		self->context = context;
		GLint swapInterval = 1;
		[context setValues:&swapInterval forParameter:NSOpenGLContextParameterSwapInterval];
	}
	return self;
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (const ProjectMGL *)gl {
	return &desktopGL;
}

- (CALayer *)makeBackingLayer {
	return nil;
}

- (void)attachToView:(NSView *)view {
	if(self->view != view) {
		[[NSNotificationCenter defaultCenter] removeObserver:self];
		view.wantsBestResolutionOpenGLSurface = YES;
		// Moving to another screen changes the drawable.
		[[NSNotificationCenter defaultCenter] addObserver:self
		                                         selector:@selector(surfaceNeedsUpdate:)
		                                             name:NSViewGlobalFrameDidChangeNotification
		                                           object:view];
		self->view = view;
	}
	context.view = view;
	[self updateForView:view];
}

- (void)surfaceNeedsUpdate:(NSNotification *)notification {
	[self lock];
	[context update];
	[self unlock];
}

- (void)updateForView:(NSView *)view {
	[context update];
	NSSize size = [view convertRectToBacking:view.bounds].size;
	width = (size_t)MAX(size.width, 1);
	height = (size_t)MAX(size.height, 1);
}

- (void)detachFromView {
	[context clearDrawable];
}

- (void)lock {
	CGLLockContext(context.CGLContextObj);
}

- (void)unlock {
	CGLUnlockContext(context.CGLContextObj);
}

- (BOOL)makeCurrent {
	if(!context.view) return NO;
	[context makeCurrentContext];
	return YES;
}

- (void)clearCurrent {
	[NSOpenGLContext clearCurrentContext];
}

- (void)present {
	[context flushBuffer];
}

- (void)getDrawableWidth:(size_t *)width height:(size_t *)height {
	*width = self->width;
	*height = self->height;
}

@end

id<ProjectMSurface> ProjectMMakeSurface(void) {
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
	NSOpenGLContext *context = pixelFormat ? [[NSOpenGLContext alloc] initWithFormat:pixelFormat shareContext:nil] : nil;
	if(!context) {
		ALog(@"projectM: no OpenGL 4.1 Core context");
		return nil;
	}
	return [[ProjectMSurfaceCGL alloc] initWithContext:context];
}

#endif
