//
//  InputNode.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CoreAudio/AudioHardware.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>

#import "SoundFile.h"
#import "Node.h"

@interface InputNode : Node {
	AudioStreamBasicDescription format;
	
	SoundFile *soundFile;
}

- (void)process;
- (AudioStreamBasicDescription) format;

@end
