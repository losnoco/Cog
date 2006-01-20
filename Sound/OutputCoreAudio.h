//
//  OutputCoreAudio.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
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
- (void)start;

- (void)setVolume:(double) v;

@end
