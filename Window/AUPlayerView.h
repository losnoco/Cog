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
#import <AudioToolbox/AudioToolbox.h>

void equalizerApplyGenre(AudioUnit au, NSString *genre);
void equalizerLoadPreset(AudioUnit au);
void equalizerApplyPreset(AudioUnit au, NSDictionary * preset);

@interface AUPluginUI : NSObject
{
    AudioUnit au;
    
    BOOL windowOpen;
    
    /* Cocoa */
    
    NSWindow*           cocoa_window;
    NSView*             au_view;
    NSRect              last_au_frame;
}

- (id) initWithSampler:(AudioUnit)_au bringToFront:(BOOL)front orWindowNumber:(NSInteger)window;
- (void) dealloc;

- (BOOL) isOpen;

- (void) bringToFront;

@end

@interface AUPluginWindow : NSWindow<NSSplitViewDelegate> {
    AudioUnit au;
    AUParameterListenerRef listenerRef;
    
    NSView *topView;
    NSView *auView;
    NSSplitView *splitView;
    NSPopUpButton * presetButton;
}

- (id) initWithAuView:(NSView *)_auView withAu:(AudioUnit)au bringToFront:(BOOL)front relativeToWindow:(NSInteger)window;
@end

#endif