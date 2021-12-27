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

@class OutputNode;

@interface OutputCoreAudio : NSObject {
	OutputNode * outputController;
    
    BOOL primed;
    BOOL running;
    BOOL stopping;
    BOOL stopped;
    
    float volume;

    AudioDeviceID outputDeviceID;
    AudioStreamBasicDescription deviceFormat;    // info about the default device

    AudioQueueRef audioQueue;
    AudioQueueBufferRef *buffers;
    UInt32 numberOfBuffers;
    UInt32 bufferByteSize;
    
	AudioUnit outputUnit;
	AURenderCallbackStruct renderCallback;	
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

@end
