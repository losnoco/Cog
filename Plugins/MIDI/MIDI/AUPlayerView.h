//
//  AUPlayerView.h
//  MIDI
//
//  Created by Christopher Snowhill on 1/29/16.
//  Copyright © 2021 Christopher Snowhill. All rights reserved.
//

#ifndef __AUPlayerView_h__
#define __AUPlayerView_h__

#include <vector>
#include <string>

#include <AppKit/AppKit.h>
#include <Carbon/Carbon.h>
#include <AudioUnit/AudioUnitCarbonView.h>
#include <AudioUnit/AudioUnit.h>

class AUPluginUI
{
public:
    AUPluginUI (AudioUnit & sampler);
    ~AUPluginUI ();
    
private:
    AudioUnit & au;
    int prefheight;
    int prefwidth;
    
    bool mapped;
    bool resizable;
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
    
    bool test_cocoa_view_support ();
    int  create_cocoa_view ();
    
    bool plugin_class_valid (Class pluginClass);
};

#endif
