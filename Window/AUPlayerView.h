//
//  AUPlayerView.h
//  Output
//
//  Created by Christopher Snowhill on 1/29/16.
//  Copyright © 2016-2022 Christopher Snowhill. All rights reserved.
//

#ifndef __AUPlayerView_h__
#define __AUPlayerView_h__

#import <AppKit/AppKit.h>
#import <AudioUnit/AudioUnitCarbonView.h>
#import <AudioUnit/AudioUnit.h>

@interface AUPluginUI : NSObject
{
    AudioUnit au;
    int prefheight;
    int prefwidth;
    
    BOOL windowOpen;
    
    BOOL resizable;
    int  min_width;
    int  min_height;
    int  req_width;
    int  req_height;
    int  alo_width;
    int  alo_height;
    
    /* Cocoa */
    
    NSWindow*           cocoa_window;
    NSView*             au_view;
    NSRect              last_au_frame;
}

- (id) initWithSampler:(AudioUnit)_au bringToFront:(BOOL)front orWindowNumber:(NSInteger)window;
- (void) dealloc;

- (BOOL) isOpen;
- (BOOL) isForeground;

- (void) bringToFront;

- (NSInteger) windowNumber;

@end

#endif
