//
//  OutputCoreAudio.h
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import <AssertMacros.h>
#import <Cocoa/Cocoa.h>

#import <CoreAudio/AudioHardware.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreAudio/CoreAudioTypes.h>

#import <stdatomic.h>

#import "Semaphore.h"

//#define OUTPUT_LOG
#ifdef OUTPUT_LOG
#import <stdio.h>
#endif

@class OutputNode;

@interface OutputCoreAudio : NSObject {
	OutputNode * outputController;
    
    Semaphore * writeSemaphore;
    Semaphore * readSemaphore;
    
    BOOL stopInvoked;
    BOOL running;
    BOOL stopping;
    BOOL stopped;
    BOOL started;
    BOOL paused;
    
    BOOL eqEnabled;
    
    atomic_long bytesRendered;
    atomic_long bytesHdcdSustained;
    
    BOOL listenerapplied;
    
    float volume;
    
    AVAudioFormat *_deviceFormat;

    AudioDeviceID outputDeviceID;
    AudioStreamBasicDescription deviceFormat;    // info about the default device

    AUAudioUnit *_au;
    size_t _bufferSize;
    
    AudioUnit _eq;
    
#ifdef OUTPUT_LOG
    FILE *_logFile;
#endif
}

- (id)initWithController:(OutputNode *)c;

- (BOOL)setup;
- (OSStatus)setOutputDeviceByID:(AudioDeviceID)deviceID;
- (BOOL)setOutputDeviceWithDeviceDict:(NSDictionary *)deviceDict;
- (void)start;
- (void)pause;
- (void)resume;
- (void)stop;

- (void)setVolume:(double) v;

- (void)setEqualizerEnabled:(BOOL)enabled;

- (void)sustainHDCD;

@end
