//
//  AUPlayerView.m
//  Output
//
//  Created by Christopher Snowhill on 1/29/16.
//  Copyright © 2016-2022 Christopher Snowhill. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AudioUnit/AUCocoaUIView.h>
#import <CoreAudioKit/AUGenericView.h>

#import "AUPlayerView.h"

@interface AUPluginUI (Private)
- (BOOL)test_cocoa_view_support;
- (int)create_cocoa_view;
- (BOOL)plugin_class_valid:(Class)pluginClass;
@end

@implementation AUPluginUI
- (id) initWithSampler:(AudioUnit)_au bringToFront:(BOOL)front orWindowNumber:(NSInteger)window
{
    self = [super init];
    if (self)
    {
        au = _au;
        
        windowOpen = NO;
        
        cocoa_window = nil;
        au_view = nil;
        
        if ([self test_cocoa_view_support])
        {
            [self create_cocoa_view];
        }
        
        if (au_view)
        {
            cocoa_window = [[AUPluginWindow alloc] initWithAuView:au_view bringToFront:front relativeToWindow:window];
            
            if (cocoa_window) {
                windowOpen = YES;
                
                [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowClosed:) name:NSWindowWillCloseNotification object:cocoa_window];
            }
        }
    }
    
    return self;
}

- (void) dealloc
{
    if (windowOpen)
    {
        [self windowClosed:nil];
    }
    [cocoa_window close];
    cocoa_window = nil;
    au_view = nil;
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (BOOL) isOpen
{
    return windowOpen;
}

- (void) bringToFront
{
    [cocoa_window orderFront:cocoa_window];
}

- (void)windowClosed:(NSNotification*)notification
{
    [cocoa_window saveFrameUsingName:@"GraphicEQ.position"];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    windowOpen = NO;
}

- (BOOL)test_cocoa_view_support
{
    UInt32 dataSize   = 0;
    Boolean isWritable = 0;
    OSStatus err = AudioUnitGetPropertyInfo(au,
                                            kAudioUnitProperty_CocoaUI, kAudioUnitScope_Global,
                                            0, &dataSize, &isWritable);
    
    return dataSize > 0 && err == noErr;
}

- (BOOL) plugin_class_valid: (Class)pluginClass
{
    if([pluginClass conformsToProtocol: @protocol(AUCocoaUIBase)]) {
        if([pluginClass instancesRespondToSelector: @selector(interfaceVersion)] &&
           [pluginClass instancesRespondToSelector: @selector(uiViewForAudioUnit:withSize:)]) {
            return true;
        }
    }
    return false;
}

- (int)create_cocoa_view
{
    bool wasAbleToLoadCustomView = false;
    AudioUnitCocoaViewInfo* cocoaViewInfo = NULL;
    UInt32               numberOfClasses = 0;
    UInt32     dataSize;
    Boolean    isWritable;
    NSString*	    factoryClassName = 0;
    NSURL*	            CocoaViewBundlePath = NULL;
    
    OSStatus result = AudioUnitGetPropertyInfo (au,
                                                kAudioUnitProperty_CocoaUI,
                                                kAudioUnitScope_Global,
                                                0,
                                                &dataSize,
                                                &isWritable );
    
    numberOfClasses = (dataSize - sizeof(CFURLRef)) / sizeof(CFStringRef);
    
    // Does view have custom Cocoa UI?
    
    if ((result == noErr) && (numberOfClasses > 0) ) {
        
        cocoaViewInfo = (AudioUnitCocoaViewInfo *)malloc(dataSize);
        
        if(AudioUnitGetProperty(au,
                                kAudioUnitProperty_CocoaUI,
                                kAudioUnitScope_Global,
                                0,
                                cocoaViewInfo,
                                &dataSize) == noErr) {
            
            CocoaViewBundlePath	= (__bridge NSURL *)cocoaViewInfo->mCocoaAUViewBundleLocation;
            
            // we only take the first view in this example.
            factoryClassName	= (__bridge NSString *)cocoaViewInfo->mCocoaAUViewClass[0];
        } else {
            if (cocoaViewInfo != NULL) {
                free (cocoaViewInfo);
                cocoaViewInfo = NULL;
            }
        }
    }
    
    // [A] Show custom UI if view has it
    
    if (CocoaViewBundlePath && factoryClassName) {
        NSBundle *viewBundle  	= [NSBundle bundleWithPath:[CocoaViewBundlePath path]];
        
        if (viewBundle == NULL) {
            return -1;
        } else {
            Class factoryClass = [viewBundle classNamed:factoryClassName];
            if (!factoryClass) {
                return -1;
            }
            
            // make sure 'factoryClass' implements the AUCocoaUIBase protocol
            if (![self plugin_class_valid: factoryClass]) {
                return -1;
            }
            // make a factory
            id factory = [[factoryClass alloc] init];
            if (factory == NULL) {
                return -1;
            }
            
            // make a view
            au_view = [factory uiViewForAudioUnit:au withSize:NSZeroSize];
            
            // cleanup
            if (cocoaViewInfo) {
                UInt32 i;
                for (i = 0; i < numberOfClasses; i++)
                    CFRelease(cocoaViewInfo->mCocoaAUViewClass[i]);
                
                free (cocoaViewInfo);
            }
            wasAbleToLoadCustomView = true;
        }
    }
    
    if (!wasAbleToLoadCustomView) {
        // load generic Cocoa view
        au_view = [[AUGenericView alloc] initWithAudioUnit:au];
        [(AUGenericView *)au_view setShowsExpertParameters:1];
    }
    
    return 0;
}

@end

@implementation AUPluginWindow
- (id) initWithAuView:(NSView *)_auView bringToFront:(BOOL)front relativeToWindow:(NSInteger)window
{
    NSRect  frame = [_auView frame];
    CGFloat req_width  = frame.size.width;
    CGFloat req_height = frame.size.height;
    //BOOL resizable  = [_auView autoresizingMask];

    self = [super initWithContentRect:NSMakeRect(0, 0, req_width, req_height + 32)
                            styleMask:(NSWindowStyleMaskTitled |
                                       NSWindowStyleMaskClosable)
                              backing:NSBackingStoreBuffered
                                defer:NO];
    if (self)
    {
        auView = _auView;
        
        [self setAutodisplay:YES];
        [self setOneShot:YES];

        if (front)
        {
            [self orderFront:self];
        }
        else
            [self orderWindow:NSWindowBelow relativeTo:window];
        
        [self setReleasedWhenClosed:NO];
        
        NSRect topRect = NSMakeRect(0, 0, req_width, 32);
        
        topView = [[NSView alloc] initWithFrame:topRect];
        
        NSRect topFrame = NSMakeRect(0, req_height, req_width, req_height);
        NSRect newFrame = NSMakeRect(0, 0, req_width, topRect.size.height);
        
        topRect = NSMakeRect(0, 0, req_width, req_height + topRect.size.height);
        
        splitView = [[NSSplitView alloc] initWithFrame:topRect];
        
        [splitView setSubviews:[NSArray arrayWithObjects:topView, auView, nil]];
        
        [self setContentView:splitView];
        
        [topView setFrame:topFrame];
        [auView setFrame:newFrame];
        
        BOOL enabled = [[NSUserDefaults standardUserDefaults] boolForKey:@"GraphicEQenable"];
        
        NSButton *button = [NSButton checkboxWithTitle:@"Enabled" target:self action:@selector(toggleEnable:)];
        [button setState:enabled ? NSControlStateValueOn : NSControlStateValueOff];
        
        NSRect buttonFrame = [button frame];
        buttonFrame.origin = NSMakePoint( 18, 4 );
        [button setFrame:buttonFrame];
        
        [topView addSubview:button];
        
        [splitView adjustSubviews];
        
        [splitView setDelegate:self];
        
        [self setFrameUsingName:@"GraphicEQ.position"];
    }
    
    return self;
}

- (NSRect)splitView:(NSSplitView *)splitView effectiveRect:(NSRect)proposedEffectiveRect forDrawnRect:(NSRect)drawnRect ofDividerAtIndex:(NSInteger)dividerIndex
{
    return NSZeroRect;
}

- (void) toggleEnable:(id)sender
{
    BOOL enabled = [sender state] == NSControlStateValueOn;
    
    [[NSUserDefaults standardUserDefaults] setBool:enabled forKey:@"GraphicEQenable"];
}
@end

