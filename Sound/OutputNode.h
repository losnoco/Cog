//
//  OutputNode.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CoreAudio/AudioHardware.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>

#import "Node.h"
#import "OutputCoreAudio.h"

@interface OutputNode : Node {
	AudioStreamBasicDescription format;
	
	int amountPlayed;
	OutputCoreAudio *output;
}

- (id)initWithController:(id)c previousLink:p;

- (double)amountPlayed;

- (void)setup;
- (void)process;
- (void)close;

- (int)readData:(void *)ptr amount:(int)amount;

- (void)setFormat:(AudioStreamBasicDescription *)f;
- (AudioStreamBasicDescription) format;

- (void)setVolume:(double) v;

@end
