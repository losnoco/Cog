//
//  OutputCoreAudio.h
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CoreAudio/AudioHardware.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>

@interface OutputCoreAudio : NSObject {
	id outputController;
	
	AudioUnit outputUnit;
    AURenderCallbackStruct renderCallback;	
	AudioStreamBasicDescription deviceFormat;	// info about the default device
}

- (id)initWithController:(id)c;

- (BOOL)setup;
- (BOOL)setOutputDevice:(AudioDeviceID)outputDevice;
- (void)start;
- (void)pause;
- (void)resume;
- (void)stop;

- (void)setVolume:(double) v;

@end
