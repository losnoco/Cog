//
//  OutputNode.h
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CoreAudio/AudioHardware.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>

#import "Node.h"
#import "OutputCoreAudio.h"

@interface OutputNode : Node {
	AudioStreamBasicDescription format;
	
	unsigned long long amountPlayed;
	OutputCoreAudio *output;
    
    BOOL paused;
}

- (double)amountPlayed;

- (void)setup;
- (void)process;
- (void)close;
- (void)seek:(double)time;

- (int)readData:(void *)ptr amount:(int)amount;

- (void)setFormat:(AudioStreamBasicDescription *)f;
- (AudioStreamBasicDescription) format;

- (void)setVolume:(double) v;

- (void)setShouldContinue:(BOOL)s;

- (void)pause;
- (void)resume;
- (void)resumeWithFade;

- (BOOL)isPaused;

@end
