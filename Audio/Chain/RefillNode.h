//
//  RefillNode.h
//  Cog
//
//  Created by Christopher SNowhill on 1/13/22.
//  Copyright 2022 __LoSnoCo__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CoreAudio/AudioHardware.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>

#import "Node.h"
#import "Plugin.h"

#define INPUT_NODE_SEEK

@interface RefillNode : Node {
    // This node just slaps pre-converted data into its buffer for re-buffering
}

- (void) setFormat:(AudioStreamBasicDescription)format;

@end
