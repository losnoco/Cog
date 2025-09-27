//
//  SCView.m
//  Cog
//
//  Created by Christopher Snowhill on 9/25/25.
//

#import <Cocoa/Cocoa.h>

#import <MetalKit/MetalKit.h>

#import "SCView.h"

#import "NSView+Visibility.h"

#import <CogAudio/VisualizationController.h>

#import <nuked_sc55/api.h>

#import "SCVis.h"

#import "metal_shader_types.h"

extern NSString *CogPlaybackDidPrebufferNotification;

static void *kSCViewContext = &kSCViewContext;

static NSString *CogSCVisUpdateNotification = @"CogSCVisUpdateNotification";

extern NSString *CogPlaybackDidBeginNotificiation;
extern NSString *CogPlaybackDidPauseNotificiation;
extern NSString *CogPlaybackDidResumeNotificiation;
extern NSString *CogPlaybackDidStopNotificiation;

static NSString *getRomName(NSString *baseName) {
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	NSString *basePath = [[paths firstObject] stringByAppendingPathComponent:@"Cog"];
	basePath = [basePath stringByAppendingPathComponent:@"Roms"];
	basePath = [basePath stringByAppendingPathComponent:@"Nuked-SC55"];
	basePath = [basePath stringByAppendingPathComponent:baseName];
	return basePath;
}

static int loadRom(void *context, const char *name, uint8_t *buffer, uint32_t *size) {
	@autoreleasepool {
		NSString *_name = [NSString stringWithUTF8String:name];
		NSString *romName;
		if([_name isEqualToString:@"back.data"]) {
			NSBundle *bundle = [NSBundle bundleWithIdentifier:@"org.losnoco.nuked-sc55"];
			romName = [bundle pathForResource:@"back" ofType:@"data"];
		} else {
			romName = getRomName(_name);
		}
		BOOL dir = NO;
		NSFileManager *defaultManager = [NSFileManager defaultManager];
		if(![defaultManager fileExistsAtPath:romName isDirectory:&dir]) {
			return -1;
		}
		if(size) {
			NSData *fileData = [defaultManager contentsAtPath:romName];
			if(!fileData)
				return -1;
			if([fileData length] > *size) {
				*size = (uint32_t) [fileData length];
				return -1;
			}
			*size = (uint32_t) [fileData length];
			if(buffer) {
				memcpy(buffer, [fileData bytes], *size);
			}
		}
		return 0;
	}
}

/* macOS requires constants in a buffer to have a 256 byte alignment. */
#ifdef TARGET_OS_MAC
#define kMetalBufferAlignment 256
#else
#define kMetalBufferAlignment 4
#endif

#define MTL_ALIGN_BUFFER(size) ((size + kMetalBufferAlignment - 1) & (~(kMetalBufferAlignment - 1)))

typedef struct
{
   void *data;
   NSUInteger offset;
   __unsafe_unretained id<MTLBuffer> buffer;
} BufferRange;

@interface BufferNode : NSObject
@property (nonatomic, readonly) id<MTLBuffer> src;
@property (nonatomic, readwrite) NSUInteger allocated;
@property (nonatomic, readwrite) BufferNode *next;
@end

@interface BufferChain : NSObject
- (instancetype)initWithDevice:(id<MTLDevice>)device blockLen:(NSUInteger)blockLen;
- (bool)allocRange:(BufferRange *)range length:(NSUInteger)length;
- (void)commitRanges;
- (void)discard;
@end

@interface Texture : NSObject
@property (nonatomic, readwrite) id<MTLTexture> texture;
@property (nonatomic, readwrite) id<MTLSamplerState> sampler;
@end

enum { _ChainCount = 3 };

@implementation SCView {
	VisualizationController *visController;
	NSTimer *timer;

	BOOL paused;
	BOOL stopped;
	BOOL isSetup;
	BOOL isListening;
	BOOL observersAdded;
	BOOL isOccluded;

	BOOL prebuffered;

	unsigned int numDisplays;
	NSRect initFrame;
	uint32_t lcdWidth;
	uint32_t lcdHeight;

	void *lcdClearedState;

	NSURL *currentTrack;
	NSMutableArray *files;

	uint32_t lcdBackground[lcd_background_size];
	uint32_t renderBuffer[lcd_buffer_size];

	BOOL EDR;
	MTLPixelFormat pixelFormat;

	id<MTLCommandQueue> mtlCmdQueue;
	id<MTLCommandBuffer> _commandBuffer;
	id<MTLRenderCommandEncoder> _rce;

	Texture *mtlTexture[3];
	id<MTLSamplerState> mtlSampler;

	id<MTLLibrary> _library;

	id<MTLRenderPipelineState> _drawState;
	id<MTLRenderPipelineState> _clearState;

	MTKView *_view;
	id<CAMetalDrawable> nextDrawable;

	Uniforms uniforms;

	NSUInteger _currentChain;
	BufferChain *_chain[_ChainCount];
}

@synthesize isListening;

+ (Class) layerClass {
	return [CAMetalLayer class];
}

- (BOOL)wantsUpdateLayer
{
	return YES;
}

- (CALayer *)makeBackingLayer {
	return [CAMetalLayer layer];
}

- (CAMetalLayer *)metalLayer {
	return (CAMetalLayer *)_view.layer;
}

- (void)_nextChain
{
   _currentChain = (_currentChain + 1) % _ChainCount;
   [_chain[_currentChain] discard];
}

- (bool)allocRange:(BufferRange *)range length:(NSUInteger)length
{
   return [_chain[_currentChain] allocRange:range length:length];
}

- (MTLVertexDescriptor *)_spriteVertexDescriptor
{
   MTLVertexDescriptor *vd = [MTLVertexDescriptor new];
   vd.attributes[0].offset = 0;
   vd.attributes[0].format = MTLVertexFormatFloat2;
   vd.attributes[1].offset = offsetof(SpriteVertex, texCoord);
   vd.attributes[1].format = MTLVertexFormatFloat2;
   vd.attributes[2].offset = offsetof(SpriteVertex, color);
   vd.attributes[2].format = MTLVertexFormatFloat4;
   vd.layouts[0].stride    = sizeof(SpriteVertex);
   return vd;
}

- (id<MTLRenderCommandEncoder>)rce {
	assert(_commandBuffer != nil);
	if (_rce == nil)
	{
		MTLRenderPassDescriptor *rpd = [MTLRenderPassDescriptor new];
		rpd.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
		rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
		rpd.colorAttachments[0].texture = nextDrawable.texture;
		_rce       = [_commandBuffer renderCommandEncoderWithDescriptor:rpd];
		_rce.label = @"Frame command encoder";
	}
	return _rce;
}

matrix_float4x4 matrix_proj_ortho(float left, float right, float top, float bottom)
{
   float sx            = 2 / (right - left);
   float sy            = 2 / (top   - bottom);
   float tx            = (right + left)   / (left   - right);
   float ty            = (top   + bottom) / (bottom - top);
   simd_float4 P       = simd_make_float4(sx,  0, 0, 0);
   simd_float4 Q       = simd_make_float4(0,  sy, 0, 0);
   simd_float4 R       = simd_make_float4(0,   0, 1, 0);
   simd_float4 S       = simd_make_float4(tx, ty, 0, 1);
   matrix_float4x4 mat = {P, Q, R, S};
   return mat;
}

- (id)init {
	struct sc55_state *st = sc55_init(0, NONE, loadRom, NULL);
	if(!st)
		return nil;

	uint32_t width = 0;
	uint32_t height = 0;
	sc55_lcd_get_size(st, &width, &height);
	sc55_free(st);

	if(!width || !height)
		return nil;

	lcdWidth = width;
	lcdHeight = height;

	NSRect frame = NSMakeRect(0, 0, width, height);

	id<MTLDevice> dev = MTLCreateSystemDefaultDevice();

	self = [super initWithFrame:frame device:dev];
	if(self) {
		initFrame = frame;

		self.wantsLayer = YES;

		self.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
		self.enableSetNeedsDisplay = YES;

		uint32_t size = sizeof(lcdBackground);
		int ret = loadRom(NULL, "back.data", (uint8_t *) &lcdBackground, &size);
		if(ret < 0 || size != sizeof(lcdBackground))
			return nil;

		size = sc55_lcd_state_size();
		lcdClearedState = malloc(size);
		if(!lcdClearedState)
			return nil;
		sc55_lcd_clear(lcdClearedState, size, width, height);

		visController = [NSClassFromString(@"VisualizationController") sharedController];
		timer = nil;
		stopped = YES;
		paused = NO;
		isListening = NO;

		currentTrack = nil;
		files = [NSMutableArray new];

		[self addObservers];

		self.delegate = self;
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

- (BOOL)_initClearState {
	NSError *err;
	MTLVertexDescriptor          *vd = [self _spriteVertexDescriptor];
	MTLRenderPipelineDescriptor *psd = [MTLRenderPipelineDescriptor new];
	psd.label                        = @"clear_state";

	MTLRenderPipelineColorAttachmentDescriptor *ca = psd.colorAttachments[0];
	ca.pixelFormat                 = pixelFormat;

	psd.vertexDescriptor = vd;
	psd.vertexFunction = [_library newFunctionWithName:@"stock_vertex"];
	psd.fragmentFunction = [_library newFunctionWithName:@"stock_fragment_color"];

	_clearState = [self.device newRenderPipelineStateWithDescriptor:psd error:&err];
	if(err != nil)
		return NO;

	return YES;
}

- (BOOL)_initDrawState {
	NSError *err;
	MTLVertexDescriptor          *vd = [self _spriteVertexDescriptor];
	MTLRenderPipelineDescriptor *psd = [MTLRenderPipelineDescriptor new];
	psd.label                        = @"draw_state";

	MTLRenderPipelineColorAttachmentDescriptor *ca = psd.colorAttachments[0];
	ca.pixelFormat                 = pixelFormat;
	ca.blendingEnabled             = NO;
	ca.sourceRGBBlendFactor        = MTLBlendFactorSourceAlpha;
	ca.destinationRGBBlendFactor   = MTLBlendFactorOneMinusSourceAlpha;
	ca.sourceAlphaBlendFactor      = MTLBlendFactorSourceAlpha;
	ca.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

	psd.sampleCount = 1;
	psd.vertexDescriptor = vd;
	psd.vertexFunction = [_library newFunctionWithName:@"stock_vertex"];
	psd.fragmentFunction = [_library newFunctionWithName:@"stock_fragment"];

	_drawState = [self.device newRenderPipelineStateWithDescriptor:psd error:&err];
	if(err != nil)
		return NO;

	return YES;
}

- (BOOL)setup {
	if(@available(macOS 10.15, *)) {
		NSWindow *window = self.window;
		EDR = (window) ? (window.screen.maximumPotentialExtendedDynamicRangeColorComponentValue > 1.0) : NO;
	} else {
		EDR = NO;
	}
	
	pixelFormat = MTLPixelFormatBGRA8Unorm;
	CFStringRef colorSpaceName = kCGColorSpaceITUR_709;
	if(@available(macOS 10.15.4, *)) {
		if(EDR) {
			[self metalLayer].wantsExtendedDynamicRangeContent = YES;
			pixelFormat = MTLPixelFormatBGR10A2Unorm;
			if(@available(macOS 11, *)) {
				colorSpaceName = kCGColorSpaceITUR_2100_PQ;
			} else {
				colorSpaceName = kCGColorSpaceITUR_2020_PQ_EOTF;
			}
		}
	}
	CGColorSpaceRef colorspace = CGColorSpaceCreateWithName(colorSpaceName);

	self.colorPixelFormat = pixelFormat;
	[self metalLayer].pixelFormat = pixelFormat;
	[self metalLayer].colorspace = colorspace;
	CGColorSpaceRelease(colorspace);

	mtlCmdQueue = [self.device newCommandQueue];
	if(!mtlCmdQueue)
		return NO;

	for(size_t i = 0; i < 3; ++i)
		mtlTexture[i] = [Texture new];
	for(size_t i = 0; i < _ChainCount; ++i)
		_chain[i] = [[BufferChain alloc] initWithDevice:self.device blockLen:65536];

	MTLTextureDescriptor *textureDescriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:lcdWidth height:lcdHeight mipmapped:NO];
	mtlTexture[0].texture = [self.device newTextureWithDescriptor:textureDescriptor];
	if(!mtlTexture[0].texture)
		return nil;
	mtlTexture[1].texture = [self.device newTextureWithDescriptor:textureDescriptor];
	if(!mtlTexture[1].texture)
		return nil;
	mtlTexture[2].texture = [self.device newTextureWithDescriptor:textureDescriptor];
	if(!mtlTexture[2].texture)
		return nil;

	MTLSamplerDescriptor *sd = [MTLSamplerDescriptor new];
	sd.label = @"nearest";
	mtlSampler = [self.device newSamplerStateWithDescriptor:sd];
	if(!mtlSampler)
		return NO;

	for(size_t i = 0; i < 3; ++i)
		mtlTexture[i].sampler = mtlSampler;

	_library = [self.device newDefaultLibrary];
	if(!_library)
		return NO;

	if(![self _initClearState])
		return NO;
	if(![self _initDrawState])
		return NO;

	for(uint32_t i = 0; i < 3; ++i)
		[self renderEmptyPanel:i];

	uniforms.outputSize = simd_make_float2(initFrame.size.width, initFrame.size.height);
	uniforms.projectionMatrix = matrix_proj_ortho(0, 1, 0, 1);

	isSetup = YES;

	return YES;
}

- (void)observeValueForKeyPath:(NSString *)keyPath
					  ofObject:(id)object
						change:(NSDictionary<NSKeyValueChangeKey, id> *)change
					   context:(void *)context {
	if(context == kSCViewContext) {
		if([keyPath isEqualToString:@"self.window.visible"]) {
			[self updateVisListening];
		}
	} else {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
	}
}

- (void)removeTrack:(NSURL *)url {
	@synchronized (self) {
		assert([url isEqualTo:files[0][@"url"]]);
		[files removeObjectAtIndex:0];
	}
}

- (void)addTrack:(NSURL *)url {
	NSMutableArray *events = [NSMutableArray new];
	NSMutableDictionary *file = [@{@"url": url, @"events": events} mutableCopy];
	[files addObject:file];
}

- (NSArray *)getEventsForTimestamp:(uint64_t)timestamp {
	@synchronized (self) {
		NSMutableArray *events = [files count] ? files[0][@"events"] : nil;
		if(!events || ![events count]) return nil;

		size_t i = 0;
		for(SCVisUpdate *event in events) {
			if(event.timestamp > timestamp)
				break;
			++i;
		}

		if(!i)
			return nil;

		size_t count = [events count];
		NSArray *ret = [events subarrayWithRange:NSMakeRange(0, i)];
		if(i < count)
			files[0][@"events"] = [[events subarrayWithRange:NSMakeRange(i, count - i)] mutableCopy];
		else
			files[0][@"events"] = [NSMutableArray new];

		return ret;
	}
}

- (NSMutableArray *)findEventsForUrl:(NSURL *)url withTimestamp:(uint64_t)timestamp {
	for(NSDictionary *file in files) {
		if([file[@"url"] isEqualTo:url]) {
			NSMutableArray *events = file[@"events"];
			if(![events count])
				return events;
			SCVisUpdate *event = [events count] ? [events lastObject] : nil;
			if(event) {
				if(timestamp >= event.timestamp) {
					return events;
				}
			}
		}
	}
	return nil;
}

- (void)postEvent:(NSNotification *)notification {
	SCVisUpdate *update = notification.object;

	@synchronized (self) {
		NSMutableArray *events = [self findEventsForUrl:update.file withTimestamp:update.timestamp];
		if(!events) {
			[self addTrack:update.file];
			events = [files lastObject][@"events"];
		}

		[events addObject:update];
	}
}

- (void)addObservers {
	if(!observersAdded) {
		[self addObserver:self forKeyPath:@"self.window.visible" options:0 context:kSCViewContext];

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
						  selector:@selector(playbackPrebuffered:)
							  name:CogPlaybackDidPrebufferNotification
							object:nil];

		[defaultCenter addObserver:self
						  selector:@selector(windowChangedOcclusionState:)
							  name:NSWindowDidChangeOcclusionStateNotification
							object:nil];

		[defaultCenter addObserver:self
						  selector:@selector(postEvent:)
							  name:CogSCVisUpdateNotification
							object:nil];

		observersAdded = YES;
	}
}

- (void)dealloc {
	[self removeObservers];
	if(lcdClearedState) {
		free(lcdClearedState);
	}
}

- (void)removeObservers {
	if(observersAdded) {
		[self removeObserver:self forKeyPath:@"self.window.visible" context:kSCViewContext];

		NSNotificationCenter *defaultCenter = [NSNotificationCenter defaultCenter];
		[defaultCenter removeObserver:self
								 name:NSSystemColorsDidChangeNotification
							   object:nil];
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
								 name:CogPlaybackDidPrebufferNotification
							   object:nil];

		[defaultCenter removeObserver:self
								 name:NSWindowDidChangeOcclusionStateNotification
							   object:nil];

		[defaultCenter removeObserver:self
								 name:CogSCVisUpdateNotification
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

- (void)startPlayback {
	[self playbackDidBegin:nil];
}

- (void)playbackDidBegin:(NSNotification *)notification {
	stopped = NO;
	paused = NO;
	[self updateVisListening];
	[self nextTrack];
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
	prebuffered = NO;
	stopped = YES;
	paused = NO;
	[self updateVisListening];
	@synchronized (self) {
		[files removeAllObjects];
		currentTrack = nil;
	}
	[self repaint];
	numDisplays = 1;
	[self resizeDisplay];
}

- (void)playbackPrebuffered:(NSNotification *)notification {
	prebuffered = YES;
}

- (void)uploadTexture:(uint32_t)which {
	id<MTLTexture> tex = mtlTexture[which].texture;
	[tex replaceRegion:MTLRegionMake2D(0, 0, lcdWidth, lcdHeight)
		   mipmapLevel:0
				 slice:0
			 withBytes:&renderBuffer[0]
		   bytesPerRow:1024 * 4
		 bytesPerImage:0];
}

- (void)renderEmptyPanel:(uint32_t)which {
	sc55_lcd_render_screen(lcdBackground, renderBuffer, lcdClearedState, sc55_lcd_state_size());
	[self uploadTexture:which];
}

- (void)renderAndDrawPanel:(SCVisUpdate *)event {
	sc55_lcd_render_screen(lcdBackground, renderBuffer, [event.state bytes], [event.state length]);
	[self uploadTexture:event.which];
	[self drawPanel:event.which];
}

+ (const float *)defaultVertices
{
   static float dummy[8] = {
	  0.0f, 0.0f,
	  1.0f, 0.0f,
	  0.0f, 1.0f,
	  1.0f, 1.0f,
   };
   return &dummy[0];
}

+ (const float *)defaultTexCoords
{
   static float dummy[8] = {
	  0.0f, 1.0f,
	  1.0f, 1.0f,
	  0.0f, 0.0f,
	  1.0f, 0.0f,
   };
   return &dummy[0];
}

+ (const float *)defaultColor
{
   static float dummy[16] = {
	  1.0f, 1.0f, 1.0f, 1.0f,
	  1.0f, 1.0f, 1.0f, 1.0f,
	  1.0f, 1.0f, 1.0f, 1.0f,
	  1.0f, 1.0f, 1.0f, 1.0f,
   };
   return &dummy[0];
}

- (void)drawPanel:(uint32_t)which {
	id<MTLRenderCommandEncoder> rce = [self rce];

	BufferRange range;
	NSUInteger vertex_count;
	SpriteVertex *pv;

	vertex_count = 4;

	const float *vertex    = [self.class defaultVertices];
	const float *tex_coord = [self.class defaultTexCoords];
	const float *color     = [self.class defaultColor];
	NSUInteger needed      = sizeof(SpriteVertex) * vertex_count;
	if(![self allocRange:&range length:needed])
		return;

	pv = (SpriteVertex *)range.data;

	for (NSUInteger i = 0; i < vertex_count; i++, pv++)
	{
	   pv->position = simd_make_float2(vertex[0], 1.0f - vertex[1]);
	   vertex += 2;

	   pv->texCoord = simd_make_float2(tex_coord[0], tex_coord[1]);
	   tex_coord += 2;

	   pv->color = simd_make_float4(color[0], color[1], color[2], color[3]);
	   color += 4;
	}

	CGFloat x = 0;
	CGFloat y = which * lcdHeight;
	CGFloat scale = self.window.screen.backingScaleFactor;

	MTLViewport vp = {
	   .originX = x * scale,
	   .originY = y * scale,
	   .width   = lcdWidth * scale,
	   .height  = lcdHeight * scale,
	   .znear   = 0,
	   .zfar    = 1,
	};
	[rce setViewport:vp];

	[rce setRenderPipelineState:_drawState];

	Uniforms uniforms = {
		  .projectionMatrix = self->uniforms.projectionMatrix
	};
	[rce setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:BufferIndexUniforms];
	[rce setVertexBuffer:range.buffer offset:range.offset atIndex:BufferIndexPositions];
	[rce setFragmentTexture:mtlTexture[which].texture atIndex:TextureIndexColor];
	[rce setFragmentSamplerState:mtlTexture[which].sampler atIndex:SamplerIndexDraw];
	[rce drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:vertex_count];
}

- (void)resizeDisplay {
	NSWindow *window = self.window;
	if(window) {
		NSRect contentRect = initFrame;
		contentRect.size.height = lcdHeight * numDisplays;

		NSRect frameRect = window.frame;
		NSRect originalContentRect = [window contentRectForFrameRect:frameRect];
		contentRect.origin = originalContentRect.origin;
		NSRect windowFrame = [[self window] frameRectForContentRect:contentRect];
		[window setFrame:windowFrame display:YES animate:YES];
		[window setMinSize:windowFrame.size];
		[window setMaxSize:windowFrame.size];

		initFrame = contentRect;
	}
}

- (void)nextTrack {
	@synchronized (self) {
		if(currentTrack) {
			[self removeTrack:currentTrack];
			if([files count]) {
				currentTrack = files[0][@"url"];
			} else {
				currentTrack = nil;
			}
			[self renderEmptyPanel:0];
			[self renderEmptyPanel:1];
			[self renderEmptyPanel:2];
			numDisplays = 1;
			[self resizeDisplay];
		}
	}
}

- (void) mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
}

- (void)drawInMTKView:(MTKView *)view {
	if(!isSetup) {
		if(![self setup])
			return;
	}

	_view = view;
	nextDrawable = [self metalLayer].nextDrawable;
	if(!nextDrawable)
		return;

	assert(_commandBuffer == nil);
	_commandBuffer = [mtlCmdQueue commandBuffer];
	if(!_commandBuffer)
		return;

	BOOL rendered[3] = {NO};

	size_t count;
	@synchronized (self) {
		count = [files count];
	}
	
	if(!prebuffered || stopped || !count) {
		[self renderEmptyPanel:0];
		[self renderEmptyPanel:1];
		[self renderEmptyPanel:2];
	}

	[self updateVisListening];

	NSArray *events;

	if(stopped || !count) goto _END;

	if(!currentTrack) {
		currentTrack = files[0][@"url"];
	}

	if(!prebuffered) goto _END;

	uint64_t currentTimestamp = 0;
	uint64_t mslatency = 0;
	@synchronized (self) {
		if([files count]) {
			NSArray *events = files[0][@"events"];
			if(events && [events count]) {
				SCVisUpdate *event = [events lastObject];
				currentTimestamp = event.timestamp;
				double latency = [visController getFullLatency];
				mslatency = floor(latency * 1000.0);
				if(currentTimestamp > mslatency)
					currentTimestamp -= mslatency;
				else
					currentTimestamp = 0;
			}
		}
	}

	BOOL present[3] = {NO};
	events = [self getEventsForTimestamp:currentTimestamp];
	for(SCVisUpdate *event in events) {
		present[event.which] = YES;
	}
	for(SCVisUpdate *event in [events reverseObjectEnumerator]) {
		[self renderAndDrawPanel:event];
		rendered[event.which] = YES;
		if(rendered[0] == present[0] &&
		   rendered[1] == present[1] &&
		   rendered[2] == present[2])
			break;
	}

	if(rendered[2] && numDisplays < 3) {
		numDisplays = 3;
		[self resizeDisplay];
	} else if(rendered[1] && numDisplays < 2) {
		numDisplays = 2;
		[self resizeDisplay];
	}

_END:
	if(!rendered[0])
		[self drawPanel:0];
	if(!rendered[1])
		[self drawPanel:1];
	if(!rendered[2])
		[self drawPanel:2];

	[_chain[_currentChain] commitRanges];
	if(_rce) {
		[_rce endEncoding];
		_rce = nil;
	}
	if(nextDrawable)
		[_commandBuffer presentDrawable:nextDrawable];
	[_commandBuffer commit];
	_commandBuffer = nil;
	nextDrawable = nil;
	[self _nextChain];
}

@end

@implementation Texture
@end

@implementation BufferNode

- (instancetype)initWithBuffer:(id<MTLBuffer>)src
{
   if (self = [super init])
	  _src = src;
   return self;
}

@end

@implementation BufferChain
{
   id<MTLDevice> _device;
   NSUInteger _blockLen;
   BufferNode *_head;
   NSUInteger _offset; /* offset into _current */
   BufferNode *_current;
   NSUInteger _length;
   NSUInteger _allocated;
}

/* macOS requires constants in a buffer to have a 256 byte alignment. */
#ifdef TARGET_OS_MAC
static const NSUInteger kConstantAlignment = 256;
#else
static const NSUInteger kConstantAlignment = 4;
#endif

- (instancetype)initWithDevice:(id<MTLDevice>)device blockLen:(NSUInteger)blockLen
{
   if (self = [super init])
   {
	  _device   = device;
	  _blockLen = blockLen;
   }
   return self;
}

- (NSString *)debugDescription
{
   return [NSString stringWithFormat:@"length=%ld, allocated=%ld", _length, _allocated];
}

- (void)commitRanges
{
	// Only needed for Managed, not Shared
#ifndef __arm64__
	BufferNode *n;
	for (n = _head; n != nil; n = n.next) {
		if (n.allocated > 0)
			[n.src didModifyRange:NSMakeRange(0, n.allocated)];
	}
#endif
}

- (void)discard
{
   _current   = _head;
   _offset    = 0;
   _allocated = 0;
}

#ifdef __arm64__
#define PLATFORM_METAL_RESOURCE_STORAGE_MODE MTLResourceStorageModeShared
#else
#define PLATFORM_METAL_RESOURCE_STORAGE_MODE MTLResourceStorageModeManaged
#endif

- (bool)allocRange:(BufferRange *)range length:(NSUInteger)length
{
   MTLResourceOptions opts = PLATFORM_METAL_RESOURCE_STORAGE_MODE;
   memset(range, 0, sizeof(*range));

   if (!_head)
   {
	  _head    = [[BufferNode alloc] initWithBuffer:[_device newBufferWithLength:_blockLen options:opts]];
	  _length += _blockLen;
	  _current = _head;
	  _offset  = 0;
   }

   if ([self _subAllocRange:range length:length])
	  return YES;

   while (_current.next)
   {
	  [self _nextNode];
	  if ([self _subAllocRange:range length:length])
		 return YES;
   }

   NSUInteger blockLen = _blockLen;
   if (length > blockLen)
	  blockLen = length;

   _current.next = [[BufferNode alloc] initWithBuffer:[_device newBufferWithLength:blockLen options:opts]];
   if (!_current.next)
	  return NO;

   _length += blockLen;

   [self _nextNode];
   assert([self _subAllocRange:range length:length]);
   return YES;
}

- (void)_nextNode
{
   _current = _current.next;
   _offset  = 0;
}

- (BOOL)_subAllocRange:(BufferRange *)range length:(NSUInteger)length
{
   NSUInteger nextOffset  = _offset + length;
   if (nextOffset <= _current.src.length)
   {
	  _current.allocated  = nextOffset;
	  _allocated         += length;
	  range->data         = _current.src.contents + _offset;
	  range->buffer       = _current.src;
	  range->offset       = _offset;
	  _offset             = MTL_ALIGN_BUFFER(nextOffset);
	  return YES;
   }
   return NO;
}

@end
