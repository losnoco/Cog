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
        resizable = NO;
        min_width = 0;
        min_height = 0;
        req_width = 0;
        req_height = 0;
        alo_width = 0;
        alo_height = 0;
        
        windowOpen = NO;
        
        cocoa_window = nil;
        au_view = nil;
        
        if ([self test_cocoa_view_support])
        {
            [self create_cocoa_view];
        }
        
        if (au_view)
        {
            cocoa_window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, req_width, req_height)
                                                       styleMask:(NSWindowStyleMaskTitled |
                                                              NSWindowStyleMaskClosable)
                                                         backing:NSBackingStoreBuffered
                                                           defer:NO];

            [cocoa_window setAutodisplay:YES];
            [cocoa_window setOneShot:YES];
        
            [cocoa_window setContentView:au_view];
        
            if (front)
            {
                [cocoa_window orderFront:cocoa_window];
            }
            else
                [cocoa_window orderWindow:NSWindowBelow relativeTo:window];
            
            [cocoa_window setReleasedWhenClosed:NO];
            
            [cocoa_window setFrameUsingName:@"EqualizerWindowPosition"];
            
            windowOpen = YES;

            [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(windowClosed:) name:NSWindowWillCloseNotification object:cocoa_window];
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

- (NSInteger) windowNumber
{
    return cocoa_window.windowNumber;
}

- (void)windowClosed:(NSNotification*)notification
{
    [cocoa_window saveFrameUsingName:@"EqualizerWindowPosition"];
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
    
    // Get the initial size of the new AU View's frame
    NSRect  frame = [au_view frame];
    min_width  = req_width  = CGRectGetWidth(NSRectToCGRect(frame));
    min_height = req_height = CGRectGetHeight(NSRectToCGRect(frame));
    resizable  = [au_view autoresizingMask];
    
    return 0;
}

@end
