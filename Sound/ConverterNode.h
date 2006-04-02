//
//  ConverterNode.h
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

@interface ConverterNode : Node {
	AudioConverterRef converter;
	void *callbackBuffer;
	
	AudioStreamBasicDescription inputFormat;
	AudioStreamBasicDescription outputFormat;
}

- (void)setupWithInputFormat:(AudioStreamBasicDescription)inputFormat outputFormat:(AudioStreamBasicDescription)outputFormat;
- (void)cleanUp;

- (void)process;
- (int)convert:(void *)dest amount:(int)amount;

@end
