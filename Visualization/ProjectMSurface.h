//
//  ProjectMSurface.h
//  Cog
//
//  Created by Kevin López Brante on 2026-11-16.
//

#import <Cocoa/Cocoa.h>

NS_ASSUME_NONNULL_BEGIN

/// The GL functions ProjectMView calls itself, from ANGLE's OpenGL ES, looked
/// up through EGL as libEGL is opened at run time. The types are the ones the
/// GL typedefs stand for.
typedef struct {
	void (*Viewport)(int x, int y, int width, int height);
	void (*Disable)(unsigned int capability);
	void (*GenTextures)(int count, unsigned int *textures);
	void (*DeleteTextures)(int count, const unsigned int *textures);
	void (*BindTexture)(unsigned int target, unsigned int texture);
	void (*TexImage2D)(unsigned int target, int level, int internalFormat, int width, int height, int border,
	                   unsigned int format, unsigned int type, const void *_Nullable pixels);
	void (*TexParameteri)(unsigned int target, unsigned int name, int value);
	void (*GenFramebuffers)(int count, unsigned int *framebuffers);
	void (*DeleteFramebuffers)(int count, const unsigned int *framebuffers);
	void (*BindFramebuffer)(unsigned int target, unsigned int framebuffer);
	void (*FramebufferTexture2D)(unsigned int target, unsigned int attachment, unsigned int textarget,
	                             unsigned int texture, int level);
	void (*BlitFramebuffer)(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1,
	                        unsigned int mask, unsigned int filter);
} ProjectMGL;

/// Where projectM draws: a GL context and the drawable in a view. Everything
/// but makeBackingLayer and attach/update/detach runs with the surface
/// locked, on whichever thread draws.
@protocol ProjectMSurface <NSObject>

@property(readonly) const ProjectMGL *gl;

/// The view's layer, made by the surface, or nil for AppKit's own.
- (CALayer *_Nullable)makeBackingLayer;

/// Main thread, with the surface locked.
- (void)attachToView:(NSView *)view;
- (void)updateForView:(NSView *)view;
- (void)detachFromView;

- (void)lock;
- (void)unlock;
/// Makes the context current on this thread, drawing to the view.
- (BOOL)makeCurrent;
- (void)clearCurrent;
/// Shows what was drawn.
- (void)present;
/// The drawable's size, in pixels.
- (void)getDrawableWidth:(size_t *)width height:(size_t *)height;

@end

/// A surface, or nil when ANGLE can't draw here.
id<ProjectMSurface> _Nullable ProjectMMakeSurface(void);

NS_ASSUME_NONNULL_END
